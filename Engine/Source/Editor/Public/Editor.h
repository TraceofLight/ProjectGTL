#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Gizmo.h"
#include "Runtime/Level/Public/Level.h"

class FEditorRenderResources;

UCLASS()
class UEditor :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UEditor, UObject)

public:
	UEditor();
	~UEditor() override;

	void Initialize();
	void Update();
	void RenderEditor();

	void SetViewMode(EViewMode InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewMode GetViewMode() const { return CurrentViewMode; }

	// ShowFlag 관리 함수들
	bool IsShowFlagEnabled(EEngineShowFlags ShowFlag) const;
	void SetShowFlag(EEngineShowFlags ShowFlag, bool bEnabled);
	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	FVector GetCameraLocation() { return Camera ? Camera->GetCameraComponent()->GetRelativeLocation() : FVector(); }
	FViewProjConstants GetViewProjConstData() const { return Camera ? Camera->GetCameraComponent()->GetFViewProjConstants() : FViewProjConstants(); }

	ACameraActor* GetCamera() const { return Camera.Get(); }

	// Selection Management
	TObjectPtr<AActor> GetSelectedActor() const { return SelectedActor; }
	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; }
	bool HasSelectedActor() const { return SelectedActor != nullptr; }

	// Gizmo Management
	UGizmo* GetGizmo() const { return Gizmo.Get(); }

	// Editor rendering now handled through FRendererModule::GetEditorResources()

private:
	TObjectPtr<ACameraActor> Camera = nullptr;
	UObjectPicker ObjectPicker;
	TObjectPtr<UGizmo> Gizmo = nullptr;

	// Selection Management
	TObjectPtr<AActor> SelectedActor = nullptr;

	const float MinScale = 0.01f;
	// Editor rendering resources moved to FRendererModule::GetEditorResources()

	EViewMode CurrentViewMode = EViewMode::Lit;

	// Editor ShowFlags - 기본적으로 모든 표시 옵션 활성화
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);

	// Viewport Interaction
	void HandleViewportInteraction();
	void UpdateSelectionBounds();
	void HandleGlobalShortcuts();
	static FRay CreateWorldRayFromMouse(const ACameraActor* InPickingCamera);
	void UpdateGizmoDrag(const FRay& InWorldRay, ACameraActor& InPickingCamera);
	void HandleNewInteraction(const FRay& InWorldRay);
	static TArray<UPrimitiveComponent*> FindCandidateActors(const ULevel* InLevel);

	FVector GetGizmoDragLocation(const FRay& InWorldRay, ACameraActor& InCamera);
	FVector GetGizmoDragLocationForPerspective(const FRay& InWorldRay, ACameraActor& InCamera);
	FVector GetGizmoDragLocationForOrthographic(const ACameraActor& InCamera);
	FVector GetGizmoDragRotation(const FRay& InWorldRay);
	FVector GetGizmoDragScale(const FRay& InWorldRay, ACameraActor& InCamera);

	// View Type에 따른 기저 변환
	static void CalculateBasisVectorsForViewType(EViewType InViewType, FVector& OutForward, FVector& OutRight, FVector& OutUp);

	static ACameraActor* GetActivePickingCamera();
};
