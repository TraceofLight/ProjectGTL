#include "pch.h"
#include "Runtime/Actor/Public/CameraActor.h"

#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Subsystem/Config/Public/ConfigSubsystem.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Renderer/Public/RendererModule.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

IMPLEMENT_CLASS(ACameraActor, AActor)

ACameraActor::ACameraActor()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(FName("CameraComponent"));
	SetRootComponent(CameraComponent.Get());

	if (UConfigSubsystem* ConfigSubsystem = GEngine ? GEngine->GetEngineSubsystem<UConfigSubsystem>() : nullptr)
	{
		CurrentMoveSpeed = ConfigSubsystem->GetCameraSensitivity();
	}
	else
	{
		CurrentMoveSpeed = DEFAULT_SPEED;
	}
}

ACameraActor::~ACameraActor()
{
	if (UConfigSubsystem* ConfigSubsystem = GEngine ? GEngine->GetEngineSubsystem<UConfigSubsystem>() : nullptr)
	{
		ConfigSubsystem->SetCameraSensitivity(CurrentMoveSpeed);
	}
}

void ACameraActor::BeginPlay()
{
	AActor::BeginPlay();
}

void ACameraActor::Tick(float DeltaSeconds)
{
	AActor::Tick(DeltaSeconds);

	if (!CameraComponent)
		return;

	// SceneHierarchyWidget에서 카메라 애니메이션 상태 확인
	auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
	TObjectPtr<UWidget> SceneHierarchyWidgetPtr = UISS->FindWidget(FName("Scene Hierarchy Widget"));
	USceneHierarchyWidget* SceneHierarchyWidget = Cast<USceneHierarchyWidget>(SceneHierarchyWidgetPtr);

	bool bIsAnimating = false;

	if (SceneHierarchyWidget)
	{
		bIsAnimating = SceneHierarchyWidget->IsCameraAnimating(TObjectPtr<ACameraActor>(this));
	}

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 마우스 우클릭 제어: 투영 타입별로 동작을 분기
	// 애니메이션 중 우클릭이 들어오면 애니메이션을 중단하고 카메라 조작을 시작합니다
	if (InputSubsystem->IsKeyDown(EKeyInput::MouseRight))
	{
		// 애니메이션 중이면 중단
		if (bIsAnimating && SceneHierarchyWidget)
		{
			SceneHierarchyWidget->StopCameraAnimation(TObjectPtr<ACameraActor>(this));
		}
		if (CameraComponent->GetCameraType() == ECameraType::ECT_Orthographic)
		{
			// 마우스 드래그 -> 패닝 (스크린 공간 X -> Right, Y -> Up)
			const FVector MouseDelta = InputSubsystem->GetMouseDelta();
			if (MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f)
			{
				// OrthoWidth/Height 기반 픽셀당 월드 단위 추정
				float widthPx = 1.0f, heightPx = 1.0f;
				// TODO: 대체 API를 통해 viewport 정보 가져오기 필요
				/*
				if (GEngine->GetRenderModule()->GetDeviceResources())
				{
					widthPx = max(1.0f, GEngine->GetRenderModule()->GetDeviceResources()->GetViewportInfo().Width);
					heightPx = max(1.0f, GEngine->GetRenderModule()->GetDeviceResources()->GetViewportInfo().Height);
				}
				*/
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
		}
		else // Perspective
		{
			// 기존 FPS 스타일 유지 (WASD + 마우스 회전 + 휠로 속도 조절)
			FVector Direction = { 0,0,0 };
			if (InputSubsystem->IsKeyDown(EKeyInput::A)) { Direction += -CameraComponent->GetRight(); }
			if (InputSubsystem->IsKeyDown(EKeyInput::D)) { Direction += CameraComponent->GetRight(); }
			if (InputSubsystem->IsKeyDown(EKeyInput::W)) { Direction += CameraComponent->GetForward(); }
			if (InputSubsystem->IsKeyDown(EKeyInput::S)) { Direction += -CameraComponent->GetForward(); }
			if (InputSubsystem->IsKeyDown(EKeyInput::Q)) { Direction += -CameraComponent->GetUp(); }
			if (InputSubsystem->IsKeyDown(EKeyInput::E)) { Direction += CameraComponent->GetUp(); }
			Direction.Normalize();

			FVector NewLocation = CameraComponent->GetRelativeLocation();
			NewLocation += Direction * CurrentMoveSpeed * GDeltaTime;
			CameraComponent->SetRelativeLocation(NewLocation);

			float WheelDelta = InputSubsystem->GetMouseWheelDelta();
			if (WheelDelta != 0.0f)
			{
				AdjustMoveSpeed(WheelDelta * SPEED_ADJUST_STEP);
				InputSubsystem->SetMouseWheelDelta(0.0f);
			}

			const FVector MouseDelta = InputSubsystem->GetMouseDelta();
			FVector NewRotation = CameraComponent->GetRelativeRotation();
			NewRotation.Z += MouseDelta.X * KeySensitivityDegPerPixel;
			NewRotation.Y += MouseDelta.Y * KeySensitivityDegPerPixel;

			CameraComponent->SetRelativeRotation(NewRotation);
		}
	}

	// TODO: 대체 API를 통해 viewport 정보 가져오기 필요
	/*
	if (GEngine->GetRenderModule()->GetDeviceResources())
	{
		float Width = GEngine->GetRenderModule()->GetDeviceResources()->GetViewportInfo().Width;
		float Height = GEngine->GetRenderModule()->GetDeviceResources()->GetViewportInfo().Height;
		CameraComponent->SetAspect(Width / Height);
	}
	*/

	switch (CameraComponent->GetCameraType())
	{
	case ECameraType::ECT_Perspective:
		CameraComponent->UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		CameraComponent->UpdateMatrixByOrth();
		break;
	}

	// TODO: 새로운 렌더링 시스템에서 Constant 업데이트 방식 결정 필요
	// GEngine->GetRenderModule()->UpdateConstant(CameraComponent->GetFViewProjConstants());
}
