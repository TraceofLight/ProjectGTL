#pragma once
#include "Runtime/Core/Public/Object.h"

class UUIWindow;
class UImGuiHelper;
class UMainMenuWindow;

/**
 * @brief UI 매니저 클래스
 * 모든 UI 윈도우를 관리하는 싱글톤 클래스
 * @param UIWindows 등록된 모든 UI 윈도우들
 * @param FocusedWindow 현재 포커스된 윈도우
 * @param bIsInitialized 초기화 상태
 * @param TotalTime 전체 경과 시간
 */
UCLASS()
class UUIManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UUIManager, UObject)

public:
	void Initialize();
	void Initialize(HWND InWindowHandle);
	void Shutdown();
	void Update();
	void Render();
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
	size_t GetUIWindowCount() const { return UIWindows.size(); }
	const TArray<TObjectPtr<UUIWindow>>& GetAllUIWindows() const { return UIWindows; }
	TObjectPtr<UUIWindow> GetFocusedWindow() const { return FocusedWindow; }

	void SetFocusedWindow(TObjectPtr<UUIWindow> InWindow);

	// ImGui 관련 메서드
	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

	void RepositionImGuiWindows() const;

	// 메인 메뉴바 관련 메서드
	void RegisterMainMenuWindow(TObjectPtr<UMainMenuWindow> InMainMenuWindow);
	float GetMainMenuBarHeight() const;

private:
	TArray<TObjectPtr<UUIWindow>> UIWindows;
	TObjectPtr<UUIWindow> FocusedWindow = nullptr;
	bool bIsInitialized = false;
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

	void SortUIWindowsByPriority();
	void UpdateFocusState();
};
