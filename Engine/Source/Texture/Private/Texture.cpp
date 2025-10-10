#include "pch.h"
#include "Texture/Public/Texture.h"
#include "Runtime/Renderer/Public/TextureResource.h"
#include "Runtime/Renderer/Public/TextureRenderProxy.h"
#include "Runtime/RHI/Public/RHIDevice.h"

IMPLEMENT_CLASS(UTexture, UObject)

/**
 * @brief Default Constructor
 * 따로 사용할 의도는 없지만 매크로 규격에 맞게 추가
 * 생성에 필요한 내용을 갖추도록 구현
 */
UTexture::UTexture()
{
	SetName(FName::FName_None);
}

UTexture::UTexture(const FString& InFilePath, FName InName)
	: TextureFilePath(InFilePath)
{
	SetName(InName);

	// RenderProxy 생성
	if (!InFilePath.empty())
	{
		RenderProxy = new FTextureRenderProxy(InFilePath);
	}
}

UTexture::~UTexture()
{
	// RenderProxy 해제
	if (RenderProxy)
	{
		RenderProxy->BeginReleaseResource();
		delete RenderProxy;
		RenderProxy = nullptr;
	}
}

ID3D11ShaderResourceView* UTexture::GetShaderResourceView() const
{
	UE_LOG_WARNING("UTexture::GetShaderResourceView - Deprecated! Use GetRenderProxy() instead.");
	return nullptr;
}

void UTexture::SetTexturePath(const FString& InTexturePath)
{
	// 언리얼 방식: 경로만 저장, GPU 리소스는 렌더링 시점에 생성
	TextureFilePath = InTexturePath;

	// 기존 RenderProxy 해제 후 새로 생성
	if (RenderProxy)
	{
		RenderProxy->BeginReleaseResource();
		delete RenderProxy;
		RenderProxy = nullptr;
	}

	// 새로운 RenderProxy 생성
	if (!InTexturePath.empty())
	{
		RenderProxy = new FTextureRenderProxy(InTexturePath);
	}
}

FString UTexture::GetTexturePath() const
{
	return TextureFilePath;
}
