#pragma once
#include "Runtime/Component/Public/MeshComponent.h"
#include "Asset/Public/StaticMesh.h"

class UMaterialInterface;

/**
 * @brief UStaticMeshComponent: 스태틱 메시 렌더링을 위한 컴포넌트
 * @note 언리얼 엔진의 UStaticMeshComponent에 해당
 * UStaticMesh 애셋에 대한 참조를 보유하고 렌더링함
 */
UCLASS()
class UStaticMeshComponent :
	public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
	UStaticMeshComponent();

	/**
	 * @brief 렌더링할 스태틱 메시 애셋을 설정
	 * @param InStaticMesh 스태틱 메시 애셋에 대한 포인터
	 */
	void SetStaticMesh(UStaticMesh* InStaticMesh);

	/**
	 * @brief 현재 스태틱 메시 애셋을 가져옴
	 * @return 스태틱 메시 애셋에 대한 포인터, 설정되지 않은 경우 nullptr
	 */
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	// UMeshComponent로부터 재정의
	virtual bool HasValidMeshData() const override;
	virtual uint32 GetNumVertices() const override;
	virtual uint32 GetNumTriangles() const override;

	// 공통 렌더링 인터페이스 오버라이드
	virtual bool HasRenderData() const override;
	virtual ID3D11Buffer* GetRenderVertexBuffer() const override;
	virtual ID3D11Buffer* GetRenderIndexBuffer() const override;
	virtual uint32 GetRenderVertexCount() const override;
	virtual uint32 GetRenderIndexCount() const override;
	virtual uint32 GetRenderVertexStride() const override;
	virtual bool UseIndexedRendering() const override;
	virtual EShaderType GetShaderType() const override;

	// BoundingVolume 관련 기능
	/**
	 * @brief 스태틱 메시를 기반으로 AABB 계산
	 * @return 계산된 AABB
	 */
	FAABB GetAABB() const;

	// PrimitiveComponent 오버라이드
	virtual void GetWorldAABB(FVector& OutMin, FVector& OutMax) const override;

	// Material Override 관련 기능
	/**
	 * @brief 특정 슬롯의 머티리얼을 오버라이드
	 * @param SlotIndex 머티리얼 슬롯 인덱스
	 * @param Material 오버라이드할 머티리얼
	 */
	void SetMaterialOverride(int32 SlotIndex, UMaterialInterface* Material);

	/**
	 * @brief 특정 슬롯의 오버라이드 머티리얼을 가져옴
	 * @param SlotIndex 머티리얼 슬롯 인덱스
	 * @return 오버라이드된 머티리얼, 없으면 nullptr
	 */
	UMaterialInterface* GetMaterialOverride(int32 SlotIndex) const;

	/**
	 * @brief 특정 슬롯의 머티리얼 오버라이드를 제거
	 * @param SlotIndex 머티리얼 슬롯 인덱스
	 */
	void RemoveMaterialOverride(int32 SlotIndex);

	/**
	 * @brief 모든 머티리얼 오버라이드를 제거
	 */
	void ClearMaterialOverrides();

	/**
	 * @brief MaterialOverrideMap을 가져옴
	 * @return MaterialOverrideMap 참조
	 */
	const TMap<int32, UMaterialInterface*>& GetMaterialOverrideMap() const { return MaterialOverrideMap; }

protected:
	// UMeshComponent로부터 재정의
	virtual void InitializeMeshRenderData() override;

	/**
	 * @brief 스태틱 메시 데이터로부터 정점 및 인덱스 버퍼를 업데이트
	 */
	void UpdateRenderData();

private:
	/** 렌더링할 스태틱 메시 애셋 */
	UStaticMesh* StaticMesh = nullptr;

	/** 스태틱 메시 애셋 자체 머터리얼 대신 사용할 머티리얼 */
	TMap<int32, UMaterialInterface*> MaterialOverrideMap;
};
