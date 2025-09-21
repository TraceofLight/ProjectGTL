#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Manager/Config/Public/ConfigManager.h"

enum class ECameraType
{
	ECT_Orthographic,
	ECT_Perspective
};

UCLASS()
class UCamera :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UCamera, UObject)

public:
	// Camera Speed Constants
	static constexpr float MIN_SPEED = 10.0f;
	static constexpr float MAX_SPEED = 70.0f;
	static constexpr float DEFAULT_SPEED = 20.0f;
	static constexpr float SPEED_ADJUST_STEP = 1.0f;

	UCamera() :
		ViewProjConstants(FViewProjConstants()),
		RelativeLocation(FVector(-15.0f, 0.f, 10.0f)), RelativeRotation(FVector(0, 0, 0)),
		FovY(90.f), Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT),
		NearZ(0.1f), FarZ(1000.f), CameraType(ECameraType::ECT_Perspective)
	{
		CurrentMoveSpeed = UConfigManager::GetInstance().GetCameraSensitivity();
	}

	~UCamera() override
	{
		UConfigManager::GetInstance().SetCameraSensitivity(CurrentMoveSpeed);
	}

	void Update();
	void UpdateMatrixByPers();
	void UpdateMatrixByOrth();

	/**
	 * @brief Setter
	 */
	void SetLocation(const FVector& InOtherPosition) { RelativeLocation = InOtherPosition; }
	void SetRotation(const FVector& InOtherRotation) { RelativeRotation = InOtherRotation; }
	void SetFovY(const float InOtherFovY) { FovY = InOtherFovY; }
	void SetAspect(const float InOtherAspect) { Aspect = InOtherAspect; }
	void SetNearZ(const float InOtherNearZ) { NearZ = InOtherNearZ; }
	void SetFarZ(const float InOtherFarZ) { FarZ = InOtherFarZ; }
	void SetCameraType(const ECameraType InCameraType) { CameraType = InCameraType; }

	/**
	 * @brief Getter
	 */
	const FViewProjConstants& GetFViewProjConstants() const { return ViewProjConstants; }
	const FViewProjConstants GetFViewProjConstantsInverse() const;

	FRay ConvertToWorldRay(float NdcX, float NdcY) const;

	FVector CalculatePlaneNormal(const FVector4& Axis);
	FVector CalculatePlaneNormal(const FVector& Axis);
	FVector& GetLocation() { return RelativeLocation; }
	FVector& GetRotation() { return RelativeRotation; }
	const FVector& GetForward() const { return Forward; }
	const FVector& GetUp() const { return Up; }
	const FVector& GetRight() const { return Right; }
	float GetFovY() const { return FovY; }
	float GetAspect() const { return Aspect; }
	float GetNearZ() const { return NearZ; }
	float GetFarZ() const { return FarZ; }
	ECameraType GetCameraType() const { return CameraType; }

	// Camera Movement Speed Control
	float GetMoveSpeed() const { return CurrentMoveSpeed; }

	void SetMoveSpeed(float InSpeed)
	{
		CurrentMoveSpeed = clamp(InSpeed, MIN_SPEED, MAX_SPEED);
		// CurrentMoveSpeed = min(InSpeed, MAX_SPEED);
	}

	void AdjustMoveSpeed(float InDelta) { SetMoveSpeed(CurrentMoveSpeed + InDelta); }

	/* *
	 * @brief 행렬 형태로 저장된 좌표와 변환 행렬과의 연산한 결과를 반환합니다.
	 */
	inline FVector4 MultiplyPointWithMatrix(const FVector4& Point, const FMatrix& Matrix) const
	{
		FVector4 Result = Point * Matrix;
		/* *
		 * @brief 좌표가 왜곡된 공간에 남는 것을 방지합니다.
		 */
		if (Result.W != 0.f) { Result *= (1.f / Result.W); }

		return Result;
	}

private:
	FViewProjConstants ViewProjConstants = {};
	FVector RelativeLocation = {};
	FVector RelativeRotation = {};
	FVector Forward = {1, 0, 0};
	FVector Up = {0, 0, 1};
	FVector Right = {0, 1, 0};
	float FovY = {};
	float Aspect = {};
	float NearZ = {};
	float FarZ = {};
	float OrthoWidth = {};
	ECameraType CameraType = {};

	// Dynamic Movement Speed
	float CurrentMoveSpeed = DEFAULT_SPEED;
};
