#pragma once
#include "Component/Public/PrimitiveComponent.h"

/**
 * @brief UMeshComponent: 모든 메시 기반 컴포넌트의 기본 클래스
 * @note 언리얼 엔진의 UMeshComponent에 해당
 * 메시를 렌더링하는 컴포넌트의 공통 기능을 제공
 */
UCLASS()
class UMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)

public:
	UMeshComponent();

	/**
	 * @brief 컴포넌트가 렌더링을 위한 유효한 메시 데이터를 가지고 있는지 확인
	 * @return 메시 데이터가 사용 가능하고 유효하면 true
	 */
	virtual bool HasValidMeshData() const;

	/**
	 * @brief 메시의 정점 수를 가져옴
	 * @return 정점 수
	 */
	virtual uint32 GetNumVertices() const;

	/**
	 * @brief 메시의 삼각형 수를 가져옴
	 * @return 삼각형 수
	 */
	virtual uint32 GetNumTriangles() const;

protected:
	/**
	 * @brief 메시 관련 렌더링 데이터를 초기화
	 * 메시 데이터가 변경될 때 호출됨
	 */
	virtual void InitializeMeshRenderData();
};
