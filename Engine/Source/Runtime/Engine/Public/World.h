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

private:
	TObjectPtr<ULevel> Level = nullptr;
	EWorldType WorldType = EWorldType::None;
};
