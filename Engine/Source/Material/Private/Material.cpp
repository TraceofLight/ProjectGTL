#include "pch.h"
#include "Material/Public/Material.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Runtime/Renderer/Public/MaterialRenderProxy.h"

IMPLEMENT_CLASS(UMaterialInterface, UObject)
IMPLEMENT_CLASS(UMaterial, UMaterialInterface)
IMPLEMENT_CLASS(UMaterialInstance, UMaterialInterface)

UMaterial::~UMaterial()
{
	// RenderProxy 해제 (언리얼 엔진 방식)
	if (RenderProxy)
	{
		RenderProxy->BeginReleaseResource();
		delete RenderProxy;
		RenderProxy = nullptr;
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

// DrawIndexedPrimitivesCommand 호환성을 위한 메서드들 구현 - 언리얼 방식
UTexture* UMaterial::GetTexture() const
{
	// DiffuseTexture 경로가 비어있으면 nullptr 반환
	if (MaterialInfo.DiffuseTexturePath.empty())
	{
		return nullptr;
	}

	// AssetSubsystem에서 UTexture 로드/가져오기
	UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
	if (!AssetSubsystem)
	{
		UE_LOG_ERROR("UMaterial::GetTexture - AssetSubsystem is null!");
		return nullptr;
	}

	// AssetSubsystem에서 텍스처 로드 (TObjectPtr에서 UTexture*로 변환)
	TObjectPtr<UTexture> TexturePtr = AssetSubsystem->LoadTexture(MaterialInfo.DiffuseTexturePath);
	return TexturePtr.Get(); // TObjectPtr에서 실제 포인터 반환
}

bool UMaterial::HasTexture() const
{
	// 텍스처 경로가 비어있지 않으면 텍스처가 있는 것
	return !MaterialInfo.DiffuseTexturePath.empty() ||
	       !MaterialInfo.NormalTexturePath.empty() ||
	       !MaterialInfo.SpecularTexturePath.empty();
}

FMaterialRenderProxy* UMaterial::GetRenderProxy() const
{
	// 지연 로딩: RenderProxy가 없으면 생성
	if (!RenderProxy)
	{
		// const_cast 사용 (언리얼 엔진에서도 사용하는 패턴)
		UMaterial* NonConstThis = const_cast<UMaterial*>(this);
		NonConstThis->RenderProxy = new FMaterialRenderProxy(NonConstThis);
		UE_LOG("UMaterial::GetRenderProxy - Created RenderProxy for material: %s", GetName().ToString().data());
	}

	return RenderProxy;
}

