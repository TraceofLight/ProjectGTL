#include "pch.h"
#include "Runtime/Renderer/Public/MaterialRenderProxy.h"
#include "Runtime/Renderer/Public/TextureRenderProxy.h"
#include "Material/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Runtime/RHI/Public/RHIDevice.h"

FMaterialRenderProxy::FMaterialRenderProxy(UMaterial* InMaterial)
    : bIsReleasing(false)
{
    if (InMaterial)
    {
        CreateTextureProxy(InMaterial);
        UE_LOG("FMaterialRenderProxy created for material: %s", InMaterial->GetName().ToString().data());
    }
    else
    {
        UE_LOG_WARNING("FMaterialRenderProxy created with null material");
    }
}

FMaterialRenderProxy::~FMaterialRenderProxy()
{
    if (TextureProxy)
    {
        delete TextureProxy;
        TextureProxy = nullptr;
    }
    UE_LOG("FMaterialRenderProxy destroyed");
}

ID3D11ShaderResourceView* FMaterialRenderProxy::GetTextureForRendering_RenderThread(FRHIDevice* RHIDevice)
{
    // 렌더 스레드에서만 호출되어야 함
    // checkSlow(IsInRenderingThread()); // Unreal의 스레드 체크 함수

    assert(!bIsReleasing && "Material is being released!");
    assert(TextureProxy != nullptr && "TextureProxy is null!");
    assert(RHIDevice != nullptr && "RHIDevice is null!");

    // TextureProxy를 통해 실제 텍스처 리소스 요청
    ID3D11ShaderResourceView* Result = TextureProxy->GetTextureForRendering_RenderThread(RHIDevice);
    assert(Result != nullptr && "TextureProxy returned null SRV!");
    return Result;
}

bool FMaterialRenderProxy::IsValid() const
{
    return !bIsReleasing && TextureProxy != nullptr;
}

void FMaterialRenderProxy::BeginReleaseResource()
{
    // 게임 스레드에서 호출 - 렌더 스레드에 리소스 해제 명령 전달
    bIsReleasing = true;

    if (TextureProxy)
    {
        TextureProxy->BeginReleaseResource();
    }

    UE_LOG("FMaterialRenderProxy - BeginReleaseResource called");
}

void FMaterialRenderProxy::CreateTextureProxy(UMaterial* Material)
{
    if (!Material)
    {
        return;
    }

    UTexture* TextureAsset = Material->GetTexture();
    if (TextureAsset)
    {
        FString TexturePath = TextureAsset->GetTexturePath();
        UE_LOG("[TEXTURE_DEBUG] Material texture path: '%s'", TexturePath.empty() ? "(EMPTY)" : TexturePath.c_str());
        
        if (!TexturePath.empty())
        {
            TextureProxy = new FTextureRenderProxy(TexturePath);
            UE_LOG("FMaterialRenderProxy - Created TextureProxy for: %s", TexturePath.c_str());
        }
        else
        {
            UE_LOG_WARNING("FMaterialRenderProxy - Material has texture but no valid path");
        }
    }
    else
    {
        UE_LOG("FMaterialRenderProxy - Material has no texture");
    }
}
