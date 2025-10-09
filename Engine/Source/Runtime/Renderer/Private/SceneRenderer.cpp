#include "pch.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"

#include "Runtime/Renderer/Public/BasePass.h"
#include "Runtime/Renderer/Public/DepthPrePass.h"
#include "Runtime/Renderer/Public/DebugPass.h"
#include "Runtime/Renderer/Public/RenderPass.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"
#include "Runtime/Renderer/Public/RHICommandList.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"
#include "Runtime/Renderer/Public/RenderGizmoPrimitiveCommand.h"

FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily& InViewFamily)
{
	return new FSceneRenderer(InViewFamily);
}

FSceneRenderer::FSceneRenderer(const FSceneViewFamily& InViewFamily)
	: ViewFamily(&InViewFamily)
	  , CommandList(nullptr)
{
	// RenderCommandList 생성
	if (GDynamicRHI)
	{
		CommandList = new FRHICommandList(GDynamicRHI);
	}

	CreateDefaultRenderPasses();
}

FSceneRenderer::~FSceneRenderer()
{
	Cleanup();
}

void FSceneRenderer::Render()
{
	if (!ViewFamily || !ViewFamily->IsValid() || !GDynamicRHI)
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
	if (!InSceneView || !CommandList || !GDynamicRHI)
	{
		return;
	}

	// CommandList 초기화
	CommandList->Clear();

	// 렌더링 시작 전, 뷰포트 설정 및 덷스/스텐실 클리어
	const FRect& ViewRect = InSceneView->GetViewRect();

	CommandList->SetViewport(static_cast<float>(ViewRect.X), static_cast<float>(ViewRect.Y),
	                         static_cast<float>(ViewRect.W), static_cast<float>(ViewRect.H), 0.0f, 1.0f);

	// Scissor Rect 설정으로 뷰포트 경계 밖 렌더링 방지
	GDynamicRHI->SetScissorRect(ViewRect.X, ViewRect.Y, ViewRect.X + ViewRect.W, ViewRect.Y + ViewRect.H);

	GDynamicRHI->ClearDepthStencilView(1.0f, 0);

	// 각 렌더 패스 실행
	for (IRenderPass* Pass : RenderPasses)
	{
		if (Pass && Pass->IsEnabled())
		{
			Pass->Execute(InSceneView, this);
		}
	}

	// 멀티스레딩 정렬로 성능 극대화
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

void FSceneRenderer::EnqueueGizmoPrimitive(const FEditorPrimitive& Primitive, const FRenderState& RenderState,
                                           const FVector& CameraLocation, float ViewportWidth, float ViewportHeight)
{
	if (!CommandList || !GDynamicRHI)
	{
		UE_LOG_ERROR("EnqueueGizmoPrimitive: CommandList 또는 RHI가 유효하지 않습니다");
		return;
	}

	// RenderCommand를 CommandList에 추가
	// EnqueueCommand 템플릿이 첫 번째 인수로 RHIDevice를 자동으로 추가하므로
	// 나머지 파라미터만 전달
	CommandList->EnqueueCommand<FRHIRenderGizmoPrimitiveCommand>(
		Primitive,
		RenderState,
		CameraLocation,
		ViewportWidth,
		ViewportHeight
	);
}

void FSceneRenderer::RenderWithCommandList(const FSceneView* SceneView, FRHICommandList* ExternalCommandList)
{
	if (!SceneView || !ExternalCommandList || !GDynamicRHI)
	{
		return;
	}

	// 기존 CommandList 대신 외부 CommandList 사용
	FRHICommandList* OriginalCommandList = CommandList;
	CommandList = ExternalCommandList;

	// 각 렌더 패스 실행
	for (IRenderPass* Pass : RenderPasses)
	{
		if (Pass && Pass->IsEnabled())
		{
			Pass->Execute(SceneView, this);
		}
	}

	// 원래 CommandList 복구
	CommandList = OriginalCommandList;
}
