#include "pch.h"
#include "PhYsics/Public/AABB.h"

void FAABB::Merge(const FAABB& Other)
{
	Min.X = std::min(Min.X, Other.Min.X);
	Min.Y = std::min(Min.Y, Other.Min.Y);
	Min.Z = std::min(Min.Z, Other.Min.Z);

	Max.X = std::max(Max.X, Other.Max.X);
	Max.Y = std::max(Max.Y, Other.Max.Y);
	Max.Z = std::max(Max.Z, Other.Max.Z);
}

FAABB FAABB::Merge(const FAABB& A, const FAABB& B)
{
	FAABB result = A;
	result.Merge(B);
	return result;
}

float FAABB::GetArea() const
{
	FVector d = Max - Min;
	return 2.0f * (d.X * d.Y + d.X * d.Z + d.Y * d.Z);
}

float FAABB::GetPerimeter() const
{
	FVector d = Max - Min;
	return 4.0f * (d.X + d.Y + d.Z);
}

bool FAABB::RaycastHit() const
{
	return false;
}
