#pragma once
#include "Source/Global/CoreTypes.h"
#include "Source/Global/Enum.h"

class FViewport;

class FViewportClient
{
public:
	FViewportClient();
	~FViewportClient();

	// ---------- 구성/질의 ----------
	void SetViewType(EViewType InType) { ViewType = InType; }
	EViewType GetViewType() const { return ViewType; }

	void SetViewMode(EViewMode InMode) { ViewMode = InMode; }
	EViewMode GetViewMode() const { return ViewMode; }

	// Camera Transform
	void SetViewLocation(const FVector& NewLocation) { ViewLocation = NewLocation; }
	void SetViewRotation(const FVector& NewRotation) { ViewRotation = NewRotation; }
	const FVector& GetViewLocation() const { return ViewLocation; }
	const FVector& GetViewRotation() const { return ViewRotation; }

	// Camera Parameters
	void SetFovY(float NewFovY) { FovY = NewFovY; }
	void SetOrthoWidth(float NewWidth) { OrthoWidth = NewWidth; }
	void SetNearZ(float NewNear) { NearZ = NewNear; }
	void SetFarZ(float NewFar) { FarZ = NewFar; }
	float GetFovY() const { return FovY; }
	float GetOrthoWidth() const { return OrthoWidth; }
	float GetNearZ() const { return NearZ; }
	float GetFarZ() const { return FarZ; }

	// View/Proj Matrix
	FMatrix GetViewMatrix() const;
	FMatrix GetProjectionMatrix() const;
	FViewProjConstants GetViewProjConstants() const;

	bool IsOrtho() const { return ViewType != EViewType::Perspective; }

public:
	void Tick(float DeltaSeconds) const;
	void Draw(FViewport* InViewport);

	static void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/)
	{
	}

	void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
	{
		LastDrag = {X, Y};
	}

	// ---------- 리사이즈/포커스 ----------
	void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }

	static void OnGainFocus()
	{
	}

	static void OnLoseFocus()
	{
	}

	static void OnClose()
	{
	}


private:
	// 상태
	EViewType ViewType = EViewType::Perspective;
	EViewMode ViewMode = EViewMode::Lit;

	// Camera Transform
	FVector ViewLocation;
	FVector ViewRotation;

	// Camera Parameters
	float FovY = 90.0f;
	float OrthoWidth = 100.0f;
	float NearZ = 0.01f;  // Near clipping plane - 더 가까운 것도 보이도록
	float FarZ = 100000.0f;  // Far clipping plane - 더 먼 것도 보이도록

	// 뷰/입력 상태
	FPoint ViewSize{0, 0};
	FPoint LastDrag{0, 0};
};
