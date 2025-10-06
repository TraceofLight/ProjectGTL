#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"
#include <d2d1.h>
#include <dwrite.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

/**
 * @brief 오버레이 표시 타입 열거형
 */
enum class EOverlayType : uint8
{
	None = 0, // 오버레이 없음
	FPS = 1, // FPS 표시
	Memory = 2, // 메모리 정보 표시
	All = 3 // 모든 정보 표시 (FPS + Memory)
};

/**
 * @brief DX2D를 활용한 오버레이 매니저 서브시스템
 * stat 명령어를 통해 FPS, 메모리 정보를 화면에 오버레이로 표시
 */
UCLASS()
class UOverlayManagerSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UOverlayManagerSubsystem, UEngineSubsystem)

public:
	// USubsystem interface
	void Initialize() override;
	void Deinitialize() override;
	void Tick(float DeltaSeconds) override;
	bool IsTickable() const override { return true; }

	// Overlay control
	void SetOverlayType(EOverlayType InType);
	EOverlayType GetOverlayType() const { return CurrentOverlayType; }

	// Command process
	void ProcessStatCommand(const FString& InCommand);

	// Render function
	void RenderOverlay();

	// Special member function
	UOverlayManagerSubsystem();

private:
	// Overlay type
	EOverlayType CurrentOverlayType = EOverlayType::None;

	// Direct2D resources
	ID2D1Factory* D2DFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* TextBrush = nullptr;

	// DirectWrite resources
	IDWriteFactory* DWriteFactory = nullptr;
	IDWriteTextFormat* TextFormat = nullptr;

	// FPS variable
	float CurrentFPS = 0.0f;
	static constexpr int32 FPS_SAMPLE_COUNT = 60;
	float FrameSpeedSamples[FPS_SAMPLE_COUNT] = {};
	int32 FrameSpeedSampleIndex = 0;

	// memory variable
	SIZE_T CurrentMemoryUsage = 0;
	SIZE_T PeakMemoryUsage = 0;
	uint32 CurrentAllocationCount = 0;
	uint32 CurrentAllocationBytes = 0;
	float MemoryUpdateTimer = 0.0f;

	// DX function
	bool InitializeDirect2D();
	bool InitializeDirectWrite();
	void ReleaseDirect2D();
	void ReleaseDirectWrite();

	// Update overlays
	void UpdateFPS();
	void UpdateMemoryInfo();

	// Render helper function
	void RenderFPSOverlay() const;
	void RenderMemoryOverlay() const;
	void DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor = 0xFFFFFFFF) const;

	static FString GetMemorySizeString(SIZE_T InBytes);
	static uint32 GetFPSColor(float InFPS);
};
