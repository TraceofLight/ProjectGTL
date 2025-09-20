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
	// Getter & Setter
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }

	// Special member function
	UTexture();
	UTexture(const FString& InFilePath, FName InName);
	~UTexture() override;

private:
	FName TextureFilePath;
	uint32 Width = 0;
	uint32 Height = 0;

	FTextureRenderProxy* RenderProxy = nullptr;

	friend class UAssetManager;
};
