#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Actor/Public/StaticMeshActor.h"

IMPLEMENT_CLASS(UBatchLines, UObject)

UBatchLines::UBatchLines()
	: Grid()
	, BoundingBoxLines()
	, StaticMeshAABBVertexOffset(0)
{
	StaticMeshAABBVertexOffset = Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices();

	Vertices.reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.resize(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	URenderer& Renderer = URenderer::GetInstance();

	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.NumIndices = static_cast<uint32>(Indices.size());
	Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), Primitive.NumIndices * sizeof(uint32));
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		Vertices.data(), Primitive.NumVertices * sizeof(FVector), true);

	Primitive.VertexShader = UAssetManager::GetInstance().GetVertexShader(EShaderType::BatchLine);
	Primitive.InputLayout = UAssetManager::GetInstance().GetInputLayout(EShaderType::BatchLine);
	Primitive.PixelShader = UAssetManager::GetInstance().GetPixelShader(EShaderType::BatchLine);
}

UBatchLines::~UBatchLines()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
	Primitive.InputLayout->Release();
	Primitive.VertexShader->Release();
	Primitive.IndexBuffer->Release();
	Primitive.PixelShader->Release();
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize())
	{
		return;
	}
	Grid.UpdateVerticesBy(newCellSize);
	Grid.MergeVerticesAt(Vertices, 0);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const FAABB& newBoundingBoxInfo)
{
	FAABB curBoudingBoxInfo = BoundingBoxLines.GetRenderedBoxInfo();
	if (newBoundingBoxInfo.Min == curBoudingBoxInfo.Min && newBoundingBoxInfo.Max == curBoudingBoxInfo.Max)
	{
		return;
	}

	BoundingBoxLines.UpdateVertices(newBoundingBoxInfo);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
	bChangedVertices = true;
}

void UBatchLines::UpdateBatchLineVertices(const float newCellSize, const FAABB& newBoundingBoxInfo)
{
	UpdateUGridVertices(newCellSize);
	UpdateBoundingBoxVertices(newBoundingBoxInfo);
	bChangedVertices = true;
}

void UBatchLines::UpdateVertexBuffer()
{
	if (bChangedVertices)
	{
		URenderer::GetInstance().UpdateVertexBuffer(Primitive.Vertexbuffer, Vertices);
	}
	bChangedVertices = false;
}

void UBatchLines::Render()
{
	URenderer& Renderer = URenderer::GetInstance();

	// to do: 아래 함수를 batch에 맞게 수정해야 함.
	Renderer.RenderPrimitiveIndexed(Primitive, Primitive.RenderState, false, sizeof(FVector), sizeof(uint32));
}

void UBatchLines::SetIndices()
{
	const uint32 numGridVertices = Grid.GetNumVertices();

	// 기존 그리드 라인 인덱스
	for (uint32 index = 0; index < numGridVertices; ++index)
	{
		Indices.push_back(index);
	}

	// Bounding Box 라인 인덱스 (LineList)
	uint32 boundingBoxLineIdx[] = {
		// 앞면
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// 뒷면
		4, 5,
		5, 6,
		6, 7,
		7, 4,

		// 옆면 연결
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	// numGridVertices 이후에 추가된 8개의 꼭짓점에 맞춰 오프셋 적용
	for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
	{
		Indices.push_back(numGridVertices + boundingBoxLineIdx[i]);
	}
}

/**
 * @brief StaticMeshActor들의 AABB를 업데이트하는 함수
 * @param InStaticMeshActors 렌더링할 StaticMeshActor 배열
 */
void UBatchLines::UpdateStaticMeshActorAABBs(const TArray<AStaticMeshActor*>& InStaticMeshActors)
{
	// 기존 StaticMesh AABB 라인들 클리어
	StaticMeshAABBLines.clear();

	// 각 StaticMeshActor의 AABB를 BoundingBoxLines로 변환
	for (AStaticMeshActor* StaticMeshActor : InStaticMeshActors)
	{
		if (!StaticMeshActor)
		{
			continue;
		}

		// 월드 좌표계 AABB 가져오기
		FAABB WorldAABB = StaticMeshActor->GetWorldAABB();

		// 새로운 BoundingBoxLines 생성
		UBoundingBoxLines NewAABBLines;
		NewAABBLines.UpdateVertices(WorldAABB);

		StaticMeshAABBLines.push_back(NewAABBLines);
	}

	// 전체 정점 배열 크기 재계산
	uint32 TotalAABBVertices = StaticMeshAABBLines.size() * 8; // 각 AABB당 8개 정점
	uint32 TotalVertices = Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices() + TotalAABBVertices;

	// 정점 배열 리사이즈
	Vertices.resize(TotalVertices);

	// 기존 그리드와 기본 바운딩 박스 유지
	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	// StaticMesh AABB들 추가
	uint32 CurrentOffset = StaticMeshAABBVertexOffset;
	for (const auto& AABBLines : StaticMeshAABBLines)
	{
		const_cast<UBoundingBoxLines&>(AABBLines).MergeVerticesAt(Vertices, CurrentOffset);
		CurrentOffset += 8; // 각 AABB는 8개 정점
	}

	// 인덱스 재구성 (기존 + 새로운 AABB들)
	RebuildIndicesWithStaticMeshAABBs();

	// 변경 플래그 설정
	bChangedVertices = true;
}

/**
 * @brief StaticMesh AABB들을 포함한 인덱스 재구성
 */
void UBatchLines::RebuildIndicesWithStaticMeshAABBs()
{
	// 기존 인덱스 클리어하고 재구성
	Indices.clear();

	uint32 gridVerticesNum = Grid.GetNumVertices();

	// 그리드 인덱스 (기존 로직)
	for (uint32 i = 0; i < gridVerticesNum; i++)
	{
		Indices.push_back(i);
	}

	// 기본 바운딩 박스 인덱스 (기존 로직)
	constexpr uint32 boundingBoxLineIdx[] = {
		0, 1, 1, 2, 2, 3, 3, 0, // 앞면
		4, 5, 5, 6, 6, 7, 7, 4, // 뒷면
		0, 4, 1, 5, 2, 6, 3, 7  // 연결
	};

	uint32 boundingBoxOffset = gridVerticesNum;
	for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
	{
		Indices.push_back(boundingBoxOffset + boundingBoxLineIdx[i]);
	}

	// StaticMesh AABB 인덱스들 추가
	uint32 staticMeshOffset = StaticMeshAABBVertexOffset;
	for (size_t aabbIndex = 0; aabbIndex < StaticMeshAABBLines.size(); ++aabbIndex)
	{
		for (uint32 i = 0; i < std::size(boundingBoxLineIdx); ++i)
		{
			Indices.push_back(staticMeshOffset + boundingBoxLineIdx[i]);
		}
		staticMeshOffset += 8; // 다음 AABB로 이동
	}
}
