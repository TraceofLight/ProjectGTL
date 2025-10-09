#include "pch.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Utility/Public/Archive.h"

IMPLEMENT_CLASS(UWorld, UObject)

TObjectPtr<UWorld> GWorld = nullptr;

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

// ShowFlag 래퍼 함수들 구현 - Level의 ShowFlag를 사용
bool UWorld::IsShowFlagEnabled(EEngineShowFlags ShowFlag) const
{
	if (!Level)
	{
		// Level이 없으면 기본값으로 true 반환 (모든 것을 표시)
		return true;
	}

	uint64 LevelShowFlags = Level->GetShowFlags();
	return (LevelShowFlags & static_cast<uint64>(ShowFlag)) != 0;
}

void UWorld::SetShowFlag(EEngineShowFlags ShowFlag, bool bEnabled)
{
	if (!Level)
	{
		return;
	}

	uint64 LevelShowFlags = Level->GetShowFlags();

	if (bEnabled)
	{
		LevelShowFlags |= static_cast<uint64>(ShowFlag);
	}
	else
	{
		LevelShowFlags &= ~static_cast<uint64>(ShowFlag);
	}

	Level->SetShowFlags(LevelShowFlags);
}

uint64 UWorld::GetShowFlags() const
{
	if (!Level)
	{
		// Level이 없으면 기본값 반환 (모든 표시 옵션 활성화)
		return static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
			static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
			static_cast<uint64>(EEngineShowFlags::SF_Bounds) |
			static_cast<uint64>(EEngineShowFlags::SF_Grid) |
			static_cast<uint64>(EEngineShowFlags::SF_StaticMeshes) |
			static_cast<uint64>(EEngineShowFlags::SF_BoundingBoxes);
	}

	return Level->GetShowFlags();
}

void UWorld::SetShowFlags(uint64 InShowFlags)
{
	if (!Level)
	{
		return;
	}

	Level->SetShowFlags(InShowFlags);
}
