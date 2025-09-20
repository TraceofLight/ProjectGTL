#include "pch.h"
#include "Runtime/Component/Public/MeshComponent.h"

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
	// 삼각형 토폴로지로 가정
	return NumVertices / 3;
}

void UMeshComponent::InitializeMeshRenderData()
{
	// 기본 구현 - 파생 클래스에서 재정의해야 함
}
