#pragma once
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

class UObjectPicker : public UObject
{
public:
	UObjectPicker() = default;
	// Assign camera to use for ray tests (per-frame/per-viewport)
	void SetCamera(UCamera* InCamera) { Camera = InCamera; }
	UPrimitiveComponent* PickPrimitive(const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* Distance);
	void PickGizmo(const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);

private:
	bool IsRayPrimitiveCollided(const FRay& ModelRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);
private:
	UCamera* Camera = nullptr;
};
