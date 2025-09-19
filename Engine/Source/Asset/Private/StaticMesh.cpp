#include "pch.h"
#include "Asset/Public/StaticMesh.h"

IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
{
}

void UStaticMesh::SetStaticMeshData(const FStaticMesh& InStaticMeshData)
{
	StaticMeshData = InStaticMeshData;
}

bool UStaticMesh::IsValidMesh() const
{
	return !StaticMeshData.Vertices.empty() && !StaticMeshData.Indices.empty();
}
