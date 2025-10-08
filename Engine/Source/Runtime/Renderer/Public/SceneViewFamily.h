#pragma once

class FViewport;
class FSceneView;

/**
 * @brief 렌더링 피처 레벨
 * 현재 엔진은 DX11만 사용하므로 SM5만 필요
 */
enum class ERenderingFeatureLevel : uint8
{
    SM5 // Shader Model 5.0 (DX11)
};

/**
 * @brief 렌더 타겟 모드
 * 현재 엔진은 단순한 텍스처 설정만 사용
 */
enum class ESceneRenderTargetsMode : uint8
{
    SetTextures // 직접 텍스처 설정
};

/**
 * @brief SceneView들을 그룹화하고 기본 렌더링 설정을 관리하는 클래스
 */
class FSceneViewFamily
{
public:
    FSceneViewFamily();
    ~FSceneViewFamily();

    // 뷰 관리
    void AddView(FSceneView* InView);
    void RemoveView(FSceneView* InView);
    void ClearViews();

    const TArray<FSceneView*>& GetViews() const { return Views; }
    int32 GetViewCount() const { return Views.Num(); }
    FSceneView* GetView(int32 InIndex) const;

    // 렌더 타겟 설정
    void SetRenderTarget(FViewport* InRenderTarget) { RenderTarget = InRenderTarget; }
    FViewport* GetRenderTarget() const { return RenderTarget; }

    // 기본 시간 정보
    void SetCurrentTime(float InCurrentTime) { CurrentTime = InCurrentTime; }
    float GetCurTime() const { return CurrentTime; }

    void SetDeltaWorldTime(float InDeltaTime) { DeltaWorldTime = InDeltaTime; }
    float GetDeltaWorldTime() const { return DeltaWorldTime; }

    // 기본 렌더링 설정
    void SetFeatureLevel(ERenderingFeatureLevel InFeatureLevel) { FeatureLevel = InFeatureLevel; }
    ERenderingFeatureLevel GetFeatureLevel() const { return FeatureLevel; }

    void SetSceneRenderTargetsMode(ESceneRenderTargetsMode InMode) { SceneRenderTargetsMode = InMode; }
    ESceneRenderTargetsMode GetSceneRenderTargetsMode() const { return SceneRenderTargetsMode; }

    // 실시간 업데이트 설정 (에디터에서 필요)
    void SetRealtimeUpdate(bool bInRealtime) { bRealtimeUpdate = bInRealtime; }
    bool IsRealtimeUpdate() const { return bRealtimeUpdate; }

    // 유틸리티
    bool IsValid() const;
    void Reset();

private:
    // 뷰 컨테이너
    TArray<FSceneView*> Views;

    // 렌더 타겟
    FViewport* RenderTarget = nullptr;

    // 기본 시간 정보
    float CurrentTime = 0.0f;
    float DeltaWorldTime = 0.0f;

    // 기본 렌더링 설정
    ERenderingFeatureLevel FeatureLevel = ERenderingFeatureLevel::SM5;
    ESceneRenderTargetsMode SceneRenderTargetsMode = ESceneRenderTargetsMode::SetTextures;

    // 에디터 기본 플래그
    bool bRealtimeUpdate = true;
};
