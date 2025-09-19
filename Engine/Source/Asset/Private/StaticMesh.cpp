#include "pch.h"
#include "Asset/Public/StaticMesh.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
{
}

UStaticMesh::~UStaticMesh()
{
	ReleaseRenderBuffers();
}

void UStaticMesh::SetStaticMeshData(const FStaticMesh& InStaticMeshData)
{
	ReleaseRenderBuffers();
	StaticMeshData = InStaticMeshData;
	CreateRenderBuffers();
	CalculateAABB();
}

bool UStaticMesh::IsValidMesh() const
{
	return !StaticMeshData.Vertices.empty() && !StaticMeshData.Indices.empty();
}

void UStaticMesh::CreateRenderBuffers()
{
	if (!IsValidMesh())
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();

	const TArray<FVertex>& Vertices = StaticMeshData.Vertices;
	const TArray<uint32>& Indices = StaticMeshData.Indices;

	if (!Vertices.empty())
	{
		const uint32 VertexBufferSize = static_cast<uint32>(Vertices.size()) * sizeof(FVertex);
		VertexBuffer = Renderer.CreateVertexBuffer(const_cast<FVertex*>(Vertices.data()), VertexBufferSize);
	}

	if (!Indices.empty())
	{
		const uint32 IndexBufferSize = static_cast<uint32>(Indices.size()) * sizeof(uint32);
		IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), IndexBufferSize);
	}
}

void UStaticMesh::ReleaseRenderBuffers()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}

	if (IndexBuffer)
	{
		IndexBuffer->Release();
		IndexBuffer = nullptr;
	}
}

/**
 * @brief 메시 데이터로부터 AABB를 계산하는 함수
 */
void UStaticMesh::CalculateAABB()
{
	if (StaticMeshData.Vertices.empty())
	{
		// 정점이 없으면 기본 AABB (0,0,0)으로 초기화
		LocalAABB = FAABB(FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
		return;
	}

	// 첫 번째 정점의 위치로 Min/Max 초기화
	FVector MinBounds = StaticMeshData.Vertices[0].Position;
	FVector MaxBounds = StaticMeshData.Vertices[0].Position;

	// 모든 정점을 순회하며 최소/최대 값 찾기
	for (const FVertex& Vertex : StaticMeshData.Vertices)
	{
		const FVector& Position = Vertex.Position;

		// X 좌표 비교
		if (Position.X < MinBounds.X) MinBounds.X = Position.X;
		if (Position.X > MaxBounds.X) MaxBounds.X = Position.X;

		// Y 좌표 비교
		if (Position.Y < MinBounds.Y) MinBounds.Y = Position.Y;
		if (Position.Y > MaxBounds.Y) MaxBounds.Y = Position.Y;

		// Z 좌표 비교
		if (Position.Z < MinBounds.Z) MinBounds.Z = Position.Z;
		if (Position.Z > MaxBounds.Z) MaxBounds.Z = Position.Z;
	}

	// 계산된 Min/Max로 AABB 설정
	LocalAABB = FAABB(MinBounds, MaxBounds);

	UE_LOG("StaticMesh AABB 계산 완료: Min(%.2f, %.2f, %.2f), Max(%.2f, %.2f, %.2f)",
		MinBounds.X, MinBounds.Y, MinBounds.Z,
		MaxBounds.X, MaxBounds.Y, MaxBounds.Z);
}
