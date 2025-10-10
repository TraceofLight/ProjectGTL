#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB() : Min(0.f, 0.f, 0.f), Max(0.f, 0.f, 0.f) {}
	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

	// 이 AABB와 다른 AABB를 병합합니다.
	void Merge(const FAABB& Other);

	// 두 개의 AABB를 병합하여 새로운 AABB를 반환합니다.
	static FAABB Merge(const FAABB& A, const FAABB& B);

	// AABB의 표면적을 계산합니다.
	float GetArea() const;

	// AABB의 둘레를 계산합니다.
	float GetPerimeter() const;

	bool RaycastHit() const override;
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }
};
