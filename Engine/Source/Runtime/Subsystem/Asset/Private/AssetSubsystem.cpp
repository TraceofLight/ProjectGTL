#include "pch.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"

#include "Asset/Public/ObjImporter.h"
#include "Asset/Public/StaticMesh.h"
#include "Material/Public/Material.h"
#include "Factory/Public/NewObject.h"
#include "Runtime/Core/Public/ObjectIterator.h"
#include "Utility/Public/Archive.h"

IMPLEMENT_CLASS(UAssetSubsystem, UEngineSubsystem)

void UAssetSubsystem::Initialize()
{
	Super::Initialize();

	// CPU 데이터만 초기화 (GPU 리소스는 Renderer가 생성)
	InitializeDefaultMaterial();

	UE_LOG("AssetSubsystem: 초기화 완료");
}

void UAssetSubsystem::Deinitialize()
{
	// CPU 리소스만 해제
	ReleaseDefaultMaterial();

	// StaticMesh 애셋 해제 - TObjectIterator를 사용하여 모든 StaticMesh 삭제
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		delete StaticMesh;
	}

	Super::Deinitialize();
}

void UAssetSubsystem::InitializeDefaultMaterial() const
{
	if (DefaultMaterial)
	{
		return;
	}

	// CPU 데이터만 생성 (텍스처 GPU 업로드는 Renderer에서 처리)
	FObjMaterialInfo DefaultMaterialInfo(FString("DefaultMaterial"));
	DefaultMaterialInfo.DiffuseTexturePath = "Data\\Texture\\DefaultTexture.png";

	UMaterial* MaterialAsset = NewObject<UMaterial>();
	if (MaterialAsset == nullptr)
	{
		UE_LOG_ERROR("AssetSubsystem: 기본 머티리얼 생성 실패");
		return;
	}

	MaterialAsset->SetMaterialInfo(DefaultMaterialInfo);
	// GPU 텍스처 로딩은 Renderer에서 필요할 때 수행

	const_cast<UMaterialInterface*&>(DefaultMaterial) = MaterialAsset;
	UE_LOG("AssetSubsystem: 기본 머티리얼 CPU 데이터 생성 완료");
}

UMaterialInterface* UAssetSubsystem::GetDefaultMaterial() const
{
	return DefaultMaterial;
}

void UAssetSubsystem::ReleaseDefaultMaterial()
{
	if (DefaultMaterial)
	{
		SafeDelete(DefaultMaterial);
	}
}

TObjectPtr<UStaticMesh> UAssetSubsystem::LoadStaticMesh(const FString& InFilePath)
{
	// 이미 로드된 StaticMesh인지 확인
	for (TObjectIterator<UStaticMesh> Iter; Iter; ++Iter)
	{
		TObjectPtr<UStaticMesh> StaticMesh = TObjectPtr(*Iter);
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return StaticMesh;
		}
	}

	// 새 StaticMesh 로드
	TObjectPtr<UStaticMesh> NewStaticMesh = NewObject<UStaticMesh>();
	if (!NewStaticMesh)
	{
		return nullptr;
	}

	bool bLoadSuccess = false;

	// 바이너리 캐시가 유효한지 확인
	if (UStaticMesh::IsBinaryCacheValid(InFilePath))
	{
		FString BinaryPath = UStaticMesh::GetBinaryFilePath(InFilePath);
		UE_LOG("AssetSubsystem: Loading from binary cache: %s", BinaryPath.c_str());

		// 바이너리에서 로드 시도
		bLoadSuccess = NewStaticMesh->LoadFromBinary(BinaryPath);

		if (bLoadSuccess)
		{
			UE_LOG_SUCCESS("StaticMesh 바이너리 캐시 로드 성공: %s", InFilePath.c_str());
			return NewStaticMesh;
		}
		else
		{
			UE_LOG("AssetSubsystem: Binary cache load failed, falling back to OBJ parsing");
		}
	}

	// 바이너리 캐시가 없거나 실패한 경우 OBJ 파싱
	UE_LOG("AssetSubsystem: Parsing OBJ file: %s", InFilePath.c_str());
	FStaticMesh StaticMeshData;
	TArray<FObjInfo> ObjInfos;
	bLoadSuccess = FObjImporter::ImportStaticMesh(InFilePath, StaticMeshData, ObjInfos);

	if (bLoadSuccess)
	{
		TArray<UMaterialInterface*> MaterialSlots;
		TMap<FString, int32> MaterialNameToSlot;

		BuildMaterialSlots(ObjInfos, MaterialSlots, MaterialNameToSlot);
		AssignSectionMaterialSlots(StaticMeshData, MaterialNameToSlot);

		NewStaticMesh->SetStaticMeshData(StaticMeshData);
		NewStaticMesh->SetMaterialSlots(MaterialSlots);

		if (CheckEmptyMaterialSlots(NewStaticMesh->GetMeshGroupInfo()))
		{
			InsertDefaultMaterial(*NewStaticMesh, MaterialSlots);
		}

		// OBJ 파싱 성공 시 바이너리 캐시로 저장
		FString BinaryPath = UStaticMesh::GetBinaryFilePath(InFilePath);
		if (NewStaticMesh->SaveToBinary(BinaryPath))
		{
			UE_LOG("AssetSubsystem: Successfully saved binary cache: %s", BinaryPath.c_str());
		}
		else
		{
			UE_LOG("AssetSubsystem: Failed to save binary cache: %s", BinaryPath.c_str());
		}

		UE_LOG_SUCCESS("StaticMesh OBJ 파싱 로드 성공: %s", InFilePath.c_str());
		return NewStaticMesh;
	}
	else
	{
		delete NewStaticMesh;
		UE_LOG_ERROR("StaticMesh 로드 실패: %s", InFilePath.c_str());
		return nullptr;
	}
}

TObjectPtr<UStaticMesh> UAssetSubsystem::GetStaticMesh(const FString& InFilePath)
{
	// TObjectIterator를 사용하여 로드된 StaticMesh 검색
	for (TObjectIterator<UStaticMesh> Iter; Iter; ++Iter)
	{
		TObjectPtr<UStaticMesh> StaticMesh = TObjectPtr(*Iter);
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return StaticMesh;
		}
	}
	return nullptr;
}

void UAssetSubsystem::ReleaseStaticMesh(const FString& InFilePath)
{
	// TObjectIterator를 사용하여 해당 StaticMesh 찾아서 삭제
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			delete StaticMesh;
			break;
		}
	}
}

bool UAssetSubsystem::HasStaticMesh(const FString& InFilePath) const
{
	// TObjectIterator를 사용하여 해당 StaticMesh가 존재하는지 확인
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return true;
		}
	}
	return false;
}

void UAssetSubsystem::CollectSectionMaterialNames(const TArray<FObjInfo>& ObjInfos,
                                                  TArray<FString>& OutMaterialNames) const
{
	OutMaterialNames.Empty();

	TSet<FString> RegisteredNames;

	for (const FObjInfo& ObjInfo : ObjInfos)
	{
		for (const FObjSectionInfo& Section : ObjInfo.Sections)
		{
			if (Section.IndexCount <= 0)
			{
				continue;
			}

			if (Section.MaterialName.IsEmpty())
			{
				continue;
			}

			if (RegisteredNames.Contains(Section.MaterialName))
			{
				continue;
			}

			RegisteredNames.Add(Section.MaterialName); // TSet에 키 추가

			OutMaterialNames.Add(Section.MaterialName);
		}
	}
}

const FObjMaterialInfo* UAssetSubsystem::FindMaterialInfoByName(const TArray<FObjInfo>& ObjInfos,
                                                                const FString& MaterialName) const
{
	for (const FObjInfo& Info : ObjInfos)
	{
		const FObjMaterialInfo* FoundInfoPtr = Info.MaterialInfos.Find(MaterialName);

		if (FoundInfoPtr)
		{
			return FoundInfoPtr;
		}
	}

	// 전체 순회 후 찾지 못했으면 nullptr을 반환합니다.
	return nullptr;
}

UMaterialInterface* UAssetSubsystem::CreateMaterial(const FObjMaterialInfo& MaterialInfo) const
{
	UMaterial* MaterialAsset = NewObject<UMaterial>();
	if (!MaterialAsset)
	{
		return nullptr;
	}

	MaterialAsset->SetMaterialInfo(MaterialInfo);
	MaterialAsset->ImportAllTextures();

	return MaterialAsset;
}

void UAssetSubsystem::BuildMaterialSlots(const TArray<FObjInfo>& ObjInfos,
                                         TArray<UMaterialInterface*>& OutMaterialSlots,
                                         TMap<FString, int32>& OutMaterialNameToSlot)
{
	OutMaterialSlots.Empty();
	OutMaterialNameToSlot.Empty();

	// ObjInfos를 이루는 각 FObjInfo 객체에 명시된 머티리얼 전부 모아 저장
	TArray<FString> MaterialNames;
	CollectSectionMaterialNames(ObjInfos, MaterialNames);

	for (const FString& MaterialName : MaterialNames)
	{
		// 이미 등록된 머티리얼인지 확인
		if (OutMaterialNameToSlot.Contains(MaterialName))
		{
			continue;
		}

		// MaterialInfo 찾기 및 생성
		const FObjMaterialInfo* MaterialInfoPtr = FindMaterialInfoByName(ObjInfos, MaterialName);
		FObjMaterialInfo MaterialInfo = MaterialInfoPtr ? *MaterialInfoPtr : FObjMaterialInfo(MaterialName);

		// 머티리얼 에셋 생성 및 등록
		if (UMaterialInterface* MaterialAsset = CreateMaterial(MaterialInfo))
		{
			int32 SlotIndex = OutMaterialSlots.Num();

			OutMaterialSlots.Add(MaterialAsset);
			OutMaterialNameToSlot.Emplace(MaterialName, SlotIndex);
		}
	}
}

void UAssetSubsystem::AssignSectionMaterialSlots(FStaticMesh& StaticMeshData,
                                                 const TMap<FString, int32>& MaterialNameToSlot) const
{
	TArray<FStaticMeshSection>& Sections = StaticMeshData.Sections;

	for (FStaticMeshSection& Section : Sections)
	{
		// FString 유효성 검사
		if (!Section.MaterialName.IsEmpty())
		{
			// 슬롯 인덱스 포인터를 탐색
			const int32* FoundSlotIndexPtr = MaterialNameToSlot.Find(Section.MaterialName);

			// 포인터 유효성 검사
			if (FoundSlotIndexPtr)
			{
				// 찾은 경우 찾은 값을 할당
				Section.MaterialSlotIndex = *FoundSlotIndexPtr;
			}
			else
			{
				// 찾지 못한 경우 -1 할당
				Section.MaterialSlotIndex = -1;
			}
		}
		// MaterialName이 비어있는 경우
		else
		{
			Section.MaterialSlotIndex = -1;
		}
	}
}

bool UAssetSubsystem::CheckEmptyMaterialSlots(const TArray<FStaticMeshSection>& Sections) const
{
	for (const FStaticMeshSection& Section : Sections)
	{
		if (Section.MaterialSlotIndex == -1)
		{
			return true;
		}
	}
	return false;
}

void UAssetSubsystem::InsertDefaultMaterial(UStaticMesh& InStaticMeshData, TArray<UMaterialInterface*>& InMaterialSlots)
{
	// 기본 머티리얼을 슬롯에 추가하고, 해당 슬롯 인덱스를 아직 머티리얼이 없는 섹션에 할당
	int32 DefaultMaterialSlot = InMaterialSlots.Num();
	InMaterialSlots.Add(GetDefaultMaterial());

	for (FStaticMeshSection& Section : InStaticMeshData.GetMeshGroupInfo())
	{
		if (Section.MaterialSlotIndex == -1)
		{
			Section.MaterialSlotIndex = DefaultMaterialSlot;
		}
	}
}

TObjectPtr<UShader> UAssetSubsystem::LoadShader(const FString& InFilePath, const TArray<D3D11_INPUT_ELEMENT_DESC>& InLayoutDesc)
{
	// 캐시 확인
	if (ShaderCache.Contains(InFilePath))
	{
		return ShaderCache[InFilePath];
	}

	// 캐시에 없으면 새로 생성
	TObjectPtr<UShader> NewShader = NewObject<UShader>();
	// EVertexLayoutType을 LayoutDesc로부터 추론하거나, 인자로 받아야 함. 임시로 PositionColorTextureNormal 사용.
	if (NewShader->Initialize(InFilePath, EVertexLayoutType::PositionColorTextureNormal))
	{
		ShaderCache.Add(InFilePath, NewShader);
		return NewShader;
	}

	// 3. 실패 시 null 반환
	NewShader->Release(); // Release if creation failed
	return nullptr;
}
