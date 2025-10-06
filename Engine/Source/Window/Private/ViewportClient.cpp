#include "pch.h"
#include "Source/Window/Public/ViewportClient.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Runtime/Component/Public/CameraComponent.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"

#include "Source/Window/Public/Viewport.h"

#include "Renderer/SceneViewFamily/Public/SceneViewFamily.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Renderer/SceneRenderer.h"


FViewportClient::FViewportClient() = default;

FViewportClient::~FViewportClient() = default;

void FViewportClient::Tick(float DeltaSeconds) const
{
	// 필요 시 뷰 모드/기즈모 상태 업데이트 등
	// 카메라 입력은 ACameraActor::Tick()에서 처리
    if (IsOrtho())
    {
        if (OrthoGraphicCameras)
        {
            OrthoGraphicCameras->Tick(DeltaSeconds);                     // 내부에서 입력 처리 + View/Proj 갱신
        }
    }
    else if (PerspectiveCamera)
    {
        if (PerspectiveCamera->GetCameraComponent())
        {
            PerspectiveCamera->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
        }
        PerspectiveCamera->Tick(DeltaSeconds);
    }
}

void FViewportClient::Draw(const FViewport* InViewport) const
{
	if (!InViewport) { return; }

	// Get World
	UWorld* World = GEngine->GetWorld();
	if (!World) { return; }

	// Get Camera
	ACameraActor* CurrentCamera = IsOrtho() ? OrthoGraphicCameras : PerspectiveCamera;
	if (!CurrentCamera) { return; }

	// SceneViewFamily
	FSceneViewFamily ViewFamily;
	ViewFamily.SetRenderTarget(const_cast<FViewport*>(InViewport));
	ViewFamily.SetCurrentTime(GEngine->GetWorld()->GetTimeSeconds());
	ViewFamily.SetDeltaWorldTime(GEngine->GetWorld()->GetDeltaSeconds());

	// SceneView
	FSceneView* View = new FSceneView();
	View->Initialize(CurrentCamera, const_cast<FViewport*>(InViewport), World);
	ViewFamily.AddView(View);

	// SceneRenderer
	if (FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(ViewFamily))
	{
		SceneRenderer->Render();
		delete SceneRenderer;
	}

	// UI 렌더링 (언리얼 패턴: 3D 장면 렌더링 후 UI 렌더링)
	if (auto* UISubsystem = GEngine->GetEngineSubsystem<UUISubsystem>())
	{
		UISubsystem->Render();
	}

	delete View;
}
