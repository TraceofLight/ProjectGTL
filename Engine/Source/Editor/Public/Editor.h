#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Editor/Public/Camera.h"
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

	void Update();
	void RenderEditor();

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	FVector& GetCameraLocation() { return Camera.GetLocation(); }
	FViewProjConstants GetViewProjConstData() const { return Camera.GetFViewProjConstants(); }

	UCamera* GetCamera() { return &Camera; }
	UBatchLines* GetBatchLines() { return &BatchLines; }
	UGizmo* GetGizmo() { return &Gizmo; }

private:
	UCamera Camera; // legacy editor camera (not used for picking)
	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;

	// Viewport Interaction
	void HandleViewportInteraction();
	void UpdateSelectionBounds();
	void HandleGlobalShortcuts();
	static FRay CreateWorldRayFromMouse(const UCamera* InPickingCamera);
	void UpdateGizmoDrag(const FRay& InWorldRay, UCamera& InPickingCamera);
	void HandleNewInteraction(const FRay& InWorldRay);
	static TArray<UPrimitiveComponent*> FindCandidateActors(const ULevel* InLevel);

	FVector GetGizmoDragLocation(const FRay& InWorldRay, UCamera& InCamera);
	FVector GetGizmoDragLocationForPerspective(const FRay& InWorldRay, UCamera& InCamera);
	FVector GetGizmoDragLocationForOrthographic(const UCamera& InCamera);
	FVector GetGizmoDragRotation(const FRay& InWorldRay);
	FVector GetGizmoDragScale(const FRay& InWorldRay, UCamera& InCamera);

	// View Type에 따른 기저 변환
	static void CalculateBasisVectorsForViewType(EViewType InViewType, FVector& OutForward, FVector& OutRight, FVector& OutUp);

	static UCamera* GetActivePickingCamera();
};
