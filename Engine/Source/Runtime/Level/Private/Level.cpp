#include "pch.h"
#include "Runtime/Level/Public/Level.h"

#include "Runtime/Actor/Public/Actor.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel()
{
	ULevel::GetClass()->IncrementGenNumber();
}

ULevel::ULevel(const FName& InName)
	: UObject(InName)
{
	ULevel::GetClass()->IncrementGenNumber();
}

ULevel::~ULevel()
{
	for (auto& Actor : Actors)
	{
		if (Actor)
		{
			Actor->SetOuter(nullptr);
		}
	}

	Actors.Empty();

	UE_LOG_SYSTEM("Level: GUObjectArray에 존재하는 제거된 객체들을 정리합니다");
	CleanupGUObjectArray();
}

/**
 * @brief 소멸자가 아닌 레벨이 detached 될 때 처리되어야 하는 내용을 정리한 함수
 */
void ULevel::Release()
{
	for (auto& Actor : Actors)
	{
		if (Actor)
		{
			// TODO(KHJ): 이후 GC로 처리할 경우, Outer만 배제해주도록 하면 됨
			// Actor->SetOuter(nullptr);
			SafeDelete(Actor);
		}
	}

	Actors.Empty();

	UE_LOG_SYSTEM("Level: GUObjectArray에 존재하는 제거된 객체들을 정리합니다");
	CleanupGUObjectArray();
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}

/**
 * @brief Level에서 Actor를 제거하는 함수
 */
bool ULevel::DestroyActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return false;
	}

	// LevelActors 리스트에서 제거
	Actors.RemoveSingle(InActor);

	// Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
	InActor->SetOuter(nullptr);

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}
