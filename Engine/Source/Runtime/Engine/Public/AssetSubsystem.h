#pragma once

#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/Containers/TMap.h"
#include "Engine/Classes/Engine/Engine.h"

class UMaterialInterface;
class UTexture;

/**
 * @brief 언리얼 엔진 스타일의 에셋 서브시스템
 * 
 * UResourceManager를 대체하는 언리얼 스타일 에셋 관리 시스템.
 * Engine Subsystem으로 동작하며 Material, Texture 등의 에셋을 관리합니다.
 */
UCLASS()
class ENGINE_API UAssetSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// Material 관리
	/**
	 * @brief Material을 로드하거나 캐시에서 가져옴
	 * @param MaterialPath Material 파일 경로
	 * @return 로드된 MaterialInterface 포인터 (실패시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset")
	UMaterialInterface* LoadMaterial(const FString& MaterialPath);

	/**
	 * @brief 기본 Material 반환
	 * @return 기본 Material 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset") 
	UMaterialInterface* GetDefaultMaterial() const;

	// Texture 관리
	/**
	 * @brief Texture를 로드하거나 캐시에서 가져옴
	 * @param TexturePath Texture 파일 경로
	 * @return 로드된 Texture 포인터 (실패시 nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset")
	UTexture* LoadTexture(const FString& TexturePath);

	/**
	 * @brief 기본 Texture 반환 (체커보드 패턴)
	 * @return 기본 Texture 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset")
	UTexture* GetDefaultTexture() const;

	// 에셋 캐시 관리
	/**
	 * @brief 모든 캐시된 에셋 해제
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset")
	void ReleaseAllAssets();

	/**
	 * @brief Material 캐시 해제
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset") 
	void ReleaseMaterials();

	/**
	 * @brief Texture 캐시 해제
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset")
	void ReleaseTextures();

private:
	// 내부 헬퍼 함수들
	UMaterialInterface* CreateMaterialFromPath(const FString& MaterialPath);
	UTexture* CreateTextureFromPath(const FString& TexturePath);

	void InitializeDefaultAssets();

private:
	// 에셋 캐시 (언리얼 스타일: UObject 포인터 사용)
	UPROPERTY()
	TMap<FString, UMaterialInterface*> MaterialCache;

	UPROPERTY()
	TMap<FString, UTexture*> TextureCache;

	// 기본 에셋들
	UPROPERTY()
	UMaterialInterface* DefaultMaterial;

	UPROPERTY()
	UTexture* DefaultTexture;
};