#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"

class UEditor;
class ULevel;
struct FLevelMetadata;

/**
 * @brief World / Level 관리를 담당하는 엔진 서브시스템
 */
UCLASS()
class UWorldSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UWorldSubsystem, UEngineSubsystem)

public:
	void Initialize() override;
	void PostInitialize() override;
	void Deinitialize() override;
	bool IsTickable() const override;
	void Tick() override;

	void CreateAndRegisterLevel();
	void RegisterLevel(const FName& InName, TObjectPtr<ULevel> InLevel);
	void LoadLevel(const FName& InName);
	void Shutdown();

	TObjectPtr<ULevel> GetCurrentLevel() const { return CurrentLevel; }

	void ClearCurrentLevel();

	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);
	bool CreateNewLevel();
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	static FLevelMetadata ConvertLevelToMetadata(TObjectPtr<ULevel> InLevel);
	static bool LoadLevelFromMetadata(TObjectPtr<ULevel> InLevel, const FLevelMetadata& InMetadata);

	UEditor* GetEditor() const { return Editor.Get(); }

private:
	TObjectPtr<ULevel> CurrentLevel = nullptr;
	TMap<FName, TObjectPtr<ULevel>> Levels;
	TUniquePtr<UEditor> Editor = nullptr;

	static void RestoreCameraFromMetadata(const struct FCameraMetadata& InCameraMetadata);
};
