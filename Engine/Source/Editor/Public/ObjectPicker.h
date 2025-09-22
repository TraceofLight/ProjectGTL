#pragma once
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

UCLASS()
class UObjectPicker :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObjectPicker, UObject)

public:
	UObjectPicker() = default;
	// Assign camera to use for ray tests (per-frame/per-viewport)
	void SetCamera(UCamera* InCamera) { Camera = InCamera; }
	UPrimitiveComponent* PickPrimitive(const FRay& InWorldRay, const TArray<UPrimitiveComponent*>& InCandidate,
	                                   float* InDistance);
	void PickGizmo(const FRay& InWorldRay, UGizmo& InGizmo, FVector& InCollisionPoint,
	               float InViewportWidth = 0.0f, float InViewportHeight = 0.0f, int32 InViewportIndex = 0);
	bool IsRayCollideWithPlane(const FRay& InWorldRay, FVector InPlanePoint, FVector InNormal, FVector& InPointOnPlane);

private:
	UCamera* Camera = nullptr;

	bool IsRayPrimitiveCollided(const FRay& InModelRay, UPrimitiveComponent* InPrimitive, const FMatrix& InModelMatrix,
	                            float* InShortestDistance);
	static FRay GetModelRay(const FRay& InRay, const UPrimitiveComponent* InPrimitive);
	bool IsRayTriangleCollided(const FRay& InRay,
	                           const FVector& InVertex1, const FVector& InVertex2, const FVector& InVertex3,
	                           const FMatrix& InModelMatrix, float* InDistance);
};
