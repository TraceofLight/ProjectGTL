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

    // 직교 뷰는 Forward + Right 직접 정의
    // 왼손 좌표계: Up = Forward × Right
    switch (ViewType)
    {
    case EViewType::OrthoTop:
        Forward = FVector(0.0f, 0.0f, -1.0f);   // -Z
        Right = FVector(0.0f, -1.0f, 0.0f);     // -Y
        Up = FVector(-1.0f, 0.0f, 0.0f);        // -X
        break;

    case EViewType::OrthoBottom:
        Forward = FVector(0.0f, 0.0f, 1.0f);    // +Z
        Right = FVector(0.0f, 1.0f, 0.0f);      // +Y
        Up = FVector(-1.0f, 0.0f, 0.0f);        // -X
        break;

    case EViewType::OrthoFront:
        // Front: 화면 오른쪽=-Y, 화면 위=+Z
        Forward = FVector(1.0f, 0.0f, 0.0f);    // +X
        Right = FVector(0.0f, 1.0f, 0.0f);      // +Y
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::OrthoBack:
        // Back: 화면 오른쪽=+Y, 화면 위=+Z
        Forward = FVector(-1.0f, 0.0f, 0.0f);   // -X
        Right = FVector(0.0f, -1.0f, 0.0f);     // -Y
        Up = FVector(0.0f, 0.0f, 1.0f);         // +Z
        break;

    case EViewType::OrthoLeft:
        // Left: 화면 오른쪽=-X, 화면 위=+Z
        Forward = FVector(0.0f, -1.0f, 0.0f);   // -Y
        Right = FVector(1.0f, 0.0f, 0.0f);      // +X
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
            // Pitch 부호 반전 (언리얼 표준: 양수 = 위를 봄)
            FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, -Radians.X, Radians.Z);
            Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
            Forward.Normalize();
            Up = FMatrix::VectorMultiply(FVector::UpVector(), RotationMatrix);
            Up.Normalize();
            Right = Forward.Cross(Up);  // 왼손 좌표계: Right = Forward × Up
            Right.Normalize();
            Up = Forward.Cross(Right);  // Up = Forward × Right (재계산하여 직교성 보장)
            Up.Normalize();
        }
        break;
    }

    // DirectX View Matrix 구성 (Row-major 순서)
    //
    // DirectX 렌더링 파이프라인에서 기대하는 순서:
    //   Row 0: Right Vector  (NDC X축 매핑)
    //   Row 1: Up Vector     (NDC Y축 매핑)
    //   Row 2: Forward Vector (NDC Z축 매핑)
    //   Row 3: Translation   (카매라 위치 변환)
    //
    // 참고: ViewLocation은 월드 좌표이므로 -Right.Dot(ViewLocation)로 내적

    ViewMatrix.Data[0][0] = Right.X;
    ViewMatrix.Data[0][1] = Right.Y;
    ViewMatrix.Data[0][2] = Right.Z;
    ViewMatrix.Data[0][3] = -Right.Dot(ViewLocation);    // 오른쪽 방향 투영 + 카메라 오프셋

    ViewMatrix.Data[1][0] = Up.X;
    ViewMatrix.Data[1][1] = Up.Y;
    ViewMatrix.Data[1][2] = Up.Z;
    ViewMatrix.Data[1][3] = -Up.Dot(ViewLocation);       // 위 방향 투영 + 카메라 오프셋

    ViewMatrix.Data[2][0] = Forward.X;
    ViewMatrix.Data[2][1] = Forward.Y;
    ViewMatrix.Data[2][2] = Forward.Z;
    ViewMatrix.Data[2][3] = -Forward.Dot(ViewLocation);  // 전진 방향 투영 + 카메라 오프셋

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
        // 뷰포트가 좌우로 줄어들면 보이는 폭도 줄어듬 (AspectRatio에 따른 보정 제거)
        // 뷰포트가 상하로 줄어들면 보이는 높이도 줄어듬
        const float EffectiveWidth = OrthoWidth * AspectRatio;   // 폭은 AspectRatio에 비례
        const float EffectiveHeight = OrthoWidth;                 // 높이는 기준값 사용
        return FMatrix::MatrixOrthoLH(EffectiveWidth, EffectiveHeight, NearZ, FarZ);
    }
    else
    {
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
