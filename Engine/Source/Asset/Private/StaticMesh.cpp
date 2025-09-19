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
