#pragma once
#include "RenderPass.h"

class UPrimitiveComponent;
class AActor;

/**
 * @brief Geometry Render Pass
 */
class FBasePass :
	public IRenderPass
{
public:
	FBasePass() : IRenderPass(ERenderPassType::BasePass)
	{
	}

	~FBasePass() override;

	void Execute(const FSceneView* View, FSceneRenderer* SceneRenderer) override;
	void Initialize() override;
	void Cleanup() override;

private:
	bool RenderActor(AActor* Actor, const FSceneView* View, class FRHICommandList* RHICmdList,
	                 const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, const FVector& HighlightColor);
	void RenderPrimitiveComponent(UPrimitiveComponent* Component, const FSceneView* View,
	                              FRHICommandList* RHICmdList, const FMatrix& ViewMatrix,
	                              const FMatrix& ProjectionMatrix);
};
