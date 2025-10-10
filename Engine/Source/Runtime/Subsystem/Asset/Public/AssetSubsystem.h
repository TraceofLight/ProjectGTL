#pragma once
#include "Runtime/Subsystem/Public/EngineSubsystem.h"
#include "Shader/Public/Shader.h"

struct FStaticMeshSection;
class UStaticMesh;
class UTexture;
struct FObjMaterialInfo;
struct FObjInfo;
struct FStaticMesh;
class UMaterialInterface;

/**
 * @brief 전역의 On-Memory Asset을 관리하는 엔진 서브시스템
 */
UCLASS()
class UAssetSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UAssetSubsystem, UEngineSubsystem)

public:
	void Initialize() override;
	void Deinitialize() override;

	// Material 관련 함수들 (CPU 데이터만)
	UMaterialInterface* GetDefaultMaterial() const;
	UMaterialInterface* CreateMaterial(const FObjMaterialInfo& MaterialInfo) const;

	// Shader 관련 함수들
	TObjectPtr<UShader> LoadShader(const FString& InFilePath, const TArray<D3D11_INPUT_ELEMENT_DESC>& InLayoutDesc);

	// StaticMesh 관련 함수들 (CPU 데이터 로딩 및 캐싱)
	TObjectPtr<UStaticMesh> LoadStaticMesh(const FString& InFilePath);
	TObjectPtr<UStaticMesh> GetStaticMesh(const FString& InFilePath);
	void ReleaseStaticMesh(const FString& InFilePath);
	bool HasStaticMesh(const FString& InFilePath) const;

	// Texture 관련 함수들 (CPU 데이터만 - 경로 정보만)
	TObjectPtr<UTexture> LoadTexture(const FString& InFilePath);
	TObjectPtr<UTexture> GetTexture(const FString& InFilePath);
	void ReleaseTexture(const FString& InFilePath);
	bool HasTexture(const FString& InFilePath) const;

	// 텍스처 파일 검색 (파일명만으로 전체 경로 찾기)
	FString FindTextureFilePath(const FString& InFileName) const;

private:
	// Default Material (CPU 데이터만)
	mutable UMaterialInterface* DefaultMaterial = nullptr;

	// 셰이더 캐시
	TMap<FString, TObjectPtr<UShader>> ShaderCache;

	// 텍스처 캐시 (CPU 데이터만 - 경로 정보만)
	TMap<FString, TObjectPtr<UTexture>> TextureCache;

	void InitializeDefaultMaterial() const;
	void ReleaseDefaultMaterial();

	// Material Helpers
	void CollectSectionMaterialNames(const TArray<FObjInfo>& ObjInfos, TArray<FString>& OutMaterialNames) const;
	const FObjMaterialInfo* FindMaterialInfoByName(const TArray<FObjInfo>& ObjInfos, const FString& MaterialName) const;
	void BuildMaterialSlots(const TArray<FObjInfo>& ObjInfos, TArray<UMaterialInterface*>& OutMaterialSlots,
	                        TMap<FString, int32>& OutMaterialNameToSlot);
	void AssignSectionMaterialSlots(FStaticMesh& StaticMeshData, const TMap<FString, int32>& MaterialNameToSlot) const;

	bool CheckEmptyMaterialSlots(const TArray<FStaticMeshSection>& Sections) const;
	void InsertDefaultMaterial(UStaticMesh& InStaticMeshData, TArray<UMaterialInterface*>& InMaterialSlots);
};
