#include "pch.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"

#include "Runtime/Renderer/Public/BasePass.h"
#include "Runtime/Renderer/Public/DepthPrePass.h"
#include "Runtime/Renderer/Public/DebugPass.h"
#include "Runtime/Renderer/Public/RenderPass.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"
#include "Runtime/Renderer/Public/RHICommandList.h"

// 전역 RHI 인스턴스
URHIDevice* FSceneRenderer::GlobalRHI = nullptr;

FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily& InViewFamily)
{
    return new FSceneRenderer(InViewFamily);
}

FSceneRenderer::FSceneRenderer(const FSceneViewFamily& InViewFamily)
    : ViewFamily(&InViewFamily)
    , CommandList(nullptr)
{
    // RenderCommandList 생성
    if (GlobalRHI)
    {
        CommandList = new FRHICommandList(GlobalRHI);
    }

    CreateDefaultRenderPasses();
}

FSceneRenderer::~FSceneRenderer()
{
    Cleanup();
}

void FSceneRenderer::Render()
{
    if (!ViewFamily || !ViewFamily->IsValid() || !GlobalRHI)
    {
        return;
    }

    // ViewFamily에서 Views 추출
    const TArray<FSceneView*>& Views = ViewFamily->GetViews();

    for (FSceneView* SceneView : Views)
    {
        if (SceneView)
        {
            RenderView(SceneView);
        }
    }
}

void FSceneRenderer::RenderView(const FSceneView* InSceneView)
{
    if (!InSceneView || !CommandList)
    {
        return;
    }

    // CommandList 초기화
    CommandList->Clear();

    // 각 렌더 패스 실행
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass && Pass->IsEnabled())
        {
            Pass->Execute(InSceneView, this);
        }
    }

    // 10K+ 커맨드에서는 멀티스레딩 정렬로 성능 극대화
    if (CommandList->GetCommandCount() > 10000)
    {
        CommandList->ExecuteWithMultithreadedSorting();
    }
    else
    {
        CommandList->ExecuteWithMaterialSorting();
    }
}

void FSceneRenderer::Cleanup()
{
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass)
        {
            Pass->Cleanup();
            delete Pass;
        }
    }

    RenderPasses.Empty();

    // CommandList 정리
    if (CommandList)
    {
        delete CommandList;
        CommandList = nullptr;
    }
}

void FSceneRenderer::CreateDefaultRenderPasses()
{
    // 기본 렌더 패스들 생성 및 초기화
    FDepthPrePass* DepthPass = new FDepthPrePass();
    DepthPass->Initialize();
    RenderPasses.Add(DepthPass);

    FBasePass* BasePass = new FBasePass();
    BasePass->Initialize();
    RenderPasses.Add(BasePass);

    FDebugPass* DebugPass = new FDebugPass();
    DebugPass->Initialize();
    RenderPasses.Add(DebugPass);
}
