#pragma once
#include "Widget.h"

class AActor;
class ULevel;
class UCamera;

/**
 * @brief 현재 Level의 모든 Actor들을 트리 형태로 표시하는 Widget
 * Actor를 클릭하면 Level에서 선택되도록 하는 기능 포함
 */
class USceneHierarchyWidget :
	public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special member function
	USceneHierarchyWidget();
	~USceneHierarchyWidget() override;

	// 뷰포트 애니메이션 제어
	void StopViewportAnimation(int32 ViewportIndex);
	bool IsViewportAnimating(int32 ViewportIndex) const;
	bool IsAnyViewportAnimating() const;
	void StopAllViewportAnimations();

private:
	// UI 상태
	bool bShowDetails = true;

	// 검색 기능
	char SearchBuffer[256] = "";
	FString SearchFilter;
	TArray<int32> FilteredIndices; // 필터링된 Actor 인덱스 캐시
	bool bNeedsFilterUpdate = true; // 필터 업데이트 필요 여부

	// 이름 변경 기능
	TObjectPtr<AActor> RenamingActor = nullptr;
	char RenameBuffer[256] = "";
	double LastClickTime = 0.0f;
	TObjectPtr<AActor> LastClickedActor = nullptr;
	static constexpr float RENAME_CLICK_DELAY = 0.3f; // 두 번째 클릭 간격

	// Viewport focus animation
	TMap<int32, bool> ViewportAnimatingStates;
	TMap<int32, float> ViewportAnimationTimes;
	TMap<int32, FVector> ViewportStartLocations;
	TMap<int32, FVector> ViewportTargetLocations;
	TMap<int32, TObjectPtr<AActor>> ViewportAnimationTargets;

	// Heuristic constant
	static constexpr float CAMERA_ANIMATION_DURATION = 0.6f;
	static constexpr float FOCUS_DISTANCE = 5.0f;

	// Viewport movement
	void RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex);
	void SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera = false);
	void FocusOnActor(int32 ViewportIndex, TObjectPtr<AActor> InActor);
	void UpdateViewportAnimation(int32 ViewportIndex);

	// 검색 기능
	void RenderSearchBar();
	void UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors);
	static bool IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm);

	// 이름 변경 기능
	void StartRenaming(TObjectPtr<AActor> InActor);
	void FinishRenaming(bool bInConfirm);
	bool IsRenaming() const { return RenamingActor != nullptr; }
};
