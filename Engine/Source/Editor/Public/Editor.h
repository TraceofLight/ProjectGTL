#pragma once
#include "Core/Public/Object.h"
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

class UEditor : public UObject
{
public:
	UEditor();
	~UEditor() = default;

	void Update();
	void RenderEditor();

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	FVector& GetCameraLocation() { return Camera.GetLocation(); }
	FViewProjConstants GetViewProjConstData() const { return Camera.GetFViewProjConstants(); }

	UCamera* GetCamera()
	{
		return &Camera;
	}

private:
	void ProcessMouseInput(ULevel* InLevel);
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	FVector GetGizmoDragLocation(FRay& WorldRay);
	FVector GetGizmoDragRotation(FRay& WorldRay);
	FVector GetGizmoDragScale(FRay& WorldRay);

	UCamera Camera;
	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;
	//UGrid Grid;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;
};
