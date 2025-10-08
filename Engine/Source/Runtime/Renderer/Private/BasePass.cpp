#include "pch.h"
#include "../Public/BasePass.h"

#include "Runtime/Renderer/Public/SceneRenderer.h"
#include "Runtime/Renderer/Public/RHICommandList.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Runtime/Engine/Public/World.h"
#include "Window/Public/Viewport.h"

FBasePass::~FBasePass()
{
}

void FBasePass::Initialize()
{
	// BasePass 초기화 작업
}

void FBasePass::Cleanup()
{
	// BasePass 정리 작업
}

void FBasePass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
	if (!View || !SceneRenderer) return;

	UWorld* World = View->GetWorld();
	if (!World)
	{
		printf("[BasePass] World is null!\n");
		return;
	}

	FRHICommandList* RHICmdList = SceneRenderer->GetCommandList();
	if (!RHICmdList) return;

	// 비 매트릭스 계산
	ACameraActor* Camera = View->GetCamera();
	FViewport* Viewport = View->GetViewport();

	if (!Camera || !Viewport) return;

	// UCameraComponent에서 ViewProjConstants 가져오기
	UCameraComponent* CameraComp = Camera->GetCameraComponent();
	if (!CameraComp) return;

	const FViewProjConstants& ViewProjConsts = CameraComp->GetFViewProjConstants();
	FMatrix ViewMatrix = ViewProjConsts.View;
	FMatrix ProjectionMatrix = ViewProjConsts.Projection;

	// 뉴 모드 설정 (CommandList로 처리)
	EViewMode ViewModeIndex = View->GetViewModeIndex();
	// TODO: ViewMode Command 추가 예정

	FVector HighlightColor(1.0f, 1.0f, 1.0f);

	// === 라인 배치 시작 ===
	// TODO: 라인 배치는 별도 RenderPass로 분리할 예정

	// === 일반 액터들 렌더링 ===
	// Primitives Show Flag 체크
	if (World->IsShowFlagEnabled(EEngineShowFlags::SF_Primitives))
	{
		ULevel* Level = World->GetLevel();
		if (!Level) return;

		const TArray<TObjectPtr<AActor>>& Actors = Level->GetLevelActors();
		int32 CulledCount = 0;
		int32 RenderedCount = 0;

		for (const TObjectPtr<AActor>& ActorPtr : Actors)
		{
			AActor* Actor = ActorPtr.Get();
			if (!Actor) continue;
			if (Actor->GetActorHiddenInGame()) continue;

			// StaticMesh Show Flag 체크
			if (Cast<AStaticMeshActor>(Actor) && !World->IsShowFlagEnabled(
				EEngineShowFlags::SF_StaticMeshes))
				continue;

			// RenderActor 호출 및 결과 처리
			bool bWasRendered = RenderActor(Actor, View, RHICmdList, ViewMatrix, ProjectionMatrix, HighlightColor);

			if (bWasRendered)
			{
				RenderedCount++;
			}
			else
			{
				CulledCount++;
			}
		}

		// Frustum Culling 통계 출력 (프레임마다 출력하지 말고 주기적으로)
		static int32 FrameCount = 0;
		FrameCount++;
		if (FrameCount % 120 == 0) // 2초마다 출력 (60fps 기준)
		{
			int32 TotalActors = Actors.Num();
			if (TotalActors > 0)
			{
				UE_LOG("[BasePass] Frustum Culling Stats - Total: %d, Rendered: %d, Culled: %d (%.1f%%)\n",
				       TotalActors, RenderedCount, CulledCount, (float)CulledCount * 100.0f / TotalActors);
			}
		}
	}

	// === 엔진 액터들 (그리드 등) 렌더링 ===
	// 참고: 엔진 액터들은 일반 액터들과 함께 렌더링되므로 별도 처리 불필요
}

bool FBasePass::RenderActor(AActor* Actor, const FSceneView* View, FRHICommandList* RHICmdList,
                            const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix,
                            const FVector& HighlightColor)
{
	if (!Actor || !View || !RHICmdList)
	{
		return false; // 렌더링 실패
	}

	// === Frustum Culling 임시 비활성화 ===
	// 렌더링 문제 해결을 위해 임시로 Frustum Culling을 비활성화
	bool bShouldCull = false; // 항상 false로 설정


	// 바운딩 박스 기반 컵링 체크 (비활성화 - 컴포넌트 미존재)
	bool bFoundBoundingBox = false;
	// 참고: UAABoundingBoxComponent와 UOBoundingBoxComponent가 존재하지 않으므로
	// 바운딩 박스 컵링을 비활성화합니다.

	// 바운딩 박스가 없는 경우 액터 위치 기준 컬링 (비활성화 - FBound 미존재)
	// TODO: 적절한 바운딩 볼륨과 프러스텀 컬링 시스템 구현 필요

	if (bShouldCull)
	{
		return false; // Frustum 외부에 있으므로 컬링
	}

	UWorld* World = View->GetWorld();

	// 선택 상태 확인 (임시로 비활성화 - USelectionManager 미존재)
	bool bIsSelected = false;
	// TODO: UEditor의 GetSelectedActor()를 사용하여 선택 상태 확인 구현

	// 하이라이트 상수 버퍼 업데이트 (CommandList로 처리)
	// TODO: UpdateHighlightBuffers Command 추가

	// 액터의 모든 컴포넌트 렌더링
	for (USceneComponent* Component : Actor->GetComponents<USceneComponent>())
	{
		if (!Component) continue;

		if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
		{
			if (!ActorComp->IsActive()) continue;
		}

		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
		{
			RenderPrimitiveComponent(Primitive, View, RHICmdList, ViewMatrix, ProjectionMatrix);
		}
	}

	// 블렌드 스테이트 종료 (CommandList로 처리)
	RHICmdList->SetBlendState(false);

	return true; // 렌더링 성공
}

void FBasePass::RenderPrimitiveComponent(UPrimitiveComponent* Component, const FSceneView* View,
                                         FRHICommandList* RHICmdList, const FMatrix& ViewMatrix,
                                         const FMatrix& ProjectionMatrix)
{
	if (!Component || !View || !RHICmdList)
	{
		return;
	}

	// 뷰 모드 설정 (CommandList로 처리)
	// TODO: ViewMode Command 추가

	// Primitive Component 렌더링 (CommandList로 처리)
	RHICmdList->DrawIndexedPrimitive(Component, ViewMatrix, ProjectionMatrix);

	// 깊이 스텐실 상태 복원 (CommandList로 처리)
	RHICmdList->SetDepthStencilState(EComparisonFunc::LessEqual);
}
