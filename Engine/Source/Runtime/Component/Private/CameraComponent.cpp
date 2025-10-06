#include "pch.h"
#include "Runtime/Component/Public/CameraComponent.h"

IMPLEMENT_CLASS(UCameraComponent, USceneComponent)

UCameraComponent::UCameraComponent()
	: ViewProjConstants(FViewProjConstants())
	  , FovY(90.f)
	  , Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT)
	  , NearZ(0.1f)
	  , FarZ(1000.f)
	  , CameraType(ECameraType::ECT_Perspective)
{
}

void UCameraComponent::UpdateVectors()
{
	FMatrix rotationMatrix = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(GetRelativeRotation()));

	FVector4 Forward4 = FVector4(1, 0, 0, 1) * rotationMatrix;
	Forward = FVector(Forward4.X, Forward4.Y, Forward4.Z);
	Forward.Normalize();

	FVector4 worldUp4 = FVector4(0, 0, 1, 1) * rotationMatrix;
	FVector worldUp = {worldUp4.X, worldUp4.Y, worldUp4.Z};
	Right = Forward.Cross(worldUp);
	Right.Normalize();
	Up = Right.Cross(Forward);
	Up.Normalize();
}

void UCameraComponent::UpdateMatrixByPers()
{
	UpdateVectors();

	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(GetRelativeLocation());
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	ViewProjConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FMatrix P = FMatrix::Identity();
	// | f/aspect   0        0         0 |
	// |    0       f        0         0 |
	// |    0       0   zf/(zf-zn)     1 |
	// |    0       0  -zn*zf/(zf-zn)  0 |
	P.Data[0][0] = F / Aspect;
	P.Data[1][1] = F;
	P.Data[2][2] = FarZ / (FarZ - NearZ);
	P.Data[2][3] = 1.0f;
	P.Data[3][2] = (-NearZ * FarZ) / (FarZ - NearZ);
	P.Data[3][3] = 0.0f;

	ViewProjConstants.Projection = P;
}

void UCameraComponent::UpdateMatrixByOrth()
{
	UpdateVectors();

	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(GetRelativeLocation());
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	ViewProjConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 */
	// FovY 안전성 검사 추가
	float SafeFovY = max(5.0f, min(179.0f, abs(FovY)));
	OrthoWidth = 2.0f * std::tanf(FVector::GetDegreeToRadian(SafeFovY) * 0.5f);
	const float OrthoHeight = OrthoWidth / max(0.1f, Aspect);
	const float Left = -OrthoWidth * 0.5f;
	const float Right = OrthoWidth * 0.5f;
	const float Bottom = -OrthoHeight * 0.5f;
	const float Top = OrthoHeight * 0.5f;

	FMatrix P = FMatrix::Identity();
	P.Data[0][0] = 2.0f / (Right - Left);
	P.Data[1][1] = 2.0f / (Top - Bottom);
	P.Data[2][2] = 1.0f / (FarZ - NearZ);
	P.Data[3][0] = -(Right + Left) / (Right - Left);
	P.Data[3][1] = -(Top + Bottom) / (Top - Bottom);
	P.Data[3][2] = -NearZ / (FarZ - NearZ);
	P.Data[3][3] = 1.0f;
	ViewProjConstants.Projection = P;
}

const FViewProjConstants UCameraComponent::GetFViewProjConstantsInverse() const
{
	/*
	* @brief View^(-1) = R * T
	*/
	FViewProjConstants Result = {};
	FMatrix R = FMatrix(Right, Up, Forward);
	FMatrix T = FMatrix::TranslationMatrix(GetRelativeLocation());
	Result.View = R * T;

	if (CameraType == ECameraType::ECT_Orthographic)
	{
		// 안전한 OrthoWidth 사용 (음수나 비정상 값 방지)
		float SafeOrthoWidth = max(0.1f, abs(OrthoWidth));
		const float OrthoHeight = SafeOrthoWidth / max(0.1f, Aspect);
		const float Left = -SafeOrthoWidth * 0.5f;
		const float Right = SafeOrthoWidth * 0.5f;
		const float Bottom = -OrthoHeight * 0.5f;
		const float Top = OrthoHeight * 0.5f;

		FMatrix P = FMatrix::Identity();
		// A^{-1} (대각)
		P.Data[0][0] = (Right - Left) * 0.5f; // (r-l)/2
		P.Data[1][1] = (Top - Bottom) * 0.5f; // (t-b)/2
		P.Data[2][2] = (FarZ - NearZ); // (zf-zn)
		// -b A^{-1} (마지막 행의 x,y,z)
		P.Data[3][0] = (Right + Left) * 0.5f; // (r+l)/2
		P.Data[3][1] = (Top + Bottom) * 0.5f; // (t+b)/2
		P.Data[3][2] = NearZ; // zn
		P.Data[3][3] = 1.0f;
		Result.Projection = P;
	}
	else if ((CameraType == ECameraType::ECT_Perspective))
	{
		const float FovRadian = FVector::GetDegreeToRadian(FovY);
		const float F = 1.0f / std::tanf(FovRadian * 0.5f);
		FMatrix P = FMatrix::Identity();
		// | aspect/F   0      0         0 |
		// |    0      1/F     0         0 |
		// |    0       0      0   -(zf-zn)/(zn*zf) |
		// |    0       0      1        zf/(zn*zf)  |
		P.Data[0][0] = Aspect / F;
		P.Data[1][1] = 1.0f / F;
		P.Data[2][2] = 0.0f;
		P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
		P.Data[3][2] = 1.0f;
		P.Data[3][3] = FarZ / (NearZ * FarZ);
		Result.Projection = P;
	}

	return Result;
}

float UCameraComponent::GetOrthographicHeight() const
{
	// 직교 투영에서 FovY로부터 실제 화면 높이 계산
	// FovY 값 안전성 검사 추가 (음수나 비정상 값 방지)
	float SafeFovY = max(5.0f, min(179.0f, abs(FovY))); // 절대값 사용 + 5도~179도 제한
	float FovYRadians = FVector::GetDegreeToRadian(SafeFovY);
	float TanHalfFov = std::tanf(FovYRadians * 0.5f);

	// TanHalfFov 값도 안전성 검사
	if (TanHalfFov <= 0.0f || !isfinite(TanHalfFov))
	{
		return 10.0f; // 비정상 값이면 기본값 반환
	}

	float OrthoWidth = 2.0f * TanHalfFov;
	float SafeAspect = max(0.1f, Aspect); // Aspect 비율 안전 보장
	float OrthographicHeight = OrthoWidth / SafeAspect;

	// 최소/최대 크기 보장 (기즈모가 사라지거나 너무 커지지 않도록)
	return max(0.1f, min(1000.0f, OrthographicHeight));
}

/**
 * @brief 행렬 형태로 저장된 좌표와 변환 행렬과의 연산한 결과를 반환합니다.
 */
FVector4 UCameraComponent::MultiplyPointWithMatrix(const FVector4& InPoint, const FMatrix& InMatrix)
{
	FVector4 Result = InPoint * InMatrix;

	// 좌표가 왜곡된 공간에 남는 것을 방지합니다.
	if (Result.W != 0.f)
	{
		Result *= (1.f / Result.W);
	}

	return Result;
}

FRay UCameraComponent::ConvertToWorldRay(float NdcX, float NdcY) const
{
	/**
	 * @brief 반환할 타입의 객체 선언
	 */
	FRay Ray = {};

	const FViewProjConstants& ViewProjMatrix = GetFViewProjConstantsInverse();

	/**
	 * @brief NDC 좌표 정보를 행렬로 변환합니다.
	 */
	const FVector4 NdcNear(NdcX, NdcY, 0.0f, 1.0f);
	const FVector4 NdcFar(NdcX, NdcY, 1.0f, 1.0f);

	/**
	 * @brief Projection 행렬을 View 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 ViewNear = MultiplyPointWithMatrix(NdcNear, ViewProjMatrix.Projection);
	const FVector4 ViewFar = MultiplyPointWithMatrix(NdcFar, ViewProjMatrix.Projection);

	/**
	 * @brief View 행렬을 World 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 WorldNear = MultiplyPointWithMatrix(ViewNear, ViewProjMatrix.View);
	const FVector4 WorldFar = MultiplyPointWithMatrix(ViewFar, ViewProjMatrix.View);

	/**
	 * @brief 카메라의 월드 좌표를 추출합니다.
	 * Row-major 기준, 마지막 행 벡터는 위치 정보를 가지고 있음
	 */
	const FVector4 CameraPosition(
		ViewProjMatrix.View.Data[3][0],
		ViewProjMatrix.View.Data[3][1],
		ViewProjMatrix.View.Data[3][2],
		ViewProjMatrix.View.Data[3][3]);

	if (CameraType == ECameraType::ECT_Perspective)
	{
		FVector4 DirectionVector = WorldFar - CameraPosition;
		DirectionVector.Normalize();

		Ray.Origin = CameraPosition;
		Ray.Direction = DirectionVector;
	}
	else if (CameraType == ECameraType::ECT_Orthographic)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}

	return Ray;
}

FVector UCameraComponent::CalculatePlaneNormal(const FVector4& Axis)
{
	return Forward.Cross(FVector(Axis.X, Axis.Y, Axis.Z));
}

FVector UCameraComponent::CalculatePlaneNormal(const FVector& Axis)
{
	return Forward.Cross(FVector(Axis.X, Axis.Y, Axis.Z));
}
