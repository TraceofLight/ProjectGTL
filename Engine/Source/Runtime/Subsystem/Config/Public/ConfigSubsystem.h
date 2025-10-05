#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/Name.h"
#include "Runtime/Subsystem/Public/EngineSubsystem.h"

UCLASS()
class UConfigSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UConfigSubsystem, UEngineSubsystem)

public:
	UConfigSubsystem();
	~UConfigSubsystem() override;

	// UEngineSubsystem 인터페이스
	void Initialize() override;
	void Deinitialize() override;
	void SaveEditorSetting();
	void LoadEditorSetting();

	float GetCellSize() const
	{
		return CellSize;
	}

	float GetCameraSensitivity() const
	{
		return CameraSensitivity;
	}

	void SetCellSize(const float cellSize)
	{
		CellSize = cellSize;
	}

	void SetCameraSensitivity(const float cameraSensitivity)
	{
		CameraSensitivity = cameraSensitivity;
	}

	float GetSplitV() const { return SplitV; }
	float GetSplitH() const { return SplitH; }
	void  SetSplitV(float v) { SplitV = v; }
	void  SetSplitH(float v) { SplitH = v; }

private:
	FName EditorIniFileName;
	float CellSize;
	float CameraSensitivity;

	float SplitV = 0.5f;
	float SplitH = 0.5f;
};
