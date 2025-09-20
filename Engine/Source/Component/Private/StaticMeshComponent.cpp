#include "pch.h"
#include "Component/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	// 프리미티브 타입을 StaticMesh로 설정
	Type = EPrimitiveType::StaticMesh;
}

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	StaticMesh = InStaticMesh;

	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		//UpdateRenderData();
		InitializeMeshRenderData();
		//UpdateMeshBounds();
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

	// 정점 데이터 포인터 업데이트
	Vertices = &StaticMesh->GetVertices();
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());
}

void UStaticMeshComponent::UpdateRenderData()
{
	if (!HasValidMeshData())
	{
		Vertices = nullptr;
		NumVertices = 0;
		return;
	}

	// 기본 렌더 데이터 업데이트
	//Vertices = &StaticMesh->GetVertices();
	//NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());
}

// 공통 렌더링 인터페이스 구현
bool UStaticMeshComponent::HasRenderData() const
{
	return HasValidMeshData();
}

ID3D11Buffer* UStaticMeshComponent::GetRenderVertexBuffer() const
{
	return StaticMesh ? StaticMesh->GetVertexBuffer() : nullptr;
}

ID3D11Buffer* UStaticMeshComponent::GetRenderIndexBuffer() const
{
	return StaticMesh ? StaticMesh->GetIndexBuffer() : nullptr;
}

uint32 UStaticMeshComponent::GetRenderVertexCount() const
{
	return GetNumVertices();
}

uint32 UStaticMeshComponent::GetRenderIndexCount() const
{
	if (StaticMesh)
	{
		return static_cast<uint32>(StaticMesh->GetIndices().size());
	}
	return 0;
}

uint32 UStaticMeshComponent::GetRenderVertexStride() const
{
	return sizeof(FVertex);
}

bool UStaticMeshComponent::UseIndexedRendering() const
{
	// StaticMesh는 인덱스 렌더링 사용
	return HasRenderData() && GetRenderIndexCount() > 0;
}

EShaderType UStaticMeshComponent::GetShaderType() const
{
	// StaticMesh는 전용 셰이더 사용
	return EShaderType::StaticMesh;
}
