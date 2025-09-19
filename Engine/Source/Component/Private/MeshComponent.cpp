#include "pch.h"
#include "Component/Public/MeshComponent.h"

IMPLEMENT_CLASS(UMeshComponent, UPrimitiveComponent)

UMeshComponent::UMeshComponent()
{
}

bool UMeshComponent::HasValidMeshData() const
{
	return Vertices != nullptr && NumVertices > 0;
}

uint32 UMeshComponent::GetNumVertices() const
{
	return NumVertices;
}

uint32 UMeshComponent::GetNumTriangles() const
{
	// Assuming triangle topology
	return NumVertices / 3;
}

void UMeshComponent::InitializeMeshRenderData()
{
	// Base implementation - derived classes should override
}

void UMeshComponent::UpdateMeshBounds()
{
	// Base implementation - derived classes should override
}
