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

	// 카메라 애니메이션 제어
	void StopCameraAnimation(TObjectPtr<ACameraActor> InCamera);
	bool IsCameraAnimating(TObjectPtr<ACameraActor> InCamera) const;
	bool IsAnyCameraAnimating() const;
	void StopAllCameraAnimations();

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

	// Camera focus animation
	TMap<FName, bool> CameraAnimatingStates;
	TMap<FName, float> CameraAnimationTimes;
	TMap<FName, FVector> CameraStartLocations;
	TMap<FName, FVector> CameraTargetLocations;
	TMap<FName, FVector> CameraCurrentRotations;
	TMap<FName, TObjectPtr<AActor>> CameraAnimationTargets;

	// Heuristic constant
	static constexpr float CAMERA_ANIMATION_DURATION = 0.6f;
	static constexpr float FOCUS_DISTANCE = 5.0f;

	// Camera movement
	void RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex);
	void SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera = false);
	void FocusOnActor(TObjectPtr<ACameraActor> InCamera, TObjectPtr<AActor> InActor);
	void UpdateCameraAnimation(TObjectPtr<ACameraActor> InCamera);

	// 검색 기능
	void RenderSearchBar();
	void UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors);
	static bool IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm);

	// 이름 변경 기능
	void StartRenaming(TObjectPtr<AActor> InActor);
	void FinishRenaming(bool bInConfirm);
	bool IsRenaming() const { return RenamingActor != nullptr; }
};
