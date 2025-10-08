#include "pch.h"
#include "Runtime/Renderer/Public/RendererModule.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"
#include "Runtime/Core/Public/ModuleManager.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

void FRendererModule::StartupModule()
{
    UE_LOG("FRendererModule: StartupModule 시작");

    InitializeRenderingResources();

    UE_LOG("FRendererModule: StartupModule 완료");
}

void FRendererModule::ShutdownModule()
{
    UE_LOG("FRendererModule: ShutdownModule 시작");

    CleanupRenderingResources();

    // GlobalRHI 정리
    FSceneRenderer::SetGlobalRHI(nullptr);
    RHIDevice = nullptr;

    UE_LOG("FRendererModule: ShutdownModule 완료");
}

void FRendererModule::SetRHIDevice(URHIDevice* InRHIDevice)
{
    RHIDevice = InRHIDevice;

    // GlobalRHI로 설정하여 기존 코드와의 호환성 유지
    FSceneRenderer::SetGlobalRHI(RHIDevice);

    // EditorRenderResources 초기화
    if (RHIDevice)
    {
        EditorResources = std::make_unique<FEditorRenderResources>();
        EditorResources->Initialize(RHIDevice);
    }

    UE_LOG("FRendererModule: RHIDevice 설정 완료");
}

void FRendererModule::BeginRendering()
{
    if (!RHIDevice)
    {
        UE_LOG_ERROR("FRendererModule: BeginRendering 실패 - RHIDevice가 null입니다");
        return;
    }

    if (bIsRenderingActive)
    {
        UE_LOG_WARNING("FRendererModule: 렌더링이 이미 활성화되어 있습니다");
        return;
    }

    bIsRenderingActive = true;

    // 렌더링 시작 로직 (필요시 추가)
}

void FRendererModule::EndRendering()
{
    if (!bIsRenderingActive)
    {
        return;
    }

    bIsRenderingActive = false;

    // 렌더링 종료 로직 (필요시 추가)
}

FRendererModule& FRendererModule::Get()
{
    return FModuleManager::Get().LoadModuleChecked<FRendererModule>("Renderer");
}

void FRendererModule::InitializeRenderingResources()
{
    // 렌더링 리소스 초기화 로직
    // 필요시 추가 구현
}

void FRendererModule::RenderGizmoPrimitive(const FEditorPrimitive& Primitive, const FRenderState& RenderState,
                                          const FVector& CameraLocation, float ViewportWidth, float ViewportHeight)
{
    // 기즈모 프리미티브 렌더링 구현
    // 실제 기즈모 렌더링 로직이 필요하지만
    // 현재는 링킹 오류만 해결하기 위해 빈 구현으로 남겨둘

    UE_LOG("FRendererModule::RenderGizmoPrimitive - 호출되었지만 렌더링 로직이 구현되지 않음");
}

void FRendererModule::CleanupRenderingResources()
{
    EndRendering();

    // EditorRenderResources 정리
    if (EditorResources)
    {
        EditorResources->Shutdown();
        EditorResources.reset();
    }

    // 렌더링 리소스 정리 로직
    // 필요시 추가 구현
}
