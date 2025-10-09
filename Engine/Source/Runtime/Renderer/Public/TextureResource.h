#pragma once
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class FRHIDevice;

/**
 * @brief 개별 텍스처의 렌더링 리소스를 관리하는 클래스 (언리얼 엔진 방식)
 * 각 UTexture마다 하나의 FTextureResource가 생성됨
 */
class FTextureResource
{
public:
    FTextureResource();
    explicit FTextureResource(const FString& InTexturePath);
    ~FTextureResource();

    /**
     * @brief 텍스처 리소스 초기화 (렌더 스레드에서 호출)
     * @param RHIDevice RHI 디바이스
     * @return 초기화 성공 여부
     */
    bool InitRHI(FRHIDevice* RHIDevice);

    /**
     * @brief 텍스처 리소스 해제 (렌더 스레드에서 호출)
     */
    void ReleaseRHI();

    /**
     * @brief ShaderResourceView 가져오기
     */
    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView.Get(); }

    /**
     * @brief 텍스처가 초기화되었는지 확인
     */
    bool IsInitialized() const { return ShaderResourceView.Get() != nullptr; }

    /**
     * @brief 텍스처 경로 가져오기
     */
    const FString& GetTexturePath() const { return TexturePath; }

private:
    /** 텍스처 파일 경로 */
    FString TexturePath;

    /** DirectX 11 Shader Resource View (ComPtr로 자동 관리) */
    ComPtr<ID3D11ShaderResourceView> ShaderResourceView;

    /** 초기화 상태 */
    bool bIsInitialized = false;
};