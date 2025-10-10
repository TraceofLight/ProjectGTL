#include "pch.h"

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
	Result.Data[0][3] = InOtherVector.X;
	Result.Data[1][3] = InOtherVector.Y;
	Result.Data[2][3] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][3] = -InOtherVector.X;
	Result.Data[1][3] = -InOtherVector.Y;
	Result.Data[2][3] = -InOtherVector.Z;
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
* 좌표계 변환 제거 후에도 기존 회전 방식 유지
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// 기존 회전 순서 유지 (작동하던 방식)
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	// 언리얼 엔진 표준 회전 순서: Yaw(Z축) -> Pitch(Y축) -> Roll(X축)
	// Yaw = Z축 회전(좌우), Pitch = Y축 회전(위아래), Roll = X축 회전(기울임)
	// 행렬 곱셈은 오른쪽에서 왼쪽으로 적용되므로 역순으로 곱함
	return RotationX(roll) * RotationY(pitch) * RotationZ(yaw);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	// 기존 역회전 순서 유지
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

	// 좌표계 변환 제거: 정점 데이터와 Transform 모두 UE 좌표계로 통일
	// 임포터에서 이미 UE 좌표계로 변환 완료됨
	return modelMatrix;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	// 좌표계 변환 제거: 일관된 UE 좌표계 사용
	return modelMatrixInverse;
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
    // 언리얼 엔진 표준 LookAt 행렬 (왼손 좌표계)
    // UE는 행 우선(Row-major) 행렬 사용
    FVector ZAxis = (FocusPosition - EyePosition).Normalized();  // Forward
    FVector XAxis = ZAxis.Cross(UpDirection).Normalized();       // Right = Forward × Up (왼손 좌표계 수정!)
    FVector YAxis = XAxis.Cross(ZAxis);                          // Up = Right × Forward

    // 행 우선 방식으로 View Matrix 구성
    FMatrix Result;
    // Row 0: Right (XAxis)
    Result.Data[0][0] = XAxis.X;
    Result.Data[0][1] = XAxis.Y;
    Result.Data[0][2] = XAxis.Z;
    Result.Data[0][3] = -XAxis.Dot(EyePosition);

    // Row 1: Up (YAxis)
    Result.Data[1][0] = YAxis.X;
    Result.Data[1][1] = YAxis.Y;
    Result.Data[1][2] = YAxis.Z;
    Result.Data[1][3] = -YAxis.Dot(EyePosition);

    // Row 2: Forward (ZAxis)
    Result.Data[2][0] = ZAxis.X;
    Result.Data[2][1] = ZAxis.Y;
    Result.Data[2][2] = ZAxis.Z;
    Result.Data[2][3] = -ZAxis.Dot(EyePosition);

    // Row 3: Homogeneous
    Result.Data[3][0] = 0.0f;
    Result.Data[3][1] = 0.0f;
    Result.Data[3][2] = 0.0f;
    Result.Data[3][3] = 1.0f;

    return Result;
}

FMatrix FMatrix::MatrixOrthoLH(float ViewWidth, float ViewHeight, float NearZ, float FarZ)
{
    // Orthographic Projection
    // Ortho는 원래 작동했으므로 원래대로 유지
    FMatrix Result = FMatrix::Identity();
    Result.Data[0][0] = 2.0f / ViewWidth;
    Result.Data[1][1] = 2.0f / ViewHeight;
    Result.Data[2][2] = 1.0f / (FarZ - NearZ);
    Result.Data[3][2] = -NearZ / (FarZ - NearZ);
    return Result;
}

FMatrix FMatrix::MatrixPerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
{
    // DirectX 표준 Projection은 column-vector용 (M*v)
    // HLSL에서 mul(vector, matrix)는 row-vector 연산 (v*M)이므로 Transpose 필요

    float SinFov = sinf(0.5f * FovAngleY);
    float CosFov = cosf(0.5f * FovAngleY);
    float Height = CosFov / SinFov; // cot(fovY/2)

    // 표준 column-vector용 Projection
    FMatrix Standard = {};
    Standard.Data[0][0] = Height / AspectRatio;
    Standard.Data[1][1] = -Height;  // Y축 반전 (DX NDC는 Y+가 위지만 렌더링 보정 필요)
    Standard.Data[2][2] = FarZ / (FarZ - NearZ);
    Standard.Data[2][3] = 1.0f;  // perspective divide: w' = z
    Standard.Data[3][2] = -NearZ * FarZ / (FarZ - NearZ);

    // row-vector 연산을 위해 Transpose
    return Standard.Transpose();
}

/**
 * @brief 4x4 행렬의 역행렬을 계산합니다 (Gauss-Jordan 소거법)
 */
FMatrix FMatrix::Inverse() const
{
    FMatrix result = *this;
    FMatrix inverse = Identity();

    // Gauss-Jordan 소거법
    for (int32 i = 0; i < 4; i++)
    {
        // Pivot 찾기
        int32 pivot = i;
        float pivotValue = std::abs(result.Data[i][i]);
        for (int32 j = i + 1; j < 4; j++)
        {
            float val = std::abs(result.Data[j][i]);
            if (val > pivotValue)
            {
                pivot = j;
                pivotValue = val;
            }
        }

        // 행 교환
        if (pivot != i)
        {
            for (int32 j = 0; j < 4; j++)
            {
                std::swap(result.Data[i][j], result.Data[pivot][j]);
                std::swap(inverse.Data[i][j], inverse.Data[pivot][j]);
            }
        }

        // Pivot이 0에 가까우면 역행렬이 존재하지 않음
        if (std::abs(result.Data[i][i]) < 1e-10f)
        {
            return Identity(); // 역행렬이 없으면 항등행렬 반환
        }

        // Pivot으로 나누기
        float pivotDiv = result.Data[i][i];
        for (int32 j = 0; j < 4; j++)
        {
            result.Data[i][j] /= pivotDiv;
            inverse.Data[i][j] /= pivotDiv;
        }

        // 다른 행 소거
        for (int32 j = 0; j < 4; j++)
        {
            if (j != i)
            {
                float factor = result.Data[j][i];
                for (int32 k = 0; k < 4; k++)
                {
                    result.Data[j][k] -= factor * result.Data[i][k];
                    inverse.Data[j][k] -= factor * inverse.Data[i][k];
                }
            }
        }
    }

    return inverse;
}

/**
 * @brief 지정된 행의 벡터를 FVector로 반환합니다 (x, y, z 성분만)
 */
FVector FMatrix::GetRow(int32 RowIndex) const
{
    if (RowIndex < 0 || RowIndex >= 4)
    {
        return FVector();
    }
    return FVector(Data[RowIndex][0], Data[RowIndex][1], Data[RowIndex][2]);
}

/**
 * @brief 지정된 행의 벡터를 FVector4로 반환합니다 (x, y, z, w 모든 성분)
 */
FVector4 FMatrix::GetRow4(int32 RowIndex) const
{
    if (RowIndex < 0 || RowIndex >= 4)
    {
        return FVector4();
    }
    return FVector4(Data[RowIndex][0], Data[RowIndex][1], Data[RowIndex][2], Data[RowIndex][3]);
}

