#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Render/Renderer/Public/PrimitiveSceneProxy.h"

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
		WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);


		for (USceneComponent* Ancester = ParentAttachment; Ancester && Ancester->ParentAttachment; Ancester = Ancester->ParentAttachment)
		{
			WorldTransformMatrix *= FMatrix::GetModelMatrix(Ancester->RelativeLocation, FVector::GetDegreeToRadian(Ancester->RelativeRotation), Ancester->RelativeScale3D);
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

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax) const
{
	if (!BoundingBox)	return;

	if (BoundingBox->GetType() == EBoundingVolumeType::AABB)
	{
		const FAABB* LocalAABB = static_cast<const FAABB*>(BoundingBox);
		FVector LocalCorners[8] =
		{
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Min.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Max.Z),
			FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Max.Z)
		};
		const FMatrix& WorldTransform = GetWorldTransformMatrix();
		FVector WorldMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
		FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int i = 0; i < 8; i++)
		{
			FVector4 WorldCorner = FVector4(LocalCorners[i].X, LocalCorners[i].Y, LocalCorners[i].Z, 1.0f) * WorldTransform;
			WorldMin.X = std::min(WorldMin.X, WorldCorner.X);
			WorldMin.Y = std::min(WorldMin.Y, WorldCorner.Y);
			WorldMin.Z = std::min(WorldMin.Z, WorldCorner.Z);
			WorldMax.X = std::max(WorldMax.X, WorldCorner.X);
			WorldMax.Y = std::max(WorldMax.Y, WorldCorner.Y);
			WorldMax.Z = std::max(WorldMax.Z, WorldCorner.Z);
		}
		OutMin = WorldMin;
		OutMax = WorldMax;
	}
}

// 공통 렌더링 인터페이스 기본 구현
bool UPrimitiveComponent::HasRenderData() const
{
	return GetVerticesData() != nullptr && !GetVerticesData()->empty();
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
	return VerticesData ? static_cast<uint32>(VerticesData->size()) : 0;
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

EShaderType UPrimitiveComponent::GetShaderType() const
{
	// 기본 프리미티브는 기본 셰이더 사용
	return EShaderType::Default;
}

FPrimitiveSceneProxy* UPrimitiveComponent::CreateSceneProxy() const
{
	// 기본 프리미티브는 기본 FPrimitiveSceneProxy 사용
	return new FPrimitiveSceneProxy(this);
}

/*
* 리소스는 Manager가 관리하고 component는 참조만 함.
*
*/
