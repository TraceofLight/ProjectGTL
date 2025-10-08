#include "pch.h"
#include "Material/Public/Material.h"
#include "Runtime/RHI/Public/RHIDevice.h"

IMPLEMENT_CLASS(UMaterialInterface, UObject)
IMPLEMENT_CLASS(UMaterial, UMaterialInterface)
IMPLEMENT_CLASS(UMaterialInstance, UMaterialInterface)

UMaterial::~UMaterial()
{
	SafeDelete(RenderProxy);
	if (DiffuseTexture)
	{
		URHIDevice::GetInstance().ReleaseTexture(DiffuseTexture);
	}
	if (NormalTexture)
	{
		URHIDevice::GetInstance().ReleaseTexture(NormalTexture);
	}
	if (SpecularTexture)
	{
		URHIDevice::GetInstance().ReleaseTexture(SpecularTexture);
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
	URHIDevice& RHIDevice = URHIDevice::GetInstance();

	const auto ImportTexture = [&](const FString& TexturePath, void (UMaterial::*Setter)(ID3D11ShaderResourceView*))
	{
		if (TexturePath.empty())
		{
			return;
		}

		std::wstring WTexturePath(TexturePath.begin(), TexturePath.end());
		ID3D11ShaderResourceView* SRV = RHIDevice.CreateTextureFromFile(WTexturePath);

		if (SRV)
		{
			(this->*Setter)(SRV);
		}
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

// DrawIndexedPrimitivesCommand 호환성을 위한 메서드들 구현
UTexture* UMaterial::GetTexture() const
{
	// 기본적으로 DiffuseTexture를 UTexture로 캐스팅해서 반환
	// 실제로는 UTexture 시스템과 연동해야 함
	return nullptr; // 임시로 nullptr 반환
}

bool UMaterial::HasTexture() const
{
	return DiffuseTexture != nullptr || NormalTexture != nullptr || SpecularTexture != nullptr;
}

