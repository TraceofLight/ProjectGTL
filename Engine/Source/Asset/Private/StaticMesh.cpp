#include "pch.h"
#include "Asset/Public/StaticMesh.h"
#include "Asset/Public/StaticMeshData.h"
#include "Runtime/Core/Public/ObjectIterator.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Utility/Public/Archive.h"

IMPLEMENT_CLASS(UStaticMesh, UObject)

UStaticMesh::UStaticMesh()
{
}

UStaticMesh::~UStaticMesh()
{
	ReleaseRenderBuffers();
}

void UStaticMesh::SetStaticMeshData(const FStaticMesh& InStaticMeshData)
{
	ReleaseRenderBuffers();
	StaticMeshData = InStaticMeshData;
	CreateRenderBuffers();
}

bool UStaticMesh::IsValidMesh() const
{
	return !StaticMeshData.Vertices.IsEmpty() && !StaticMeshData.Indices.IsEmpty();
}

void UStaticMesh::CreateRenderBuffers()
{
	if (!IsValidMesh())
	{
		UE_LOG_WARNING("StaticMesh::CreateRenderBuffers - Invalid mesh data");
		return;
	}

	// GDynamicRHI를 통해 버퍼 생성
	extern FRHIDevice* GDynamicRHI;
	if (!GDynamicRHI)
	{
		UE_LOG_ERROR("StaticMesh::CreateRenderBuffers - GDynamicRHI is null!");
		return;
	}

	const TArray<FVertex>& Vertices = StaticMeshData.Vertices;
	const TArray<uint32>& Indices = StaticMeshData.Indices;

	UE_LOG("StaticMesh::CreateRenderBuffers - %s: Vertices=%d, Indices=%d", 
	       StaticMeshData.PathFileName.c_str(), Vertices.Num(), Indices.Num());

	if (!Vertices.IsEmpty())
	{
		const uint32 VertexBufferSize = static_cast<uint32>(Vertices.Num()) * sizeof(FVertex);
		VertexBuffer = GDynamicRHI->CreateVertexBuffer(Vertices.GetData(), VertexBufferSize);
		if (VertexBuffer)
		{
			UE_LOG_SUCCESS("VertexBuffer created: %p", VertexBuffer);
		}
		else
		{
			UE_LOG_ERROR("Failed to create VertexBuffer!");
		}
	}

	if (!Indices.IsEmpty())
	{
		const uint32 IndexBufferSize = static_cast<uint32>(Indices.Num()) * sizeof(uint32);
		IndexBuffer = GDynamicRHI->CreateIndexBuffer(Indices.GetData(), IndexBufferSize);
		if (IndexBuffer)
		{
			UE_LOG_SUCCESS("IndexBuffer created: %p", IndexBuffer);
		}
		else
		{
			UE_LOG_ERROR("Failed to create IndexBuffer!");
		}
	}
}
void UStaticMesh::ReleaseRenderBuffers()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		VertexBuffer = nullptr;
	}

	if (IndexBuffer)
	{
		IndexBuffer->Release();
		IndexBuffer = nullptr;
	}
}

FAABB UStaticMesh::CalculateAABB() const
{
	if (StaticMeshData.Vertices.IsEmpty())
	{
		return FAABB();
	}

	FVector Min = StaticMeshData.Vertices[0].Position;
	FVector Max = StaticMeshData.Vertices[0].Position;

	for (const FVertex& Vertex : StaticMeshData.Vertices)
	{
		Min.X = std::min(Min.X, Vertex.Position.X);
		Min.Y = std::min(Min.Y, Vertex.Position.Y);
		Min.Z = std::min(Min.Z, Vertex.Position.Z);

		Max.X = std::max(Max.X, Vertex.Position.X);
		Max.Y = std::max(Max.Y, Vertex.Position.Y);
		Max.Z = std::max(Max.Z, Vertex.Position.Z);
	}

	return FAABB(Min, Max);
}

/**
 * @brief 메시 데이터를 바이너리 파일로 저장
 */
bool UStaticMesh::SaveToBinary(const FString& FilePath) const
{
	FBinaryWriter Writer(FilePath);
	if (!Writer.IsOpen())
	{
		UE_LOG("UStaticMesh: Failed to open file for writing: %s", FilePath.c_str());
		return false;
	}

	try
	{
		// 헤더 정보
		FString MagicNumber = "MESH";
		uint32 Version = 1;
		Writer << MagicNumber;
		Writer << Version;

		// StaticMeshData -> PathFileName 저장
		FString PathFileName = StaticMeshData.PathFileName;
		Writer << PathFileName;

		// StaticMeshData -> Vertices 저장
		uint32 VertexCount = static_cast<uint32>(StaticMeshData.Vertices.Num());
		Writer << VertexCount;
		for (FVertex Vertex : StaticMeshData.Vertices)
		{
			Writer << Vertex.Position;
			Writer << Vertex.Color;
			Writer << Vertex.TextureCoord;
			Writer << Vertex.Normal;
		}

		// StaticMeshData -> Indices 저장
		uint32 IndexCount = static_cast<uint32>(StaticMeshData.Indices.Num());
		Writer << IndexCount;
		for (uint32 Index : StaticMeshData.Indices)
		{
			Writer << Index;
		}

		// StaticMeshData -> Sections 저장
		uint32 SectionCount = static_cast<uint32>(StaticMeshData.Sections.Num());
		Writer << SectionCount;
		for (const FStaticMeshSection& Section : StaticMeshData.Sections)
		{
			Writer << (int32&)Section.StartIndex;
			Writer << (int32&)Section.IndexCount;
			Writer << (int32&)Section.MaterialSlotIndex;
			Writer << (FString&)Section.MaterialName;
		}

		// MaterialSlots 저장
		uint32 MaterialSlotCount = static_cast<uint32>(MaterialSlots.Num());
		Writer << MaterialSlotCount;
		for (UMaterialInterface* MaterialInterface : MaterialSlots)
		{
			UMaterial* Material = Cast<UMaterial>(MaterialInterface);
			const FObjMaterialInfo& MaterialInfo = Material ? Material->GetMaterialInfo() : FObjMaterialInfo();
			Writer << (FString&)MaterialInfo.MaterialName;
			Writer << (FString&)MaterialInfo.DiffuseTexturePath;
			Writer << (FString&)MaterialInfo.NormalTexturePath;
			Writer << (FString&)MaterialInfo.SpecularTexturePath;
			Writer << (FVector&)MaterialInfo.AmbientColorScalar;
			Writer << (FVector&)MaterialInfo.DiffuseColorScalar;
			Writer << (FVector&)MaterialInfo.SpecularColorScalar;
			Writer << (float&)MaterialInfo.ShininessScalar;
			Writer << (float&)MaterialInfo.TransparencyScalar;
		}

		Writer.Close();

		UE_LOG("UStaticMesh: Successfully saved binary mesh: %s", FilePath.c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG("UStaticMesh: Exception during binary save: %s", e.what());
		return false;
	}
}

/**
 * @brief 바이너리 파일에서 메시 데이터를 로드
 */
bool UStaticMesh::LoadFromBinary(const FString& FilePath)
{
	FBinaryReader Reader(FilePath);
	if (!Reader.IsOpen())
	{
		UE_LOG("UStaticMesh: Failed to open file for reading: %s", FilePath.c_str());
		return false;
	}

	try
	{
		// 헤더 정보 확인
		FString MagicNumber;
		uint32 Version;
		Reader << MagicNumber;
		Reader << Version;

		if (MagicNumber != "MESH" || Version != 1)
		{
			UE_LOG("UStaticMesh: Invalid binary format or version: %s", FilePath.c_str());
			return false;
		}

		// 기존 렌더 버퍼 해제
		ReleaseRenderBuffers();

		// StaticMeshData -> PathFileName 로드
		Reader << StaticMeshData.PathFileName;

		// StaticMeshData -> Vertices 로드
		uint32 VertexCount;
		Reader << VertexCount;
		StaticMeshData.Vertices.SetNum(VertexCount);
		for (uint32 i = 0; i < VertexCount; ++i)
		{
			Reader << StaticMeshData.Vertices[i].Position;
			Reader << StaticMeshData.Vertices[i].Color;
			Reader << StaticMeshData.Vertices[i].TextureCoord;
			Reader << StaticMeshData.Vertices[i].Normal;
		}

		// StaticMeshData -> Indices 로드
		uint32 IndexCount;
		Reader << IndexCount;
		StaticMeshData.Indices.SetNum(IndexCount);
		for (uint32 i = 0; i < IndexCount; ++i)
		{
			Reader << StaticMeshData.Indices[i];
		}

		// StaticMeshData -> Sections 로드
		uint32 SectionCount;
		Reader << SectionCount;
		StaticMeshData.Sections.SetNum(SectionCount);
		for (uint32 i = 0; i < SectionCount; ++i)
		{
			FStaticMeshSection& Section = StaticMeshData.Sections[i];
			Reader << (int32&)Section.StartIndex;
			Reader << (int32&)Section.IndexCount;
			Reader << (int32&)Section.MaterialSlotIndex;
			Reader << (FString&)Section.MaterialName;
		}

		// MaterialSlots 로드
		uint32 MaterialSlotCount;
		Reader << MaterialSlotCount;
		MaterialSlots.SetNum(MaterialSlotCount);
		for (uint32 i = 0; i < MaterialSlotCount; ++i)
		{
			FObjMaterialInfo MaterialInfo;
			Reader << (FString&)MaterialInfo.MaterialName;
			Reader << (FString&)MaterialInfo.DiffuseTexturePath;
			Reader << (FString&)MaterialInfo.NormalTexturePath;
			Reader << (FString&)MaterialInfo.SpecularTexturePath;
			Reader << (FVector&)MaterialInfo.AmbientColorScalar;
			Reader << (FVector&)MaterialInfo.DiffuseColorScalar;
			Reader << (FVector&)MaterialInfo.SpecularColorScalar;
			Reader << (float&)MaterialInfo.ShininessScalar;
			Reader << (float&)MaterialInfo.TransparencyScalar;

			// 먼저 기존 머티리얼 중에서 이름이 같은 것이 있는지 확인
			bool bFound = false;
			for (UMaterial* Material : MakeObjectRange<UMaterial>())
			{
				// TODO: nullptr 체크 안 하면 nullptr 참조로 터지는 경우 있음. 원인 조사 필요.
				if (Material && Material->GetName() == MaterialInfo.MaterialName)
				{
					MaterialSlots[i] = Material;
					bFound = true;
					break;
				}
			}

			// 기존 머티리얼 중 없으면 새로 만들어 적용
			if (!bFound)
			{
				UMaterialInterface* Material = GEngine->GetEngineSubsystem<UAssetSubsystem>()->CreateMaterial(
					MaterialInfo);
				if (Material)
				{
					MaterialSlots[i] = Material;
				}
				else
				{
					UE_LOG("UStaticMesh: Failed to create material: %s", MaterialInfo.MaterialName.c_str());
					MaterialSlots[i] = nullptr;
				}
			}
		}

		Reader.Close();

		// 새 렌더 버퍼 생성
		CreateRenderBuffers();

		UE_LOG("UStaticMesh: Successfully loaded binary mesh: %s (%d vertices, %d indices)",
		       FilePath.c_str(), StaticMeshData.Vertices.Num(), StaticMeshData.Indices.Num());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG("UStaticMesh: Exception during binary load: %s", e.what());
		return false;
	}
}

/**
 * @brief 바이너리 캐시가 유효한지 확인
 */
bool UStaticMesh::IsBinaryCacheValid(const FString& ObjFilePath)
{
	return FArchiveHelpers::IsBinaryCacheValid(ObjFilePath);
}

/**
 * @brief OBJ 파일로부터 바이너리 파일 경로 생성
 */
FString UStaticMesh::GetBinaryFilePath(const FString& ObjFilePath)
{
	return FArchiveHelpers::GetBinaryFilePath(ObjFilePath);
}

void UStaticMesh::SetMaterialSlots(const TArray<UMaterialInterface*>& InMaterialSlots)
{
	MaterialSlots = InMaterialSlots;
}

EVertexLayoutType UStaticMesh::GetVertexType() const
{
	// FVertex 사용시 기본 Vertex Layout (Position + Color + Texture + Normal)
	return EVertexLayoutType::PositionColorTextureNormal;
}
