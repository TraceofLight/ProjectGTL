#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"

enum class EViewModeIndex : uint32
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,
};

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

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	FVector GetCameraLocation() { return Camera ? Camera->GetCameraComponent()->GetRelativeLocation() : FVector(); }
	FViewProjConstants GetViewProjConstData() const { return Camera ? Camera->GetCameraComponent()->GetFViewProjConstants() : FViewProjConstants(); }

	ACameraActor* GetCamera() const { return Camera.Get(); }
	UBatchLines* GetBatchLines() const { return Grid.Get(); }
	UGizmo* GetGizmo() const { return Gizmo.Get(); }

private:
	TObjectPtr<ACameraActor> Camera = nullptr;
	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	TUniquePtr<UGizmo> Gizmo;
	TUniquePtr<UAxis> Axis;
	TUniquePtr<UBatchLines> Grid;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;

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
