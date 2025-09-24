#pragma once
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;
class UCamera;

UCLASS()
class UBillBoardComponent :
	public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillBoardComponent, UPrimitiveComponent)

public:
	UBillBoardComponent();
	~UBillBoardComponent() override = default;

	void UpdateRotationMatrix(const UCamera* InCamera);

	FMatrix GetRTMatrix() const { return RTMatrix; }

	// 공통 렌더링 인터페이스 오버라이드
	virtual bool HasRenderData() const override;
	virtual bool UseIndexedRendering() const override;
	virtual EShaderType GetShaderType() const override;

private:
	FMatrix RTMatrix;
};
