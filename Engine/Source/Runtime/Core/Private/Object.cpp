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

	GUObjectArray.Add(TObjectPtr(this));
	InternalIndex = static_cast<uint32>(GUObjectArray.Num()) - 1;
}

UObject::UObject(const FName& InName)
	: Name(InName)
	  , Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.Add(TObjectPtr(this));
	InternalIndex = GUObjectArray.Num() - 1;
}

UObject::~UObject()
{
	// GUObjectArray에서 이 오브젝트 제거
	if (static_cast<int32>(InternalIndex) < GUObjectArray.Num() &&
		GUObjectArray[static_cast<int32>(InternalIndex)].Get() == this)
	{
		// 현재 위치의 포인터를 nullptr로 설정 (배열 크기는 유지)
		GUObjectArray[static_cast<int32>(InternalIndex)] = nullptr;
	}
	else
	{
		// InternalIndex가 유효하지 않은 경우, 전체 배열에서 검색하여 제거
		for (int32 i = 0; i < GUObjectArray.Num(); ++i)
		{
			if (GUObjectArray[i].Get() == this)
			{
				GUObjectArray[i] = nullptr;
				break;
			}
		}
	}

	CheckAndCleanupGUObjectArray();
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	Outer = InObject;
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
	size_t OriginalSize = GUObjectArray.Num();
	uint32 NullCount = 0;

	// nullptr이 아닌 요소들만 보존
	int32 RemovedCount = GUObjectArray.RemoveAll(
		[](const TObjectPtr<UObject>& Pointer)
		{
			// 유효하지 않거나 nullptr인 경우 true를 반환하여 제거 대상임을 알림
			// TObjectPtr<UObject>는 Get() 없이 바로 포인터 비교가 가능합니다.
			return !Pointer;
		}
	);

	// 모든 유효한 객체들의 InternalIndex 재설정
	for (int32 i = 0; i < GUObjectArray.Num(); ++i)
	{
		if (GUObjectArray[i])
		{
			GUObjectArray[i]->SetInternalIndex(i);
		}
	}

	UE_LOG_SYSTEM("GUObjectArray: Compaction 완료 | 기존: %zu, 정리후: %d, 제거된 nullptr: %u",
	              OriginalSize, GUObjectArray.Num(), NullCount);
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
 * @brief 현재 UObject를 복제하여 새로운 인스턴스를 생성하는 함수
 * 얕은 복사를 통해 NewObject를 생성, 이후 깊은 복사가 필요한 서브오브젝트들은 재귀적으로 처리
 * @return 복제된 새로운 UObject 인스턴스에 대한 포인터
 */
UObject* UObject::Duplicate()
{
	UObject* NewObject = new UObject(*this);
	NewObject->DuplicateSubObjects();

	return NewObject;
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
	size_t TotalSize = GUObjectArray.Num();

	// 임계치 체크: 절대값 또는 비율
	if (NullCount >= CLEANUP_THRESHOLD ||
		(TotalSize > 0 && static_cast<float>(NullCount) / TotalSize >= CLEANUP_RATIO && TotalSize >= CLEANUP_MIN))
	{
		UE_LOG_INFO("GUObjectArray: 설정한 임계치에 도달하여 자동 GUObjectArray 정리 시작 (nullptr: %u / %zu, %.1f%%)",
		            NullCount, TotalSize, TotalSize > 0 ? static_cast<float>(NullCount) / TotalSize * 100.0f : 0.0f);

		CleanupGUObjectArray();
	}
}
