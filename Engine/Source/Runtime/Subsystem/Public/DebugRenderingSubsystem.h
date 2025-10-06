#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"
#include <d2d1.h>
#include <dwrite.h>

/**
 * @brief 디버그 정보 표시 타입 (언리얼 스타일)
 */
enum class EDebugDisplayType : uint8
{
    None = 0,       // 디버그 정보 없음
    FPS = 1,        // FPS 정보 표시
    Memory = 2,     // 메모리 정보 표시
    All = 3         // 모든 디버그 정보 표시
};

/**
 * @brief 언리얼 엔진 스타일의 디버그 렌더링 서브시스템
 * RHI를 통한 추상화된 접근으로 디버그 오버레이를 표시
 *
 * 언리얼 엔진의 FDebugDrawService 및 stat 시스템을 모방하여 구현
 */
UCLASS()
class UDebugRenderingSubsystem :
public UEngineSubsystem
{
    GENERATED_BODY()
    DECLARE_CLASS(UDebugRenderingSubsystem, UEngineSubsystem)

public:
    // USubsystem interface
    void Initialize() override;
    void Deinitialize() override;
    void Tick(float DeltaSeconds) override;
    bool IsTickable() const override { return true; }

    // Debug display control (언리얼 stat 시스템 스타일)
    void SetDebugDisplayType(EDebugDisplayType InType);
    EDebugDisplayType GetDebugDisplayType() const { return CurrentDisplayType; }

    // Stat command processing (언리얼 스타일)
    void ProcessStatCommand(const FString& InCommand);

    // RHI-based rendering (언리얼 스타일)
    void RenderDebugOverlay(class FRHICommandList* InCommandList);

    // Constructor
    UDebugRenderingSubsystem();

private:
    // Current display settings
    EDebugDisplayType CurrentDisplayType = EDebugDisplayType::None;

    // Direct2D resources (호환성을 위해 유지)
    ID2D1Factory* D2DFactory = nullptr;
    ID2D1RenderTarget* D2DRenderTarget = nullptr;
    ID2D1SolidColorBrush* TextBrush = nullptr;

    // DirectWrite resources
    IDWriteFactory* DWriteFactory = nullptr;
    IDWriteTextFormat* TextFormat = nullptr;

    // Performance statistics (FStat 시스템 스타일)
    float CurrentFPS = 0.0f;
    static constexpr int32 FPS_SAMPLE_COUNT = 60;
    float FrameTimeSamples[FPS_SAMPLE_COUNT] = {};
    int32 FrameTimeSampleIndex = 0;

    // Memory statistics
    SIZE_T CurrentMemoryUsage = 0;
    SIZE_T PeakMemoryUsage = 0;
    uint32 CurrentAllocationCount = 0;
    uint32 CurrentAllocationBytes = 0;
    float MemoryUpdateTimer = 0.0f;

    // RHI-based initialization
    bool InitializeRHIResources(class FRHICommandList* InCommandList);
    bool InitializeDirectWrite();
    void ReleaseRHIResources();
    void ReleaseDirectWrite();

    // Statistics update (FStat 스타일)
    void UpdatePerformanceStats();
    void UpdateMemoryStats();

    // RHI-based rendering methods
    void RenderFPSOverlay() const;
    void RenderMemoryOverlay() const;
    void DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor = 0xFFFFFFFF) const;

    // Utility functions
    static FString GetMemorySizeString(SIZE_T InBytes);
    static uint32 GetFPSColor(float InFPS);
};
