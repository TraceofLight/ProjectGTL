#pragma once
#include "Global/Types.h"
#include "Global/CoreTypes.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/BoundingBoxLines.h"

struct FVertex;
class AStaticMeshActor;

class UBatchLines : UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UBatchLines, UObject)

public:
	UBatchLines();
	~UBatchLines();

	// 종류별 Vertices 업데이트
	void UpdateUGridVertices(const float newCellSize);
	void UpdateBoundingBoxVertices(const FAABB& newBoundingBoxInfo);

	// 전체 업데이트
	void UpdateBatchLineVertices(const float newCellSize, const FAABB& newBoundingBoxInfo);

	// GPU VertexBuffer에 복사
	void UpdateVertexBuffer();

	float GetCellSize() const
	{
		return Grid.GetCellSize();
	}

	void DisableRenderBoundingBox()
	{
		UpdateBoundingBoxVertices({ {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
	}

	void Render();

private:
	void SetIndices();
	void RebuildIndicesWithStaticMeshAABBs();

	bool bChangedVertices = false;

	TArray<FVector> Vertices; // 그리드 라인 정보 + (offset 후)디폴트 바운딩 박스 라인 정보(minx, miny가 0,0에 정의된 크기가 1인 cube)
	TArray<uint32> Indices; // 월드 그리드는 그냥 정점 순서, 바운딩 박스는 실제 인덱싱

	FEditorPrimitive Primitive;

	UGrid Grid;
	UBoundingBoxLines BoundingBoxLines;

	// StaticMeshActor들의 AABB 관리
	TArray<UBoundingBoxLines> StaticMeshAABBLines;
	uint32 StaticMeshAABBVertexOffset;

	bool bRenderBox;
};
