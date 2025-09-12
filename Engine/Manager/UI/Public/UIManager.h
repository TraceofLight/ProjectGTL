#pragma once
#include "Core/Public/Object.h"

class UUIWindow;
class UImGuiHelper;

/**
 * @brief UI 매니저 클래스
 * 모든 UI 윈도우를 관리하는 싱글톤 클래스
 * @param UIWindows 등록된 모든 UI 윈도우들
 * @param FocusedWindow 현재 포커스된 윈도우
 * @param bIsInitialized 초기화 상태
 * @param TotalTime 전체 경과 시간
 */
class UUIManager : public UObject
{
	DECLARE_SINGLETON(UUIManager)

public:
	void Initialize();
	void Initialize(HWND InWindowHandle);
	void Shutdown();
	void Update();
	void Render();
	bool RegisterUIWindow(UUIWindow* InWindow);
	bool UnregisterUIWindow(UUIWindow* InWindow);
	void PrintDebugInfo() const;

	UUIWindow* FindUIWindow(const FString& InWindowName) const;
	UWidget* FindWidget(const FString& InWidgetName) const;
	void HideAllWindows() const;
	void ShowAllWindows() const;

	// 윈도우 최소화 / 복원 처리
	void OnWindowMinimized();
	void OnWindowRestored();

	// Getter & Setter
	size_t GetUIWindowCount() const { return UIWindows.size(); }
	const TArray<UUIWindow*>& GetAllUIWindows() const { return UIWindows; }
	UUIWindow* GetFocusedWindow() const { return FocusedWindow; }

	void SetFocusedWindow(UUIWindow* InWindow);

	// ImGui 관련 메서드
	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

	void RepositionImGuiWindows();
private:
	TArray<UUIWindow*> UIWindows;
	UUIWindow* FocusedWindow = nullptr;
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

	void SortUIWindowsByPriority();
	void UpdateFocusState();
};
