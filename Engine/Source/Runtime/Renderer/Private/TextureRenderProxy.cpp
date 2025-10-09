#include "pch.h"
#include "Runtime/Renderer/Public/TextureRenderProxy.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Global/Paths.h"

FTextureRenderProxy::FTextureRenderProxy(const FString& InTexturePath)
    : TexturePath(InTexturePath)
    , bIsResourceLoaded(false)
    , bIsReleasing(false)
{
    UE_LOG("FTextureRenderProxy created for texture: %s", TexturePath.c_str());
}

FTextureRenderProxy::~FTextureRenderProxy()
{
    // 소멸자에서 즉시 해제 (이미 렌더 스레드에서 호출됨을 가정)
    ShaderResourceView.Reset();
    UE_LOG("FTextureRenderProxy destroyed for texture: %s", TexturePath.c_str());
}

ID3D11ShaderResourceView* FTextureRenderProxy::GetTextureForRendering_RenderThread(FRHIDevice* RHIDevice)
{
    // 렌더 스레드에서만 호출되어야 함
    // checkSlow(IsInRenderingThread()); // Unreal의 스레드 체크 함수

    if (bIsReleasing || !RHIDevice)
    {
        return nullptr;
    }

    // 이미 로딩된 경우 바로 반환
    if (bIsResourceLoaded && ShaderResourceView.Get())
    {
        return ShaderResourceView.Get();
    }

    // 아직 로딩되지 않은 경우 로딩 시도
    if (!bIsResourceLoaded)
    {
        if (LoadTextureResource_RenderThread(RHIDevice))
        {
            bIsResourceLoaded = true;
            UE_LOG("FTextureRenderProxy - Successfully loaded texture: %s", TexturePath.c_str());
        }
        else
        {
            UE_LOG_ERROR("FTextureRenderProxy - Failed to load texture: %s", TexturePath.c_str());
            return nullptr;
        }
    }

    return ShaderResourceView.Get();
}

void FTextureRenderProxy::BeginReleaseResource()
{
    // 게임 스레드에서 호출 - 렌더 스레드에 리소스 해제 명령 전달
    bIsReleasing = true;

    // TODO: Unreal Engine에서는 여기서 ENQUEUE_RENDER_COMMAND를 사용하여
    // 렌더 스레드에서 실제 리소스 해제를 수행
    // 현재는 단순화하여 즉시 해제
    ShaderResourceView.Reset();
    bIsResourceLoaded = false;

    UE_LOG("FTextureRenderProxy: BeginReleaseResource called for texture: %s", TexturePath.c_str());
}

bool FTextureRenderProxy::LoadTextureResource_RenderThread(FRHIDevice* RHIDevice)
{
    assert(RHIDevice != nullptr && "RHIDevice is null in LoadTextureResource_RenderThread!");
    assert(!TexturePath.empty() && "TexturePath is empty in LoadTextureResource_RenderThread!");

    path ActualTexturePath = TexturePath;

    path TexturePathObj = TexturePath;

    // 절대 경로이고 파일이 존재하는 경우 그대로 사용
    if (TexturePathObj.is_absolute() && exists(TexturePathObj))
    {
        ActualTexturePath = TexturePath;
    }
    else
    {
        // AssetSubsystem을 통해 파일 검색
        UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
        if (AssetSubsystem)
        {
            FString FoundPath = AssetSubsystem->FindTextureFilePath(TexturePath);
            if (!FoundPath.empty())
            {
                ActualTexturePath = FoundPath;
                UE_LOG("FTextureRenderProxy: Resolved texture path: %s -> %ls", TexturePath.c_str(), ActualTexturePath.c_str());
            }
            else
            {
                UE_LOG_ERROR("FTextureRenderProxy: Could not find texture file: %s", TexturePath.c_str());
                return false;
            }
        }
        else
        {
            UE_LOG_ERROR("FTextureRenderProxy: AssetSubsystem not available");
            return false;
        }
    }

    // RHI를 통해 텍스처 생성
    ID3D11ShaderResourceView* SRV = RHIDevice->CreateTextureFromFile(ActualTexturePath.wstring());

    // 이것이 실패하는 주요 원인일 것
    if (!SRV)
    {
        // assert 대신 로그만 사용 (예상되는 실패이므로)
        UE_LOG_ERROR("FTextureRenderProxy - RHIDevice failed to create texture: %s", TexturePath.c_str());
        return false;
    }

    // ComPtr에 할당 (자동 참조 카운트 관리)
    ShaderResourceView.Attach(SRV);
    return true;
}
