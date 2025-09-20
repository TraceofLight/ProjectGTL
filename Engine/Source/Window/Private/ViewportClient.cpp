#include "pch.h"
#include "Editor/Public/Camera.h"
#include "Source/Manager/Viewport/Public/ViewportManager.h"
#include "Source/Window/Public/Viewport.h"
#include "Source/Window/Public/ViewportClient.h"

FViewportClient::FViewportClient()
{
}

FViewportClient::~FViewportClient()
{
}

void FViewportClient::Tick(float InDeltaSeconds)
{
	// 필요 시 뷰 모드/기즈모 상태 업데이트 등
	// 카메라 입력은 UCamera::Update()가 UInputManager를 직접 읽는 구조라면 여기서 호출만 하면 됨.
	if (IsOrtho())
	{
		UCamera* Camera = UViewportManager::GetInstance().GetOrthographicCamera();
		ApplyOrthoBasisForViewType(*Camera);  // Top/Front/Right 등 기준 축 세팅
		Camera->Update();                     // 내부에서 입력 처리 + View/Proj 갱신
	}
	else
	{
		PerspectiveCamera->Update();
	}
}

void FViewportClient::Draw(FViewport* InViewport)
{
	if (!InViewport) { return; }

	// 리프 Rect 기준 Aspect 반영
	const float Aspect = InViewport->GetAspect();
	FMatrix ViewMatrix, ProjMMatrix;

	if (IsOrtho())
	{
		UCamera* Camera = UViewportManager::GetInstance().GetOrthographicCamera();
		Camera->SetAspect(Aspect);
		Camera->SetCameraType(ECameraType::ECT_Orthographic);
		Camera->UpdateMatrixByOrth();               
		ViewMatrix = Camera->GetFViewProjConstants().View;
		ProjMMatrix = Camera->GetFViewProjConstants().Projection;
	}
	else
	{
		PerspectiveCamera->SetAspect(Aspect);
		PerspectiveCamera->SetCameraType(ECameraType::ECT_Perspective);
		PerspectiveCamera->UpdateMatrixByPers();
		ViewMatrix = PerspectiveCamera->GetFViewProjConstants().View;
		ProjMMatrix = PerspectiveCamera->GetFViewProjConstants().Projection;
	}

	// 렌더러에 바인딩 (프로젝트의 렌더 경로에 맞춰 호출)
	//SetViewProjection(ViewMatrix, ProjM);
	//RenderWorld();
	//RenderEditorOverlays();
}

void FViewportClient::ApplyOrthoBasisForViewType(UCamera& OutCamera)
{
	// 카메라의 Forward / Up / Right를 뷰 타입 평면에 맞춰 잡아준다.
	// (Top=+Z에서 내려다봄, Bottom=-Z에서 올려봄, Front=+Y에서 -Y로, Back=-Y에서 +Y로, Right=+X, Left=-X)
	switch (ViewType)
	{
		case EViewType::OrthoTop:     OutCamera.GetRotation() = FVector(0, -90, 0); break; // Pitch -90 (Z-로 바라봄)
		case EViewType::OrthoBottom:  OutCamera.GetRotation() = FVector(0, 90, 0); break; // Pitch +90 (Z+로 바라봄)
		case EViewType::OrthoFront:   OutCamera.GetRotation() = FVector(0, 0, 0); break; // Y-쪽을 Forward로 쓰는 좌표계면 여기 보정
		case EViewType::OrthoBack:    OutCamera.GetRotation() = FVector(0, 0, 180); break;
		case EViewType::OrthoRight:   OutCamera.GetRotation() = FVector(0, 0, 90); break;
		case EViewType::OrthoLeft:    OutCamera.GetRotation() = FVector(0, 0, -90); break;
	default: break;
	}
}
