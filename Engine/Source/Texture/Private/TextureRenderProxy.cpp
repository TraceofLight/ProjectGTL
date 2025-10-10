#include "pch.h"
#include "Texture/Public/TextureRenderProxy.h"

FTextureRenderProxy::FTextureRenderProxy()
    : ShaderResourceView(nullptr)
    , bOwnsResource(false)
{
}

FTextureRenderProxy::FTextureRenderProxy(ID3D11ShaderResourceView* InSRV)
    : ShaderResourceView(InSRV)
    , bOwnsResource(false) // 기본적으로 소유권을 가지지 않음 (Material이 관리)
{
}

FTextureRenderProxy::~FTextureRenderProxy()
{
    if (bOwnsResource && ShaderResourceView)
    {
        ShaderResourceView->Release();
        ShaderResourceView = nullptr;
    }
}

void FTextureRenderProxy::SetShaderResourceView(ID3D11ShaderResourceView* InSRV)
{
    // 기존 리소스 해제 (소유권이 있는 경우에만)
    if (bOwnsResource && ShaderResourceView)
    {
        ShaderResourceView->Release();
    }

    ShaderResourceView = InSRV;
    bOwnsResource = false; // 기본적으로 소유권 없음
}

void FTextureRenderProxy::Release()
{
    if (ShaderResourceView)
    {
        if (bOwnsResource)
        {
            ShaderResourceView->Release();
        }
        ShaderResourceView = nullptr;
    }
    bOwnsResource = false;
}