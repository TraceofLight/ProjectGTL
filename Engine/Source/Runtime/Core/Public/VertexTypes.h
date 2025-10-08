#pragma once
#include "Global/Vector.h"

/**
 * @brief 간단한 버텍스 구조체 (Position + Color)
 * Line 렌더링이나 기본적인 도형 렌더링에 사용
 */
struct FVertexSimple
{
	FVector Position;
	FVector4 Color;

	FVertexSimple() = default;
	
	FVertexSimple(const FVector& InPosition, const FVector4& InColor)
		: Position(InPosition), Color(InColor)
	{
	}

	FVertexSimple(float X, float Y, float Z, float R, float G, float B, float A = 1.0f)
		: Position(X, Y, Z), Color(R, G, B, A)
	{
	}
};

/**
 * @brief 동적 버텍스 구조체 (Position + Color + Texture + Normal)
 * StaticMesh 렌더링에 주로 사용
 */
struct FVertexDynamic
{
	FVector Position;
	FVector4 Color;
	FVector2 TexCoord;
	FVector Normal;

	FVertexDynamic() = default;

	FVertexDynamic(const FVector& InPosition, const FVector4& InColor, 
	               const FVector2& InTexCoord, const FVector& InNormal)
		: Position(InPosition), Color(InColor), TexCoord(InTexCoord), Normal(InNormal)
	{
	}
};

/**
 * @brief 텍스처 전용 버텍스 구조체 (Position + Texture)
 */
struct FVertexTexture
{
	FVector Position;
	FVector2 TexCoord;

	FVertexTexture() = default;

	FVertexTexture(const FVector& InPosition, const FVector2& InTexCoord)
		: Position(InPosition), TexCoord(InTexCoord)
	{
	}
};

/**
 * @brief 노멀 전용 버텍스 구조체 (Position + Normal)
 */
struct FVertexNormal
{
	FVector Position;
	FVector Normal;

	FVertexNormal() = default;

	FVertexNormal(const FVector& InPosition, const FVector& InNormal)
		: Position(InPosition), Normal(InNormal)
	{
	}
};