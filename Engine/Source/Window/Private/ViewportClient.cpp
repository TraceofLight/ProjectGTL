#include "pch.h"
#include "Source/Window/Public/ViewportClient.h"

#include "Source/Window/Public/Viewport.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"

#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

FViewportClient::FViewportClient() = default;

FViewportClient::~FViewportClient() = default;

void FViewportClient::Tick(float DeltaSeconds) const
{
	// The responsibility for ticking camera actors has been moved to another system (e.g., UViewportSubsystem).
	// This function is now reserved for viewport-specific state updates if needed in the future.
}

FMatrix FViewportClient::GetViewMatrix() const
{
    // 오일러 각도를 라디안으로 변환
    FVector Radians = FVector::GetDegreeToRadian(ViewRotation);

    // 회전 행렬 생성
    FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);

    // 전방, 위, 오른쪽 벡터 계산
    FVector Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
    FVector Up = FMatrix::VectorMultiply(FVector::UpVector(), RotationMatrix);

    // LookAt 행렬 생성 (이 함수는 Matrix.cpp에 추가 필요)
    return FMatrix::MatrixLookAtLH(ViewLocation, ViewLocation + Forward, Up);
}

FMatrix FViewportClient::GetProjectionMatrix() const
{
    const float AspectRatio = ViewSize.X > 0.0f ? (float)ViewSize.Y / (float)ViewSize.X : 1.0f;

    if (IsOrtho())
    {
        // 직교 투영 행렬 생성 (이 함수는 Matrix.cpp에 추가 필요)
        return FMatrix::MatrixOrthoLH(OrthoWidth, OrthoWidth * AspectRatio, NearZ, FarZ);
    }
    else
    {
        // 원근 투영 행렬 생성 (이 함수는 Matrix.cpp에 추가 필요)
        return FMatrix::MatrixPerspectiveFovLH(FVector::GetDegreeToRadian(FovY), AspectRatio, NearZ, FarZ);
    }}

FViewProjConstants FViewportClient::GetViewProjConstants() const
{
    FViewProjConstants Constants;
    Constants.View = GetViewMatrix();
    Constants.Projection = GetProjectionMatrix();
    return Constants;
}

void FViewportClient::Draw(FViewport* InViewport)
{
    if (!InViewport)
    {
        UE_LOG("ViewportClient::Draw - Invalid viewport");
        return;
    }

	// Get World
	TObjectPtr<UWorld> World = GEngine->GetWorld();
	if (!World)
	{
		return;
	}

	// Get Current Camera from ViewportSubsystem
	UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	ACameraActor* CurrentCamera = nullptr;
	if (ViewportSS)
	{
		// Find the index for the given viewport pointer
		int32 ViewportIndex = -1;
		const auto& Viewports = ViewportSS->GetViewports();
		for (int32 i = 0; i < Viewports.Num(); ++i)
		{
			if (Viewports[i] == InViewport)
			{
				ViewportIndex = i;
				break;
			}
		}
		CurrentCamera = ViewportSS->GetActiveCameraForViewport(ViewportIndex);
	}

	if (!CurrentCamera)
	{
		// UE_LOG("ViewportClient::Draw - No active camera for this viewport");
		return; // 렌더링할 카메라가 없으면 종료
	}

	// SceneViewFamily
	FSceneViewFamily ViewFamily;
	ViewFamily.SetRenderTarget(InViewport);
	ViewFamily.SetCurrentTime(World->GetTimeSeconds());	ViewFamily.SetDeltaWorldTime(World->GetDeltaSeconds());

	// SceneView
	FSceneView View;
	View.Initialize(TObjectPtr<ACameraActor>(CurrentCamera), InViewport, World);
	ViewFamily.AddView(&View);

	// SceneRenderer
	if (FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(ViewFamily))
	{
		SceneRenderer->Render();
		delete SceneRenderer;
	}

	// UI 렌더링
	if (auto* UISubsystem = GEngine->GetEngineSubsystem<UUISubsystem>())
	{
		UISubsystem->Render();
	}
}
