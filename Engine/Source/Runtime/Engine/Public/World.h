#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Engine/Public/WorldTypes.h"
#include "Runtime/Level/Public/Level.h"

/**
 * @brief 게임 월드를 관리하는 핵심 클래스
 */
UCLASS()
class UWorld :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UWorld, UObject)

public:
	UWorld();
	explicit UWorld(EWorldType InWorldType);
	~UWorld() override;

	void Tick(float DeltaSeconds) const;

	static TObjectPtr<UWorld> DuplicateWorldForPIE(UWorld* InSourceWorld);
	void InitializeActorsForPlay() const;
	void CleanupWorld() const;

	// Getter & Setter
	EWorldType GetWorldType() const { return WorldType; }
	void SetWorldType(EWorldType InWorldType) { WorldType = InWorldType; }

	TObjectPtr<ULevel> GetLevel() const { return Level; }
	void SetLevel(TObjectPtr<ULevel> InLevel) { Level = InLevel; }

	bool IsPIEWorld() const { return WorldType == EWorldType::PIE; }
	bool IsEditorWorld() const { return WorldType == EWorldType::Editor; }
	bool IsGameWorld() const { return WorldType == EWorldType::Game; }

	// 시간 관리 (언리얼 엔진 스타일)
	float GetTimeSeconds() const { return CurrentTimeSeconds; }
	float GetDeltaSeconds() const { return DeltaTimeSeconds; }
	void SetTimeSeconds(float InTime) { CurrentTimeSeconds = InTime; }
	void SetDeltaSeconds(float InDelta) { DeltaTimeSeconds = InDelta; }

	// ShowFlag 래퍼 함수들 - Level의 ShowFlag를 사용
	bool IsShowFlagEnabled(EEngineShowFlags ShowFlag) const;
	void SetShowFlag(EEngineShowFlags ShowFlag, bool bEnabled);
	uint64 GetShowFlags() const;
	void SetShowFlags(uint64 InShowFlags);

private:
	TObjectPtr<ULevel> Level = nullptr;
	EWorldType WorldType = EWorldType::None;

	// 시간 관리 변수들
	float CurrentTimeSeconds = 0.0f;
	float DeltaTimeSeconds = 0.0f;
};

extern TObjectPtr<UWorld> GWorld;

// GWorld Setter
inline void SetWorld(TObjectPtr<UWorld> InWorld) { GWorld = InWorld; }
