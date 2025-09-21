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

	FVector GetGizmoDragLocation(FRay& WorldRay, UCamera& Camera);
	FVector GetGizmoDragRotation(FRay& WorldRay, UCamera& Camera);
	FVector GetGizmoDragScale(FRay& WorldRay, UCamera& Camera);

	// 현재 마우스가 위치한 뷰포트(없으면 0)의 카메라를 반환
	UCamera* GetActivePickingCamera();

	UCamera Camera; // legacy editor camera (not used for picking)
	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;
	//UGrid Grid;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;
};
