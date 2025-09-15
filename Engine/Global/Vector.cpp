#include "pch.h"
#include "Vector.h"

/**
 * @brief FVector 기본 생성자
 */
FVector::FVector()
	: X(0), Y(0), Z(0)
{
}


/**
 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
 */
FVector::FVector(float InX, float InY, float InZ)
	: X(InX), Y(InY), Z(InZ)
{
}


/**
 * @brief FVector를 Param으로 넘기는 생성자
 */
FVector::FVector(const FVector& InOther)
	: X(InOther.X), Y(InOther.Y), Z(InOther.Z)
{
}

void FVector::operator=(const FVector4& InOther)
{
	*this = FVector(InOther.X, InOther.Y, InOther.Z);
}


/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector FVector::operator+(const FVector& InOther) const
{
	return { X + InOther.X, Y + InOther.Y, Z + InOther.Z };
}

/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector FVector::operator-(const FVector& InOther) const
{
	return { X - InOther.X, Y - InOther.Y, Z - InOther.Z };
}

/**
 * @brief 자신의 벡터에서 배율을 곱한 백테를 반환하는 함수
 */
FVector FVector::operator*(const float Ratio) const
{
	return { X * Ratio, Y * Ratio, Z * Ratio };
}

/**
 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
 */
FVector& FVector::operator+=(const FVector& InOther)
{
	X += InOther.X;
	Y += InOther.Y;
	Z += InOther.Z;
	return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
}

/**
 * @brief 자신의 벡터에서 다른 벡터를 감산하는 함수
 */
FVector& FVector::operator-=(const FVector& InOther)
{
	X -= InOther.X;
	Y -= InOther.Y;
	Z -= InOther.Z;
	return *this; // 연쇄적인 연산을 위해 자기 자신을 반환
}

/**
 * @brief 자신의 벡터에서 배율을 곱한 뒤 자신을 반환
 */

FVector& FVector::operator*=(const float Ratio)
{
	X *= Ratio;
	Y *= Ratio;
	Z *= Ratio;

	return *this;
}

bool FVector::operator==(const FVector& inOther) const
{
	if (X == inOther.X && Y == inOther.Y && Z == inOther.Z)
	{
		return true;
	}
	return false;
}


	/**
	 * @brief FVector 기본 생성자
	 */
FVector4::FVector4()
		: X(0), Y(0), Z(0), W(0)
{
}

	/**
	 * @brief FVector의 멤버값을 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const float InX, const float InY, const float InZ, const float InW)
		: X(InX), Y(InY), Z(InZ), W(InW)
{
}


	/**
	 * @brief FVector를 Param으로 넘기는 생성자
	 */
FVector4::FVector4(const FVector4& InOther)
		: X(InOther.X), Y(InOther.Y), Z(InOther.Z), W(InOther.W)
{
}


/**
 * @brief 두 벡터를 더한 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator+(const FVector4& OtherVector) const
{
	return FVector4(
		X + OtherVector.X,
		Y + OtherVector.Y,
		Z + OtherVector.Z,
		W + OtherVector.W
	);
}

FVector4 FVector4::operator*(const FMatrix& Matrix) const
{
	FVector4 Result;
	Result.X = X * Matrix.Data[0][0] + Y * Matrix.Data[1][0] + Z * Matrix.Data[2][0] + W * Matrix.Data[3][0];
	Result.Y = X * Matrix.Data[0][1] + Y * Matrix.Data[1][1] + Z * Matrix.Data[2][1] + W * Matrix.Data[3][1];
	Result.Z = X * Matrix.Data[0][2] + Y * Matrix.Data[1][2] + Z * Matrix.Data[2][2] + W * Matrix.Data[3][2];
	Result.W = X * Matrix.Data[0][3] + Y * Matrix.Data[1][3] + Z * Matrix.Data[2][3] + W * Matrix.Data[3][3];

	return Result;
}
/**
 * @brief 두 벡터를 뺀 새로운 벡터를 반환하는 함수
 */
FVector4 FVector4::operator-(const FVector4& OtherVector) const
{
	return FVector4(
		X - OtherVector.X,
		Y - OtherVector.Y,
		Z - OtherVector.Z,
		W - OtherVector.W
	);
}

/**
 * @brief 자신의 벡터에 배율을 곱한 값을 반환하는 함수
 */
FVector4 FVector4::operator*(const float Ratio) const
{
	return FVector4(
		X * Ratio,
		Y * Ratio,
		Z * Ratio,
		W * Ratio
	);
}


/**
 * @brief 자신의 벡터에 다른 벡터를 가산하는 함수
 */
void FVector4::operator+=(const FVector4& OtherVector)
{
	X += OtherVector.X;
	Y += OtherVector.Y;
	Z += OtherVector.Z;
	W += OtherVector.W;
}

/**
 * @brief 자신의 벡터에 다른 벡터를 감산하는 함수
 */
void FVector4::operator-=(const FVector4& OtherVector)
{
	X -= OtherVector.X;
	Y -= OtherVector.Y;
	Z -= OtherVector.Z;
	W -= OtherVector.W;
}

/**
 * @brief 자신의 벡터에 배율을 곱하는 함수
 */
void FVector4::operator*=(const float Ratio)
{
	X *= Ratio;
	Y *= Ratio;
	Z *= Ratio;
	W *= Ratio;
}


