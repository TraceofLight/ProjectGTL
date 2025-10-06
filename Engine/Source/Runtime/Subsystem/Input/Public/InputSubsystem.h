#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"

class FAppWindow;

/**
 * @brief 입력 처리를 담당하는 엔진 서브시스템
 * 키보드, 마우스 입력을 관리하고 더블클릭 감지 등의 기능을 제공
 */
UCLASS()
class UInputSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UInputSubsystem, UEngineSubsystem)

public:
	void Initialize() override;
	void Deinitialize() override;

	void PrepareNewFrame();
	bool IsTickable() const override { return true; }
	void Tick(float DeltaSeconds) override;

	void UpdateMousePosition(const FAppWindow* InWindow);
	void ProcessKeyMessage(uint32 InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	// Key Status
	EKeyStatus GetKeyStatus(EKeyInput InKey) const;
	const TArray<EKeyInput>& GetKeysByStatus(EKeyStatus InStatus);

	const TArray<EKeyInput>& GetPressedKeys();
	const TArray<EKeyInput>& GetNewlyPressedKeys();
	const TArray<EKeyInput>& GetReleasedKeys();

	// Window Focus Management
	void SetWindowFocus(bool bInFocused);
	bool IsWindowFocused() const { return bIsWindowFocused; }

	// Helper Function
	static FString KeyInputToString(EKeyInput InKey);

	// Mouse Wheel
	float GetMouseWheelDelta() const { return MouseWheelDelta; }
	void SetMouseWheelDelta(float InMouseWheelDelta) { MouseWheelDelta = InMouseWheelDelta; }

	// Double Click Detection
	bool IsMouseDoubleClicked(EKeyInput InMouseButton) const;

	// Getter
	const FVector& GetMouseNDCPosition() const { return NDCMousePosition; }
	const FVector& GetMousePosition() const { return CurrentMousePosition; }
	const FVector& GetMouseDelta() const { return MouseDelta; }

private:
	// Key Status
	TMap<EKeyInput, bool> CurrentKeyState;
	TMap<EKeyInput, bool> PreviousKeyState;
	TMap<int32, EKeyInput> VirtualKeyMap;
	TArray<EKeyInput> KeysInStatus;

	// Mouse Position
	FVector CurrentMousePosition;
	FVector PreviousMousePosition;
	FVector MouseDelta;

	// Mouse Wheel
	float MouseWheelDelta;

	// NDC Mouse Position
	FVector NDCMousePosition;

	// Window Focus
	bool bIsWindowFocused;

	// Double Click Detection
	float DoubleClickTime;
	TMap<EKeyInput, float> LastClickTime;
	TMap<EKeyInput, bool> DoubleClickState;
	TMap<EKeyInput, int> ClickCount;

	void InitializeKeyMapping();
	void InitializeMouseClickStatus();
	void UpdateDoubleClickDetection();
};
