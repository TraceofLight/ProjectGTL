#include "pch.h"
#include "Editor/Public/Camera.h"
#include "Source/Window/Public/Viewport.h"
#include "Source/Window/Public/ViewportClient.h"

FViewportClient::FViewportClient() = default;

FViewportClient::~FViewportClient() = default;

void FViewportClient::Tick() const
{
	// 필요 시 뷰 모드/기즈모 상태 업데이트 등
	// 카메라 입력은 UCamera::Update()가 UInputManager를 직접 읽는 구조라면 여기서 호출만 하면 됨.
    if (IsOrtho())
    {
        if (OrthoGraphicCameras)
        {
            OrthoGraphicCameras->Update();                     // 내부에서 입력 처리 + View/Proj 갱신
        }
    }
    else if (PerspectiveCamera)
    {
        PerspectiveCamera->SetCameraType(ECameraType::ECT_Perspective);
        PerspectiveCamera->Update();
    }
}

void FViewportClient::Draw(const FViewport* InViewport) const
{
	if (!InViewport) { return; }

	// 리프 Rect 기준 Aspect 반영
	const float Aspect = InViewport->GetAspect();
	// FMatrix ViewMatrix, ProjMMatrix;

    if (IsOrtho())
    {
        if (OrthoGraphicCameras)
        {
            OrthoGraphicCameras->SetAspect(Aspect);
            OrthoGraphicCameras->SetCameraType(ECameraType::ECT_Orthographic);
            OrthoGraphicCameras->UpdateMatrixByOrth();
            // ViewMatrix = OrthoGraphicCameras->GetFViewProjConstants().View;
            // ProjMMatrix = OrthoGraphicCameras->GetFViewProjConstants().Projection;
        }
    }
    else if (PerspectiveCamera)
    {
        PerspectiveCamera->SetAspect(Aspect);
        PerspectiveCamera->SetCameraType(ECameraType::ECT_Perspective);
        PerspectiveCamera->UpdateMatrixByPers();
        // ViewMatrix = PerspectiveCamera->GetFViewProjConstants().View;
        // ProjMMatrix = PerspectiveCamera->GetFViewProjConstants().Projection;
    }
}
