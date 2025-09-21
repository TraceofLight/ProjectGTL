#include "pch.h"
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/EngineStatics.h"
#include "Runtime/Core/Public/Name.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<TObjectPtr<UObject>> GUObjectArray;

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Name(FName::None), Outer(nullptr)
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
