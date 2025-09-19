#include "pch.h"
#include "Render/Renderer/Public/StaticMeshSceneProxy.h"
#include "Component/Public/StaticMeshComponent.h"
#include "Asset/Public/StaticMesh.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(const UStaticMeshComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	if (!InComponent)
	{
		return;
	}

	UStaticMesh* StaticMesh = InComponent->GetStaticMesh();
	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		InitializeFromStaticMesh(StaticMesh);
	}
}

void FStaticMeshSceneProxy::InitializeFromStaticMesh(const UStaticMesh* InStaticMesh)
{
	if (!InStaticMesh)
	{
		return;
	}

	const TArray<FVertex>& Vertices = InStaticMesh->GetVertices();
	const TArray<uint32>& Indices = InStaticMesh->GetIndices();

	if (!Vertices.empty())
	{
		CreateVertexBuffer(Vertices.data(), static_cast<uint32>(Vertices.size()));
	}

	if (!Indices.empty())
	{
		CreateIndexBuffer(Indices.data(), static_cast<uint32>(Indices.size()));
	}
}