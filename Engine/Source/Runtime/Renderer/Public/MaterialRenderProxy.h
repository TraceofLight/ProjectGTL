#pragma once
#include <d3d11.h>

class UMaterial;
class UTexture;
class FTextureRenderProxy;
class FRHIDevice;

/**
 * @brief UMaterial과 렌더 스레드 사이의 인터페이스 역할을 하는 RenderProxy
 * Unreal Engine 패턴을 따라 게임 스레드 asset과 렌더 스레드 리소스를 분리
 */
class FMaterialRenderProxy
{
public:
    FMaterialRenderProxy(UMaterial* InMaterial);
    ~FMaterialRenderProxy();

    /**
     * @brief 렌더 스레드에서 머티리얼의 텍스처 리소스 요청
     * @param RHIDevice RHI 디바이스
     * @return 머티리얼의 메인 텍스처 SRV (null이면 텍스처 없음)
     */
    ID3D11ShaderResourceView* GetTextureForRendering_RenderThread(FRHIDevice* RHIDevice);

    /**
     * @brief 머티리얼이 유효한지 확인
     */
    bool IsValid() const;

    /**
     * @brief 렌더 리소스 해제 (게임 스레드에서 호출)
     */
    void BeginReleaseResource();

private:
    /** 연결된 머티리얼의 텍스처 RenderProxy */
    FTextureRenderProxy* TextureProxy = nullptr;
    
    /** 리소스 해제 상태 */
    volatile bool bIsReleasing = false;

    /**
     * @brief 머티리얼에서 텍스처 프록시 생성 (게임 스레드에서 호출)
     */
    void CreateTextureProxy(UMaterial* Material);
};