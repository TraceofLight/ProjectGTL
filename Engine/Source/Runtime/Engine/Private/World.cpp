#include "pch.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Utility/Public/Archive.h"

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
{
	UE_LOG("UWorld: Created");
}

UWorld::UWorld(EWorldType InWorldType)
	: WorldType(InWorldType)
{
	UE_LOG("UWorld: Created with WorldType: %s", EnumToString(InWorldType));
}

UWorld::~UWorld()
{
	CleanupWorld();
	UE_LOG("UWorld: Deleted WorldType: %s", EnumToString(WorldType));
}

/**
 * @brief World의 Tick 함수
 * 현재 가진 Level의 Actor의 Tick을 순차 호출한다
 */
void UWorld::Tick(float DeltaSeconds) const
{
	if (!Level)
	{
		return;
	}

	const TArray<TObjectPtr<AActor>>& Actors = Level->GetLevelActors();

	for (const auto& Actor : Actors)
	{
		if (Actor && Actor->IsActorTickEnabled())
		{
			Actor->Tick(DeltaSeconds);
		}
	}
}

/**
 * @brief PIE용 월드 복제를 위한 함수
 * EditorWorld -> PIEWorld 변환 시 사용
 */
TObjectPtr<UWorld> UWorld::DuplicateWorldForPIE(UWorld* InSourceWorld)
{
	// TBD
	return nullptr;
}

/**
 * @brief Actor ~ World까지 정리하는 함수
 */
void UWorld::CleanupWorld() const
{
	// TBD
}

/**
 * @brief PIE 시작할 때 모든 액터의 BeginPlay 호출하기 위한 함수
 */
void UWorld::InitializeActorsForPlay() const
{
	if (!Level)
	{
		UE_LOG_INFO("UWorld: Level이 nullptr입니다");
		return;
	}

	const TArray<TObjectPtr<AActor>>& Actors = Level->GetLevelActors();
	for (const auto& Actor : Actors)
	{
		if (Actor)
		{
			Actor->BeginPlay();
		}
	}
}
