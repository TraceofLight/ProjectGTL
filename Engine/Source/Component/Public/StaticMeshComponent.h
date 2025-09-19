#pragma once
#include "Component/Public/MeshComponent.h"
#include "Asset/Public/StaticMesh.h"

/**
 * @brief UStaticMeshComponent: 스태틱 메시 렌더링을 위한 컴포넌트
 * @note 언리얼 엔진의 UStaticMeshComponent에 해당
 * UStaticMesh 애셋에 대한 참조를 보유하고 렌더링함
 */
UCLASS()
class UStaticMeshComponent : public UMeshComponent
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

protected:
	// UMeshComponent로부터 재정의
	virtual void InitializeMeshRenderData() override;
	virtual void UpdateMeshBounds() override;

	/**
	 * @brief 스태틱 메시 데이터로부터 정점 및 인덱스 버퍼를 업데이트
	 */
	void UpdateRenderData();

private:
	/** 렌더링할 스태틱 메시 애셋 */
	UStaticMesh* StaticMesh = nullptr;
};