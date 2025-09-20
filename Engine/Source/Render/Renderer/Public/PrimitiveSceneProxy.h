#pragma once
#include "Core/Public/Object.h"

class UPrimitiveComponent;

/**
 * @brief FPrimitiveSceneProxy: 프리미티브의 렌더링 데이터를 관리하는 프록시 클래스
 * @note 언리얼 엔진의 FPrimitiveSceneProxy에 해당
 * 렌더 스레드에서 안전하게 사용할 수 있도록 정점 및 인덱스 버퍼를 관리
 */
class FPrimitiveSceneProxy
{
public:
	/**
	 * @brief 생성자
	 * @param InComponent 프록시를 생성할 프리미티브 컴포넌트
	 */
	explicit FPrimitiveSceneProxy(const UPrimitiveComponent* InComponent);

	/**
	 * @brief 소멸자 - 버퍼 리소스 해제
	 */
	virtual ~FPrimitiveSceneProxy();

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
	 * @brief 정점 수를 가져옴
	 * @return 정점 개수
	 */
	uint32 GetVertexCount() const { return VertexCount; }

	/**
	 * @brief 인덱스 수를 가져옴
	 * @return 인덱스 개수
	 */
	uint32 GetIndexCount() const { return IndexCount; }

	/**
	 * @brief 정점 스트라이드를 가져옴
	 * @return 정점 스트라이드 값
	 */
	uint32 GetVertexStride() const { return VertexStride; }

	/**
	 * @brief 인덱스 스트라이드를 가져옴
	 * @return 인덱스 스트라이드 값
	 */
	uint32 GetIndexStride() const { return IndexStride; }

	/**
	 * @brief 유효한 렌더링 데이터를 가지고 있는지 확인
	 * @return 렌더링 가능한 상태이면 true
	 */
	bool IsValidForRendering() const;

protected:
	/**
	 * @brief 정점 버퍼를 생성
	 * @param InVertices 정점 데이터
	 * @param InVertexCount 정점 개수
	 */
	virtual void CreateVertexBuffer(const FVertex* InVertices, uint32 InVertexCount);

	/**
	 * @brief 인덱스 버퍼를 생성
	 * @param InIndices 인덱스 데이터
	 * @param InIndexCount 인덱스 개수
	 */
	virtual void CreateIndexBuffer(const uint32* InIndices, uint32 InIndexCount);

	/**
	 * @brief 버퍼 리소스를 해제
	 */
	virtual void ReleaseBuffers();

private:
	/** 정점 버퍼 */
	ID3D11Buffer* VertexBuffer = nullptr;

	/** 인덱스 버퍼 */
	ID3D11Buffer* IndexBuffer = nullptr;

	/** 정점 개수 */
	uint32 VertexCount = 0;

	/** 인덱스 개수 */
	uint32 IndexCount = 0;

	/** 정점 스트라이드 */
	uint32 VertexStride = sizeof(FVertex);

	/** 인덱스 스트라이드 */
	uint32 IndexStride = sizeof(uint32);
};