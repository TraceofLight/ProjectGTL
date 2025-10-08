#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"

class UUIWindow;
class UImGuiHelper;
class UMainMenuWindow;

/**
 * @brief UI 관리 서브시스템
 * 기존 UUIManager의 모든 기능을 서브시스템 패턴으로 이관
 * 모든 UI 윈도우를 관리하고 ImGui 통합을 담당
 * @param UIWindows 등록된 모든 UI 윈도우들
 * @param FocusedWindow 현재 포커스된 윈도우
 * @param TotalTime 전체 경과 시간
 */
UCLASS()
class UUISubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UUISubsystem, UEngineSubsystem)

public:
	UUISubsystem();
	~UUISubsystem() override;

	// ISubsystem 인터페이스 구현
	void Initialize() override;
	void Deinitialize() override;
	bool IsTickable() const override { return true; }
	void Tick(float DeltaSeconds) override;

	void Render();
	void RenderWithCommandList(class FRHICommandList* CommandList);
	bool RegisterUIWindow(TObjectPtr<UUIWindow> InWindow);
	bool UnregisterUIWindow(TObjectPtr<UUIWindow> InWindow);
	void PrintDebugInfo() const;

	TObjectPtr<UUIWindow> FindUIWindow(const FName& InWindowName) const;
	TObjectPtr<UWidget> FindWidget(const FName& InWidgetName) const;
	void HideAllWindows() const;
	void ShowAllWindows() const;

	// 윈도우 최소화 / 복원 처리
	void OnWindowMinimized();
	void OnWindowRestored();

	// Getter & Setter
	size_t GetUIWindowCount() const { return UIWindows.Num(); }
	const TArray<TObjectPtr<UUIWindow>>& GetAllUIWindows() const { return UIWindows; }
	TObjectPtr<UUIWindow> GetFocusedWindow() const { return FocusedWindow; }

	void SetFocusedWindow(TObjectPtr<UUIWindow> InWindow);

	// ImGui 관련 메서드
	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

	void RepositionImGuiWindows() const;

	// 메인 메뉴바 관련 메서드
	void RegisterMainMenuWindow(TObjectPtr<UMainMenuWindow> InMainMenuWindow);
	float GetMainMenuBarHeight() const;
	float GetToolbarHeight() const;
	float GetBottomBarHeight() const;
	float GetRightPanelWidth() const;
	void ArrangeRightPanels();
	void ForceArrangeRightPanels();
	void OnPanelVisibilityChanged();

	// Console 패널 레이아웃 함수
	void ArrangeConsolePanel();
	void SaveConsoleHeight(float InHeight) { SavedConsoleHeight = InHeight; }

private:
	TArray<TObjectPtr<UUIWindow>> UIWindows;
	TObjectPtr<UUIWindow> FocusedWindow = nullptr;
	float TotalTime = 0.0f;

	// 윈도우 상태 저장
	struct FUIWindowSavedState
	{
		uint32 WindowID;
		ImVec2 SavedPosition;
		ImVec2 SavedSize;
		bool bWasVisible;
	};

	TArray<FUIWindowSavedState> SavedWindowStates;
	bool bIsMinimized = false;

	// ImGui Helper
	UImGuiHelper* ImGuiHelper = nullptr;

	// Main Menu Window
	UMainMenuWindow* MainMenuWindow = nullptr;

	// 오른쪽 패널 상태 추적 변수들
	float SavedOutlinerHeightForDual = 0.0f; // 두 패널이 있을 때 Outliner 높이
	float SavedDetailHeightForDual = 0.0f; // 두 패널이 있을 때 Detail 높이
	float SavedPanelWidth = 0.0f; // 기억된 패널 너비
	bool bHasSavedDualLayout = false; // 두 패널 레이아웃이 저장되어 있는지

	// Console 패널 상태 추적 변수들
	float SavedConsoleHeight = 200.0f; // 저장된 Console 높이

	void SortUIWindowsByPriority();
	void UpdateFocusState();

	// 오른쪽 패널 레이아웃 헬퍼 함수
	void ArrangeRightPanelsInitial(TObjectPtr<UUIWindow> InOutlinerWindow, TObjectPtr<UUIWindow> InDetailWindow,
	                               float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
	                               float InAvailableHeight);
	void ArrangeRightPanelsDynamic(TObjectPtr<UUIWindow> InOutlinerWindow, TObjectPtr<UUIWindow> InDetailWindow,
	                               float InScreenWidth, float InScreenHeight, float InMenuBarHeight,
	                               float InAvailableHeight, float InTargetWidth, bool bOutlinerChanged,
	                               bool bDetailChanged) const;
};
