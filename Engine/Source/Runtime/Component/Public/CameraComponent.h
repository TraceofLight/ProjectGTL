#pragma once
#include "Runtime/Component/Public/SceneComponent.h"

enum class ECameraType
{
	ECT_Orthographic,
	ECT_Perspective
};

UCLASS()
class UCameraComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraComponent, USceneComponent)

public:
	UCameraComponent();
	~UCameraComponent() override = default;

	void UpdateMatrixByPers();
	void UpdateMatrixByOrth();

	/**
	 * @brief Setter
	 */
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

	const FVector& GetForward() const { return Forward; }
	const FVector& GetUp() const { return Up; }
	const FVector& GetRight() const { return Right; }
	float GetFovY() const { return FovY; }
	float GetAspect() const { return Aspect; }
	float GetNearZ() const { return NearZ; }
	float GetFarZ() const { return FarZ; }
	ECameraType GetCameraType() const { return CameraType; }
	float GetOrthographicHeight() const;
	static FVector4 MultiplyPointWithMatrix(const FVector4& InPoint, const FMatrix& InMatrix);

	void UpdateVectors();

private:
	FViewProjConstants ViewProjConstants = {};
	FVector Forward = {1, 0, 0};
	FVector Up = {0, 0, 1};
	FVector Right = {0, 1, 0};
	float FovY = 90.f;
	float Aspect = float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT;
	float NearZ = 0.1f;
	float FarZ = 1000.f;
	float OrthoWidth = 0.f;
	ECameraType CameraType = ECameraType::ECT_Perspective;
};
