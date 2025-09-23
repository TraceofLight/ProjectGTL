#pragma once

struct FObjInfo;
struct FObjMaterialInfo;
struct FStaticMeshSection;
class UMaterialInterface;

struct FStaticMesh;
class UStaticMesh;

/**
 * @brief 전역의 On-Memory Asset을 관리하는 매니저 클래스
 */
UCLASS()
class UAssetManager
	: public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UAssetManager, UObject)

public:
	void Initialize();
	void Release();

	// Vertex 관련 함수들 (TODO: 얘네 싹다 삭제해야함)
	TArray<FVertex>* GetVertexData(EPrimitiveType InType);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType InType);
	uint32 GetNumVertices(EPrimitiveType InType);

	// Shader 관련 함수들
	ID3D11VertexShader* GetVertexShader(EShaderType Type);
	ID3D11PixelShader* GetPixelShader(EShaderType Type);
	ID3D11InputLayout* GetInputLayout(EShaderType Type);

	// Texture 관련 함수들
	ComPtr<ID3D11ShaderResourceView> LoadTexture(const FString& InFilePath, const FName& InName = FName::FName_None);
	ComPtr<ID3D11ShaderResourceView> GetTexture(const FString& InFilePath);
	void ReleaseTexture(const FString& InFilePath);
	bool HasTexture(const FString& InFilePath) const;
	ID3D11ShaderResourceView* GetDefaultTexture() const;

	// Material 관련 함수들
	UMaterialInterface* GetDefaultMaterial() const;
	UMaterialInterface* CreateMaterial(const FObjMaterialInfo& MaterialInfo) const;

	// Create Texture
	static ID3D11ShaderResourceView* CreateTextureFromFile(const path& InFilePath);
	static ID3D11ShaderResourceView* CreateTextureFromMemory(const void* InData, size_t InDataSize);

	// StaticMesh 관련 함수들
	TObjectPtr<UStaticMesh> LoadStaticMesh(const FString& InFilePath);
	TObjectPtr<UStaticMesh> GetStaticMesh(const FString& InFilePath);
	void ReleaseStaticMesh(const FString& InFilePath);
	bool HasStaticMesh(const FString& InFilePath) const;

private:
	// Vertex Resource
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;

	// Shaser Resources
	TMap<EShaderType, ID3D11VertexShader*> VertexShaders;
	TMap<EShaderType, ID3D11InputLayout*> InputLayouts;
	TMap<EShaderType, ID3D11PixelShader*> PixelShaders;

	// Texture Resource
	TMap<FString, ID3D11ShaderResourceView*> TextureCache;

	// Default Resources
	ComPtr<ID3D11ShaderResourceView> DefaultTexture = nullptr;
	UMaterialInterface* DefaultMaterial = nullptr;

	void InitializeDefaultTexture();
	void InitializeDefaultMaterial();
	void ReleaseDefaultTexture();
	void ReleaseDefaultMaterial();

	// Release Functions
	void ReleaseAllTextures();

	// Initialize Functions
	void InitializeBasicPrimitives();	// 기본 도형 버퍼 생성 (Cube, Sphere, Plane, ...)
	void LoadStaticMeshShaders();

	// Material Helpers
	void CollectSectionMaterialNames(const TArray<FObjInfo>& ObjInfos, TArray<FString>& OutMaterialNames) const;
	const FObjMaterialInfo* FindMaterialInfoByName(const TArray<FObjInfo>& ObjInfos, const FString& MaterialName) const;
	void BuildMaterialSlots(const TArray<FObjInfo>& ObjInfos, TArray<UMaterialInterface*>& OutMaterialSlots, TMap<FString, int32>& OutMaterialNameToSlot);
	void AssignSectionMaterialSlots(FStaticMesh& StaticMeshData, const TMap<FString, int32>& MaterialNameToSlot) const;

	bool CheckEmptyMaterialSlots(const TArray<FStaticMeshSection>& Sections) const;
	void InsertDefaultMaterial(FStaticMesh& InStaticMeshData, TArray<UMaterialInterface*>& InMaterialSlots);
};
