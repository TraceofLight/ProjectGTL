#include "pch.h"
#include "Runtime/Renderer/Public/TextureResource.h"
#include "Runtime/RHI/Public/RHIDevice.h"

FTextureResource::FTextureResource() = default;

FTextureResource::FTextureResource(const FString& InTexturePath)
    : TexturePath(InTexturePath)
    , bIsInitialized(false)
{
}

FTextureResource::~FTextureResource()
{
    ReleaseRHI();
}

bool FTextureResource::InitRHI(FRHIDevice* RHIDevice)
{
    if (bIsInitialized || TexturePath.empty() || !RHIDevice)
    {
        return false;
    }

    // 파일 경로를 와이드 스트링으로 변환
    std::wstring WTexturePath(TexturePath.begin(), TexturePath.end());
    
    // RHI를 통해 텍스처 생성
    ID3D11ShaderResourceView* SRV = RHIDevice->CreateTextureFromFile(WTexturePath);
    if (!SRV)
    {
        UE_LOG_ERROR("FTextureResource::InitRHI - Failed to load texture: %s", TexturePath.c_str());
        return false;
    }

    // ComPtr에 할당 (자동 참조 카운트 관리)
    ShaderResourceView.Attach(SRV);
    bIsInitialized = true;

    UE_LOG("FTextureResource::InitRHI - Successfully loaded texture: %s", TexturePath.c_str());
    return true;
}

void FTextureResource::ReleaseRHI()
{
    if (bIsInitialized)
    {
        ShaderResourceView.Reset(); // ComPtr이 자동으로 Release 호출
        bIsInitialized = false;
    }
}