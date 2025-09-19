#include "pch.h"
#include "Render/Renderer/Public/PrimitiveSceneProxy.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Public/PrimitiveComponent.h"

FPrimitiveSceneProxy::FPrimitiveSceneProxy(const UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	const TArray<FVertex>* Vertices = InComponent->GetVerticesData();
	if (Vertices && !Vertices->empty())
	{
		CreateVertexBuffer(Vertices->data(), static_cast<uint32>(Vertices->size()));
	}
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy()
{
	ReleaseBuffers();
}

bool FPrimitiveSceneProxy::IsValidForRendering() const
{
	return VertexBuffer != nullptr && VertexCount > 0;
}

void FPrimitiveSceneProxy::CreateVertexBuffer(const FVertex* InVertices, uint32 InVertexCount)
{
	if (!InVertices || InVertexCount == 0)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	const uint32 ByteWidth = InVertexCount * VertexStride;

	VertexBuffer = Renderer.CreateVertexBuffer(const_cast<FVertex*>(InVertices), ByteWidth);
	VertexCount = InVertexCount;
}

void FPrimitiveSceneProxy::CreateIndexBuffer(const uint32* InIndices, uint32 InIndexCount)
{
	if (!InIndices || InIndexCount == 0)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	const uint32 ByteWidth = InIndexCount * IndexStride;

	IndexBuffer = Renderer.CreateIndexBuffer(InIndices, ByteWidth);
	IndexCount = InIndexCount;
}

void FPrimitiveSceneProxy::ReleaseBuffers()
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

	VertexCount = 0;
	IndexCount = 0;
}
