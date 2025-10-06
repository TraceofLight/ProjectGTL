#include "pch.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/ImGui/Public/ImGuiHelper.h"
#include "Render/UI/Widget/Public/Widget.h"
#include "Render/UI/Window/Public/MainMenuWindow.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Core/Public/AppWindow.h"

// For overlay rendering after ImGui::NewFrame()
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

// 이전 사이즈 추적을 위한 정적 변수
static ImVec2 LastOutlinerSize = ImVec2(0, 0);
static ImVec2 LastDetailSize = ImVec2(0, 0);
static bool bFirstRun = true;

IMPLEMENT_CLASS(UUISubsystem, UEngineSubsystem)

UUISubsystem::UUISubsystem()
{
	ImGuiHelper = new UImGuiHelper();
}

UUISubsystem::~UUISubsystem()
{
	if (ImGuiHelper)
	{
		delete ImGuiHelper;
		ImGuiHelper = nullptr;
	}
}

/**
 * @brief UI 서브시스템 초기화
 */
void UUISubsystem::Initialize()
{
	UE_LOG("UISubsystem: UI System 초기화 진행 중...");

	UIWindows.Empty();
	FocusedWindow = nullptr;
	TotalTime = 0.0f;

	// GEngine에서 AppWindow 가져와서 ImGui 초기화
	if (GEngine && GEngine->GetAppWindow())
	{
		HWND WindowHandle = GEngine->GetAppWindow()->GetWindowHandle();
		if (ImGuiHelper)
		{
			ImGuiHelper->Initialize(WindowHandle);
			cout << "UISubsystem: ImGui Initialized Successfully." << "\n";
		}
	}

	UE_LOG("UISubsystem: UI System 초기화 성공");
}

/**
 * @brief UI 서브시스템 종료 및 정리
 */
void UUISubsystem::Deinitialize()
{
	UE_LOG("UISubsystem: UI system 종료 중...");

	// ImGui 정리
	if (ImGuiHelper)
	{
		ImGuiHelper->Release();
	}

	// 모든 UI 윈도우 정리
	for (auto Window : UIWindows)
	{
		if (Window && !Window->IsSingleton())
		{
			Window->Cleanup();
			delete Window;
		}
	}

	UIWindows.Empty();
	FocusedWindow = nullptr;

	UE_LOG("UISubsystem: UI 시스템 종료 완료");
}

/**
 * @brief 모든 UI 윈도우 업데이트 (매 프레임 호출)
 */
void UUISubsystem::Tick(float DeltaSeconds)
{
	TotalTime += DeltaSeconds;

	// 모든 UI 윈도우 업데이트
	for (auto Window : UIWindows)
	{
		if (Window && Window->IsVisible())
		{
			Window->Update();
		}
	}

	// 포커스 상태 업데이트
	UpdateFocusState();

	// 오른쪽 패널 레이아웃 자동 정리
	ArrangeRightPanels();
}

/**
 * @brief 모든 UI 윈도우 렌더링
 */
void UUISubsystem::Render()
{
	if (!ImGuiHelper)
	{
		return;
	}

	// ImGui 프레임 시작
	ImGuiHelper->BeginFrame();

	// 뷰포트 자동 조정을 위해 메인 메뉴바를 가장 먼저 렌더링
	if (MainMenuWindow && MainMenuWindow->IsVisible())
	{
		MainMenuWindow->RenderWidget();
	}

	// 우선순위에 따라 정렬
	SortUIWindowsByPriority();

	// 나머지 UI 윈도우 렌더링
	for (auto Window : UIWindows)
	{
		if (Window && Window != MainMenuWindow)
		{
			Window->RenderWindow();
		}
	}

	// Overlay (splitter handles etc.) should render after NewFrame() and before ImGui::Render()
	if (auto* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>())
	{
		ViewportSS->RenderOverlay();
	}

	// ImGui 프레임 종료
	ImGuiHelper->EndFrame();
}

/**
 * @brief UI 윈도우 등록
 * @param InWindow 등록할 UI 윈도우
 * @return 등록 성공 여부
 */
bool UUISubsystem::RegisterUIWindow(TObjectPtr<UUIWindow> InWindow)
{
	if (!InWindow)
	{
		UE_LOG("UISubsystem: Error: Attempted To Register Null Window!");
		return false;
	}

	// 이미 등록된 윈도우인지 확인
	auto Iter = std::find(UIWindows.begin(), UIWindows.end(), InWindow);
	if (Iter != UIWindows.end())
	{
		UE_LOG("UISubsystem: Warning: Window Already Registered: %u", InWindow->GetWindowID());
		return false;
	}

	// 윈도우 초기화
	try
	{
		InWindow->Initialize();
	}
	catch (const exception& Exception)
	{
		UE_LOG("UISubsystem: Error: Window 생성에 실패했습니다 %u: %s", InWindow->GetWindowID(), Exception.what());
		return false;
	}

	UIWindows.Add(InWindow);

	UE_LOG("UISubsystem: UI Window 등록: %s", InWindow->GetWindowTitle().ToString().data());
	UE_LOG("UISubsystem: 전체 등록된 Window 갯수: %zu", UIWindows.Num());

	// 오른쪽 패널 윈도우가 등록되면 레이아웃 정리 호출
	const FString& WindowTitle = InWindow->GetWindowTitle().ToString();
	if (WindowTitle == "Outliner" || WindowTitle == "Details")
	{
		// 다른 UI 초기화가 완료된 후 레이아웃 정리
		bFirstRun = true;
		UE_LOG("UISubsystem: 오른쪽 패널 윈도우 등록 및 초기 레이아웃 세팅 예약");
	}

	return true;
}

/**
 * @brief UI 윈도우 등록 해제
 * @param InWindow 해제할 UI 윈도우
 * @return 해제 성공 여부
 */
bool UUISubsystem::UnregisterUIWindow(TObjectPtr<UUIWindow> InWindow)
{
	if (!InWindow)
	{
		return false;
	}

	const int32 FoundIndex = UIWindows.Find(InWindow);

	// 검색 실패 여부를 확인
	if (FoundIndex == -1)
	{
		UE_LOG("UISubsystem: Warning: Attempted to unregister non-existent window: %u", InWindow->GetWindowID());
		return false;
	}

	// 포커스된 윈도우였다면 포커스 해제
	if (FocusedWindow == InWindow)
	{
		FocusedWindow = nullptr;
	}

	// 윈도우 정리
	InWindow->Cleanup();

	UIWindows.RemoveAt(FoundIndex);

	UE_LOG("UISubsystem: UI Window 등록 해제: %u", InWindow->GetWindowID());
	UE_LOG("UISubsystem: 전체 등록된 Window 갯수: %d", UIWindows.Num());

	return true;
}

/**
 * @brief 이름으로 UI 윈도우 검색
 * @param InWindowName 검색할 윈도우 제목
 * @return 찾은 윈도우 (없으면 nullptr)
 */
TObjectPtr<UUIWindow> UUISubsystem::FindUIWindow(const FName& InWindowName) const
{
	for (auto Window : UIWindows)
	{
		if (Window && Window->GetWindowTitle() == InWindowName)
		{
			return Window;
		}
	}
	return nullptr;
}

TObjectPtr<UWidget> UUISubsystem::FindWidget(const FName& InWidgetName) const
{
	for (const auto Window : UIWindows)
	{
		for (auto Widget : Window->GetWidgets())
		{
			if (Widget->GetName() == InWidgetName)
			{
				return Widget;
			}
		}
	}
	return nullptr;
}

/**
 * @brief 모든 UI 윈도우 숨기기
 */
void UUISubsystem::HideAllWindows() const
{
	for (const auto Window : UIWindows)
	{
		if (Window)
		{
			Window->SetWindowState(EUIWindowState::Hidden);
		}
	}
	UE_LOG("UISubsystem: All Windows Hidden.");
}

/**
 * @brief 모든 UI 윈도우 보이기
 */
void UUISubsystem::ShowAllWindows() const
{
	for (const auto Window : UIWindows)
	{
		if (Window)
		{
			Window->SetWindowState(EUIWindowState::Visible);
		}
	}
	UE_LOG("UISubsystem: All Windows Shown.");
}

/**
 * @brief 특정 윈도우에 포커스 설정
 */
void UUISubsystem::SetFocusedWindow(TObjectPtr<UUIWindow> InWindow)
{
	if (FocusedWindow != InWindow)
	{
		if (FocusedWindow)
		{
			FocusedWindow->OnFocusLost();
		}

		FocusedWindow = InWindow;

		if (FocusedWindow)
		{
			FocusedWindow->OnFocusGained();
		}
	}
}

/**
 * @brief 디버그 정보 출력
 * 필요한 지점에서 호출해서 로그로 체크하는 용도
 */
void UUISubsystem::PrintDebugInfo() const
{
	UE_LOG("");
	UE_LOG("=== UI Subsystem Debug Info ===");
	UE_LOG("Total Time: %.2fs", TotalTime);
	UE_LOG("Registered Windows: %d", UIWindows.Num());
	UE_LOG("Focused Window: %s", (FocusedWindow ? to_string(FocusedWindow->GetWindowID()).c_str() : "None"));

	UE_LOG("UISubsystem: All ImGui windows hidden due to minimization.");
	UE_LOG("--- Window List ---");
	for (size_t i = 0; i < UIWindows.Num(); ++i)
	{
		auto Window = UIWindows[i];
		if (Window)
		{
			UE_LOG("[%zu] %u (%s)", i, Window->GetWindowID(), Window->GetWindowTitle().ToString().data());
			UE_LOG("    State: %s", (Window->IsVisible() ? "Visible" : "Hidden"));
			UE_LOG("    Priority: %d", Window->GetPriority());
			UE_LOG("    Focused: %s", (Window->IsFocused() ? "Yes" : "No"));
		}
	}
	UE_LOG("===========================");
	UE_LOG("UISubsystem: All ImGui windows hidden due to minimization.");
}

/**
 * @brief UI 윈도우들을 우선순위에 따라 정렬
 */
void UUISubsystem::SortUIWindowsByPriority()
{
	// 우선순위가 낮을수록 먼저 렌더링되고 가려짐
	std::sort(UIWindows.begin(), UIWindows.end(), [](const UUIWindow* A, const UUIWindow* B)
	{
		if (!A)
		{
			return false;
		}
		if (!B)
		{
			return true;
		}

		return A->GetPriority() < B->GetPriority();
	});
}

/**
 * @brief 포커스 상태 업데이트
 */
void UUISubsystem::UpdateFocusState()
{
	// ImGui에서 현재 포커스된 윈도우 찾기
	TObjectPtr<UUIWindow> NewFocusedWindow = nullptr;

	for (auto Window : UIWindows)
	{
		if (Window && Window->IsVisible() && Window->IsFocused())
		{
			NewFocusedWindow = Window;
			break;
		}
	}

	// 포커스 변경시 처리
	if (FocusedWindow != NewFocusedWindow)
	{
		SetFocusedWindow(NewFocusedWindow);
	}
}

/**
 * @brief 윈도우 프로시저 핸들러
 */
LRESULT UUISubsystem::WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	return UImGuiHelper::WndProcHandler(hwnd, msg, wParam, lParam);
}

void UUISubsystem::RepositionImGuiWindows() const
{
	// 1. 현재 화면(Viewport)의 작업 영역을 가져옵니다.
	for (auto& Window : UIWindows)
	{
		Window->SetIsResized(true);
	}
}

/**
 * @brief 메인 윈도우가 최소화될 때 호출되는 함수
 * 모든 ImGui 윈도우의 현재 상태를 저장
 */
void UUISubsystem::OnWindowMinimized()
{
	UE_LOG("UISubsystem: UI Minimize 작업 시작");

	if (bIsMinimized)
	{
		return;
	}

	bIsMinimized = true;
	SavedWindowStates.Empty();
	UE_LOG("UISubsystem: %d개의 윈도우에 대해 상태 저장 시도", UIWindows.Num());

	// 모든 UI 윈도우의 현재 상태 저장
	for (auto Window : UIWindows)
	{
		if (Window)
		{
			FUIWindowSavedState SavedState;
			SavedState.WindowID = Window->GetWindowID();
			SavedState.SavedPosition = Window->GetLastWindowPosition();
			SavedState.SavedSize = Window->GetLastWindowSize();
			SavedState.bWasVisible = Window->IsVisible();

			UE_LOG("UISubsystem: Saving Window ID=%u, Position=(%.1f,%.1f), Size=(%.1f,%.1f), Visible=%s",
			       SavedState.WindowID,
			       SavedState.SavedPosition.x, SavedState.SavedPosition.y,
			       SavedState.SavedSize.x, SavedState.SavedSize.y,
			       (SavedState.bWasVisible ? "true" : "false"));

			SavedWindowStates.Add(SavedState);
		}
	}

	UE_LOG("UISubsystem: 최소화로 인한 %d개의 윈도우 상태 저장 완료", SavedWindowStates.Num());
}

/**
 * @brief 메인 윈도우가 복원될 때 호출되는 함수
 * 저장된 상태로 모든 ImGui 윈도우를 복원
 */
void UUISubsystem::OnWindowRestored()
{
	UE_LOG("UISubsystem: UI Restore 작업 시작");

	if (!bIsMinimized)
	{
		return;
	}

	bIsMinimized = false;
	UE_LOG("UISubsystem: %d개의 윈도우에 대해 상태 복원 시도", SavedWindowStates.Num());

	// 저장된 상태로 모든 UI 윈도우 복원
	for (auto Window : UIWindows)
	{
		if (Window)
		{
			uint32 CurrentWindowID = Window->GetWindowID();
			UE_LOG("UISubsystem: Restoring Window ID=%u", CurrentWindowID);

			// 저장된 상태에서 해당 윈도우 찾기
			FUIWindowSavedState* FoundState = nullptr;
			for (auto& SavedState : SavedWindowStates)
			{
				if (SavedState.WindowID == CurrentWindowID)
				{
					FoundState = &SavedState;
					break;
				}
			}

			if (FoundState)
			{
				UE_LOG(
					"UISubsystem: Restoring Window ID=%u: Position=(%.1f,%.1f) -> (%.1f,%.1f), Size=(%.1f,%.1f) -> (%.1f,%.1f)",
					CurrentWindowID,
					Window->GetLastWindowPosition().x, Window->GetLastWindowPosition().y,
					FoundState->SavedPosition.x, FoundState->SavedPosition.y,
					Window->GetLastWindowSize().x, Window->GetLastWindowSize().y,
					FoundState->SavedSize.x, FoundState->SavedSize.y);

				// 위치와 크기 복원
				Window->SetLastWindowPosition(FoundState->SavedPosition);
				Window->SetLastWindowSize(FoundState->SavedSize);
				UE_LOG("UISubsystem: %u번 윈도우에 대해 이후 10프레임 동안 복원을 시도합니다", CurrentWindowID);

				// 가시성 복원
				if (FoundState->bWasVisible)
				{
					Window->SetWindowState(EUIWindowState::Visible);
					UE_LOG("UISubsystem: %u번 윈도우가 Visible 상태로 복원됩니다", CurrentWindowID);
				}
				else
				{
					Window->SetWindowState(EUIWindowState::Hidden);
					UE_LOG("UISubsystem: %u번 윈도우가 Hidden 상태로 복원됩니다", CurrentWindowID);
				}
			}
			else
			{
				UE_LOG("UISubsystem: %u번 윈도우에 대한 정보를 찾을 수 없습니다", CurrentWindowID);
			}
		}
	}

	UE_LOG("UISubsystem: %d개의 윈도우 상태가 복원되었습니다", SavedWindowStates.Num());
	SavedWindowStates.Empty();
}

/**
 * @brief 메인 메뉴바 윈도우를 등록하는 함수
 */
void UUISubsystem::RegisterMainMenuWindow(TObjectPtr<UMainMenuWindow> InMainMenuWindow)
{
	if (MainMenuWindow)
	{
		UE_LOG("UISubsystem: 메인 메뉴바 윈도우가 이미 등록되어 있습니다. 기존 윈도우를 교체합니다.");
	}

	MainMenuWindow = InMainMenuWindow;

	if (MainMenuWindow)
	{
		UE_LOG("UISubsystem: 메인 메뉴바 윈도우가 등록되었습니다");
	}
}

/**
 * @brief 메인 메뉴바의 높이를 반환하는 함수
 */
float UUISubsystem::GetMainMenuBarHeight() const
{
	if (MainMenuWindow)
	{
		return MainMenuWindow->GetMenuBarHeight();
	}

	return 0.0f;
}

/**
 * @brief 툴바의 높이를 반환하는 함수
 */
float UUISubsystem::GetToolbarHeight() const
{
	if (MainMenuWindow)
	{
		return MainMenuWindow->GetToolbarHeight();
	}

	return 0.0f;
}

/**
 * @brief 하단바의 높이를 반환하는 함수
 */
float UUISubsystem::GetBottomBarHeight() const
{
	if (MainMenuWindow)
	{
		return MainMenuWindow->GetBottomBarHeight();
	}

	return 0.0f;
}

/**
 * @brief 오른쪽 UI 패널(Outliner, Details)이 차지하는 너비를 계산하는 함수
 * @return 오른쪽 패널이 차지하는 픽셀 너비
 */
float UUISubsystem::GetRightPanelWidth() const
{
	// 화면 너비 가져오기
	const float ScreenWidth = ImGui::GetIO().DisplaySize.x;
	if (ScreenWidth <= 0.0f)
	{
		return 0.0f;
	}

	// 오른쪽 패널에 있는 UI 윈도우들의 가장 왼쪽 시작점 찾기
	float LeftmostX = ScreenWidth;
	bool bHasRightPanel = false;

	for (auto Window : UIWindows)
	{
		if (!Window || !Window->IsVisible())
		{
			continue;
		}

		// 오른쪽 패널로 간주되는 윈도우들 확인
		const FString& Title = Window->GetWindowTitle().ToString();
		if (Title == "Outliner" || Title == "Details")
		{
			// 현재 윈도우의 실제 위치 정보 가져오기
			ImVec2 WindowPosition = Window->GetLastWindowPosition();

			// 가장 왼쪽 시작점 업데이트
			LeftmostX = min(LeftmostX, WindowPosition.x);
			bHasRightPanel = true;
		}
	}

	// 오른쪽 패널이 없다면 0 반환
	if (!bHasRightPanel)
	{
		return 0.0f;
	}

	// 오른쪽 패널이 차지하는 너비 = 화면 전체 너비 - 가장 왼쪽 시작점
	const float RightPanelWidth = ScreenWidth - LeftmostX;

	return RightPanelWidth;
}

/**
 * @brief 오른쪽 UI 패널(Outliner, Details)의 레이아웃을 자동으로 정리
 * 사이즈 변경을 감지하여 마지막으로 변경된 윈도우의 너비를 기준으로 정리
 */
void UUISubsystem::ArrangeRightPanels()
{
	// 화면 크기와 메뉴바 높이 가져오기
	const float ScreenWidth = ImGui::GetIO().DisplaySize.x;
	const float ScreenHeight = ImGui::GetIO().DisplaySize.y;
	const float MenuBarHeight = GetMainMenuBarHeight();
	const float ToolbarHeight = GetToolbarHeight();
	const float BottomBarHeight = GetBottomBarHeight();
	const float AvailableHeight = ScreenHeight - MenuBarHeight - ToolbarHeight - BottomBarHeight;

	if (ScreenWidth <= 0.0f || AvailableHeight <= 0.0f)
	{
		return;
	}

	// Outliner와 Detail 윈도우 찾기
	TObjectPtr<UUIWindow> OutlinerWindow = nullptr;
	TObjectPtr<UUIWindow> DetailWindow = nullptr;

	for (auto Window : UIWindows)
	{
		if (!Window || !Window->IsVisible())
		{
			continue;
		}

		const FString& Title = Window->GetWindowTitle().ToString();
		if (Title == "Outliner")
		{
			OutlinerWindow = Window;
		}
		else if (Title == "Details")
		{
			DetailWindow = Window;
		}
	}

	// 적어도 하나의 윈도우는 있어야 정리 가능
	if (!OutlinerWindow && !DetailWindow)
	{
		return;
	}

	// 현재 화면에 존재하는 윈도우 사이즈 가져오기
	ImVec2 CurrentOutlinerSize = OutlinerWindow ? OutlinerWindow->GetLastWindowSize() : ImVec2(0, 0);
	ImVec2 CurrentDetailSize = DetailWindow ? DetailWindow->GetLastWindowSize() : ImVec2(0, 0);

	// 초기 실행 처리
	if (bFirstRun)
	{
		LastOutlinerSize = CurrentOutlinerSize;
		LastDetailSize = CurrentDetailSize;
		bFirstRun = false;

		// 초기 위치 설정
		ArrangeRightPanelsInitial(OutlinerWindow, DetailWindow, ScreenWidth, ScreenHeight, MenuBarHeight + ToolbarHeight,
		                          AvailableHeight);
		return;
	}

	// 두 패널이 모두 있을 때 크기 저장
	if (OutlinerWindow && DetailWindow)
	{
		SavedOutlinerHeightForDual = CurrentOutlinerSize.y;
		SavedDetailHeightForDual = CurrentDetailSize.y;
		SavedPanelWidth = max(CurrentOutlinerSize.x, CurrentDetailSize.x);
		bHasSavedDualLayout = true;
	}
	else if (OutlinerWindow || DetailWindow)
	{
		// 한 패널만 있을 때 너비 저장
		if (OutlinerWindow)
		{
			SavedPanelWidth = CurrentOutlinerSize.x;
		}
		else if (DetailWindow)
		{
			SavedPanelWidth = CurrentDetailSize.x;
		}
	}

	// 사이즈 변경 감지
	const float Tolerance = 1.0f;
	bool bOutlinerSizeChanged = OutlinerWindow && (abs(CurrentOutlinerSize.x - LastOutlinerSize.x) > Tolerance ||
		abs(CurrentOutlinerSize.y - LastOutlinerSize.y) > Tolerance);
	bool bDetailSizeChanged = DetailWindow && (abs(CurrentDetailSize.x - LastDetailSize.x) > Tolerance ||
		abs(CurrentDetailSize.y - LastDetailSize.y) > Tolerance);

	// 사이즈 변경이 없으면 빠르게 return
	if (!bOutlinerSizeChanged && !bDetailSizeChanged)
	{
		return;
	}

	// 마지막으로 변경된 윈도우의 너비를 기준으로 사용
	float TargetWidth;
	if (bOutlinerSizeChanged)
	{
		TargetWidth = CurrentOutlinerSize.x;
		UE_LOG_DEBUG("UISubsystem: Outliner 사이즈 변경 감지: 새 너비: %.1f", TargetWidth);
	}
	else // bDetailSizeChanged
	{
		TargetWidth = CurrentDetailSize.x;
		UE_LOG_DEBUG("UISubsystem: Detail 사이즈 변경 감지: 새 너비: %.1f", TargetWidth);
	}

	// 이전 사이즈 업데이트
	LastOutlinerSize = CurrentOutlinerSize;
	LastDetailSize = CurrentDetailSize;

	// 동적 레이아웃 업데이트
	ArrangeRightPanelsDynamic(OutlinerWindow, DetailWindow, ScreenWidth, ScreenHeight, MenuBarHeight + ToolbarHeight, AvailableHeight,
	                          TargetWidth, bOutlinerSizeChanged, bDetailSizeChanged);
}

/**
 * @brief 오른쪽 패널들의 초기 위치를 설정하는 함수
 */
void UUISubsystem::ArrangeRightPanelsInitial(TObjectPtr<UUIWindow> InOutlinerWindow, TObjectPtr<UUIWindow> InDetailWindow,
                                           float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
                                           float InAvailableHeight)
{
	// 너비 결정: 저장된 너비가 있으면 사용, 없으면 기본 너비 사용
	const float DefaultPanelWidth = InScreenWidth * 0.2f;
	const float ActualPanelWidth = (SavedPanelWidth > 0) ? SavedPanelWidth : DefaultPanelWidth;

	// 사용할 너비를 저장 (다음에 사용하기 위해)
	SavedPanelWidth = ActualPanelWidth;

	if (InOutlinerWindow && InDetailWindow)
	{
		// 두 패널 모두 있는 경우: 합리적인 크기로 분할
		const float OutlinerHeight = max(300.0f, InAvailableHeight * 0.4f); // 최소 300px 또는 40%
		const float DetailHeight = InAvailableHeight - OutlinerHeight;
		const float StartX = InScreenWidth - ActualPanelWidth;

		// Detail이 너무 작아지지 않도록 조정
		if (DetailHeight < 200.0f)
		{
			// Detail이 너무 작으면 Outliner를 줄임
			const float AdjustedOutlinerHeight = InAvailableHeight - 200.0f;
			const float AdjustedDetailHeight = 200.0f;

			InOutlinerWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight));
			InOutlinerWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, AdjustedOutlinerHeight));

			InDetailWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight + AdjustedOutlinerHeight));
			InDetailWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, AdjustedDetailHeight));
		}
		else
		{
			InOutlinerWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight));
			InOutlinerWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, OutlinerHeight));

			InDetailWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight + OutlinerHeight));
			InDetailWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, DetailHeight));
		}

		UE_LOG("UISubsystem: 초기 레이아웃 - 두 패널 모두 설정 (%.1fx%.1f)", DefaultPanelWidth, InAvailableHeight);
	}
	// Outliner만 있는 경우: 전체 오른쪽 영역 차지
	else if (InOutlinerWindow)
	{
		const float StartX = InScreenWidth - ActualPanelWidth;

		InOutlinerWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight));
		InOutlinerWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, InAvailableHeight));

		UE_LOG("UISubsystem: 초기 레이아웃 - Outliner만 설정 (%.1fx%.1f)", ActualPanelWidth, InAvailableHeight);
	}
	// Detail만 있는 경우: 전체 오른쪽 영역 차지
	else if (InDetailWindow)
	{
		const float StartX = InScreenWidth - ActualPanelWidth;

		InDetailWindow->SetLastWindowPosition(ImVec2(StartX, InMenuBarHeight));
		InDetailWindow->SetLastWindowSize(ImVec2(ActualPanelWidth, InAvailableHeight));

		UE_LOG("UISubsystem: 초기 레이아웃 - Detail만 설정 (%.1fx%.1f)", ActualPanelWidth, InAvailableHeight);
	}
}

/**
 * @brief 오른쪽 패널들의 동적 레이아웃을 업데이트하는 함수
 */
void UUISubsystem::ArrangeRightPanelsDynamic(TObjectPtr<UUIWindow> InOutlinerWindow, TObjectPtr<UUIWindow> InDetailWindow,
                                           float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
                                           float InAvailableHeight, float InTargetWidth, bool bOutlinerChanged,
                                           bool bDetailChanged) const
{
	// 너비는 항상 사용자가 조정한 InTargetWidth 사용 (동적 레이아웃에서는 사용자 조정 존중)
	float ActualWidth = InTargetWidth;
	float NewX = InScreenWidth - ActualWidth;
	if (InOutlinerWindow && InDetailWindow)
	{
		// 두 패널이 모두 있을 때: 사용자 조정 너비 사용
		ActualWidth = InTargetWidth;
	}
	else
	{
		// 단일 패널일 때: 저장된 너비 사용
		ActualWidth = (SavedPanelWidth > 0) ? SavedPanelWidth : InTargetWidth;
	}

	if (InOutlinerWindow && InDetailWindow)
	{
		// 두 패널 모두 있는 경우
		const float CurrentOutlinerHeight = InOutlinerWindow->GetLastWindowSize().y;
		const float CurrentDetailHeight = InDetailWindow->GetLastWindowSize().y;
		const ImVec2 CurrentDetailPos = InDetailWindow->GetLastWindowPosition();

		// 만약 한 패널이 전체 높이를 차지하고 있다면 저장된 레이아웃 복원
		if (CurrentOutlinerHeight >= InAvailableHeight * 0.9f || CurrentDetailHeight >= InAvailableHeight * 0.9f)
		{
			float OutlinerHeight, DetailHeight;

			// 저장된 레이아웃이 있으면 복원, 없으면 기본 레이아웃 사용
			if (bHasSavedDualLayout && SavedOutlinerHeightForDual > 0 && SavedDetailHeightForDual > 0)
			{
				OutlinerHeight = SavedOutlinerHeightForDual;
				DetailHeight = SavedDetailHeightForDual;
				UE_LOG("UISubsystem: 저장된 레이아웃 복원 (Outliner: %.1f, Detail: %.1f)", OutlinerHeight, DetailHeight);
			}
			else
			{
				// 기본 레이아웃
				OutlinerHeight = max(300.0f, InAvailableHeight * 0.4f);
				DetailHeight = InAvailableHeight - OutlinerHeight;
			}

			// Detail이 너무 작아지지 않도록 조정
			if (DetailHeight < 200.0f)
			{
				const float AdjustedOutlinerHeight = InAvailableHeight - 200.0f;
				const float AdjustedDetailHeight = 200.0f;

				InOutlinerWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
				InOutlinerWindow->SetLastWindowSize(ImVec2(ActualWidth, AdjustedOutlinerHeight));

				InDetailWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight + AdjustedOutlinerHeight));
				InDetailWindow->SetLastWindowSize(ImVec2(ActualWidth, AdjustedDetailHeight));
			}
			else
			{
				InOutlinerWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
				InOutlinerWindow->SetLastWindowSize(ImVec2(ActualWidth, OutlinerHeight));

				InDetailWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight + OutlinerHeight));
				InDetailWindow->SetLastWindowSize(ImVec2(ActualWidth, DetailHeight));
			}

			UE_LOG("UISubsystem: 동적 레이아웃: 두 패널 초기 분할 (%.1fx%.1f)", InTargetWidth, InAvailableHeight);
		}

		// Detail 위쪽 경계 드래그 감지
		else if (bDetailChanged && abs(CurrentDetailPos.y - (InMenuBarHeight + LastOutlinerSize.y)) > 1.0f)
		{
			// Detail의 위쪽을 드래그한 경우 -> Outliner 높이 조정
			const float NewOutlinerHeight = CurrentDetailPos.y - InMenuBarHeight;

			if (NewOutlinerHeight > 100.0f) // 최소 높이 체크
			{
				InOutlinerWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
				InOutlinerWindow->SetLastWindowSize(ImVec2(ActualWidth, NewOutlinerHeight));

				// Detail도 나머지 공간에 맞게 재조정
				const float NewDetailHeight = InAvailableHeight - NewOutlinerHeight;
				InDetailWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight + NewOutlinerHeight));
				InDetailWindow->SetLastWindowSize(ImVec2(ActualWidth, NewDetailHeight));
				UE_LOG_DEBUG("UISubsystem: Detail 위쪽 드래그 -> Outliner 조정 (%.1f)", NewOutlinerHeight);
			}
		}
		else
		{
			// 기존 크기 유지하면서 너비만 조정
			const float DetailHeight = InAvailableHeight - CurrentOutlinerHeight;

			// Outliner: 상단 고정
			InOutlinerWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
			InOutlinerWindow->SetLastWindowSize(ImVec2(ActualWidth, CurrentOutlinerHeight));

			// Detail: Outliner 바로 아래, 하단까지
			InDetailWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight + CurrentOutlinerHeight));
			InDetailWindow->SetLastWindowSize(ImVec2(ActualWidth, DetailHeight));

			UE_LOG("UISubsystem: 동적 레이아웃: 두 패널 업데이트 (%.1fx%.1f)", InTargetWidth, InAvailableHeight);
		}
	}

	// Outliner만 있는 경우: 저장된 너비 사용, Y 위치와 높이만 조정
	else if (InOutlinerWindow)
	{
		InOutlinerWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
		InOutlinerWindow->SetLastWindowSize(ImVec2(ActualWidth, InAvailableHeight));

		UE_LOG("UISubsystem: 동적 레이아웃: Outliner만 업데이트 (너비: %.1fx%.1f)", ActualWidth, InAvailableHeight);
	}

	// Detail만 있는 경우: 저장된 너비 사용, Y 위치와 높이만 조정
	else if (InDetailWindow)
	{
		InDetailWindow->SetLastWindowPosition(ImVec2(NewX, InMenuBarHeight));
		InDetailWindow->SetLastWindowSize(ImVec2(ActualWidth, InAvailableHeight));

		UE_LOG("UISubsystem: 동적 레이아웃: Detail만 업데이트 (너비: %.1fx%.1f)", ActualWidth, InAvailableHeight);
	}
}

/**
 * @brief 오른쪽 패널 레이아웃을 강제로 적용하는 함수
 * 트리거를 거쳐 외부에서 호출하기 위한 함수
 */
void UUISubsystem::ForceArrangeRightPanels()
{
	// bFirstRun 플래그를 리셋하여 초기 레이아웃 강제 실행
	bFirstRun = true;
	UE_LOG("UISubsystem: 오른쪽 패널 레이아웃 강제 적용 요청");

	// 즉시 레이아웃 정리 실행
	ArrangeRightPanels();
}

/**
 * @brief Console 패널을 하단에 배치하는 함수
 */
void UUISubsystem::ArrangeConsolePanel()
{
	// Console 윈도우 찾기
	TObjectPtr<UUIWindow> ConsoleWindow = FindUIWindow(FName("Console"));
	if (!ConsoleWindow)
	{
		return;
	}

	const float ScreenWidth = ImGui::GetIO().DisplaySize.x;
	const float ScreenHeight = ImGui::GetIO().DisplaySize.y;
	const float MenuBarHeight = GetMainMenuBarHeight();
	const float ToolbarHeight = GetToolbarHeight();
	const float BottomBarHeight = GetBottomBarHeight();
	const float RightPanelWidth = GetRightPanelWidth();

	// 저장된 높이 사용 (최소/최대 제한 적용)
	const float MinConsoleHeight = 100.0f;
	const float MaxConsoleHeight = ScreenHeight - MenuBarHeight - ToolbarHeight - BottomBarHeight - 100.0f;
	const float ConsoleHeight = clamp(SavedConsoleHeight, MinConsoleHeight, MaxConsoleHeight);

	const float ConsoleWidth = ScreenWidth - RightPanelWidth;
	const float ConsolePosX = 0.0f;
	const float ConsolePosY = ScreenHeight - BottomBarHeight - ConsoleHeight;

	ConsoleWindow->SetLastWindowPosition(ImVec2(ConsolePosX, ConsolePosY));
	ConsoleWindow->SetLastWindowSize(ImVec2(ConsoleWidth, ConsoleHeight));
}

/**
 * @brief 메뉴바에서 패널 활성화 / 비활성화 시 호출하는 함수
 */
void UUISubsystem::OnPanelVisibilityChanged()
{
	UE_LOG_INFO("UISubsystem: 패널 가시성 변경 감지, 레이아웃 재정리 시작");
	ForceArrangeRightPanels();
}
