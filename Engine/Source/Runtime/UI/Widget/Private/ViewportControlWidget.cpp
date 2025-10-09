#include "pch.h"
#include "Runtime/UI/Widget/Public/ViewportControlWidget.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Renderer/Public/EditorGrid.h"
#include "Window/Public/ViewportClient.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/Splitter.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

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
	auto* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	if (!ViewportSS->GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	RenderSplitterLines();

	// 뷰포트 툴바들 렌더링
	const auto& Viewports = ViewportSS->GetViewports();
	int32 N = static_cast<int32>(Viewports.Num());

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportSS->GetViewportChange() == EViewportChange::Single)
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
	auto* ViewportManager = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	const auto& Viewports = ViewportManager->GetViewports();
	const auto& Clients = ViewportManager->GetClients();

	if (ViewportIndex >= Viewports.Num() || ViewportIndex >= Clients.Num())
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

			// Perspective 선택 시 하위 옵션 표시 - Camera deprecated
			if (CurType == EViewType::Perspective)
			{
				// TODO: Camera가 deprecated되어 FOV 등의 설정을 다른 방식으로 처리해야 함
				// ImGui::Separator();
				// ImGui::TextDisabled("VIEW");
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
		ImGui::BeginDisabled(ViewportManager->IsAnimating());

		// 레이아웃 전환 버튼들
		if (ViewportManager->GetViewportChange() == EViewportChange::Single)
		{
			if (FUEImgui::ToolbarIconButton("LayoutQuad", EUEViewportIcon::Quad, CurrentLayout == ELayout::Quad))
			{
				CurrentLayout = ELayout::Quad;
				ViewportManager->SetViewportChange(EViewportChange::Quad);

				// 애니메이션 시작: Single → Quad
				ViewportManager->StartLayoutAnimation(true, ViewportIndex);
			}
		}
		if (ViewportManager->GetViewportChange() == EViewportChange::Quad)
		{
			if (FUEImgui::ToolbarIconButton("LayoutSingle", EUEViewportIcon::Single, CurrentLayout == ELayout::Single))
			{
				CurrentLayout = ELayout::Single;
				ViewportManager->SetViewportChange(EViewportChange::Single);

				// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
				ViewportManager->PersistSplitterRatios();
				ViewportManager->StartLayoutAnimation(false, ViewportIndex);
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
	auto* ViewportManager = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	SWindow* Root = ViewportManager->GetRoot();
	if (!Root)
	{
		return;
	}

	// 스플리터 선들을 UI 뒤에서 렌더링 (GetBackgroundDrawList 사용)
	Root->OnPaint();
}

void UViewportControlWidget::RenderCameraSpeedControl(int32 ViewportIndex)
{
	auto* ViewportManager = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	const auto& Clients = ViewportManager->GetClients();

	if (ViewportIndex >= Clients.Num())
	{
		return;
	}

	FViewportClient* Client = Clients[ViewportIndex];
	if (!Client)
	{
		return;
	}

	// Perspective 뷰에서만 속도 컨트롤 표시
	if (Client->GetViewType() != EViewType::Perspective)
	{
		return;
	}

	// ViewportSubsystem에서 Perspective 카메라 속도 가져오기
	float CurrentSpeed = ViewportManager->GetPerspectiveMoveSpeed();
	ImGui::Text("Speed: %.1f", CurrentSpeed);

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
		constexpr float SpeedOptions[] = {10.0f, 20.0f, 30.0f, 50.0f, 100.0f, 200.0f, 300.0f};
		for (float Speed : SpeedOptions)
		{
			bool bIsSelected = (abs(CurrentSpeed - Speed) < 0.1f);
			char SpeedText[32];
			(void)snprintf(SpeedText, sizeof(SpeedText), "%.1f", Speed);
			if (ImGui::MenuItem(SpeedText, nullptr, bIsSelected))
			{
				ViewportManager->SetPerspectiveMoveSpeed(Speed);
			}
		}

		ImGui::Separator();

		// 슬라이더로 세밀 조정
		float TempSpeed = CurrentSpeed;
		if (ImGui::SliderFloat("세밀 조정", &TempSpeed, 10.0f, 500.0f, "%.1f"))
		{
			ViewportManager->SetPerspectiveMoveSpeed(TempSpeed);
		}

		ImGui::EndPopup();
	}
}

void UViewportControlWidget::RenderGridSizeControl()
{
	// 현재 그리드 사이즈
	float CurrentGridSize = FEditorGrid::GetCellSize();

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
				FEditorGrid::SetCellSize(GridSize);
				UE_LOG("뷰포트컨트롤: 그리드 사이즈를 %.1f로 변경", GridSize);
			}
		}

		ImGui::Separator();

		// 슬라이더로 세밀 조정
		float TempGridSize = CurrentGridSize;
		if (ImGui::SliderFloat("세밀 조정", &TempGridSize, 0.1f, 50.0f, "%.1f"))
		{
			FEditorGrid::SetCellSize(TempGridSize);
		}

		ImGui::EndPopup();
	}
}

void UViewportControlWidget::HandleCameraBinding(int32 ViewportIndex, EViewType NewType, int32 TypeIndex)
{
	auto* ViewportManager = GEngine->GetEngineSubsystem<UViewportSubsystem>();

	// The ViewportSubsystem is now responsible for handling camera bindings when the view type changes.
	// We just need to notify it of the change.
	ViewportManager->SetViewportViewType(ViewportIndex, NewType);
}
