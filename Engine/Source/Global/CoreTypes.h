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
	FVector2 TextureCoord;
	FVector Normal;

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
		: Position(InPosition), Color(FVector4(1.0f, 1.0f, 1.0f, 1.0f)), Normal(FVector(0.0f, 0.0f, 1.0f)),
		  TextureCoord(FVector2(0.0f, 0.0f))
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

	bool operator==(const FTransform& InOther) const
	{
		return this->Location == InOther.Location && this->Rotation == InOther.Rotation && this->Scale == InOther.Scale;
	}
};

/**
 * @brief 2D 정수 좌표
 */
struct FPoint
{
	INT X, Y;

	FPoint() = default;

	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 2D 정수 사각형
 */
struct FRect
{
	LONG X, Y, W, H;

	FRect() = default;

	constexpr FRect(LONG InX, LONG InY, LONG InW, LONG InH) : X(InX), Y(InY), W(InW), H(InH)
	{
	}
};
