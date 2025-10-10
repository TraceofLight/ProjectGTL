#pragma once
#include "Runtime/Core/Public/Object.h"

class FTextureRenderProxy;

UCLASS()
class UTexture :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UTexture, UObject)

public:
	// DrawIndexedPrimitivesCommand에서 필요한 메서드
	virtual ID3D11ShaderResourceView* GetShaderResourceView() const;

	// 텍스처 유효성 검사 (경로가 비어있지 않으면 유효)
	bool IsValid() const { return !TextureFilePath.empty(); }

	// FTextureRenderProxy 접근자 (렌더 스레드용)
	FTextureRenderProxy* GetRenderProxy() const { return RenderProxy; }

	// 텍스처 경로 설정
	void SetTexturePath(const FString& InTexturePath);

	// 텍스처 경로 가져오기 (렌더링용)
	FString GetTexturePath() const;

public:
	// Getter & Setter
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }

	// Special member function
	UTexture();
	UTexture(const FString& InFilePath, FName InName);
	~UTexture() override;

private:
	// 텍스처 파일 경로
	FString TextureFilePath;
	uint32 Width = 0;
	uint32 Height = 0;

	// 렌더링 프록시
	FTextureRenderProxy* RenderProxy = nullptr;
};
