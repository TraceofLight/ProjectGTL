#include "pch.h"
#include "Material/Public/Material.h"

IMPLEMENT_CLASS(UMaterialInterface, UObject)
IMPLEMENT_CLASS(UMaterial, UMaterialInterface)
IMPLEMENT_CLASS(UMaterialInstance, UMaterialInterface)

void UMaterial::SetMaterialInfo(const FObjMaterialInfo& InMaterialInfo)
{
	MaterialInfo = InMaterialInfo;
}

const FObjMaterialInfo& UMaterial::GetMaterialInfo() const
{
	return MaterialInfo;
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
