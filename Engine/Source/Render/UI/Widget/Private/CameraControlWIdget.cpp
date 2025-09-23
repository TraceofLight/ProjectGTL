#include "pch.h"

#include "Manager/Viewport/Public/ViewportManager.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/ViewportClient.h"

IMPLEMENT_CLASS(UCameraControlWidget, UWidget)

#include "Editor/Public/Camera.h"

// Camera Mode
static const char* CameraMode[] = {
	"Perspective",
	"Orthographic"
};

UCameraControlWidget::UCameraControlWidget()
	: UWidget("Camera Control Widget")
{
}

UCameraControlWidget::~UCameraControlWidget() = default;

void UCameraControlWidget::Initialize()
{
}

/**
 * @brief 카메라 정보 업데이트 함수
 * Viewport Manager로부터 현재 사용하는 카메라가 있는지 확인하고, 있다면 해당 정보를 세팅
 */
void UCameraControlWidget::Update()
{
}

void UCameraControlWidget::RenderWidget()
{
	if (!Camera)
	{
		ImGui::TextUnformatted("Camera not set.");
		ImGui::Separator();
		ImGui::TextUnformatted("Call SetCamera(camera*) after creating this window.");
		return;
	}

	ImGui::TextUnformatted("Camera Transform");
	ImGui::Spacing();

	// 카메라 이동속도 표시 및 조절
	// 슬라이더 값이 변경된 경우, 실제 카메라 객체에 새로운 속도를 설정
	float CurrentSpeed = Camera->GetMoveSpeed();
	if (ImGui::SliderFloat("Move Speed", &CurrentSpeed, UCamera::MIN_SPEED, UCamera::MAX_SPEED, "%.1f"))
	{
		Camera->SetMoveSpeed(CurrentSpeed);
	}
	ImGui::Spacing();

	if (ImGui::Combo("Mode", &CameraModeIndex, CameraMode, IM_ARRAYSIZE(CameraMode)))
	{
		PushToCamera();
	}

	auto& Location = Camera->GetLocation();
	auto& Rotation = Camera->GetRotation();


	if (ImGui::DragFloat3("Camera Location", &Location.X, 0.05f))
	{
		// 위치 바뀌면 바로 뷰 갱신
		if (CameraModeIndex == 0)
		{
			Camera->UpdateMatrixByPers();
		}
		else
		{
			Camera->UpdateMatrixByOrth();
		}
	}

	// 회전 입력 및 보정 (degree 단위)
	bool RotationChanged = false;
	RotationChanged |= ImGui::DragFloat3("Camera Rotation", &Rotation.X, 0.1f);

	// Pitch / Yaw 간단 보정
	Rotation.X = max(-89.0f, Rotation.X);
	Rotation.X = min(89.0f, Rotation.X);
	Rotation.Y = max(-180.0f, Rotation.Y);
	Rotation.Y = min(180.0f, Rotation.Y);

	if (RotationChanged)
	{
		if (CameraModeIndex == 0) Camera->UpdateMatrixByPers();
		else Camera->UpdateMatrixByOrth();
	}

	ImGui::TextUnformatted("Camera Optics");
	ImGui::Spacing();

	// FOV: 원근에서 주로 사용, 직교에서는 화면 폭에만 영향(너는 Ortho에서도 FovY로 OrthoWidth 계산함)
	// 카메라 코드에 맞춰 그대로 노출
	bool bChanged = false;
	bChanged |= ImGui::SliderFloat("FOV", &UiFovY, 1.0f, 170.0f, "%.1f");

	// Near / Far
	bChanged |= ImGui::DragFloat("Z Near", &UiNearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
	bChanged |= ImGui::DragFloat("Z Far", &UiFarZ, 0.1f, 0.001f, 1e7f, "%.3f");

	if (bChanged)
	{
		PushToCamera();
	}

	// 퀵 버튼
	if (ImGui::Button("Reset Optics"))
	{
		UiFovY = 80.0f;
		UiNearZ = 0.1f;
		UiFarZ = 1000.0f;
		PushToCamera();
	}
}

void UCameraControlWidget::SyncFromCamera()
{
	if (!Camera)
	{
		return;
	}

	CameraModeIndex = (Camera->GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	UiFovY = Camera->GetFovY();
	UiNearZ = Camera->GetNearZ();
	UiFarZ = Camera->GetFarZ();
}

void UCameraControlWidget::PushToCamera()
{
	if (!Camera)
	{
		return;
	}

	// 카메라 모드 설정
	Camera->SetCameraType(CameraModeIndex == 0 ? ECameraType::ECT_Perspective : ECameraType::ECT_Orthographic);

	// 카메라 파라미터 설정
	UiNearZ = max(0.0001f, UiNearZ);
	UiFarZ = max(UiNearZ + 0.0001f, UiFarZ);
	UiFovY = min(170.0f, max(1.0f, UiFovY));

	Camera->SetNearZ(UiNearZ);
	Camera->SetFarZ(UiFarZ);
	Camera->SetFovY(UiFovY);

	// 카메라 업데이트
	if (CameraModeIndex == 0)
	{
		Camera->UpdateMatrixByPers();
	}
	else
	{
		Camera->UpdateMatrixByOrth();
	}
}
