#pragma once

class FSceneView;
class FSceneRenderer;

enum class ERenderPassType : uint8
{
    DepthPrePass, // 깊이 버퍼 채우기
    BasePass, // 기본 지오메트리 렌더링
    SkyboxPass, // 스카이박스 렌더링, 현재는 없음
    TranslucentPass, // 반투명 오브젝트 렌더링, 현재 없음
    PostProcessPass, // 포스트프로세싱 효과
    UIPass, // UI 오버레이 렌더링
    DebugPass, // 기즈모, 그리드, 디버그 정보

    Num
};

/**
 * @brief RenderPass Interface 클래스
 */
class IRenderPass
{
public:
    IRenderPass(ERenderPassType InPassType) : PassType(InPassType)
    {
    }

    virtual ~IRenderPass() = default;

    virtual void Execute(const FSceneView* View, FSceneRenderer* SceneRenderer) = 0;

    virtual void Initialize()
    {
    }

    virtual void Cleanup()
    {
    }

    virtual bool IsEnabled() const { return bEnabled; }
    virtual void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

    ERenderPassType GetPassType() const { return PassType; }

protected:
    ERenderPassType PassType;
    bool bEnabled = true;
};
