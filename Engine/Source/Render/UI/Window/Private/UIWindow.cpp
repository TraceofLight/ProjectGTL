#include "pch.h"
#include "Render/UI/Window/Public/UIWindow.h"

#include "ImGui/imgui_internal.h"

#include "Render/UI/Widget/Public/Widget.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Render/UI/Window/Public/MainMenuWindow.h"

int UUIWindow::IssuedWindowID = 0;

UUIWindow::UUIWindow(const FUIWindowConfig& InConfig)
	: Config(InConfig)
	  , CurrentState(InConfig.InitialState)
{
	// 고유한 윈도우 ID 생성
	WindowID = ++IssuedWindowID;

	// 윈도우 플래그 업데이트
	Config.UpdateWindowFlags();

	// 초기 상태 설정
	LastWindowSize = Config.DefaultSize;
	LastWindowPosition = Config.DefaultPosition;
	bIsWindowOpen = true;

	if (IssuedWindowID == 1)
	{
		cout << "UIWindow: Created: " << WindowID << "\n";
	}
	else
	{
		UE_LOG("UIWindow: Created: %u", WindowID);
	}
}

UUIWindow::~UUIWindow()
{
	for (UWidget* Widget : Widgets)
	{
		if (!Widget->IsSingleton())
		{
			SafeDelete(Widget);
		}
	}
}

/**
 * @brief 뷰포트가 리사이징 되었을 때 앵커/좌상단 기준 상대 위치 비율을 고정하는 로직
 */
void UUIWindow::OnMainWindowResized() const
{
	if (!ImGui::GetCurrentContext() || !IsVisible())
		return;

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const ImVec2 CurrentViewportSize = Viewport->WorkSize;
	float MenuBarOffset = GetMenuBarOffset();

	const ImVec2 Anchor = PositionRatio;
	const ImVec2 Pivot = {0.f, 0.f};

	ImVec2 ResponsiveSize(
		CurrentViewportSize.x * SizeRatio.x,
		CurrentViewportSize.y * SizeRatio.y
	);

	ImVec2 TargetPosition(
		Viewport->WorkPos.x + CurrentViewportSize.x * Anchor.x,
		Viewport->WorkPos.y + CurrentViewportSize.y * Anchor.y
	);

	ImVec2 FinalPosition(
		TargetPosition.x - ResponsiveSize.x * Pivot.x,
		TargetPosition.y - ResponsiveSize.y * Pivot.y
	);

	ImGui::SetWindowSize(ResponsiveSize, ImGuiCond_Always);
	ImGui::SetWindowPos(FinalPosition, ImGuiCond_Always);
}

/**
 * @brief 윈도우가 뷰포트 범위 밖으로 나갔을 시 클램프 하는 로직
 */
void UUIWindow::ClampWindow() const
{
	if (!IsVisible())
	{
		return;
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const ImVec2 WorkPosition = Viewport->WorkPos;
	const ImVec2 WorkSize = Viewport->WorkSize;
	float MenuBarOffset = GetMenuBarOffset();

	ImVec2 Position = LastWindowPosition;
	ImVec2 Size = LastWindowSize;

	bool bSizeChanged = false;

	if (Size.x > WorkSize.x)
	{
		Size.x = WorkSize.x;
		bSizeChanged = true;
	}

	if (Size.y > WorkSize.y)
	{
		Size.y = WorkSize.y;
		bSizeChanged = true;
	}

	if (bSizeChanged)
	{
		ImGui::SetWindowSize(Size);
	}

	bool bPositionChanged = false;

	if (Position.x + Size.x > WorkPosition.x + WorkSize.x)
	{
		Position.x = WorkPosition.x + WorkSize.x - Size.x;
		bPositionChanged = true;
	}

	if (Position.y + Size.y > WorkPosition.y + WorkSize.y)
	{
		Position.y = WorkPosition.y + WorkSize.y - Size.y;
		bPositionChanged = true;
	}

	if (Position.x < WorkPosition.x)
	{
		Position.x = WorkPosition.x;
		bPositionChanged = true;
	}

	if (Position.y < WorkPosition.y)
	{
		Position.y = WorkPosition.y;
		bPositionChanged = true;
	}

	if (bPositionChanged)
	{
		ImGui::SetWindowPos(Position);
	}
}

/**
 * @brief 내부적으로 사용되는 윈도우 렌더링 처리
 * 서브클래스에서 직접 호출할 수 없도록 접근한정자 설정
 */
void UUIWindow::RenderWindow()
{
	// 숨겨진 상태면 렌더링하지 않음
	if (!IsVisible())
	{
		return;
	}

	// 도킹 설정 적용
	ApplyDockingSettings();

	// 메인 메뉴바 높이 고려용 오프셋 계산
	float MenuBarOffset = GetMenuBarOffset();

	// FIXME(KHJ): ImGui 윈도우 복원 로직이 잘 작동하지 않음. 중복 코드도 많아 개편 및 작동 수정 필요
	// ImGui 윈도우 시작
	// 복원이 필요한 경우 위치와 크기 강제 설정
	if (bShouldRestorePosition && RestoreFrameCount > 0)
	{
		ImVec2 AdjustedPosition = LastWindowPosition;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		AdjustedPosition.y = max(AdjustedPosition.y, Viewport->WorkPos.y);
		ImGui::SetNextWindowPos(AdjustedPosition, ImGuiCond_Always);
		--RestoreFrameCount;
		if (RestoreFrameCount <= 0)
		{
			bShouldRestorePosition = false;
		}
	}

	else if (!bShouldRestorePosition)
	{
		ImVec2 AdjustedDefaultPosition = Config.DefaultPosition;
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		AdjustedDefaultPosition.y = max(AdjustedDefaultPosition.y, Viewport->WorkPos.y);
		ImGui::SetNextWindowPos(AdjustedDefaultPosition, ImGuiCond_FirstUseEver);
	}

	// ImGui의 내부 상태를 무시하고 강제로 적용
	if (bShouldRestoreSize && RestoreFrameCount > 0)
	{
		ImGui::SetNextWindowSize(LastWindowSize, ImGuiCond_Always);
		ImGui::SetNextWindowSizeConstraints(LastWindowSize, LastWindowSize);

		// ImGui 내부 상태 초기화를 위해 FirstUseEver도 시도
		ImGui::SetNextWindowSize(LastWindowSize, ImGuiCond_FirstUseEver);

		// 크기 복원도 같은 프레임 카운터 사용
		if (RestoreFrameCount <= 0)
		{
			bShouldRestoreSize = false;
			bForceSize = false;
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(10000, 10000));
		}
	}

	else if (!bShouldRestoreSize)
	{
		ImGui::SetNextWindowSize(Config.DefaultSize, ImGuiCond_FirstUseEver);
	}

	bool bIsOpen = bIsWindowOpen;

	// Console 윈도우가 방금 열렸다면 Collapse 상태 해제
	if (Config.WindowTitle == "Console" && bIsWindowOpen)
	{
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
	}

	if (ImGui::Begin(Config.WindowTitle.ToString().data(), &bIsOpen, Config.WindowFlags))
	{
		// 잘 적용되지 않는 문제로 인해 여러 번 강제 적용 시도
		if (bShouldRestoreSize && RestoreFrameCount > 0)
		{
			ImGui::SetWindowSize(LastWindowSize, ImGuiCond_Always);
			ImGui::SetWindowSize(LastWindowSize);
			if (bShouldRestorePosition)
			{
				ImGui::SetWindowPos(LastWindowPosition, ImGuiCond_Always);
			}
		}

		if (!bIsResized)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 currentPos = ImGui::GetWindowPos();
			ImVec2 currentSize = ImGui::GetWindowSize();
			ImVec2 pivot = {0.f, 0.f};

			PositionRatio.x = (currentPos.x - viewport->WorkPos.x + currentSize.x * pivot.x) / viewport->WorkSize.x;
			PositionRatio.y = (currentPos.y - viewport->WorkPos.y + currentSize.y * pivot.y) / viewport->WorkSize.y;

			SizeRatio.x = currentSize.x / viewport->WorkSize.x;
			SizeRatio.y = currentSize.y / viewport->WorkSize.y;
		}
		// 실제 UI 컨텐츠 렌더링
		RenderWidget();

		// 윈도우 정보 업데이트
		UpdateWindowInfo();
	}

	if (bIsResized)
	{
		OnMainWindowResized();
		bIsResized = false;
	}

	// Console 윈도우가 Collapse 상태인지 확인
	if (Config.WindowTitle == "Console" && IsVisible())
	{
		if (ImGui::IsWindowCollapsed())
		{
			// Collapse 상태면 자동으로 Hidden 처리
			bIsOpen = false;
			bIsWindowOpen = false;
			SetWindowState(EUIWindowState::Hidden);
			OnWindowHidden();

			// BottomBarWidget의 bShowConsole 플래그 동기화
			auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
			if (auto MainMenuWindow = UISS->FindUIWindow(FName("MainMenuBar")))
			{
				if (auto BottomBarWidget = static_cast<UMainMenuWindow*>(MainMenuWindow.Get())->GetBottomBarWidget())
				{
					BottomBarWidget->SetShowConsole(false);
				}
			}
		}
	}

	ImGui::End();

	// 윈도우가 닫혔는지 확인
	if (!bIsOpen && bIsWindowOpen)
	{
		// bClosable이 false인 경우
		if (!Config.bClosable)
		{
			// 열린 상태로 유지
			bIsWindowOpen = true;
			return;
		}

		// X 버튼이 클릭됨
		if (OnWindowClose())
		{
			bIsWindowOpen = false;
			SetWindowState(EUIWindowState::Hidden);

			const FString& Title = Config.WindowTitle.ToString();
			if (Title == "Outliner" || Title == "Details")
			{
				GEngine->GetEngineSubsystem<UUISubsystem>()->ForceArrangeRightPanels();
			}
			else if (Title == "Console")
			{
				// BottomBarWidget의 bShowConsole 플래그 동기화
				auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
				if (auto MainMenuWindow = UISS->FindUIWindow(FName("MainMenuBar")))
				{
					if (auto BottomBarWidget = static_cast<UMainMenuWindow*>(MainMenuWindow.Get())->GetBottomBarWidget())
					{
						BottomBarWidget->SetShowConsole(false);
					}
				}
			}

			// 윈도우 숨김 이벤트 호출
			OnWindowHidden();
			UE_LOG("UIWindow: Window closed: %s", Config.WindowTitle.ToString().data());
		}
		else
		{
			// 닫기 취소
			bIsWindowOpen = true;
		}
	}
}

void UUIWindow::RenderWidget() const
{
	for (auto Widget : Widgets)
	{
		Widget->RenderWidget();
		Widget->PostProcess();
	}
}

void UUIWindow::Update() const
{
	for (auto Widget : Widgets)
	{
		Widget->Update();
	}
}

/**
 * @brief 윈도우 위치와 크기 자동 설정
 * XXX(KHJ): 기본적으로 가볍게 어디에 위치할지 세팅할 수 있게 잡아두긴 했는데 잘 안 쓰는 느낌, 필요 없다면 Deprecated 처리할 것
 */
void UUIWindow::ApplyDockingSettings() const
{
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	float WorkPosY = Viewport ? Viewport->WorkPos.y : 0.0f;
	float WorkSizeY = Viewport ? Viewport->WorkSize.y : ScreenHeight;

	switch (Config.DockDirection)
	{
	case EUIDockDirection::Left:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, WorkSizeY), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Right:
		ImGui::SetNextWindowPos(ImVec2(ScreenWidth - Config.DefaultSize.x, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, WorkSizeY), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Top:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Bottom:
		ImGui::SetNextWindowPos(ImVec2(0, WorkPosY + WorkSizeY - Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, Config.DefaultSize.y), ImGuiCond_FirstUseEver);
		break;

	case EUIDockDirection::Center:
		{
			ImVec2 Center = ImVec2(ScreenWidth * 0.5f, WorkPosY + WorkSizeY * 0.5f);
			ImVec2 WindowPosition = ImVec2(Center.x - Config.DefaultSize.x * 0.5f,
			                               Center.y - Config.DefaultSize.y * 0.5f);
			// 최소 Y 위치는 작업 영역 시작 지점으로 제한
			WindowPosition.y = max(WindowPosition.y, WorkPosY);
			ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_FirstUseEver);
		}
		break;

	case EUIDockDirection::None:
	default:
		// 기본 위치 사용
		break;
	}
}

/**
 * @brief ImGui 컨텍스트에서 현재 윈도우 정보 업데이트
 */
void UUIWindow::UpdateWindowInfo()
{
	// 현재 윈도우가 포커스되어 있는지 확인
	bool bCurrentlyFocused = ImGui::IsWindowFocused();

	if (bCurrentlyFocused != bIsFocused)
	{
		bIsFocused = bCurrentlyFocused;

		if (bIsFocused)
		{
			OnFocusGained();
		}
		else
		{
			OnFocusLost();
		}
	}

	// 윈도우 크기와 위치 업데이트
	LastWindowSize = ImGui::GetWindowSize();
	LastWindowPosition = ImGui::GetWindowPos();

	// Console 윈도우인 경우 위치와 너비를 하단에 고정
	if (Config.WindowTitle == "Console" && IsVisible())
	{
		auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
		const float ScreenWidth = ImGui::GetIO().DisplaySize.x;
		const float ScreenHeight = ImGui::GetIO().DisplaySize.y;
		const float BottomBarHeight = UISS->GetBottomBarHeight();
		const float RightPanelWidth = UISS->GetRightPanelWidth();

		const float TargetX = 0.0f;
		const float TargetWidth = ScreenWidth - RightPanelWidth;

		// 너비 고정
		if (abs(LastWindowSize.x - TargetWidth) > 1.0f)
		{
			LastWindowSize.x = TargetWidth;
			ImGui::SetWindowSize(ImVec2(TargetWidth, LastWindowSize.y));
		}

		// Y 위치는 높이 조정 후 계산 (하단 고정)
		const float TargetY = ScreenHeight - BottomBarHeight - LastWindowSize.y;

		// 위치 고정
		if (abs(LastWindowPosition.x - TargetX) > 1.0f || abs(LastWindowPosition.y - TargetY) > 1.0f)
		{
			LastWindowPosition.x = TargetX;
			LastWindowPosition.y = TargetY;
			ImGui::SetWindowPos(LastWindowPosition);
		}

		// 높이 저장
		UISS->SaveConsoleHeight(LastWindowSize.y);
	}
}

/**
 * @brief 메인 메뉴바 높이를 고려한 오프셋을 반환하는 함수
 * @return 메뉴바 높이 (px)
 */
float UUIWindow::GetMenuBarOffset() const
{
	if (Config.WindowTitle.ToString() == "MainMenuBar")
	{
		return 0.0f;
	}

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	if (Viewport)
	{
		float menuBarHeight = Viewport->WorkPos.y - Viewport->Pos.y;

		if (menuBarHeight > 0)
		{
			return menuBarHeight;
		}
	}

	// 기본값으로 임의의 표준 메뉴바 높이 반환
	return 23.0f;
}
