#pragma once

class IRHICommand;
class IRenderPass;
class FSceneView;
class FSceneViewFamily;
class FRHICommandList;

enum class ERenderPassType : uint8;

class UWorld;
class FRHIDevice;

/**
* @brief Scene 기반 렌더링을 제공하는 클래스
* SceneView와 World 정보를 바탕으로 여러 렌더 패스를 조율하여 최종 이미지를 생성
*/
class FSceneRenderer
{
public:
    // 정적 팩토리 메소드로 생성 (Viewport Draw마다 새로 생성)
    static FSceneRenderer* CreateSceneRenderer(const FSceneViewFamily& InViewFamily);
    ~FSceneRenderer();

    void Render();
    void Cleanup();

    // RenderCommandList 접근자
    FRHICommandList* GetCommandList() const { return CommandList; }

    // 기즈모 렌더링 메서드
    static void RenderGizmoPrimitive(const struct FEditorPrimitive& Primitive, const struct FRenderState& RenderState, 
                                    const FVector& CameraLocation, float ViewportWidth, float ViewportHeight);

private:
    const FSceneViewFamily* ViewFamily;
    TArray<IRenderPass*> RenderPasses;
    FRHICommandList* CommandList;

    static FRHIDevice* GlobalRHI;

    FSceneRenderer(const FSceneViewFamily& InViewFamily);

    void CreateDefaultRenderPasses();
    void RenderView(const FSceneView* InSceneView);
};
