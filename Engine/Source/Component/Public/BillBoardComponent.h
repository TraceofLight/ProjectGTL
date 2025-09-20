#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

class UBillBoardComponent : public UPrimitiveComponent
{
public:
	UBillBoardComponent();
	~UBillBoardComponent();

	void UpdateRotationMatrix();

	FMatrix GetRTMatrix() const { return RTMatrix; }

	// Scene Proxy 생성
	class FBillBoardSceneProxy* CreateSceneProxy();

	// 공통 렌더링 인터페이스 오버라이드
	virtual bool HasRenderData() const override;
	virtual bool UseIndexedRendering() const override;
	virtual EShaderType GetShaderType() const override;
private:
	FMatrix RTMatrix;
};
