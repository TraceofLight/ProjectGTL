#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Asset/Public/StaticMeshData.h"

struct FMaterialRenderProxy;
struct ID3D11ShaderResourceView;

/**
* @brief 머티리얼 공통 인터페이스
*/
UCLASS()
class UMaterialInterface : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterialInterface, UObject)
public:
	virtual FMaterialRenderProxy* GetRenderProxy() { return nullptr; } // GPU 바인딩용
};

/*
* @brief 베이스 머터리얼
* @note 주로 스테틱 메시 애셋의 기본 머티리얼 정의에 사용됨.
*/
UCLASS()
class UMaterial : public UMaterialInterface
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterial, UMaterialInterface)
public:
	void SetMaterialInfo(const FObjMaterialInfo& InMaterialInfo);
	const FObjMaterialInfo& GetMaterialInfo() const;

	void ImportAllTextures();
	const FString& GetMaterialName() const { return MaterialInfo.MaterialName; }

	void SetDiffuseTexture(ID3D11ShaderResourceView* InTexture);
	ID3D11ShaderResourceView* GetDiffuseTexture() const;

	void SetNormalTexture(ID3D11ShaderResourceView* InTexture);
	ID3D11ShaderResourceView* GetNormalTexture() const;

	void SetSpecularTexture(ID3D11ShaderResourceView* InTexture);
	ID3D11ShaderResourceView* GetSpecularTexture() const;

	// 셰이더 핸들/경로 + 파라미터(스칼라/벡터/텍스처)
	FMaterialRenderProxy* RenderProxy = nullptr;
	FMaterialRenderProxy* GetRenderProxy() override { return RenderProxy; }

private:
	FObjMaterialInfo MaterialInfo;
	ID3D11ShaderResourceView* DiffuseTexture = nullptr;
	ID3D11ShaderResourceView* NormalTexture = nullptr;
	ID3D11ShaderResourceView* SpecularTexture = nullptr;
};

/*
* @brief 컴포넌트들이 실제 사용하는 머티리얼 인스턴스
* @note 부모 머티리얼을 참조하고 일부 파라미터만 오버라이드해 인스턴스마다 변형을 주는 데에 사용됨.
*/
UCLASS()
class UMaterialInstance : public UMaterialInterface
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterialInstance, UMaterialInterface)
public:
	UMaterial* Parent = nullptr;
	// 오버라이드 파라미터 맵…
	FMaterialRenderProxy* RenderProxy = nullptr; // 머지된 결과
	FMaterialRenderProxy* GetRenderProxy() override { return RenderProxy; }
};

