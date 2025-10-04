#include "pch.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Runtime/Component/Public/CameraComponent.h"
#include "Source/Window/Public/Viewport.h"
#include "Source/Window/Public/ViewportClient.h"

FViewportClient::FViewportClient() = default;

FViewportClient::~FViewportClient() = default;

void FViewportClient::Tick() const
{
	// 필요 시 뷰 모드/기즈모 상태 업데이트 등
	// 카메라 입력은 ACameraActor::Tick()에서 처리
    if (IsOrtho())
    {
        if (OrthoGraphicCameras)
        {
            OrthoGraphicCameras->Tick();                     // 내부에서 입력 처리 + View/Proj 갱신
        }
    }
    else if (PerspectiveCamera)
    {
        if (PerspectiveCamera->GetCameraComponent())
        {
            PerspectiveCamera->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
        }
        PerspectiveCamera->Tick();
    }
}

void FViewportClient::Draw(const FViewport* InViewport) const
{
	if (!InViewport) { return; }

	const float Aspect = InViewport->GetAspect();

    if (IsOrtho())
    {
        if (OrthoGraphicCameras && OrthoGraphicCameras->GetCameraComponent())
        {
            OrthoGraphicCameras->GetCameraComponent()->SetAspect(Aspect);
            OrthoGraphicCameras->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
            OrthoGraphicCameras->GetCameraComponent()->UpdateMatrixByOrth();
        }
    }
    else if (PerspectiveCamera && PerspectiveCamera->GetCameraComponent())
    {
        PerspectiveCamera->GetCameraComponent()->SetAspect(Aspect);
        PerspectiveCamera->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
        PerspectiveCamera->GetCameraComponent()->UpdateMatrixByPers();
    }
}
