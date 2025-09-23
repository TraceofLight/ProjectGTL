#include "pch.h"
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/EngineStatics.h"
#include "Runtime/Core/Public/Name.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<TObjectPtr<UObject>> GUObjectArray;

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Name(FName::FName_None), Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = FName("Object_" + to_string(UUID));

	GUObjectArray.emplace_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

UObject::UObject(const FName& InName)
	: Name(InName)
	  , Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.emplace_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

UObject::~UObject()
{
	// GUObjectArray에서 이 오브젝트 제거
	if (InternalIndex < GUObjectArray.size() && GUObjectArray[InternalIndex].Get() == this)
	{
		// 현재 위치의 포인터를 nullptr로 설정 (배열 크기는 유지)
		GUObjectArray[InternalIndex] = nullptr;
	}
	else
	{
		// InternalIndex가 유효하지 않은 경우, 전체 배열에서 검색하여 제거
		for (size_t i = 0; i < GUObjectArray.size(); ++i)
		{
			if (GUObjectArray[i].Get() == this)
			{
				GUObjectArray[i] = nullptr;
				break;
			}
		}
	}

	// Outer에서 메모리 사용량 제거
	if (Outer)
	{
		Outer->PropagateMemoryChange(-static_cast<int64>(AllocatedBytes), -static_cast<int32>(AllocatedCounts));
	}

	CheckAndCleanupGUObjectArray();
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있었다면, 나의 전체 메모리 사용량을 빼달라고 전파
	// 새로운 Outer가 있다면, 나의 전체 메모리 사용량을 더해달라고 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(-static_cast<int64>(AllocatedBytes), -static_cast<int32>(AllocatedCounts));
	}

	Outer = InObject;

	if (Outer)
	{
		Outer->PropagateMemoryChange(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	uint64 BytesToAdd = InBytes;

	if (!BytesToAdd)
	{
		BytesToAdd = GetClass()->GetClassSize();
	}

	// 메모리 변경 전파
	PropagateMemoryChange(BytesToAdd, InCount);
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	PropagateMemoryChange(-static_cast<int64>(InBytes), -static_cast<int32>(InCount));
}

void UObject::PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta)
{
	// 자신의 값에 변화량을 더함
	AllocatedBytes += InBytesDelta;
	AllocatedCounts += InCountDelta;

	// Outer가 있다면, 동일한 변화량을 그대로 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(InBytesDelta, InCountDelta);
	}
}

/**
 * @brief 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * 내부적으로 재귀를 활용해서 부모를 계속 탐색한 뒤 결과를 반환한다
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(TObjectPtr<UClass> InClass) const
{
	if (!InClass || this == nullptr)
	{
		return false;
	}
	return GetClass()->IsChildOf(InClass);
}

/**
 * @brief GUObjectArray의 nullptr 요소들을 정리하는 함수
 * 배치 처리로 안전한 시점에 수행하여 성능 하락을 방지
 */
void UObject::CleanupGUObjectArray()
{
	size_t OriginalSize = GUObjectArray.size();
	uint32 NullCount = 0;

	// nullptr이 아닌 요소들만 보존
	auto NewEnd = std::remove_if(
		GUObjectArray.begin(), GUObjectArray.end(), [&NullCount](const TObjectPtr<UObject>& Pointer)
		{
			if (!Pointer || Pointer.Get() == nullptr)
			{
				++NullCount;
				return true;
			}

			return false;
		});

	GUObjectArray.erase(NewEnd, GUObjectArray.end());

	// 모든 유효한 객체들의 InternalIndex 재설정
	for (size_t i = 0; i < GUObjectArray.size(); ++i)
	{
		if (GUObjectArray[i] && GUObjectArray[i].Get())
		{
			GUObjectArray[i]->SetInternalIndex(static_cast<uint32>(i));
		}
	}

	UE_LOG_SYSTEM("GUObjectArray: Compaction 완료 | 기존: %zu, 정리후: %zu, 제거된 nullptr: %u",
	              OriginalSize, GUObjectArray.size(), NullCount);
}

/**
 * @brief GUObjectArray의 nullptr 개수를 반환하는 함수
 */
uint32 UObject::GetNullObjectCount()
{
	uint32 NullCount = 0;

	for (const auto& PointerCheck : GUObjectArray)
	{
		if (!PointerCheck || PointerCheck.Get() == nullptr)
		{
			++NullCount;
		}
	}

	return NullCount;
}

/**
 * @brief 임계치 기반 GUObjectArray의 자동 정리 함수
 * 기준에 해당하는 nullptr 갯수라면 자동 정리 (nullptr이 너무 많거나, 일정 갯수 이상에서 일정 비율이 nullptr)
 */
void UObject::CheckAndCleanupGUObjectArray()
{
	// 임계 범위 세팅
	static constexpr uint32 CLEANUP_THRESHOLD = 500;
	static constexpr float CLEANUP_RATIO = 0.3f;
	static constexpr uint32 CLEANUP_MIN = 20;

	uint32 NullCount = GetNullObjectCount();
	size_t TotalSize = GUObjectArray.size();

	// 임계치 체크: 절대값 또는 비율
	if (NullCount >= CLEANUP_THRESHOLD ||
		(TotalSize > 0 && static_cast<float>(NullCount) / TotalSize >= CLEANUP_RATIO && TotalSize >= CLEANUP_MIN))
	{
		UE_LOG_INFO("GUObjectArray: 설정한 임계치에 도달하여 자동 GUObjectArray 정리 시작 (nullptr: %u / %zu, %.1f%%)",
		            NullCount, TotalSize, TotalSize > 0 ? static_cast<float>(NullCount) / TotalSize * 100.0f : 0.0f);

		CleanupGUObjectArray();
	}
}
