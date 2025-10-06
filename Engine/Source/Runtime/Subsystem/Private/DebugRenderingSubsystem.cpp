#include "pch.h"
#include "Runtime/Subsystem/Public/DebugRenderingSubsystem.h"
#include "Renderer/RenderCommand/Public/RHICommandList.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "Runtime/Core/Public/EngineStatics.h"
#include "Global/Memory.h"
#include "Runtime/Engine/Public/Engine.h"

#include <psapi.h>

// Memory update interval (언리얼 스타일)
static constexpr float MEMORY_UPDATE_INTERVAL = 1.0f;

// Overlay display position (언리얼 스타일)
static constexpr float OVERLAY_MARGIN_X = 20.0f;
static constexpr float OVERLAY_MARGIN_Y = 60.0f;
static constexpr float OVERLAY_LINE_HEIGHT = 25.0f;

// Debug display type names (언리얼 stat 시스템 스타일)
static TArray<FString> DebugDisplayTypeNames = {
    "None", "FPS", "Memory", "All"
};

IMPLEMENT_CLASS(UDebugRenderingSubsystem, UEngineSubsystem)

//=============================================================================
// Constructor & Destructor
//=============================================================================

UDebugRenderingSubsystem::UDebugRenderingSubsystem()
{
    UE_LOG("UDebugRenderingSubsystem: 디버그 렌더링 서브시스템 생성");
}

//=============================================================================
// USubsystem Interface
//=============================================================================

void UDebugRenderingSubsystem::Initialize()
{
    UE_LOG("UDebugRenderingSubsystem: 초기화 시작");
    Super::Initialize();

    // 지연 초기화: RHI가 준비될 때까지 Direct2D 초기화를 연기
    UE_LOG("UDebugRenderingSubsystem: RHI 기반 지연 초기화 방식 사용");
    UE_LOG_SUCCESS("UDebugRenderingSubsystem: 기본 초기화 완료");
}

void UDebugRenderingSubsystem::Deinitialize()
{
    ReleaseDirectWrite();
    ReleaseRHIResources();

    Super::Deinitialize();
    UE_LOG_SUCCESS("UDebugRenderingSubsystem: 정리 완료");
}

void UDebugRenderingSubsystem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdatePerformanceStats();
    UpdateMemoryStats();
}

//=============================================================================
// Debug Display Control (언리얼 stat 시스템 스타일)
//=============================================================================

void UDebugRenderingSubsystem::SetDebugDisplayType(EDebugDisplayType InType)
{
    CurrentDisplayType = InType;

    UE_LOG("UDebugRenderingSubsystem: 디버그 표시 타입 변경: %s",
           DebugDisplayTypeNames[static_cast<int>(InType)].data());
}

void UDebugRenderingSubsystem::ProcessStatCommand(const FString& InCommand)
{
    FString CommandLower = InCommand;
    std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower);

    if (CommandLower == "stat fps")
    {
        if (CurrentDisplayType == EDebugDisplayType::Memory)
        {
            SetDebugDisplayType(EDebugDisplayType::All);
        }
        else
        {
            SetDebugDisplayType(EDebugDisplayType::FPS);
        }
    }
    else if (CommandLower == "stat memory")
    {
        if (CurrentDisplayType == EDebugDisplayType::FPS)
        {
            SetDebugDisplayType(EDebugDisplayType::All);
        }
        else
        {
            SetDebugDisplayType(EDebugDisplayType::Memory);
        }
    }
    else if (CommandLower == "stat all")
    {
        SetDebugDisplayType(EDebugDisplayType::All);
    }
    else if (CommandLower == "stat none")
    {
        SetDebugDisplayType(EDebugDisplayType::None);
    }
}

//=============================================================================
// RHI-based Rendering (언리얼 스타일)
//=============================================================================

void UDebugRenderingSubsystem::RenderDebugOverlay(FRHICommandList* InCommandList)
{
    if (CurrentDisplayType == EDebugDisplayType::None || !InCommandList)
    {
        return;
    }

    // 지연 초기화: 처음 렌더링 시점에 RHI 리소스 초기화
    if (!D2DRenderTarget)
    {
        static bool bInitializationAttempted = false;
        if (!bInitializationAttempted)
        {
            UE_LOG("UDebugRenderingSubsystem: RHI 기반 지연 초기화 시도");
            bInitializationAttempted = true;

            if (!InitializeRHIResources(InCommandList))
            {
                UE_LOG_ERROR("UDebugRenderingSubsystem: RHI 리소스 초기화 실패");
                return;
            }

            if (!InitializeDirectWrite())
            {
                UE_LOG_ERROR("UDebugRenderingSubsystem: DirectWrite 초기화 실패");
                return;
            }

            UE_LOG_SUCCESS("UDebugRenderingSubsystem: RHI 기반 초기화 성공");
        }
        else
        {
            return; // 이미 초기화를 시도했지만 실패한 경우
        }
    }

    if (!D2DRenderTarget)
    {
        return;
    }

    // Direct2D 렌더링 시작
    D2DRenderTarget->BeginDraw();

    switch (CurrentDisplayType)
    {
    case EDebugDisplayType::FPS:
        RenderFPSOverlay();
        break;

    case EDebugDisplayType::Memory:
        RenderMemoryOverlay();
        break;

    case EDebugDisplayType::All:
        RenderFPSOverlay();
        RenderMemoryOverlay();
        break;

    default:
        break;
    }

    HRESULT hr = D2DRenderTarget->EndDraw();
    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: Direct2D 렌더링 실패: 0x%08lX", hr);
        if (hr == D2DERR_RECREATE_TARGET)
        {
            UE_LOG("UDebugRenderingSubsystem: 렌더 타겟 재생성 필요");
        }
    }
}

//=============================================================================
// RHI Resource Management
//=============================================================================

bool UDebugRenderingSubsystem::InitializeRHIResources(FRHICommandList* InCommandList)
{
    UE_LOG("UDebugRenderingSubsystem: RHI 리소스 초기화 시작");

    // Direct2D Factory 생성
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: D2D1Factory 생성 실패: 0x%08lX", hr);
        return false;
    }

    // RHI CommandList를 통해 BackBuffer Surface 획득 (언리얼 스타일)
    IDXGISurface* BackBufferSurface = InCommandList->GetBackBufferSurface();
    if (!BackBufferSurface)
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: BackBuffer Surface를 찾을 수 없습니다");
        return false;
    }

    // Direct2D RenderTarget 속성 설정
    D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f
    );

    // DXGI Surface와 연결된 Direct2D RenderTarget 생성
    hr = D2DFactory->CreateDxgiSurfaceRenderTarget(
        BackBufferSurface,
        &RenderTargetProperties,
        &D2DRenderTarget
    );

    // Surface 해제
    BackBufferSurface->Release();

    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: DXGI Surface RenderTarget 생성 실패: 0x%08lX", hr);
        return false;
    }

    // 텍스트 브러시 생성
    hr = D2DRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        &TextBrush
    );

    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: TextBrush 생성 실패: 0x%08lX", hr);
        return false;
    }

    UE_LOG_SUCCESS("UDebugRenderingSubsystem: RHI 리소스 초기화 완료");
    return true;
}

bool UDebugRenderingSubsystem::InitializeDirectWrite()
{
    UE_LOG("UDebugRenderingSubsystem: DirectWrite 초기화 시작");

    // DirectWrite Factory 생성
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&DWriteFactory)
    );

    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: DWriteFactory 생성 실패: 0x%08lX", hr);
        return false;
    }

    // 텍스트 포맷 생성
    hr = DWriteFactory->CreateTextFormat(
        L"Arial",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"",
        &TextFormat
    );

    if (FAILED(hr))
    {
        UE_LOG_ERROR("UDebugRenderingSubsystem: TextFormat 생성 실패: 0x%08lX", hr);
        return false;
    }

    UE_LOG_SUCCESS("UDebugRenderingSubsystem: DirectWrite 초기화 완료");
    return true;
}

void UDebugRenderingSubsystem::ReleaseRHIResources()
{
    if (TextBrush)
    {
        TextBrush->Release();
        TextBrush = nullptr;
    }

    if (D2DRenderTarget)
    {
        D2DRenderTarget->Release();
        D2DRenderTarget = nullptr;
    }

    if (D2DFactory)
    {
        D2DFactory->Release();
        D2DFactory = nullptr;
    }
}

void UDebugRenderingSubsystem::ReleaseDirectWrite()
{
    if (TextFormat)
    {
        TextFormat->Release();
        TextFormat = nullptr;
    }

    if (DWriteFactory)
    {
        DWriteFactory->Release();
        DWriteFactory = nullptr;
    }
}

//=============================================================================
// Statistics Update (FStat 스타일)
//=============================================================================

void UDebugRenderingSubsystem::UpdatePerformanceStats()
{
    float DeltaSeconds = DT;

    if (DeltaSeconds > 0.0f)
    {
        // Frame time 샘플링
        FrameTimeSamples[FrameTimeSampleIndex] = DeltaSeconds;
        FrameTimeSampleIndex = (FrameTimeSampleIndex + 1) % FPS_SAMPLE_COUNT;

        // 평균 FPS 계산
        float AverageFrameTime = 0.0f;
        for (int32 i = 0; i < FPS_SAMPLE_COUNT; ++i)
        {
            AverageFrameTime += FrameTimeSamples[i];
        }
        AverageFrameTime /= static_cast<float>(FPS_SAMPLE_COUNT);

        CurrentFPS = AverageFrameTime > 0.0f ? (1.0f / AverageFrameTime) : 0.0f;
    }
}

void UDebugRenderingSubsystem::UpdateMemoryStats()
{
    MemoryUpdateTimer += DT;

    if (MemoryUpdateTimer >= MEMORY_UPDATE_INTERVAL)
    {
        MemoryUpdateTimer = 0.0f;

        // 프로세스 메모리 사용량 가져오기
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(),
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                               sizeof(pmc)))
        {
            CurrentMemoryUsage = pmc.WorkingSetSize;
            PeakMemoryUsage = std::max(PeakMemoryUsage, CurrentMemoryUsage);
        }

        // 커스텀 메모리 할당 정보 (FMemory 클래스가 없으므로 임시로 0으로 설정)
        // TODO: 실제 메모리 할당 추적 시스템 구현 필요
        CurrentAllocationCount = 0;
        CurrentAllocationBytes = 0;
    }
}

//=============================================================================
// Rendering Methods
//=============================================================================

void UDebugRenderingSubsystem::RenderFPSOverlay() const
{
    if (!D2DRenderTarget || !TextBrush || !TextFormat)
        return;

    wchar_t FPSText[256];
    swprintf_s(FPSText, L"FPS: %.1f", CurrentFPS);

    uint32 FPSColor = GetFPSColor(CurrentFPS);
    DrawText(FPSText, OVERLAY_MARGIN_X, OVERLAY_MARGIN_Y, FPSColor);
}

void UDebugRenderingSubsystem::RenderMemoryOverlay() const
{
    if (!D2DRenderTarget || !TextBrush || !TextFormat)
        return;

    float YOffset = OVERLAY_MARGIN_Y + (CurrentDisplayType == EDebugDisplayType::All ? OVERLAY_LINE_HEIGHT : 0.0f);

    // 프로세스 메모리
    FString ProcessMemStr = GetMemorySizeString(CurrentMemoryUsage);
    FString PeakMemStr = GetMemorySizeString(PeakMemoryUsage);

    wchar_t MemoryText[512];
    (void)swprintf_s(MemoryText, L"Memory: %ls (Peak: %ls)",
               ProcessMemStr.data(), PeakMemStr.data());

    DrawText(MemoryText, OVERLAY_MARGIN_X, YOffset, 0xFFFFFFFF);

    // 커스텀 할당 정보
    YOffset += OVERLAY_LINE_HEIGHT;
    FString AllocBytesStr = GetMemorySizeString(CurrentAllocationBytes);

    wchar_t AllocText[512];
    (void)swprintf_s(AllocText, L"Allocations: %u (%ls)",
               CurrentAllocationCount, AllocBytesStr.c_str());

    DrawText(AllocText, OVERLAY_MARGIN_X, YOffset, 0xFFAAAAAA);
}

void UDebugRenderingSubsystem::DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor) const
{
    if (!D2DRenderTarget || !TextBrush || !TextFormat || !InText)
        return;

    // 색상 설정
    float R = ((InColor >> 16) & 0xFF) / 255.0f;
    float G = ((InColor >> 8) & 0xFF) / 255.0f;
    float B = (InColor & 0xFF) / 255.0f;
    float A = ((InColor >> 24) & 0xFF) / 255.0f;

    TextBrush->SetColor(D2D1::ColorF(R, G, B, A));

    // 텍스트 렌더링
    D2D1_RECT_F layoutRect = D2D1::RectF(InX, InY, InX + 400.0f, InY + 30.0f);
    D2DRenderTarget->DrawTextW(InText, static_cast<UINT>(wcslen(InText)),
                               TextFormat, layoutRect, TextBrush);
}

//=============================================================================
// Utility Functions
//=============================================================================

FString UDebugRenderingSubsystem::GetMemorySizeString(SIZE_T InBytes)
{
    // FString::Printf가 없으므로 표준 C++ 방식 사용
    char buffer[64];
    
    if (InBytes >= 1024 * 1024 * 1024) // GB
    {
        sprintf_s(buffer, "%.2f GB", static_cast<double>(InBytes) / (1024.0 * 1024.0 * 1024.0));
    }
    else if (InBytes >= 1024 * 1024) // MB
    {
        sprintf_s(buffer, "%.2f MB", static_cast<double>(InBytes) / (1024.0 * 1024.0));
    }
    else if (InBytes >= 1024) // KB
    {
        sprintf_s(buffer, "%.2f KB", static_cast<double>(InBytes) / 1024.0);
    }
    else // Bytes
    {
        sprintf_s(buffer, "%llu B", InBytes);
    }
    
    return FString(buffer);
}

uint32 UDebugRenderingSubsystem::GetFPSColor(float InFPS)
{
    if (InFPS >= 60.0f)
    {
        return 0xFF00FF00; // 녹색 (좋음)
    }
    else if (InFPS >= 30.0f)
    {
        return 0xFFFFFF00; // 노란색 (보통)
    }
    else
    {
        return 0xFFFF0000; // 빨간색 (나쁨)
    }
}
