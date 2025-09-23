#pragma once
#include "Source/Global/CoreTypes.h"
#include "Source/Global/Enum.h"
#include "Editor/Public/Camera.h"

class FViewport;
class UCamera;

class FViewportClient
{
public:
	FViewportClient();
	~FViewportClient();

public:
	// ---------- 구성/질의 ----------
	void        SetViewType(EViewType InType) { ViewType = InType; }
	EViewType   GetViewType() const { return ViewType; }

	void        SetViewMode(EViewMode InMode) { ViewMode = InMode; }
	EViewMode   GetViewMode() const { return ViewMode; }


	// 퍼스펙티브 카메라
	void     SetPerspectiveCamera(UCamera* InPerspectiveCamera) { PerspectiveCamera = InPerspectiveCamera; }
	UCamera* GetPerspectiveCamera() const { return PerspectiveCamera; }

    // 오쏘 카메라 (뷰포트별)
    void     SetOrthoCamera(UCamera* InCamera) { OrthoGraphicCameras = InCamera; }
    UCamera* GetOrthoCamera() const { return OrthoGraphicCameras; }


	bool        IsOrtho() const { return ViewType != EViewType::Perspective; }


public:
	void Tick() const;
	void Draw(const FViewport* InViewport) const;


	static void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/) {}
	void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
	{
		LastDrag = { X, Y };
	}

	// ---------- 리사이즈/포커스 ----------
	void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
	static void OnGainFocus() {}
	static void OnLoseFocus() {}
	static void OnClose() {}


    FViewProjConstants GetPerspectiveViewProjConstData() const { return PerspectiveCamera->GetFViewProjConstants(); }
    FViewProjConstants GetOrthoGraphicViewProjConstData() const { return OrthoGraphicCameras->GetFViewProjConstants(); }
private:

private:
	// 상태
	EViewType   ViewType = EViewType::Perspective;
	EViewMode   ViewMode = EViewMode::Lit;

    TObjectPtr<UCamera> PerspectiveCamera = nullptr;
    TObjectPtr<UCamera> OrthoGraphicCameras = nullptr;


	// 뷰/입력 상태
	FPoint		ViewSize{ 0, 0 };
	FPoint		LastDrag{ 0, 0 };
};

