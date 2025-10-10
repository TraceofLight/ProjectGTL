#pragma once
#include <d3d11.h>

/**
 * @brief Texture 렌더링을 위한 프록시 클래스
 * DirectX 11 리소스를 래핑하여 렌더링 시스템에서 사용
 */
class FTextureRenderProxy
{
public:
    FTextureRenderProxy();
    explicit FTextureRenderProxy(ID3D11ShaderResourceView* InSRV);
    ~FTextureRenderProxy();

    /**
     * @brief ShaderResourceView 설정
     * @param InSRV D3D11 Shader Resource View
     */
    void SetShaderResourceView(ID3D11ShaderResourceView* InSRV);

    /**
     * @brief ShaderResourceView 가져오기
     * @return D3D11 Shader Resource View 포인터
     */
    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView; }

    /**
     * @brief 텍스처가 유효한지 확인
     * @return ShaderResourceView가 null이 아니면 true
     */
    bool IsValid() const { return ShaderResourceView != nullptr; }

    /**
     * @brief 리소스 해제
     */
    void Release();

private:
    /** DirectX 11 Shader Resource View */
    ID3D11ShaderResourceView* ShaderResourceView = nullptr;

    /** 리소스 소유권 여부 (소멸자에서 Release 호출할지 결정) */
    bool bOwnsResource = false;
};