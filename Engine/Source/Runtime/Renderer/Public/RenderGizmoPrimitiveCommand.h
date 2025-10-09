#pragma once
#include "RenderCommand.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"

class FRHIDevice;

/**
 * @brief 기즈모 Primitive를 렌더링하는 RenderCommand
 * 화면 공간 스케일링을 지원하며 깊이 테스트 없이 항상 위에 표시
 */
class FRHIRenderGizmoPrimitiveCommand : public IRHICommand
{
public:
	FRHIRenderGizmoPrimitiveCommand(
		FRHIDevice* InRHIDevice,
		const FEditorPrimitive& InPrimitive,
		const FRenderState& InRenderState,
		const FVector& InCameraLocation,
		float InViewportWidth,
		float InViewportHeight)
		: RHIDevice(InRHIDevice)
		, Primitive(InPrimitive)
		, RenderState(InRenderState)
		, CameraLocation(InCameraLocation)
		, ViewportWidth(InViewportWidth)
		, ViewportHeight(InViewportHeight)
	{
		// 기즈모는 항상 최상위에 렌더링되도록 Priority를 높게 설정
		// Depth는 0 (가장 가까움), Priority는 255 (최우선)
		SortingKey = CreateSortingKey(0, 0, 0, 255);
	}

	void Execute() override;
	ERHICommandType GetCommandType() const override { return ERHICommandType::DrawIndexedPrimitives; }

private:
	FRHIDevice* RHIDevice;
	FEditorPrimitive Primitive;
	FRenderState RenderState;
	FVector CameraLocation;
	float ViewportWidth;
	float ViewportHeight;

	void CalculateScreenSpaceTransform(FMatrix& OutModelMatrix) const;
};
