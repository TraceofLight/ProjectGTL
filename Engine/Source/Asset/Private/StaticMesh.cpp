#include "pch.h"
#include "Asset/Public/StaticMesh.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Utility/Public/Archive.h"
#include "Manager/Asset/Public/AssetManager.h"

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
	return !StaticMeshData.Vertices.empty() && !StaticMeshData.Indices.empty();
}

void UStaticMesh::CreateRenderBuffers()
{
	if (!IsValidMesh())
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();

	const TArray<FVertex>& Vertices = StaticMeshData.Vertices;
	const TArray<uint32>& Indices = StaticMeshData.Indices;

	if (!Vertices.empty())
	{
		const uint32 VertexBufferSize = static_cast<uint32>(Vertices.size()) * sizeof(FVertex);
		VertexBuffer = Renderer.CreateVertexBuffer(const_cast<FVertex*>(Vertices.data()), VertexBufferSize);
	}

	if (!Indices.empty())
	{
		const uint32 IndexBufferSize = static_cast<uint32>(Indices.size()) * sizeof(uint32);
		IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), IndexBufferSize);
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
	if (StaticMeshData.Vertices.empty())
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
		uint32 VertexCount = static_cast<uint32>(StaticMeshData.Vertices.size());
		Writer << VertexCount;
		for (FVertex Vertex : StaticMeshData.Vertices)
		{
			Writer << Vertex.Position;
			Writer << Vertex.Color;
			Writer << Vertex.TextureCoord;
			Writer << Vertex.Normal;
		}

		// StaticMeshData -> Indices 저장
		uint32 IndexCount = static_cast<uint32>(StaticMeshData.Indices.size());
		Writer << IndexCount;
		for (uint32 Index : StaticMeshData.Indices)
		{
			Writer << Index;
		}

		// StaticMeshData -> Sections 저장
		uint32 SectionCount = static_cast<uint32>(StaticMeshData.Sections.size());
		Writer << SectionCount;
		for (const FStaticMeshSection& Section : StaticMeshData.Sections)
		{
			Writer << (int32&)Section.StartIndex;
			Writer << (int32&)Section.IndexCount;
			Writer << (int32&)Section.MaterialSlotIndex;
			Writer << (FString&)Section.MaterialName;
		}

		// MaterialSlots 저장
		uint32 MaterialSlotCount = static_cast<uint32>(MaterialSlots.size());
		Writer << MaterialSlotCount;
		for(UMaterialInterface* MaterialInterface : MaterialSlots)
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
		StaticMeshData.Vertices.resize(VertexCount);
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
		StaticMeshData.Indices.resize(IndexCount);
		for (uint32 i = 0; i < IndexCount; ++i)
		{
			Reader << StaticMeshData.Indices[i];
		}

		// StaticMeshData -> Sections 로드
		uint32 SectionCount;
		Reader << SectionCount;
		StaticMeshData.Sections.resize(SectionCount);
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
		MaterialSlots.resize(MaterialSlotCount);
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
			UMaterialInterface* Material = UAssetManager::GetInstance().CreateMaterial(MaterialInfo);
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

		Reader.Close();

		// 새 렌더 버퍼 생성
		CreateRenderBuffers();

		UE_LOG("UStaticMesh: Successfully loaded binary mesh: %s (%zu vertices, %zu indices)",
			FilePath.c_str(), StaticMeshData.Vertices.size(), StaticMeshData.Indices.size());
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
