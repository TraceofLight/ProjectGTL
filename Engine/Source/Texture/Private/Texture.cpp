#include "pch.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(UTexture, UObject)

/**
 * @brief Default Constructor
 * 따로 사용할 의도는 없지만 매크로 규격에 맞게 추가
 * 생성에 필요한 내용을 갖추도록 구현
 */
UTexture::UTexture()
	: TextureFilePath(FName::FName_None)
{
	SetName(FName::FName_None);
}

UTexture::UTexture(const FString& InFilePath, FName InName)
	: TextureFilePath(InFilePath)
{
	SetName(InName);
}

UTexture::~UTexture() = default;

ID3D11ShaderResourceView* UTexture::GetShaderResourceView() const
{
	// RenderProxy에서 SRV를 발취해야 하지만
	// 현재 FTextureRenderProxy 구조체가 정의되지 않음
	// 임시로 nullptr 반환
	return nullptr;
}
