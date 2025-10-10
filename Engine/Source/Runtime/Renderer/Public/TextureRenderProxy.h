#pragma once
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class FRHIDevice;

/**
 * @brief UTexture와 렌더 스레드 사이의 인터페이스 역할을 하는 RenderProxy
 * Unreal Engine 패턴을 따라 게임 스레드 asset과 렌더 스레드 리소스를 분리
 */
class FTextureRenderProxy
{
public:
    FTextureRenderProxy(const FString& InTexturePath);
    ~FTextureRenderProxy();

    /**
     * @brief 렌더 스레드에서 텍스처 리소스 요청
     * @param RHIDevice RHI 디바이스
     * @return Shader Resource View (null이면 로딩 실패)
     */
    ID3D11ShaderResourceView* GetTextureForRendering_RenderThread(FRHIDevice* RHIDevice);

    /**
     * @brief 텍스처 경로 반환
     */
    const FString& GetTexturePath() const { return TexturePath; }

    /**
     * @brief 렌더 리소스 해제 (게임 스레드에서 호출)
     */
    void BeginReleaseResource();

private:
    /** 텍스처 파일 경로 */
    FString TexturePath;
    
    /** GPU 리소스 (렌더 스레드에서만 접근) */
    ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
    
    /** 리소스 로딩 상태 */
    volatile bool bIsResourceLoaded = false;
    volatile bool bIsReleasing = false;

    /**
     * @brief 실제 텍스처 로딩 (렌더 스레드에서만 호출)
     */
    bool LoadTextureResource_RenderThread(FRHIDevice* RHIDevice);
};