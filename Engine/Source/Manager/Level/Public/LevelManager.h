#pragma once
#include "Runtime/Core/Public/Object.h"

class UEditor;
class ULevel;
struct FLevelMetadata;

UCLASS()
class ULevelManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(ULevelManager, UObject)

public:
	void Update() const;
	void CreateDefaultLevel();
	void RegisterLevel(const FName& InName, TObjectPtr<ULevel> InLevel);
	void LoadLevel(const FName& InName);
	void Shutdown();

	// Getter
	TObjectPtr<ULevel> GetCurrentLevel() const { return CurrentLevel; }

	// Level Management
	void ClearCurrentLevel();

	// Save & Load System
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);
	bool CreateNewLevel();
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	// Metadata Conversion Functions
	static FLevelMetadata ConvertLevelToMetadata(TObjectPtr<ULevel> InLevel);
	static bool LoadLevelFromMetadata(TObjectPtr<ULevel> InLevel, const FLevelMetadata& InMetadata);

	TObjectPtr<UEditor> GetEditor() const { return Editor; }

private:
	TObjectPtr<ULevel> CurrentLevel;
	TMap<FName, TObjectPtr<ULevel>> Levels;
	TObjectPtr<UEditor> Editor;

	// Helper Functions
	void RestoreCameraFromMetadata(const struct FCameraMetadata& InCameraMetadata);
};
