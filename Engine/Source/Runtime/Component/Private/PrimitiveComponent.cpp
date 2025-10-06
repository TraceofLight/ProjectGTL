#include "pch.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	ComponentType = EComponentType::Primitive;
}

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
}
void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	FVector ActualScale = Scale;
	if (ActualScale.X < MinScale)
		ActualScale.X = MinScale;
	if (ActualScale.Y < MinScale)
		ActualScale.Y = MinScale;
	if (ActualScale.Z < MinScale)
		ActualScale.Z = MinScale;
	RelativeScale3D = ActualScale;
	MarkAsDirty();
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}
const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}
const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FMatrix& USceneComponent::GetWorldTransformMatrix() const
{
	if (bIsTransformDirty)
	{
		WorldTransformMatrix = FMatrix::GetModelMatrix(GetRelativeLocation(), FVector::GetDegreeToRadian(GetRelativeRotation()), GetRelativeScale3D());

		// 부모 컴포넌트가 있는 경우 부모의 변환도 적용
		if (ParentAttachment)
		{
			FMatrix ParentTransform = ParentAttachment->GetWorldTransformMatrix();
			WorldTransformMatrix = ParentTransform * WorldTransformMatrix;
		}

		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USceneComponent::GetWorldTransformMatrixInverse() const
{

	if (bIsTransformDirtyInverse)
	{
		WorldTransformMatrixInverse = FMatrix::Identity();
		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrixInverse = FMatrix::GetModelMatrixInverse(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D) * WorldTransformMatrixInverse;

		}
		WorldTransformMatrixInverse = WorldTransformMatrixInverse * FMatrix::GetModelMatrixInverse(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);

		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
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
