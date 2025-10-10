#include "pch.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

#include "Physics/Public/AABB.h"
#include "Runtime/Subsystem/Partition/Public/WorldPartitionSubsystem.h"

IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;

	//GetWorldPartitionSubsystem()->Register(this);
}

UPrimitiveComponent::~UPrimitiveComponent()
{
	//GetWorldPartitionSubsystem()->Unregister(this);
}

void UPrimitiveComponent::OnTransformChanged()
{
	USceneComponent::OnTransformChanged();

	//GetWorldPartitionSubsystem()->Update(this);
}

const TArray<FVertex>* UPrimitiveComponent::GetVerticesData() const
{
	return Vertices;
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return Vertexbuffer;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

// 공통 렌더링 인터페이스 기본 구현
bool UPrimitiveComponent::HasRenderData() const
{
	return GetVerticesData() != nullptr && !GetVerticesData()->IsEmpty();
}

ID3D11Buffer* UPrimitiveComponent::GetRenderVertexBuffer() const
{
	return GetVertexBuffer();
}

ID3D11Buffer* UPrimitiveComponent::GetRenderIndexBuffer() const
{
	// 기본 프리미티브는 인덱스 버퍼를 사용하지 않음
	return nullptr;
}

uint32 UPrimitiveComponent::GetRenderVertexCount() const
{
	const TArray<FVertex>* VerticesData = GetVerticesData();
	return VerticesData ? static_cast<uint32>(VerticesData->Num()) : 0;
}

uint32 UPrimitiveComponent::GetRenderIndexCount() const
{
	// 기본 프리미티브는 인덱스 버퍼를 사용하지 않음
	return 0;
}

uint32 UPrimitiveComponent::GetRenderVertexStride() const
{
	return sizeof(FVertex);
}

bool UPrimitiveComponent::UseIndexedRendering() const
{
	// 기본 프리미티브는 인덱스 렌더링을 사용하지 않음
	return false;
}

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax) const
{
	// 기본 구현: AABB가 없으면 빈 박스 반환
	if (BoundingVolume && BoundingVolume->GetType() == EBoundingVolumeType::AABB)
	{
		FAABB* aabb = static_cast<FAABB*>(BoundingVolume);
		OutMin = aabb->Min;
		OutMax = aabb->Max;
	}
	else
	{
		OutMin = FVector(0.0f, 0.0f, 0.0f);
		OutMax = FVector(0.0f, 0.0f, 0.0f);
	}
}

FMatrix UPrimitiveComponent::GetWorldMatrix() const
{
	// USceneComponent에서 상속하므로 GetWorldTransformMatrix() 사용
	return GetWorldTransformMatrix();
}

/*
* 리소스는 Manager가 관리하고 component는 참조만 함.
*
*/
