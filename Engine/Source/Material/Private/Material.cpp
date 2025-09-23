#include "pch.h"
#include "Material/Public/Material.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UMaterialInterface, UObject)
IMPLEMENT_CLASS(UMaterial, UMaterialInterface)
IMPLEMENT_CLASS(UMaterialInstance, UMaterialInterface)

UMaterial::~UMaterial()
{
	SafeDelete(RenderProxy);
	if (DiffuseTexture)
	{
		DiffuseTexture->Release();
	}
	if (NormalTexture)
	{
		NormalTexture->Release();
	}
	if (SpecularTexture)
	{
		SpecularTexture->Release();
	}
}

void UMaterial::SetMaterialInfo(const FObjMaterialInfo& InMaterialInfo)
{
	MaterialInfo = InMaterialInfo;
}

const FObjMaterialInfo& UMaterial::GetMaterialInfo() const
{
	return MaterialInfo;
}

void UMaterial::ImportAllTextures()
{
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	const auto ImportTexture = [&](const FString& TexturePath, void (UMaterial::*Setter)(ID3D11ShaderResourceView*))
	{
		if (TexturePath.empty())
		{
			return;
		}

		ComPtr<ID3D11ShaderResourceView> Texture = AssetManager.LoadTexture(TexturePath);
		if (!Texture)
		{
			return;
		}

		(this->*Setter)(Texture.Get());
	};

	ImportTexture(MaterialInfo.DiffuseTexturePath, &UMaterial::SetDiffuseTexture);
	ImportTexture(MaterialInfo.NormalTexturePath, &UMaterial::SetNormalTexture);
	ImportTexture(MaterialInfo.SpecularTexturePath, &UMaterial::SetSpecularTexture);
}

void UMaterial::SetDiffuseTexture(ID3D11ShaderResourceView* InTexture)
{
	DiffuseTexture = InTexture;
}

ID3D11ShaderResourceView* UMaterial::GetDiffuseTexture() const
{
	return DiffuseTexture;
}

void UMaterial::SetNormalTexture(ID3D11ShaderResourceView* InTexture)
{
	NormalTexture = InTexture;
}

ID3D11ShaderResourceView* UMaterial::GetNormalTexture() const
{
	return NormalTexture;
}

void UMaterial::SetSpecularTexture(ID3D11ShaderResourceView* InTexture)
{
	SpecularTexture = InTexture;
}

ID3D11ShaderResourceView* UMaterial::GetSpecularTexture() const
{
	return SpecularTexture;
}

