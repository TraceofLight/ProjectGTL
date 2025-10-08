#pragma once

#include "Runtime/Core/Public/ModuleManager.h"

class URHIDevice;
class FRHICommandList;
class FEditorRenderResources;

/**
 * @brief 언리얼 스타일의 렌더러 모듈
 * 렌더링 시스템의 최상위 계층으로 RHI Device를 사용하여 렌더링 파이프라인을 관리
 */
class FRendererModule : public IModuleInterface
{
public:
    // IModuleInterface 구현
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual bool IsGameModule() const override { return false; }
    
    // RHI Device 접근자
    URHIDevice* GetRHIDevice() const { return RHIDevice; }
    void SetRHIDevice(URHIDevice* InRHIDevice);
    
    // Editor Render Resources 접근자
    FEditorRenderResources* GetEditorResources() const { return EditorResources.get(); }
    
    // 편의 함수들
    void BeginRendering();
    void EndRendering();
    
    // 기즈모 렌더링 (기즈모에서 사용)
    void RenderGizmoPrimitive(const struct FEditorPrimitive& Primitive, const struct FRenderState& RenderState, 
                             const FVector& CameraLocation, float ViewportWidth, float ViewportHeight);
    
    // 정적 접근자
    static FRendererModule& Get();

private:
    URHIDevice* RHIDevice = nullptr;
    bool bIsRenderingActive = false;
    std::unique_ptr<FEditorRenderResources> EditorResources;
    
    void InitializeRenderingResources();
    void CleanupRenderingResources();
};