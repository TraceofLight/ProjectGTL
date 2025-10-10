#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Asset/Public/StaticMeshData.h"

class UShader;
class FMaterialRenderProxy;
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
	// 이름은 UObject::GetName() 사용
	virtual FMaterialRenderProxy* GetRenderProxy() const { return nullptr; } // GPU 바인딩용

	// DrawIndexedPrimitivesCommand 호환성을 위한 메서드
	virtual class UTexture* GetTexture() const { return nullptr; }
	virtual bool HasTexture() const { return false; }
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
	~UMaterial() override;
	void SetMaterialInfo(const FObjMaterialInfo& InMaterialInfo);
	const FObjMaterialInfo& GetMaterialInfo() const;

	// Material 이름은 UObject::GetName() 사용

	// DrawIndexedPrimitivesCommand에서 필요한 메서드들 (오버라이드)
	UTexture* GetTexture() const override;
	bool HasTexture() const override;

	// 셰이더 핸들/경로 + 파라미터(스칼라/벡터/텍스처)
	FMaterialRenderProxy* GetRenderProxy() const override;

private:
	/** Material 정보 (경로 포함) - 언리얼 방식 */
	FObjMaterialInfo MaterialInfo;
	
	/** 렌더링 프록시 (언리얼 엔진 방식) */
	mutable FMaterialRenderProxy* RenderProxy = nullptr;
};

/*
* @brief 부모 머티리얼을 참조하고 일부 파라미터만 오버라이드해 인스턴스마다 변형을 주는 데에 사용됨.
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
	FMaterialRenderProxy* GetRenderProxy() const override { return RenderProxy; }
};

