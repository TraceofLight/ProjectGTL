#include "pch.h"
#include "Component/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	// Set primitive type to StaticMesh
	Type = EPrimitiveType::StaticMesh;
}

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	StaticMesh = InStaticMesh;

	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		UpdateRenderData();
		InitializeMeshRenderData();
		UpdateMeshBounds();
	}
}

bool UStaticMeshComponent::HasValidMeshData() const
{
	return StaticMesh != nullptr && StaticMesh->IsValidMesh();
}

uint32 UStaticMeshComponent::GetNumVertices() const
{
	if (StaticMesh)
	{
		return static_cast<uint32>(StaticMesh->GetVertices().size());
	}
	return 0;
}

uint32 UStaticMeshComponent::GetNumTriangles() const
{
	if (StaticMesh)
	{
		return static_cast<uint32>(StaticMesh->GetIndices().size() / 3);
	}
	return 0;
}

void UStaticMeshComponent::InitializeMeshRenderData()
{
	if (!HasValidMeshData())
	{
		return;
	}

	// Update vertex data pointer
	Vertices = &StaticMesh->GetVertices();
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());

	// TODO: Create vertex buffer for GPU rendering
	// This will be implemented when integrating with the rendering system
}

void UStaticMeshComponent::UpdateMeshBounds()
{
	if (!HasValidMeshData())
	{
		return;
	}

	// TODO: Calculate bounding box from vertex data
	// This will be implemented when integrating with the physics/culling system
}

void UStaticMeshComponent::UpdateRenderData()
{
	if (!HasValidMeshData())
	{
		Vertices = nullptr;
		NumVertices = 0;
		return;
	}

	// Update basic render data
	Vertices = &StaticMesh->GetVertices();
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());
}
