#pragma once
#include "RenderPass.h"

class AActor;
class UEditor;
/**
 * @brief 디버그 렌더링 패스
 * 기즈모, 그리드, 선택 하이라이트 등을 해당 Pass에서 처리해야 함
 */
class FDebugPass :
    public IRenderPass
{
public:
    FDebugPass() : IRenderPass(ERenderPassType::DebugPass)
    {
    }

    ~FDebugPass() override;

    void Execute(const FSceneView* View, FSceneRenderer* SceneRenderer) override;
    void Initialize() override;
    void Cleanup() override;

private:
    void RenderGrid(const FSceneView* View, FSceneRenderer* SceneRenderer);
    void RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer, UEditor* Editor);
    void RenderDebugComponents(const FSceneView* View, FSceneRenderer* SceneRenderer);
    void RenderActorLines(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer, class FEditorRenderResources* EditorResources, UEditor* Editor);
    // Octree 기능 비활성화 (현재 사용 안함)
    void RenderOctreeBoxes(const FSceneView* View, FSceneRenderer* SceneRenderer);
    void RenderOctreeNodeRecursive(struct FOctreeNode* Node);

    // FEditorRenderResources로 대체됨 - Line Batching은 EditorRenderResources에서 처리
};
