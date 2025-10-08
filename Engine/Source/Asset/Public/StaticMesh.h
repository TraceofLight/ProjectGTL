#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Asset/Public/StaticMeshData.h"
#include "Physics/Public/AABB.h"
#include "Material/Public/Material.h"

/**
 * @brief UStaticMesh: 스태틱 메시 데이터를 보유하는 애셋 클래스
 * @note 언리얼 엔진의 UStaticMesh에 해당
 * 렌더링을 위해 쿠킹된 메시 데이터를 포함
 */
UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();
	~UStaticMesh() override;

	/**
	 * @brief 스태틱 메시 데이터를 가져옴
	 * @return FStaticMesh 데이터 구조에 대한 참조
	 */
	const FStaticMesh& GetStaticMeshData() const { return StaticMeshData; }

	/**
	 * @brief 스태틱 메시 데이터를 설정
	 * @param InStaticMeshData 설정할 메시 데이터
	 */
	void SetStaticMeshData(const FStaticMesh& InStaticMeshData);

	/**
	 * @brief 원본 파일 경로를 가져옴
	 * @return 원본 메시 파일의 경로
	 */
	const FString& GetAssetPathFileName() const { return StaticMeshData.PathFileName; }

	/**
	 * @brief 렌더링을 위한 정점 데이터를 가져옴
	 * @return 정점 배열
	 */
	const TArray<FVertex>& GetVertices() const { return StaticMeshData.Vertices; }

	/**
	 * @brief 렌더링을 위한 인덱스 데이터를 가져옴
	 * @return 인덱스 배열
	 */
	const TArray<uint32>& GetIndices() const { return StaticMeshData.Indices; }

	/**
	 * @brief 메시가 유효한 데이터를 가지고 있는지 확인
	 * @return 메시가 정점과 인덱스를 가지고 있으면 true
	 */
	bool IsValidMesh() const;

	/**
	 * @brief 정점 버퍼를 가져옴
	 * @return 정점 버퍼 포인터
	 */
	ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }

	/**
	 * @brief 인덱스 버퍼를 가져옴
	 * @return 인덱스 버퍼 포인터
	 */
	ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }

	/**
	 * @brief 정점 스트라이드를 가져옴
	 * @return 정점 스트라이드
	 */
	uint32 GetVertexStride() const { return sizeof(FVertex); }

	/**
	 * @brief 정점 버퍼와 인덱스 버퍼를 생성
	 */
	void CreateRenderBuffers();

	/**
	 * @brief 버퍼 리소스를 해제
	 */
	void ReleaseRenderBuffers();

	/**
	 * @brief 메시의 AABB를 계산
	 * @return 계산된 AABB
	 */
	FAABB CalculateAABB() const;

	/**
	 * @brief 메시 데이터를 바이너리 파일로 저장
	 * @param FilePath 저장할 파일 경로
	 * @return 성공 여부
	 */
	bool SaveToBinary(const FString& FilePath) const;

	/**
	 * @brief 바이너리 파일에서 메시 데이터를 로드
	 * @param FilePath 로드할 파일 경로
	 * @return 성공 여부
	 */
	bool LoadFromBinary(const FString& FilePath);

	const TArray<UMaterialInterface*>& GetMaterialSlots() const { return MaterialSlots; }

	void SetMaterialSlots(const TArray<UMaterialInterface*>& InMaterialSlots);

	// DrawIndexedPrimitivesCommand에서 필요한 메서드들
	TArray<FStaticMeshSection>& GetMeshGroupInfo() { return StaticMeshData.Sections; } // Non-const version
	const TArray<FStaticMeshSection>& GetMeshGroupInfo() const { return StaticMeshData.Sections; }
	bool HasMaterial() const { return !MaterialSlots.IsEmpty(); }
	uint32 GetIndexCount() const { return static_cast<uint32>(StaticMeshData.Indices.Num()); }
	EVertexLayoutType GetVertexType() const;

	/**
	 * @brief 바이너리 캐시가 유효한지 확인
	 * @param ObjFilePath 원본 OBJ 파일 경로
	 * @return 바이너리 캐시 사용 가능 여부
	 */
	static bool IsBinaryCacheValid(const FString& ObjFilePath);

	/**
	 * @brief OBJ 파일로부터 바이너리 파일 경로 생성
	 * @param ObjFilePath 원본 OBJ 파일 경로
	 * @return 바이너리 파일 경로
	 */
	static FString GetBinaryFilePath(const FString& ObjFilePath);

protected:
	/** 실제 메시 데이터 */
	FStaticMesh StaticMeshData;

	TArray<UMaterialInterface*> MaterialSlots;

	/** 정점 버퍼 */
	ID3D11Buffer* VertexBuffer = nullptr;

	/** 인덱스 버퍼 */
	ID3D11Buffer* IndexBuffer = nullptr;
};

