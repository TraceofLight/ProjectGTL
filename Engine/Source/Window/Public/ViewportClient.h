#pragma once


#include "Source/Global/CoreTypes.h"
#include "Source/Global/Enum.h"

class FViewport;

class FViewportClient
{
public:
	virtual ~FViewportClient() = default;

	/** per-frame update before drawing */
	virtual void Tick(float DeltaTime) {}

	/** main draw entry: do scene render, gizmos, overlays, hit-proxy pass */
	virtual void Draw(FViewport* Viewport) = 0;

	/** camera/projection for this viewport */
	virtual FMatrix GetViewMatrix() const = 0;
	virtual FMatrix GetProjMatrix() const = 0;
	virtual bool    IsOrtho() const { return false; }

	/** rectangle of this view in pixels (for multi-viewport/splitter) */
	virtual FRect GetViewRect() const = 0;

	/** input routing */
	virtual bool InputKey(FViewport* Viewport, int32 Key, bool bPressed) { return false; }
	virtual bool InputAxis(FViewport* Viewport, int32 Axis, float Amount, float DeltaTime) { return false; }
	virtual void MouseMove(FViewport* Viewport, int32 X, int32 Y) {}
	virtual void CapturedMouseMove(FViewport* Viewport, int32 X, int32 Y) {}

	/** cursor & capture policy */
	virtual int32 GetCursor(FViewport* Viewport, int32 X, int32 Y) const { return 0; }
	virtual bool  CapturesMouse() const { return true; }
	virtual bool  LockMouseToViewport() const { return false; }

	/** lifecycle */
	virtual void OnResize(const FPoint& NewSize) {}

	


};

