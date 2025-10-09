#include "pch.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Material/Public/Material.h"

IMPLEMENT_CLASS(UStaticMeshComponent, UMeshComponent)

UStaticMeshComponent::UStaticMeshComponent()
{
	// 프리미티브 타입을 StaticMesh로 설정
	Type = EPrimitiveType::StaticMeshComp;
	UStaticMeshComponent::GetClass()->IncrementGenNumber();
}

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	StaticMesh = InStaticMesh;

	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		UE_LOG("StaticMeshComponent::SetStaticMesh - Setting mesh: %s", 
		       StaticMesh->GetAssetPathFileName().c_str());
		UE_LOG("  VertexBuffer: %p, IndexBuffer: %p", 
		       StaticMesh->GetVertexBuffer(), StaticMesh->GetIndexBuffer());
		InitializeMeshRenderData();
		MaterialOverrideMap.Empty(); // 스태틱메시 바꿀었으므로 머티리얼 오버라이드 맵 초기화
	}
	else
	{
		UE_LOG_WARNING("StaticMeshComponent::SetStaticMesh - Invalid or null mesh!");
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
		return static_cast<uint32>(StaticMesh->GetVertices().Num());
	}
	return 0;
}

uint32 UStaticMeshComponent::GetNumTriangles() const
{
	if (StaticMesh)
	{
		return static_cast<uint32>(StaticMesh->GetIndices().Num() / 3);
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
	NumVertices = static_cast<uint32>(StaticMesh->GetVertices().Num());
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
	//NumVertices = static_cast<uint32>(StaticMesh->GetVertices().Num());
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
		return static_cast<uint32>(StaticMesh->GetIndices().Num());
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

FAABB UStaticMeshComponent::GetAABB() const
{
	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		return StaticMesh->CalculateAABB();
	}
	return FAABB();
}

void UStaticMeshComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax) const
{
	if (StaticMesh && StaticMesh->IsValidMesh())
	{
		FAABB LocalAABB = StaticMesh->CalculateAABB();

		// 월드 변환 행렬 적용
		const FMatrix& WorldTransform = GetWorldTransformMatrix();

		// AABB의 8개 꼭짓점을 모두 변환하여 새로운 AABB 계산
		FVector Corners[8] = {
			FVector(LocalAABB.Min.X, LocalAABB.Min.Y, LocalAABB.Min.Z),
			FVector(LocalAABB.Max.X, LocalAABB.Min.Y, LocalAABB.Min.Z),
			FVector(LocalAABB.Min.X, LocalAABB.Max.Y, LocalAABB.Min.Z),
			FVector(LocalAABB.Max.X, LocalAABB.Max.Y, LocalAABB.Min.Z),
			FVector(LocalAABB.Min.X, LocalAABB.Min.Y, LocalAABB.Max.Z),
			FVector(LocalAABB.Max.X, LocalAABB.Min.Y, LocalAABB.Max.Z),
			FVector(LocalAABB.Min.X, LocalAABB.Max.Y, LocalAABB.Max.Z),
			FVector(LocalAABB.Max.X, LocalAABB.Max.Y, LocalAABB.Max.Z)
		};

		// 첫 번째 꼭짓점으로 초기화
		FVector TransformedCorner = FMatrix::VectorMultiply(Corners[0], WorldTransform);
		OutMin = OutMax = TransformedCorner;

		// 나머지 꼭짓점들로 Min/Max 갱신
		for (int32 i = 1; i < 8; ++i)
		{
			TransformedCorner = FMatrix::VectorMultiply(Corners[i], WorldTransform);

			OutMin.X = std::min(OutMin.X, TransformedCorner.X);
			OutMin.Y = std::min(OutMin.Y, TransformedCorner.Y);
			OutMin.Z = std::min(OutMin.Z, TransformedCorner.Z);

			OutMax.X = std::max(OutMax.X, TransformedCorner.X);
			OutMax.Y = std::max(OutMax.Y, TransformedCorner.Y);
			OutMax.Z = std::max(OutMax.Z, TransformedCorner.Z);
		}
	}

    else
	{
		// 유효한 메시가 없으면 빈 박스 반환
		OutMin = FVector(0.0f, 0.0f, 0.0f);
		OutMax = FVector(0.0f, 0.0f, 0.0f);
	}
}

// Material Override 관련 메서드 구현
void UStaticMeshComponent::SetMaterialOverride(int32 SlotIndex, UMaterialInterface* Material)
{
	if (SlotIndex < 0)
	{
		return;
	}

	if (Material)
	{
		MaterialOverrideMap[SlotIndex] = Material;
	}
	else
	{
		// nullptr이 전달되면 오버라이드 제거
		RemoveMaterialOverride(SlotIndex);
	}
}

UMaterialInterface* UStaticMeshComponent::GetMaterialOverride(int32 SlotIndex) const
{
	UMaterialInterface* const* Found = MaterialOverrideMap.Find(SlotIndex);
	if (Found)
	{
		return *Found;
	}
	return nullptr;
}

void UStaticMeshComponent::RemoveMaterialOverride(int32 SlotIndex)
{
	MaterialOverrideMap.Remove(SlotIndex);
}

void UStaticMeshComponent::ClearMaterialOverrides()
{
	MaterialOverrideMap.Empty();
}

const TArray<UMaterialInterface*>& UStaticMeshComponent::GetMaterailSlots() const
{
	// StaticMesh의 MaterialSlots를 반환
	// 버그: 실제로는 MaterialOverride도 고려해야 함
	if (StaticMesh)
	{
		return StaticMesh->GetMaterialSlots();
	}
	
	// 빈 배열 반환
	static TArray<UMaterialInterface*> EmptyMaterials;
	return EmptyMaterials;
}
