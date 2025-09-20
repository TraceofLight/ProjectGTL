#pragma once
#include "Source/Global/CoreTypes.h"
#include "Source/Global/Enum.h"


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
	UCamera* GetPerspectiveCamera() { return PerspectiveCamera; }

	// 오쏘 카메라
	void     SetSharedOrthoCamera(UCamera* InCamera) { OrthoGraphicCameraShared = InCamera; }


	bool        IsOrtho() const { return ViewType != EViewType::Perspective; }


public:
	void Tick(float InDeltaSeconds);
	void Draw(FViewport* InViewport);


	void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/) {}
	void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
	{
		LastDrag = { X, Y };
	}

	// ---------- 리사이즈/포커스 ----------
	void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
	void OnGainFocus() {}
	void OnLoseFocus() {}
	void OnClose() {}

	// ---------- 유틸 ----------
	// 현재 뷰 타입에 맞는 월드 기준 축을 카메라(Right/Up/Forward)로 세팅
	void ApplyOrthoBasisForViewType(UCamera& OutCamera);


private:

private:
	// 상태
	EViewType   ViewType = EViewType::Perspective;
	EViewMode   ViewMode = EViewMode::Lit;

	UCamera* PerspectiveCamera = nullptr;

	// 오쏘 그래픽 카메라는 여기서는 공유로 해두었는데, 언리얼에서는 설정으로 공유되지않게 할 수 도 있음 추후에 수정할라면 하세용
	UCamera* OrthoGraphicCameraShared = nullptr;


	// 뷰/입력 상태
	FPoint		ViewSize{ 0, 0 };
	FPoint		LastDrag{ 0, 0 };
};

