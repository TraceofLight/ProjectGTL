#pragma once
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class ACameraActor;
class AActor;

UCLASS()
class UBillBoardComponent :
	public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillBoardComponent, UPrimitiveComponent)

public:
	UBillBoardComponent();
	~UBillBoardComponent() override = default;

	void UpdateRotationMatrix(const ACameraActor* InCamera);

	FMatrix GetRTMatrix() const { return RTMatrix; }

	// 공통 렌더링 인터페이스 오버라이드
	bool HasRenderData() const override;
	bool UseIndexedRendering() const override;
	EShaderType GetShaderType() const override;

private:
	FMatrix RTMatrix;
};
