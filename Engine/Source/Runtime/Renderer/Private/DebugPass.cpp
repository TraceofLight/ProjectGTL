#include "pch.h"
#include "Runtime/Renderer/Public/DebugPass.h"

#include "Runtime/Core/Public/VertexTypes.h"

#include "Runtime/Renderer/Public/SceneRenderer.h"
#include "Runtime/Renderer/Public/RHICommandList.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Window/Public/Viewport.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Axis.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Renderer/Public/EditorGrid.h"
#include "Runtime/Renderer/Public/LineBatcher.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/Component/Public/CameraComponent.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Window/Public/ViewportClient.h"

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

	// Editor 시스템에서 Editor 가져오기 (Camera 사용 안 함!)
	auto* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS || !WorldSS->GetEditor()) return;

	UEditor* Editor = WorldSS->GetEditor();
	FViewport* Viewport = View->GetViewport();

	if (!Viewport)
	{
		return;
	}

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

	if (bNeedsLineRendering)
	{
		if (Editor->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
		{
			RenderGrid(View, SceneRenderer);
		}
	}

	if (bHasSelection)
	{
		RenderGizmos(View, SceneRenderer, Editor);
	}

	if (bNeedsDebugComponents)
	{
		RenderDebugComponents(View, SceneRenderer);
	}
}

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

	// FD3D11RHIModule을 통한 그리드 렌더링 리소스 가져오기
	FD3D11RHIModule& RHIModule = FD3D11RHIModule::GetInstance();
	FEditorRenderResources* EditorResources = RHIModule.GetEditorResources();
	if (!EditorResources)
	{
		return;
	}

	// LineBatcher 가져오기
	FLineBatcher* LineBatcher = EditorResources->GetLineBatcher();
	if (!LineBatcher)
	{
		return;
	}

	// Line Batching 시작
	LineBatcher->BeginBatch();

	// UAxis를 사용하여 좌표축 라인 추가
	UAxis::AddAxisLinesToBatch(EditorResources);

	// 전역 그리드 사이즈 가져오기
	const float GridSize = FEditorGrid::GetCellSize();
	FEditorGrid::AddGridLinesToBatch(EditorResources, GridSize, 250, FVector4(0.5f, 0.5f, 0.5f, 1.0f));

	// 언리얼에서는 ULevel에서 액터들을 가져옴
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;
	if (CurrentLevel)
	{
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
	}

	// Line Batching 종료 - CommandList를 통해 렌더링
	FRHICommandList* RHICmdList = SceneRenderer->GetCommandList();
	if (RHICmdList)
	{
		FMatrix ViewMatrix = View->GetViewMatrix();
		FMatrix ProjectionMatrix = View->GetProjectionMatrix();
		const FRect& ViewRect = View->GetViewRect();
		LineBatcher->EndBatch(RHICmdList, FMatrix::Identity(), ViewMatrix, ProjectionMatrix, &ViewRect);
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

	FLineBatcher* LineBatcher = EditorResources->GetLineBatcher();
	if (!LineBatcher) return;

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

					LineBatcher->AddLines(BoxStart, BoxEnd, BoxColor);
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

	// FSceneView에서 직접 매트릭스 가져오기 (Camera 사용 안 함!)
	auto* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS || !WorldSS->GetEditor()) return;

	UEditor* Editor = WorldSS->GetEditor();
	FViewport* Viewport = View->GetViewport();
	if (!Viewport) return;

	// Camera 대신 View에서 직접 매트릭스 사용
	FMatrix ViewMatrix = View->GetViewMatrix();
	FMatrix ProjectionMatrix = View->GetProjectionMatrix();

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
