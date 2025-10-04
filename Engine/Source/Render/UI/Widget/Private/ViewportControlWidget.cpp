#include "pch.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"

#include "Manager/Viewport/Public/ViewportManager.h"
#include "Window/Public/ViewportClient.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/Splitter.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/BatchLines.h"

// 정적 멤버 정의
const char* UViewportControlWidget::ViewModeLabels[] = {
	"Lit", "Unlit", "WireFrame"
};

const char* UViewportControlWidget::ViewTypeLabels[] = {
	"Perspective", "OrthoTop", "OrthoBottom", "OrthoLeft", "OrthoRight", "OrthoFront", "OrthoBack"
};

UViewportControlWidget::UViewportControlWidget()
	: UWidget("Viewport Control Widget")
{
}

UViewportControlWidget::~UViewportControlWidget() = default;

void UViewportControlWidget::Initialize()
{
	UE_LOG("ViewportControlWidget: Initialized");
}

void UViewportControlWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UViewportControlWidget::RenderWidget()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	if (!ViewportManager.GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	RenderSplitterLines();

	// 뷰포트 툴바들 렌더링
	const auto& Viewports = ViewportManager.GetViewports();
	int32 N = static_cast<int32>(Viewports.size());

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportManager.GetViewportChange() == EViewportChange::Single)
	{
		N = 1;
	}

	for (int32 i = 0; i < N; ++i)
	{
		RenderViewportToolbar(i);
	}
}

void UViewportControlWidget::RenderViewportToolbar(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	if (ViewportIndex >= static_cast<int32>(Viewports.size()) || ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return;
	}

	const FRect& Rect = Viewports[ViewportIndex]->GetRect();
	if (Rect.W <= 0 || Rect.H <= 0)
	{
		return;
	}

	const int ToolbarH = Viewports[ViewportIndex]->GetToolbarHeight();
	const ImVec2 Vec1{static_cast<float>(Rect.X), static_cast<float>(Rect.Y)};
	const ImVec2 Vec2{static_cast<float>(Rect.X + Rect.W), static_cast<float>(Rect.Y + ToolbarH)};

	// 1) 툴바 배경 그리기
	ImDrawList* DrawLine = ImGui::GetBackgroundDrawList();
	DrawLine->AddRectFilled(Vec1, Vec2, IM_COL32(30, 30, 30, 100));
	DrawLine->AddLine(ImVec2(Vec1.x, Vec2.y), ImVec2(Vec2.x, Vec2.y), IM_COL32(70, 70, 70, 120), 1.0f);

	// 2) 툴바 UI 요소들을 직접 배치 (별도 창 생성하지 않음)
	// UI 요소들을 화면 좌표계에서 직접 배치
	ImGui::SetNextWindowPos(ImVec2(Vec1.x, Vec1.y));
	ImGui::SetNextWindowSize(ImVec2(Vec2.x - Vec1.x, Vec2.y - Vec1.y));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoTitleBar;

	// 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	char WinName[64];
	(void)snprintf(WinName, sizeof(WinName), "##ViewportToolbar%d", ViewportIndex);

	ImGui::PushID(ViewportIndex);
	if (ImGui::Begin(WinName, nullptr, flags))
	{
		// ViewMode 콤보박스
		EViewMode CurrentMode = Clients[ViewportIndex]->GetViewMode();
		int32 CurrentModeIndex = static_cast<int32>(CurrentMode);

		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::Combo("##ViewMode", &CurrentModeIndex, ViewModeLabels, IM_ARRAYSIZE(ViewModeLabels)))
		{
			if (CurrentModeIndex >= 0 && CurrentModeIndex < IM_ARRAYSIZE(ViewModeLabels))
			{
				Clients[ViewportIndex]->SetViewMode(static_cast<EViewMode>(CurrentModeIndex));
			}
		}

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// ViewType 드롭다운 버튼
		EViewType CurType = Clients[ViewportIndex]->GetViewType();
		int32 CurrentIdx = static_cast<int32>(CurType);
		const char* CurrentLabel = ViewTypeLabels[CurrentIdx];

		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::BeginCombo("##ViewType", CurrentLabel))
		{
			for (int i = 0; i < IM_ARRAYSIZE(ViewTypeLabels); ++i)
			{
				bool isSelected = (CurrentIdx == i);
				if (ImGui::Selectable(ViewTypeLabels[i], isSelected))
				{
					EViewType NewType = static_cast<EViewType>(i);
					Clients[ViewportIndex]->SetViewType(NewType);
					HandleCameraBinding(ViewportIndex, NewType, i);
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			// Perspective 선택 시 하위 옵션 표시
			if (CurType == EViewType::Perspective)
			{
				ImGui::Separator();
				ImGui::TextDisabled("VIEW");

				auto& ViewportManager = UViewportManager::GetInstance();
				const auto& PerspectiveCameras = ViewportManager.GetPerspectiveCameras();

				if (ViewportIndex < static_cast<int32>(PerspectiveCameras.size()) && PerspectiveCameras[ViewportIndex])
				{
					ACameraActor* Camera = PerspectiveCameras[ViewportIndex];
					if (Camera && Camera->GetCameraComponent())
					{
						UCameraComponent* CamComp = Camera->GetCameraComponent();

						ImGui::Spacing();

						// FOV
						float fov = CamComp->GetFovY();
						ImGui::SetNextItemWidth(180.0f);
						if (ImGui::DragFloat("Field of View", &fov, 1.0f, 30.0f, 120.0f, "%.1f"))
						{
							CamComp->SetFovY(fov);
						}

						// Near Plane
						float nearZ = CamComp->GetNearZ();
						ImGui::SetNextItemWidth(180.0f);
						if (ImGui::DragFloat("Near View Plane", &nearZ, 0.01f, 0.01f, 10.0f, "%.2f"))
						{
							CamComp->SetNearZ(nearZ);
						}

						// Far Plane
						float farZ = CamComp->GetFarZ();
						ImGui::SetNextItemWidth(180.0f);
						if (ImGui::DragFloat("Far View Plane", &farZ, 10.0f, 100.0f, 10000.0f, "%.0f"))
						{
							CamComp->SetFarZ(farZ);
						}
					}
				}
			}

			ImGui::EndCombo();
		}

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// 카메라 스피드 표시 및 조정
		RenderCameraSpeedControl(ViewportIndex);

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// 그리드 사이즈 조정
		RenderGridSizeControl();

		// 레이아웃 전환 버튼들을 우측 정렬
		{
			constexpr float RightPadding = 6.0f;
			constexpr float BetweenWidth = 24.0f;

			const float ContentRight = ImGui::GetWindowContentRegionMax().x;
			float TargetX = ContentRight - RightPadding - BetweenWidth;
			TargetX = std::max(TargetX, ImGui::GetCursorPosX());

			ImGui::SameLine();
			ImGui::SetCursorPosX(TargetX);
		}

		// 애니메이션 중이면 버튼 비활성화
		ImGui::BeginDisabled(ViewportManager.IsAnimating());

		// 레이아웃 전환 버튼들
		if (ViewportManager.GetViewportChange() == EViewportChange::Single)
		{
			if (FUEImgui::ToolbarIconButton("LayoutQuad", EUEViewportIcon::Quad, CurrentLayout == ELayout::Quad))
			{
				CurrentLayout = ELayout::Quad;
				ViewportManager.SetViewportChange(EViewportChange::Quad);

				// 애니메이션 시작: Single → Quad
				ViewportManager.StartLayoutAnimation(true, ViewportIndex);
			}
		}
		if (ViewportManager.GetViewportChange() == EViewportChange::Quad)
		{
			if (FUEImgui::ToolbarIconButton("LayoutSingle", EUEViewportIcon::Single, CurrentLayout == ELayout::Single))
			{
				CurrentLayout = ELayout::Single;
				ViewportManager.SetViewportChange(EViewportChange::Single);

				// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
				ViewportManager.PersistSplitterRatios();
				ViewportManager.StartLayoutAnimation(false, ViewportIndex);
			}
		}

		ImGui::EndDisabled();
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopID();
}

void UViewportControlWidget::RenderSplitterLines()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	SWindow* Root = ViewportManager.GetRoot();
	if (!Root)
	{
		return;
	}

	// 스플리터 선들을 UI 뒤에서 렌더링 (GetBackgroundDrawList 사용)
	Root->OnPaint();
}

void UViewportControlWidget::RenderCameraSpeedControl(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Clients = ViewportManager.GetClients();

	if (ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return;
	}

	// 현재 선택된 카메라 스피드 가져오기
	ACameraActor* CurrentCamera;
	if (Clients[ViewportIndex]->GetViewType() == EViewType::Perspective)
	{
		CurrentCamera = Clients[ViewportIndex]->GetPerspectiveCamera();
	}
	else
	{
		CurrentCamera = Clients[ViewportIndex]->GetOrthoCamera();
	}

	if (!CurrentCamera)
	{
		return;
	}

	// 스피드 수치 표시
	float CurrentSpeed = CurrentCamera->GetMoveSpeed();
	ImGui::Text("Speed: %.0f", CurrentSpeed);

	// 드롭다운 아이콘
	ImGui::SameLine();
	if (ImGui::SmallButton("▼##SpeedDropdown"))
	{
		ImGui::OpenPopup("SpeedPopup");
	}

	// 드롭다운 팝업
	if (ImGui::BeginPopup("SpeedPopup"))
	{
		ImGui::Text("카메라 스피드 설정");
		ImGui::Separator();

		// 미리 정의된 스피드 옵션들
		constexpr float SpeedOptions[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f};
		for (float Speed : SpeedOptions)
		{
			bool bIsSelected = (abs(CurrentSpeed - Speed) < 0.1f);
			char SpeedText[32];
			(void)snprintf(SpeedText, sizeof(SpeedText), "%.0f", Speed);
			if (ImGui::MenuItem(SpeedText, nullptr, bIsSelected))
			{
				CurrentCamera->SetMoveSpeed(Speed);
			}
		}

		ImGui::Separator();

		// 슬라이더로 세밀 조정
		float TempSpeed = CurrentSpeed;
		if (ImGui::SliderFloat("세밀 조정", &TempSpeed, ACameraActor::MIN_SPEED, ACameraActor::MAX_SPEED, "%.0f"))
		{
			CurrentCamera->SetMoveSpeed(TempSpeed);
		}

		ImGui::EndPopup();
	}
}

void UViewportControlWidget::RenderGridSizeControl()
{
	// Editor에서 BatchLines 포인터 가져오기
	UEditor* Editor = ULevelManager::GetInstance().GetEditor();
	if (!Editor)
	{
		return;
	}

	// Editor에서 현재 그리드 사이즈 가져오기
	UBatchLines* BatchLines = Editor->GetBatchLines();
	if (!BatchLines)
	{
		return;
	}

	float CurrentGridSize = BatchLines->GetCellSize();

	// 그리드 사이즈 표시
	ImGui::Text("Grid: %.1f", CurrentGridSize);

	// 드롭다운 아이콘
	ImGui::SameLine();
	if (ImGui::SmallButton("▼##GridDropdown"))
	{
		ImGui::OpenPopup("GridPopup");
	}

	// 드롭다운 팝업
	if (ImGui::BeginPopup("GridPopup"))
	{
		ImGui::Text("그리드 사이즈 설정");
		ImGui::Separator();

		// 미리 정의된 그리드 사이즈 옵션들
		constexpr float GridOptions[] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f};
		for (float GridSize : GridOptions)
		{
			bool bIsSelected = (abs(CurrentGridSize - GridSize) < 0.01f);
			char GridText[32];
			(void)snprintf(GridText, sizeof(GridText), "%.1f", GridSize);
			if (ImGui::MenuItem(GridText, nullptr, bIsSelected))
			{
				BatchLines->UpdateUGridVertices(GridSize);
				UE_LOG("ViewportControl: 그리드 사이즈를 %.1f로 변경", GridSize);
			}
		}

		ImGui::Separator();

		// 슬라이더로 세밀 조정
		float TempGridSize = CurrentGridSize;
		if (ImGui::SliderFloat("세밀 조정", &TempGridSize, 0.1f, 50.0f, "%.1f"))
		{
			BatchLines->UpdateUGridVertices(TempGridSize);
		}

		ImGui::EndPopup();
	}
}

void UViewportControlWidget::HandleCameraBinding(int32 ViewportIndex, EViewType NewType, int32 TypeIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Clients = ViewportManager.GetClients();

	if (ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return;
	}

	FViewportClient* Client = Clients[ViewportIndex];
	if (!Client)
	{
		return;
	}

	if (NewType == EViewType::Perspective)
	{
		// Perspective 카메라 바인딩
		const auto& PerspectiveCameras = ViewportManager.GetPerspectiveCameras();
		if (ViewportIndex < static_cast<int32>(PerspectiveCameras.size()) && PerspectiveCameras[ViewportIndex])
		{
			ACameraActor* PerspCamera = PerspectiveCameras[ViewportIndex];
			Client->SetPerspectiveCamera(PerspCamera);
			PerspCamera->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
			UE_LOG("ViewportControl: Perspective 카메라로 변경됨 (ViewportIndex: %d)", ViewportIndex);
		}
	}
	else
	{
		// Orthographic 카메라 바인딩
		const auto& OrthoCameras = ViewportManager.GetOrthographicCameras();

		// ViewType에 따라 적절한 OrthoCameras 인덱스 매핑
		int32 OrthoIdx = -1;
		switch (NewType)
		{
		case EViewType::OrthoTop: OrthoIdx = 0; break;
		case EViewType::OrthoBottom: OrthoIdx = 1; break;
		case EViewType::OrthoLeft: OrthoIdx = 2; break;
		case EViewType::OrthoRight: OrthoIdx = 3; break;
		case EViewType::OrthoFront: OrthoIdx = 4; break;
		case EViewType::OrthoBack: OrthoIdx = 5; break;
		default: return;
		}

		if (OrthoIdx >= 0 && OrthoIdx < static_cast<int32>(OrthoCameras.size()) && OrthoCameras[OrthoIdx])
		{
			ACameraActor* OrthoCamera = OrthoCameras[OrthoIdx];
			Client->SetOrthoCamera(OrthoCamera);
			OrthoCamera->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
			UE_LOG("ViewportControl: Orthographic 카메라로 변경됨 (ViewportIndex: %d, OrthoIdx: %d)", ViewportIndex, OrthoIdx);
		}
	}
}
