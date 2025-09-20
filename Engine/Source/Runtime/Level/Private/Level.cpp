#include "pch.h"
#include "Runtime/Level/Public/Level.h"

#include "Runtime/Actor/Public/Actor.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Manager/Level/Public/LevelManager.h"

ULevel::ULevel() = default;

ULevel::ULevel(const FName& InName)
	: UObject(InName)
{
}

ULevel::~ULevel()
{
	for (auto Actor : LevelActors)
	{
		SafeDelete(Actor);
	}

	// Deprecated : EditorPrimitive는 에디터에서 처리
	// for (auto Actor : EditorActors)
	// {
	// 	SafeDelete(Actor);
	// }

	SafeDelete(CameraPtr);
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	uint64 AllocatedByte = GetAllocatedBytes();
	uint32 AllocatedCount = GetAllocatedCount();

	LevelPrimitiveComponents.clear();
	//Deprecated : EditorPrimitive는 에디터에서 처리
	//EditorPrimitiveComponents.clear();

	//AActor* pOldActor = nullptr;
	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			/*if (pOldActor != nullptr)
			{
				if (Actor->IsA(pOldActor->GetClass()))
				{
					UE_LOG("두개 같은 클래스");
				}
				else
				{
					UE_LOG("두개 다른 클래스");
				}
			}
			pOldActor = Actor;*/
			Actor->Tick();
			AddLevelPrimitiveComponent(Actor);
		}
	}

	//Deprecated : EditorPrimitive는 에디터에서 처리
	/*for (auto& Actor : EditorActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddEditorPrimitiveComponent(Actor);
		}
	}*/

}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			/* 3가지 경우 존재.
			1: primitive show flag가 꺼져 있으면, 도형, 빌보드 모두 렌더링 안함.
			2: primitive show flag가 켜져 있고, billboard show flag가 켜져 있으면, 도형, 빌보드 모두 렌더링
			3: primitive show flag가 켜져 있고, billboard show flag가 꺼져 있으면, 도형은 렌더링 하지만, 빌보드는 렌더링 안함. */
			// 빌보드는 무조건 피킹이 된 actor의 빌보드여야 렌더링 가능
			if (PrimitiveComponent->IsVisible() && (ShowFlags & EEngineShowFlags::SF_Primitives))
			{
				if (PrimitiveComponent->GetPrimitiveType() != EPrimitiveType::BillBoard)
				{
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
				else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard && (ShowFlags & EEngineShowFlags::SF_BillboardText) && (ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor() == Actor))
				{
					TObjectPtr<UBillBoardComponent> BillBoard = Cast<UBillBoardComponent>(PrimitiveComponent);
					BillBoard->UpdateRotationMatrix();
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
			}
		}
	}
}
//Deprecated : EditorPrimitive는 에디터에서 처리
//void ULevel::AddEditorPrimitiveComponent(AActor* Actor)
//{
//	if (!Actor) return;
//
//	for (auto& Component : Actor->GetOwnedComponents())
//	{
//		if (Component->GetComponentType() >= EComponentType::Primitive)
//		{
//			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
//			if (PrimitiveComponent->IsVisible())
//			{
//				EditorPrimitiveComponents.push_back(PrimitiveComponent);
//			}
//		}
//	}
//}

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
	//Gizmo->SetTargetActor(SelectedActor);
}

/**
 * @brief Level에서 Actor 제거하는 함수
 */
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// LevelActors 리스트에서 제거
	for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			LevelActors.erase(Iterator);
			break;
		}
	}

	//Deprecated : EditorPrimitive는 에디터에서 처리
	// 필요하다면 EditorActors 리스트에서도 제거
	/*for (auto Iterator = EditorActors.begin(); Iterator != EditorActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			EditorActors.erase(Iterator);
			break;
		}
	}*/

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		//Deprecated : Gizmo는 에디터에서 처리
		// Gizmo Target Release
		/*if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}*/
	}

	// Remove
	delete InActor;

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
}

/**
 * @brief Level에서 Actor를 실질적으로 제거하는 함수
 * 이전 Tick에서 마킹된 Actor를 제거한다
 */
void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("Level: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 대기 중인 액터들을 삭제
	for (AActor* ActorToDelete : ActorsToDelete)
	{
		if (!ActorToDelete)
			continue;

		// 혹시 남아있을 수 있는 참조 정리
		if (SelectedActor == ActorToDelete)
		{
			SelectedActor = nullptr;
		}

		// LevelActors 리스트에서 제거
		for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				LevelActors.erase(Iterator);
				break;
			}
		}

		FName DeletedActorName = ActorToDelete->GetName();

		// Release Memory
		delete ActorToDelete;
		UE_LOG("Level: Actor 제거: %s (%p)", DeletedActorName.ToString().data(), static_cast<void*>(ActorToDelete));
	}

	// Clear TArray
	ActorsToDelete.clear();
	UE_LOG("Level: 모든 지연 삭제 프로세스 완료");
}
