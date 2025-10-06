#include "pch.h"
#include "Renderer/RenderPass/Public/DebugPass.h"

#include <Renderer/Public/VertexTypes.h>

#include "Renderer/SceneRenderer.h"
#include "Renderer/RenderCommand/Public/RHICommandList.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Window/Public/Viewport.h"
#include "Editor/Public/Editor.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "Renderer/Public/EditorRenderResources.h"
#include "Renderer/Public/RendererModule.h"
#include "Runtime/Component/Public/CameraComponent.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
// LineComponent 제거 - 언리얼 엔진에 없는 컴포넌트
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Shader/Public/Shader.h"

FDebugPass::~FDebugPass()
{
	// Debug Pass 소멸자 - 필요한 경우 리소스 정리
}

void FDebugPass::Initialize()
{
	// DebugPass 초기화 작업
}

void FDebugPass::Cleanup()
{
	// DebugPass 정리 작업
}

void FDebugPass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
	if (!View || !SceneRenderer)
	{
		return;
	}

	UWorld* World = View->GetWorld();
	if (!World)
	{
		return;
	}

	FRHICommandList* RHICmdList = SceneRenderer->GetCommandList();
	if (!RHICmdList)
	{
		return;
	}

	// Editor 시스템에서 뷰 매트릭스 가져오기
	auto* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS || !WorldSS->GetEditor()) return;

	UEditor* Editor = WorldSS->GetEditor();
	ACameraActor* Camera = Editor->GetCamera();
	FViewport* Viewport = View->GetViewport();

	if (!Camera || !Viewport) return;

	// UCameraComponent에서 ViewProj 데이터 가져오기
	UCameraComponent* CameraComp = Camera->GetCameraComponent();
	if (!CameraComp) return;

	FViewProjConstants ViewProjData = CameraComp->GetFViewProjConstants();
	FMatrix ViewMatrix = ViewProjData.View;
	FMatrix ProjectionMatrix = ViewProjData.Projection;

	// Editor의 ShowFlag 사용 및 Selection 체크
	bool bHasSelection = Editor->HasSelectedActor();

	bool bNeedsLineRendering = Editor->IsShowFlagEnabled(EEngineShowFlags::SF_Grid) ||
		Editor->IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes) ||
		bHasSelection;

	bool bNeedsDebugComponents = Editor->IsShowFlagEnabled(EEngineShowFlags::SF_BillboardText);

	if (!bNeedsLineRendering && !bNeedsDebugComponents)
	{
		return; // 디버그 렌더링이 필요없으면 전체 스킵
	}

	// === 라인 배치 시작 ===
	if (bNeedsLineRendering)
	{
		// BeginLineBatch는 RenderGrid에서 FEditorRenderResources로 처리

		// === 그리드 렌더링 ===
		if (Editor->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
		{
			RenderGrid(View, SceneRenderer);
		}
	}

	// === 기즈모 렌더링 ===
	if (bHasSelection)
	{
		RenderGizmos(View, SceneRenderer, Editor);
	}

	// === Debug 컴포넌트들 렌더링 ===
	if (bNeedsDebugComponents)
	{
		RenderDebugComponents(View, SceneRenderer);
	}

	// CommandList는 SceneRenderer에서 Execute할 예정이므로 여기서는 호출하지 않음
}

// BeginLineBatch와 EndLineBatch는 FEditorRenderResources에서 처리되므로 제거됨

void FDebugPass::RenderGrid(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
	if (!View)
	{
		return;
	}

	UWorld* World = View->GetWorld();
	if (!World)
	{
		return;
	}

	URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
	if (!RHI)
	{
		return;
	}

	// FEditorRenderResources를 통한 그리드 렌더링
	FRendererModule& RendererModule = FRendererModule::Get();
	FEditorRenderResources* EditorResources = RendererModule.GetEditorResources();
	if (EditorResources)
	{
		EditorResources->RenderGrid();
	}

	// 언리얼에서는 ULevel에서 액터들을 가져옴
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;
	if (CurrentLevel && EditorResources)
	{
		// FEditorRenderResources를 통한 Line Batching 시작
		EditorResources->BeginLineBatch();

		const TArray<TObjectPtr<AActor>>& Actors = CurrentLevel->GetLevelActors();
		for (const TObjectPtr<AActor>& ActorPtr : Actors)
		{
			AActor* Actor = ActorPtr.Get();
			if (Actor && !Actor->GetActorHiddenInGame())
			{
				// Editor를 다시 가져온다 (이미 WorldSS에서 가져온 상태)
				UEditor* Editor = WorldSS->GetEditor();
				if (Editor)
				{
					RenderActorLines(Actor, View, SceneRenderer, EditorResources, Editor);
				}
			}
		}

		// Line Batching 종료
		UEditor* Editor = WorldSS->GetEditor();
		if (Editor && Editor->GetCamera() && Editor->GetCamera()->GetCameraComponent())
		{
			FViewProjConstants ViewProjData = Editor->GetCamera()->GetCameraComponent()->GetFViewProjConstants();
			FMatrix ViewMatrix = ViewProjData.View;
			FMatrix ProjectionMatrix = ViewProjData.Projection;
			EditorResources->EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
		}
	}
}

void FDebugPass::RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer, UEditor* Editor)
{
	if (!View || !Editor) return;

	// Editor에서 선택된 액터 가져오기
	AActor* SelectedActor = Editor->GetSelectedActor();
	if (!SelectedActor) return;

	// TODO: Gizmo 렌더링은 이제 FEditorRenderResources에서 처리되어야 함
	// 임시로 Gizmo 렌더링 비활성화
	// FRendererModule& RendererModule = FRendererModule::Get();
	// FEditorRenderResources* EditorResources = RendererModule.GetEditorResources();
	// if (EditorResources) {
	//     EditorResources->RenderGizmo(SelectedActor, ...);
	// }
}

void FDebugPass::RenderActorLines(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer, FEditorRenderResources* EditorResources, UEditor* Editor)
{
	if (!Actor || !View || !SceneRenderer || !EditorResources || !Editor) return;

	// 액터의 Component들을 찾아서 렌더링
	for (USceneComponent* Component : Actor->GetComponents<USceneComponent>())
	{
		if (!Component) continue;

		if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
		{
			if (!ActorComp->IsActive()) continue;
		}

		// Bounding Box 처리 - FEditorRenderResources로 직접 추가
		if (Editor->IsShowFlagEnabled(EEngineShowFlags::SF_BoundingBoxes))
		{
			// Editor에서 선택된 액터만 바운딩 박스 표시
			AActor* SelectedActor = Editor->GetSelectedActor();

			if (SelectedActor == Actor)
			{
				// UAABoundingBoxComponent와 UOBoundingBoxComponent는 아직 구현되지 않음
				// 대신 UPrimitiveComponent의 바운딩 박스 사용
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
				{
					FVector WorldMin, WorldMax;
					PrimComp->GetWorldAABB(WorldMin, WorldMax);

					// AABB 라인 생성 (12개 선)
					TArray<FVector> BoxStart, BoxEnd;
					TArray<FVector4> BoxColor;

					// 바운딩 박스 8개 모서리
					FVector Corners[8] = {
						FVector(WorldMin.X, WorldMin.Y, WorldMin.Z), // 0
						FVector(WorldMax.X, WorldMin.Y, WorldMin.Z), // 1
						FVector(WorldMax.X, WorldMax.Y, WorldMin.Z), // 2
						FVector(WorldMin.X, WorldMax.Y, WorldMin.Z), // 3
						FVector(WorldMin.X, WorldMin.Y, WorldMax.Z), // 4
						FVector(WorldMax.X, WorldMin.Y, WorldMax.Z), // 5
						FVector(WorldMax.X, WorldMax.Y, WorldMax.Z), // 6
						FVector(WorldMin.X, WorldMax.Y, WorldMax.Z)  // 7
					};

					FVector4 BBoxColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색

					// Bottom face (4 lines)
					for (int i = 0; i < 4; ++i) {
						BoxStart.Add(Corners[i]);
						BoxEnd.Add(Corners[(i + 1) % 4]);
						BoxColor.Add(BBoxColor);
					}

					// Top face (4 lines)
					for (int i = 4; i < 8; ++i) {
						BoxStart.Add(Corners[i]);
						BoxEnd.Add(Corners[4 + ((i - 4 + 1) % 4)]);
						BoxColor.Add(BBoxColor);
					}

					// Vertical lines (4 lines)
					for (int i = 0; i < 4; ++i) {
						BoxStart.Add(Corners[i]);
						BoxEnd.Add(Corners[i + 4]);
						BoxColor.Add(BBoxColor);
					}

					EditorResources->AddLines(BoxStart, BoxEnd, BoxColor);
				}
			}
		}
	}
}

// AddLinesToBatch와 RenderAllBatchedLines는 모두 FEditorRenderResources로 대체됨

void FDebugPass::RenderDebugComponents(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
	if (!View || !SceneRenderer) return;

	UWorld* World = View->GetWorld();
	if (!World) return;

	FRHICommandList* RHICmdList = SceneRenderer->GetCommandList();
	if (!RHICmdList) return;

	// Editor 시스템에서 뷰 매트릭스 가져오기
	auto* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS || !WorldSS->GetEditor()) return;

	UEditor* Editor = WorldSS->GetEditor();
	ACameraActor* Camera = Editor->GetCamera();
	FViewport* Viewport = View->GetViewport();
	if (!Camera || !Viewport) return;

	// UCameraComponent에서 ViewProj 데이터 가져오기
	UCameraComponent* CameraComp = Camera->GetCameraComponent();
	if (!CameraComp) return;

	FViewProjConstants ViewProjData = CameraComp->GetFViewProjConstants();
	FMatrix ViewMatrix = ViewProjData.View;
	FMatrix ProjectionMatrix = ViewProjData.Projection;

	// 언리얼에서 ULevel에서 액터들을 가져옴
	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();
	if (CurrentLevel)
	{
		const TArray<TObjectPtr<AActor>>& Actors = CurrentLevel->GetLevelActors();
		for (const TObjectPtr<AActor>& ActorPtr : Actors)
		{
			AActor* Actor = ActorPtr.Get();
			if (!Actor || Actor->GetActorHiddenInGame()) continue;

			for (USceneComponent* Component : Actor->GetComponents<USceneComponent>())
			{
				if (!Component) continue;

				if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
				{
					if (!ActorComp->IsActive()) continue;
				}

				// Billboard Component (기존 TextRenderComponent 대체)
				if (UBillBoardComponent* BillboardComp = Cast<UBillBoardComponent>(Component))
				{
					if (Editor->IsShowFlagEnabled(EEngineShowFlags::SF_BillboardText))
					{
						if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(BillboardComp))
						{
							RHICmdList->DrawIndexedPrimitive(Primitive, ViewMatrix, ProjectionMatrix);
						}
					}
					continue;
				}

				// AABB/OBB 처리는 이제 RenderActorLines에서 함께 처리하므로 여기서는 생략

				// 기즈모 컴포넌트들은 현재 없으므로 제거
				// TODO: 필요시 FEditorRenderResources를 통해 Gizmo 렌더링
			}
		}
	}
}

// Octree 렌더링은 현재 사용하지 않으므로 제거됨
// TODO: 필요시 Octree 시스템 구현 후 다시 추가
void FDebugPass::RenderOctreeBoxes(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
	// Octree 기능이 현재 없으므로 비활성화
	return;
}

void FDebugPass::RenderOctreeNodeRecursive(FOctreeNode* Node)
{
	// Octree 기능이 현재 없으므로 비활성화
	return;
}
