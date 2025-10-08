#include "pch.h"


FMatrix FMatrix::UEToDx = FMatrix(
	{
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

FMatrix FMatrix::DxToUE = FMatrix(
	{
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
	float M00, float M01, float M02, float M03,
	float M10, float M11, float M12, float M13,
	float M20, float M21, float M22, float M23,
	float M30, float M31, float M32, float M33)
	: Data{ {M00,M01,M02,M03},
			{M10,M11,M12,M13},
			{M20,M21,M22,M23},
			{M30,M31,M32,M33} }
{
}

FMatrix::FMatrix(const FVector& x, const FVector& y, const FVector& z)
	:Data{	{x.X, x.Y, x.Z, 0.0f},
			{y.X, y.Y, y.Z, 0.0f},
			{z.X, z.Y, z.Z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f}}
{
}

FMatrix::FMatrix(const FVector4& x, const FVector4& y, const FVector4& z)
	:Data{	{x.X,x.Y,x.Z, x.W},
			{y.X,y.Y,y.Z,y.W},
			{z.X,z.Y,z.Z,z.W},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

/**
* @brief 항등행렬
*/
FMatrix FMatrix::Identity()
{
	return FMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix)
{
	FMatrix Result;

	for (int32 i = 0; i < 4; ++i)
	{
		for (int32 j = 0; j < 4; ++j)
		{
			for (int32 k = 0; k < 4; ++k)
			{
				Result.Data[i][j] += Data[i][k] * InOtherMatrix.Data[k][j];
			}
		}
	}

	return Result;
}

void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this =  (*this)*InOtherMatrix;
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = InOtherVector.X;
	Result.Data[3][1] = InOtherVector.Y;
	Result.Data[3][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = -InOtherVector.X;
	Result.Data[3][1] = -InOtherVector.Y;
	Result.Data[3][2] = -InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = InOtherVector.X;
	Result.Data[1][1] = InOtherVector.Y;
	Result.Data[2][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}
FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = 1/InOtherVector.X;
	Result.Data[1][1] = 1/InOtherVector.Y;
	Result.Data[2][2] = 1/InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// Dx11 yaw(y), pitch(x), roll(z)
	// UE yaw(z), pitch(y), roll(x)
	// 회전 축이 바뀌어서 각 회전행렬 함수에 바뀐 값을 적용

	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;
	//return RotationZ(yaw) * RotationY(pitch) * RotationX(roll);
	//return RotationX(yaw) * RotationY(roll) * RotationZ(pitch);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	//return RotationZ(yaw) * RotationY(pitch)* RotationX(roll);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;
	//return RotationX(-yaw) * RotationY(-pitch) * RotationZ(-roll);
	return RotationX(-pitch) * RotationY(-yaw) * RotationZ(-roll);

}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[1][1] = C;
	Result.Data[1][2] = S;
	Result.Data[2][1] = -S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationY(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][2] = -S;
	Result.Data[2][0] = S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][1] = S;
	Result.Data[1][0] = -S;
	Result.Data[1][1] = C;

	return Result;
}

//
FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrix(Location);
	FMatrix R = RotationMatrix(Rotation);
	FMatrix S = ScaleMatrix(Scale);
	FMatrix modelMatrix = S * R * T;

	// Dx11 y-up 왼손좌표계에서 정의된 물체의 정점을 UE z-up 왼손좌표계로 변환
	return  FMatrix::UEToDx * modelMatrix;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	// UE 좌표계로 변환된 물체의 정점을 원래의 Dx 11 왼손좌표계 정점으로 변환
	return modelMatrixInverse * FMatrix::DxToUE;
}

FVector4 FMatrix::VectorMultiply(const FVector4& v, const FMatrix& m)
{
	FVector4 result = {};
	result.X = (v.X * m.Data[0][0]) + (v.Y * m.Data[1][0]) + (v.Z * m.Data[2][0]) + (v.W * m.Data[3][0]);
	result.Y = (v.X * m.Data[0][1]) + (v.Y * m.Data[1][1]) + (v.Z * m.Data[2][1]) + (v.W * m.Data[3][1]);
	result.Z = (v.X * m.Data[0][2]) + (v.Y * m.Data[1][2]) + (v.Z * m.Data[2][2]) + (v.W * m.Data[3][2]);
	result.W = (v.X * m.Data[0][3]) + (v.Y * m.Data[1][3]) + (v.Z * m.Data[2][3]) + (v.W * m.Data[3][3]);


	return result;
}

FVector FMatrix::VectorMultiply(const FVector& v, const FMatrix& m)
{
	FVector result = {};
	result.X = (v.X * m.Data[0][0]) + (v.Y * m.Data[1][0]) + (v.Z * m.Data[2][0]) + m.Data[3][0];
	result.Y = (v.X * m.Data[0][1]) + (v.Y * m.Data[1][1]) + (v.Z * m.Data[2][1]) + m.Data[3][1];
	result.Z = (v.X * m.Data[0][2]) + (v.Y * m.Data[1][2]) + (v.Z * m.Data[2][2]) + m.Data[3][2];

	return result;
}

FMatrix FMatrix::Transpose() const
{
	FMatrix result = {};
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0;j < 4;j++)
		{
			result.Data[i][j] = Data[j][i];
		}
	}

	return result;
}

FMatrix FMatrix::MatrixLookAtLH(const FVector& EyePosition, const FVector& FocusPosition, const FVector& UpDirection)
{
    FVector ZAxis = (FocusPosition - EyePosition).Normalized();
    FVector XAxis = UpDirection.Cross(ZAxis).Normalized();
    FVector YAxis = ZAxis.Cross(XAxis);

    FMatrix Result;
    Result.Data[0][0] = XAxis.X;
    Result.Data[0][1] = YAxis.X;
    Result.Data[0][2] = ZAxis.X;
    Result.Data[0][3] = 0.0f;

    Result.Data[1][0] = XAxis.Y;
    Result.Data[1][1] = YAxis.Y;
    Result.Data[1][2] = ZAxis.Y;
    Result.Data[1][3] = 0.0f;

    Result.Data[2][0] = XAxis.Z;
    Result.Data[2][1] = YAxis.Z;
    Result.Data[2][2] = ZAxis.Z;
    Result.Data[2][3] = 0.0f;

    Result.Data[3][0] = -XAxis.Dot(EyePosition);
    Result.Data[3][1] = -YAxis.Dot(EyePosition);
    Result.Data[3][2] = -ZAxis.Dot(EyePosition);
    Result.Data[3][3] = 1.0f;

    return Result;
}

FMatrix FMatrix::MatrixOrthoLH(float ViewWidth, float ViewHeight, float NearZ, float FarZ)
{
    FMatrix Result = FMatrix::Identity();
    Result.Data[0][0] = 2.0f / ViewWidth;
    Result.Data[1][1] = 2.0f / ViewHeight;
    Result.Data[2][2] = 1.0f / (FarZ - NearZ);
    Result.Data[3][2] = -NearZ / (FarZ - NearZ);
    return Result;
}

FMatrix FMatrix::MatrixPerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
{
    FMatrix Result = FMatrix();
    float SinFov = sinf(0.5f * FovAngleY);
    float CosFov = cosf(0.5f * FovAngleY);
    float Height = CosFov / SinFov;
    Result.Data[0][0] = Height / AspectRatio;
    Result.Data[1][1] = Height;
    Result.Data[2][2] = FarZ / (FarZ - NearZ);
    Result.Data[2][3] = 1.0f;
    Result.Data[3][2] = -NearZ * FarZ / (FarZ - NearZ);
    return Result;
}


