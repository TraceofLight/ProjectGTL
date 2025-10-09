#include "pch.h"
#include "Source/Window/Public/ViewportClient.h"

#include "Source/Window/Public/Viewport.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"
#include "Runtime/RHI/Public/RHIDevice.h"

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
    FMatrix ViewMatrix;
    FVector Right, Up, Forward;

    // 직교 뷰는 Forward + Right 직접 정의 (언리얼 표준)
    // 왼손 좌표계: Up = Forward × Right
    switch (ViewType)
    {
    case EViewType::OrthoTop:
        // Top: 화면 오른쪽=-Y, 화면 위=-X
        Forward = FVector(0.0f, 0.0f, -1.0f);   // -Z
        Right = FVector(0.0f, -1.0f, 0.0f);     // -Y
        Up = FVector(-1.0f, 0.0f, 0.0f);        // -X
        break;

    case EViewType::OrthoBottom:
        // Bottom: 화면 오른쪽=+Y, 화면 위=+X
        Forward = FVector(0.0f, 0.0f, 1.0f);    // +Z
        Right = FVector(0.0f, 1.0f, 0.0f);      // +Y
        Up = FVector(1.0f, 0.0f, 0.0f);         // +X
        break;

    case EViewType::OrthoFront:
        // Front: 화면 오른쪽=-Y, 화면 위=+Z
        Forward = FVector(1.0f, 0.0f, 0.0f);    // +X
        Right = FVector(0.0f, -1.0f, 0.0f);     // -Y
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::OrthoBack:
        // Back: 화면 오른쪽=+Y, 화면 위=+Z
        Forward = FVector(-1.0f, 0.0f, 0.0f);   // -X
        Right = FVector(0.0f, 1.0f, 0.0f);      // +Y
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::OrthoLeft:
        // Left: 화면 오른쪽=-X, 화면 위=+Z
        Forward = FVector(0.0f, -1.0f, 0.0f);   // -Y
        Right = FVector(-1.0f, 0.0f, 0.0f);     // -X
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::OrthoRight:
        // Right: 화면 오른쪽=-X, 화면 위=+Z
        Forward = FVector(0.0f, 1.0f, 0.0f);    // +Y
        Right = FVector(-1.0f, 0.0f, 0.0f);     // -X
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::Perspective:
    default:
        // Perspective는 ViewRotation 사용
        {
            FVector Radians = FVector::GetDegreeToRadian(ViewRotation);
            FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
            Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
            Forward.Normalize();
            Up = FMatrix::VectorMultiply(FVector::UpVector(), RotationMatrix);
            Up.Normalize();
            Right = Forward.Cross(Up);  // 왼손 좌표계: Right = Forward × Up
            Right.Normalize();
            Up = Right.Cross(Forward);  // Up = Right × Forward (재계산하여 직교성 보장)
            Up.Normalize();
        }
        break;
    }

    // View 행렬 직접 구성 (Row-major)
    // Row 0: Right
    ViewMatrix.Data[0][0] = Right.X;
    ViewMatrix.Data[0][1] = Right.Y;
    ViewMatrix.Data[0][2] = Right.Z;
    ViewMatrix.Data[0][3] = -Right.Dot(ViewLocation);

    // Row 1: Up
    ViewMatrix.Data[1][0] = Up.X;
    ViewMatrix.Data[1][1] = Up.Y;
    ViewMatrix.Data[1][2] = Up.Z;
    ViewMatrix.Data[1][3] = -Up.Dot(ViewLocation);

    // Row 2: Forward
    ViewMatrix.Data[2][0] = Forward.X;
    ViewMatrix.Data[2][1] = Forward.Y;
    ViewMatrix.Data[2][2] = Forward.Z;
    ViewMatrix.Data[2][3] = -Forward.Dot(ViewLocation);

    // Row 3: Homogeneous
    ViewMatrix.Data[3][0] = 0.0f;
    ViewMatrix.Data[3][1] = 0.0f;
    ViewMatrix.Data[3][2] = 0.0f;
    ViewMatrix.Data[3][3] = 1.0f;

    return ViewMatrix;
}

FMatrix FViewportClient::GetProjectionMatrix() const
{
    // AspectRatio = 너비 / 높이
    const float AspectRatio = ViewSize.Y > 0.0f ? (float)ViewSize.X / (float)ViewSize.Y : 1.0f;

    if (IsOrtho())
    {
        // 직교 투영 행렬: 언리얼 방식 - 고정된 월드 단위 크기 유지
        // OrthoWidth는 "기준 폭"이고, 픽셀 크기에 비례하여 실제 보이는 면적 결정
        // 뷰포트가 좌우로 줄어들면 보이는 폭도 줄어듦 (AspectRatio에 따른 보정 제거)
        // 뷰포트가 상하로 줄어들면 보이는 높이도 줄어듦
        const float EffectiveWidth = OrthoWidth * AspectRatio;   // 폭은 AspectRatio에 비례
        const float EffectiveHeight = OrthoWidth;                 // 높이는 기준값 사용
        return FMatrix::MatrixOrthoLH(EffectiveWidth, EffectiveHeight, NearZ, FarZ);
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
		UE_LOG("ViewportClient::Draw - No World");
		return;
	}

	// Camera deprecated - 카메라 없이 ViewportClient의 행렬을 직접 사용
	// Get Current Camera from ViewportSubsystem
	// UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	// ACameraActor* CurrentCamera = nullptr;
	// if (ViewportSS)
	// {
	// 	// Find the index for the given viewport pointer
	// 	int32 ViewportIndex = -1;
	// 	const auto& Viewports = ViewportSS->GetViewports();
	// 	for (int32 i = 0; i < Viewports.Num(); ++i)
	// 	{
	// 		if (Viewports[i] == InViewport)
	// 		{
	// 			ViewportIndex = i;
	// 			break;
	// 		}
	// 	}
	// 	CurrentCamera = ViewportSS->GetActiveCameraForViewport(ViewportIndex);
	// }

	// if (!CurrentCamera)
	// {
	// 	// UE_LOG("ViewportClient::Draw - No active camera for this viewport");
	// 	return; // 렌더링할 카메라가 없으면 종료
	// }

	// SceneViewFamily
	FSceneViewFamily ViewFamily;
	ViewFamily.SetRenderTarget(InViewport);
	ViewFamily.SetCurrentTime(World->GetTimeSeconds());
	ViewFamily.SetDeltaWorldTime(World->GetDeltaSeconds());

	// SceneView - 카메라 대신 ViewportClient의 행렬을 직접 사용
	FSceneView View;
	View.InitializeWithMatrices(
		GetViewMatrix(),
		GetProjectionMatrix(),
		ViewLocation,
		ViewRotation,
		InViewport,
		World,
		ViewMode
	);
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
