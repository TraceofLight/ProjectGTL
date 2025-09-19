#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
};


struct FVertex
{
	FVector Position;
	FVector4 Color;
	FVector Normal;
	FVector2 TextureCoord;

	FVertex() = default;

	FVertex(const FVector& InPosition, const FVector4& InColor, const FVector& InNormal, const FVector2& InTextureCoord)
		: Position(InPosition), Color(InColor), Normal(InNormal), TextureCoord(InTextureCoord)
	{
	}

	FVertex(const FVector& InPosition, const FVector4& InColor)
		: Position(InPosition), Color(InColor), Normal(FVector(0.0f, 0.0f, 1.0f)), TextureCoord(FVector2(0.0f, 0.0f))
	{
	}

	FVertex(const FVector& InPosition)
		: Position(InPosition), Color(FVector4(1.0f, 1.0f, 1.0f, 1.0f)), Normal(FVector(0.0f, 0.0f, 1.0f)), TextureCoord(FVector2(0.0f, 0.0f))
	{
	}
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	FVector Location = FVector(0.0f, 0.0f, 0.0f);
	FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FVector& InRotation = FVector::ZeroVector(),
			   const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}
};
