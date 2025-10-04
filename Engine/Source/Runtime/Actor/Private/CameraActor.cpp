#include "pch.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_CLASS(ACameraActor, AActor)

ACameraActor::ACameraActor()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(FName("CameraComponent"));
	SetRootComponent(CameraComponent.Get());

	CurrentMoveSpeed = UConfigManager::GetInstance().GetCameraSensitivity();
}

ACameraActor::~ACameraActor()
{
	UConfigManager::GetInstance().SetCameraSensitivity(CurrentMoveSpeed);
}

void ACameraActor::BeginPlay()
{
	AActor::BeginPlay();
}

void ACameraActor::Tick()
{
	AActor::Tick();

	if (!CameraComponent)
		return;

	UInputManager& Input = UInputManager::GetInstance();

	/**
	 * @brief 마우스 우클릭 제어: 투영 타입별로 동작을 분기합니다.
	 */
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		if (CameraComponent->GetCameraType() == ECameraType::ECT_Orthographic)
		{
			// 마우스 드래그 → 패닝 (스크린 공간 X→Right, Y→Up)
			const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
			if (MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f)
			{
				// OrthoWidth/Height 기반 픽셀당 월드 단위 추정
				float widthPx = 1.0f, heightPx = 1.0f;
				if (URenderer::GetInstance().GetDeviceResources())
				{
					widthPx = max(1.0f, URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Width);
					heightPx = max(1.0f, URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Height);
				}
				const float FovY = CameraComponent->GetFovY();
				const float Aspect = CameraComponent->GetAspect();
				const float OrthoWidthNow = 2.0f * std::tanf(FVector::GetDegreeToRadian(FovY) * 0.5f);
				const float OrthoHeightNow = OrthoWidthNow / max(0.0001f, Aspect);
				const float worldPerPixelX = OrthoWidthNow / widthPx;
				const float worldPerPixelY = OrthoHeightNow / heightPx;

				FVector NewLocation = CameraComponent->GetRelativeLocation();
				NewLocation += (-CameraComponent->GetRight() * MouseDelta.X * worldPerPixelX) +
				               (CameraComponent->GetUp() * MouseDelta.Y * worldPerPixelY);
				CameraComponent->SetRelativeLocation(NewLocation);
			}

			// Ortho 휠 도리는 뷰포트 매니저에서 "마우스가 올라가 있는 뷰포트" 기준으로 처리함
		}
		else // Perspective
		{
			// 기존 FPS 스타일 유지 (WASD + 마우스 회전 + 휠로 속도 조절)
			FVector Direction = { 0,0,0 };
			if (Input.IsKeyDown(EKeyInput::A)) { Direction += -CameraComponent->GetRight(); }
			if (Input.IsKeyDown(EKeyInput::D)) { Direction += CameraComponent->GetRight(); }
			if (Input.IsKeyDown(EKeyInput::W)) { Direction += CameraComponent->GetForward(); }
			if (Input.IsKeyDown(EKeyInput::S)) { Direction += -CameraComponent->GetForward(); }
			if (Input.IsKeyDown(EKeyInput::Q)) { Direction += -CameraComponent->GetUp(); }
			if (Input.IsKeyDown(EKeyInput::E)) { Direction += CameraComponent->GetUp(); }
			Direction.Normalize();

			FVector NewLocation = CameraComponent->GetRelativeLocation();
			NewLocation += Direction * CurrentMoveSpeed * GDeltaTime;
			CameraComponent->SetRelativeLocation(NewLocation);

			float WheelDelta = Input.GetMouseWheelDelta();
			if (WheelDelta != 0.0f)
			{
				AdjustMoveSpeed(WheelDelta * SPEED_ADJUST_STEP);
				Input.SetMouseWheelDelta(0.0f);
			}

			const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
			FVector NewRotation = CameraComponent->GetRelativeRotation();
			NewRotation.Z += MouseDelta.X * KeySensitivityDegPerPixel;
			NewRotation.Y += MouseDelta.Y * KeySensitivityDegPerPixel;

			if (NewRotation.Z > 180.0f) NewRotation.Z -= 360.0f;
			if (NewRotation.Z < -180.0f) NewRotation.Z += 360.0f;
			if (NewRotation.Y > 89.0f)  NewRotation.Y = 89.0f;
			if (NewRotation.Y < -89.0f) NewRotation.Y = -89.0f;

			CameraComponent->SetRelativeRotation(NewRotation);
		}
	}

	if (URenderer::GetInstance().GetDeviceResources())
	{
		float Width = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Width;
		float Height = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo().Height;
		CameraComponent->SetAspect(Width / Height);
	}

	switch (CameraComponent->GetCameraType())
	{
	case ECameraType::ECT_Perspective:
		CameraComponent->UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		CameraComponent->UpdateMatrixByOrth();
		break;
	}

	// TEST CODE
	URenderer::GetInstance().UpdateConstant(CameraComponent->GetFViewProjConstants());
}
