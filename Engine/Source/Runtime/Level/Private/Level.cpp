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
	// Actor들의 Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
	for (auto& Actor : ActorsToDelete)
	{
		if (Actor)
		{
			Actor->SetOuter(nullptr);
		}
	}

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actor->SetOuter(nullptr);
		}
	}

	ActorsToDelete.Empty();
	LevelActors.Empty();

	UE_LOG_SYSTEM("Level: GUObjectArray에 존재하는 제거된 객체들을 정리합니다");
	CleanupGUObjectArray();
}

/**
 * @brief 소멸자가 아닌 레벨이 detached 될 때 처리되어야 하는 내용을 정리한 함수
 */
void ULevel::Release()
{
	// Actor들의 Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
	for (auto& Actor : ActorsToDelete)
	{
		if (Actor)
		{
			// TODO(KHJ): 이후 GC로 처리할 경우, Outer만 배제해주도록 하면 됨
			// Actor->SetOuter(nullptr);
			SafeDelete(Actor);
		}
	}

	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			// TODO(KHJ): 이후 GC로 처리할 경우, Outer만 배제해주도록 하면 됨
			// Actor->SetOuter(nullptr);
			SafeDelete(Actor);
		}
	}

	ActorsToDelete.Empty();
	LevelActors.Empty();

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

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({0.f, 0.f, 0.f, 0.f});
				}
			}
		}
	}

	SelectedActor = InActor;
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({1.f, 0.8f, 0.2f, 0.4f});
				}
			}
		}
	}
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
	LevelActors.RemoveSingle(InActor);

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}

	// Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
	InActor->SetOuter(nullptr);

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (TObjectPtr<AActor> PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.Add(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
}
