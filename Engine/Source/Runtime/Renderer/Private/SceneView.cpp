#include "pch.h"
#include "Runtime/Renderer/Public/SceneView.h"

#include "Window/Public/Viewport.h"

/**
 * @brief FSceneView 클래스의 생성자로 모든 변환 행렬을 단위 행렬로 초기화
 */
FSceneView::FSceneView()
{
    ViewMatrix = FMatrix::Identity();
    ProjectionMatrix = FMatrix::Identity();
    ViewProjectionMatrix = FMatrix::Identity();
}

FSceneView::~FSceneView() = default;

/**
 * @brief FSceneView 객체를 초기화하고 뷰포트 정보를 설정하는 함수
 * 입력된 카메라, 뷰포트, 월드 포인터를 저장하고, 뷰포트의 크기와 종횡비를 계산
 * 최종적으로 UpdateViewMatrices()를 호출하여 뷰 행렬들을 계산한다
 * @param InCamera 뷰 정보를 제공할 ACameraActor 포인터
 * @param InViewport 뷰가 렌더링될 대상 FViewport 포인터
 * @param InWorld 뷰가 속한 UWorld 포인터
 */
void FSceneView::Initialize(TObjectPtr<ACameraActor> InCamera, FViewport* InViewport, TObjectPtr<UWorld> InWorld)
{
    Camera = InCamera;
    Viewport = InViewport;
    World = InWorld;

    if (Viewport)
    {
        ViewportSize = FVector2(
            static_cast<float>(Viewport->GetSizeX()),
            static_cast<float>(Viewport->GetSizeY())
        );

        const FRect& viewportRect = Viewport->GetRect();
        ViewRect = FRect(
            viewportRect.X, viewportRect.Y,viewportRect.X + viewportRect.W,viewportRect.Y + viewportRect.H
        );

        // Aspect Ratio 계산
        if (ViewportSize.Y > 0)
        {
            AspectRatio = ViewportSize.X / ViewportSize.Y;
        }
    }

    UpdateViewMatrices();
}

/**
 * @brief 카메라의 현재 위치, 회전 및 설정을 기반으로 뷰 행렬들을 업데이트하는 함수
 * 카메라 컴포넌트로부터 위치, FOV, 뷰 행렬, 투영 행렬을 가져와 계산
 */
void FSceneView::UpdateViewMatrices()
{
    if (!Camera || !Viewport)
    {
        return;
    }

    UCameraComponent* CameraComponent = Camera->GetCameraComponent();
    if (!CameraComponent)
    {
        return;
    }

    // Update View
    ViewLocation = Camera->GetActorLocation();

    // Convert Euler angles (FVector) to Quaternion
    const FVector& EulerRotation = Camera->GetActorRotation();
    ViewRotation = FQuaternion::FromEuler(EulerRotation);

    // Get Camera FOV (현재 시스템에맞게 GetFovY 사용)
    FOV = CameraComponent->GetFovY();

    // 매트릭스 계산 (커메라 컴포넌트의 ViewProjConstants 사용)
    // 카메라 컴포넌트에서 Aspect 비율 업데이트
    CameraComponent->SetAspect(AspectRatio);

    // 카메라 타입에 따른 매트릭스 업데이트
    switch (CameraComponent->GetCameraType())
    {
    case ECameraType::ECT_Perspective:
        CameraComponent->UpdateMatrixByPers();
        break;
    case ECameraType::ECT_Orthographic:
        CameraComponent->UpdateMatrixByOrth();
        break;
    }

    // 커메라 컴포넌트에서 계산된 매트릭스 가져오기
    const FViewProjConstants& ViewProjConstants = CameraComponent->GetFViewProjConstants();
    ViewMatrix = ViewProjConstants.View;
    ProjectionMatrix = ViewProjConstants.Projection;
    ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;

    // Frustum 업데이트
    // ViewFrustum.Update(ViewProjectionMatrix);
}

