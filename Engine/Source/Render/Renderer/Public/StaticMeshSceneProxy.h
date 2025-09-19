#pragma once
#include "Render/Renderer/Public/PrimitiveSceneProxy.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * @brief FStaticMeshSceneProxy: 스태틱 메시를 위한 특화된 씬 프록시
 */
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	/**
	 * @brief 생성자
	 * @param InComponent 스태틱 메시 컴포넌트
	 */
	explicit FStaticMeshSceneProxy(const UStaticMeshComponent* InComponent);

private:
	/**
	 * @brief 스태틱 메시 데이터로부터 버퍼를 생성
	 * @param InStaticMesh 스태틱 메시 애셋
	 */
	void InitializeFromStaticMesh(const UStaticMesh* InStaticMesh);
};
