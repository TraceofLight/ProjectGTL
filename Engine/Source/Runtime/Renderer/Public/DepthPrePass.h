#pragma once
#include "RenderPass.h"

class AActor;

/**
 * @brief Depth 우선 처리하는 전처리 Pass
 */
class FDepthPrePass :
    public IRenderPass
{
public:
    FDepthPrePass() : IRenderPass(ERenderPassType::DepthPrePass)
    {
    }

    void Execute(const FSceneView* View, FSceneRenderer* SceneRenderer) override;

private:
    void RenderActorDepth(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer);
};
