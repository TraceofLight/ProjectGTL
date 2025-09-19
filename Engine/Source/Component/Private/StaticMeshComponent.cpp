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

	// 정점 데이터 포인터 업데이트
	Vertices = &StaticMesh->GetVertices();
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());

	// TODO: GPU 렌더링을 위한 정점 버퍼 생성
	// 렌더링 시스템과 통합 시 구현될 예정
}

void UStaticMeshComponent::UpdateMeshBounds()
{
	if (!HasValidMeshData())
	{
		return;
	}

	// TODO: 정점 데이터로부터 바운딩 박스 계산
	// 물리/컬링 시스템과 통합 시 구현될 예정
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
	Vertices = &StaticMesh->GetVertices();
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().size());
}
