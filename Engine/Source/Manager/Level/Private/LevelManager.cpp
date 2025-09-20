#include "pch.h"
#include "Manager/Level/Public/LevelManager.h"

#include "Runtime/Level/Public/Level.h"
#include "Runtime/Actor/Public/CubeActor.h"
#include "Runtime/Actor/Public/SphereActor.h"
#include "Runtime/Actor/Public/TriangleActor.h"
#include "Runtime/Actor/Public/SquareActor.h"
#include "Runtime/Subsystem/Public/PathSubsystem.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Utility/Public/JsonSerializer.h"
#include "Utility/Public/Metadata.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_SINGLETON_CLASS_BASE(ULevelManager)

ULevelManager::ULevelManager()
{
	Editor = new UEditor;
}

ULevelManager::~ULevelManager() = default;

void ULevelManager::RegisterLevel(const FName& InName, TObjectPtr<ULevel> InLevel)
{
	Levels[InName] = InLevel;
	if (!CurrentLevel)
	{
		CurrentLevel = InLevel;
	}
}

void ULevelManager::LoadLevel(const FName& InName)
{
	if (Levels.find(InName) == Levels.end())
	{
		assert(!"Load할 레벨을 탐색하지 못함");
	}

	if (CurrentLevel)
	{
		CurrentLevel->Cleanup();
	}

	CurrentLevel = Levels[InName];

	CurrentLevel->Init();
}

void ULevelManager::Shutdown()
{
	for (auto& Level : Levels)
	{
		SafeDelete(Level.second);
	}
	delete Editor;
}

/**
 * @brief 기본 레벨을 생성하는 함수
 * XXX(KHJ): 이걸 지워야 할지, 아니면 Main Init에서만 배제할지 고민
 */
void ULevelManager::CreateDefaultLevel()
{
	Levels[FName("Untitled")] = new ULevel("Untitled");
	LoadLevel(FName("Untitled"));
}

void ULevelManager::Update() const
{
	if (CurrentLevel)
	{
		CurrentLevel->Update();
	}
	if (Editor)
	{
		Editor->Update();
	}
}

/**
 * @brief 현재 레벨을 지정된 경로에 저장
 */
bool ULevelManager::SaveCurrentLevel(const FString& InFilePath) const
{
	if (!CurrentLevel)
	{
		UE_LOG("LevelManager: No Current Level To Save");
		return false;
	}

	// 기본 파일 경로 생성
	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		// 기본 파일명은 Level 이름으로 세팅
		FilePath = GenerateLevelFilePath(CurrentLevel->GetName() == FName::None
			                                 ? "Untitled"
			                                 : CurrentLevel->GetName().ToString());
	}

	UE_LOG("LevelManager: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

	// LevelSerializer를 사용하여 저장
	try
	{
		// 현재 레벨의 메타데이터 생성
		FLevelMetadata Metadata = ConvertLevelToMetadata(CurrentLevel);

		bool bSuccess = FJsonSerializer::SaveLevelToFile(Metadata, FilePath.string());

		if (bSuccess)
		{
			UE_LOG("LevelManager: 레벨이 성공적으로 저장되었습니다");
		}
		else
		{
			UE_LOG("LevelManager: 레벨을 저장하는 데에 실패했습니다");
		}

		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("LevelManager: 저장 과정에서 Exception 발생: %s", Exception.what());
		return false;
	}
}

/**
 * @brief 지정된 파일로부터 Level Load & Register
 */
bool ULevelManager::LoadLevel(const FString& InLevelName, const FString& InFilePath)
{
	UE_LOG("LevelManager: Loading Level '%s' From: %s", InLevelName.data(), InFilePath.data());

	// Make New Level
	TObjectPtr<ULevel> NewLevel = TObjectPtr<ULevel>(new ULevel(InLevelName));

	// 직접 LevelSerializer를 사용하여 로드
	try
	{
		FLevelMetadata Metadata;

		bool bLoadSuccess = FJsonSerializer::LoadLevelFromFile(Metadata, InFilePath);
		if (!bLoadSuccess)
		{
			UE_LOG("LevelManager: Failed To Load Level From: %s", InFilePath.c_str());
			delete NewLevel;
			return false;
		}

		// 유효성 검사
		FString ErrorMessage;
		if (!FJsonSerializer::ValidateLevelData(Metadata, ErrorMessage))
		{
			UE_LOG("LevelManager: Level Validation Failed: %s", ErrorMessage.c_str());
			delete NewLevel;
			return false;
		}

		// 메타데이터로부터 Level 생성
		bool bSuccess = LoadLevelFromMetadata(NewLevel, Metadata);

		if (!bSuccess)
		{
			UE_LOG("LevelManager: Failed To Create Level From Metadata");
			delete NewLevel;
			return false;
		}
	}
	catch (const exception& InException)
	{
		UE_LOG("LevelManager: Exception During Load: %s", InException.what());
		delete NewLevel;
		return false;
	}

	// 위에서 이미 로드 완료했으므로 Success 처리
	bool bSuccess = true;

	if (bSuccess)
	{
		// 기존 레벨이 있다면 정리
		ULevel* OldLevel;

		FName LevelName(InLevelName);
		if (Levels.find(LevelName) != Levels.end())
		{
			OldLevel = Levels[LevelName];

			// CurrentLevel이 삭제될 레벨과 같다면 미리 nullptr로 설정
			if (CurrentLevel == OldLevel)
			{
				CurrentLevel->Cleanup();
				CurrentLevel = nullptr;
			}

			delete OldLevel;
			Levels.erase(LevelName);
		}

		// 새 레벨 등록 및 활성화
		RegisterLevel(LevelName, NewLevel);

		// 현재 레벨을 로드된 레벨로 전환
		if (CurrentLevel && CurrentLevel != NewLevel)
		{
			CurrentLevel->Cleanup();
		}

		CurrentLevel = NewLevel;
		CurrentLevel->Init();

		UE_LOG("LevelManager: Level이 성공적으로 로드되어 Level '%s' (으)로 레벨을 교체 완료했습니다", InLevelName.c_str());
	}
	else
	{
		// 로드 실패 시 정리
		delete NewLevel;
		UE_LOG("LevelManager: 파일로부터 Level을 로드하는 데에 실패했습니다");
	}

	return bSuccess;
}

/**
 * @brief New Blank Level 생성
 */
bool ULevelManager::CreateNewLevel(const FString& InLevelName)
{
	UE_LOG("LevelManager: Creating New Level: %s", InLevelName.c_str());

	// 이미 존재하는 레벨 이름인지 확인
	FName LevelName(InLevelName);
	if (Levels.find(LevelName) != Levels.end())
	{
		UE_LOG("LevelManager: Level '%s' Already Exists", InLevelName.c_str());
		return false;
	}

	// 새 레벨 생성
	TObjectPtr<ULevel> NewLevel = TObjectPtr<ULevel>(new ULevel(InLevelName));

	// 레벨 등록 및 활성화
	RegisterLevel(LevelName, NewLevel);

	// 현재 레벨을 새 레벨로 전환
	if (CurrentLevel && CurrentLevel != NewLevel)
	{
		CurrentLevel->Cleanup();
	}

	CurrentLevel = NewLevel;
	CurrentLevel->Init();

	UE_LOG("LevelManager: Successfully Created and Switched to New Level '%s'", InLevelName.c_str());

	return true;
}

/**
 * @brief 레벨 저장 디렉토리 경로 반환
 */
path ULevelManager::GetLevelDirectory()
{
	UPathSubsystem* PathSubsystem = GEngine->GetEngineSubsystem<UPathSubsystem>();
	return PathSubsystem->GetWorldPath();
}

/**
 * @brief 레벨 이름을 바탕으로 전체 파일 경로 생성
 */
path ULevelManager::GenerateLevelFilePath(const FString& InLevelName)
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".json";
	path FullPath = LevelDirectory / FileName;
	return FullPath;
}

/**
 * @brief ULevel을 FLevelMetadata로 변환
 */
FLevelMetadata ULevelManager::ConvertLevelToMetadata(TObjectPtr<ULevel> InLevel)
{
	FLevelMetadata Metadata;
	Metadata.Version = 1;
	Metadata.NextUUID = 1;

	if (!InLevel)
	{
		UE_LOG("LevelManager: ConvertLevelToMetadata: Level Is Null");
		return Metadata;
	}

	// 레벨의 액터들을 순회하며 메타데이터로 변환
	uint32 CurrentID = 1;
	for (AActor* Actor : InLevel->GetLevelActors())
	{
		if (!Actor)
			continue;

		FPrimitiveMetadata PrimitiveMeta;
		PrimitiveMeta.ID = CurrentID++;
		PrimitiveMeta.Location = Actor->GetActorLocation();
		PrimitiveMeta.Rotation = Actor->GetActorRotation();
		PrimitiveMeta.Scale = Actor->GetActorScale3D();

		// Actor 타입에 따라 EPrimitiveType 설정
		if (dynamic_cast<ACubeActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Cube;
		}
		else if (dynamic_cast<ASphereActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Sphere;
		}
		else if (dynamic_cast<ATriangleActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Triangle;
		}
		else if (dynamic_cast<ASquareActor*>(Actor))
		{
			PrimitiveMeta.Type = EPrimitiveType::Square;
		}
		else
		{
			UE_LOG("LevelManager: Unknown Actor Type, Skipping...");
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		Metadata.Primitives[PrimitiveMeta.ID] = PrimitiveMeta;
	}

	Metadata.NextUUID = CurrentID;

	UE_LOG("LevelManager: Converted %zu Actors To Metadata", Metadata.Primitives.size());
	return Metadata;
}

/**
 * @brief FLevelMetadata로부터 ULevel에 Actor Load
 */
bool ULevelManager::LoadLevelFromMetadata(TObjectPtr<ULevel> InLevel, const FLevelMetadata& InMetadata)
{
	if (!InLevel)
	{
		UE_LOG("LevelManager: LoadLevelFromMetadata: InLevel Is Null");
		return false;
	}

	UE_LOG("LevelManager: Loading %zu Primitives From Metadata", InMetadata.Primitives.size());

	// Metadata의 각 Primitive를 Actor로 생성
	for (const auto& [ID, PrimitiveMeta] : InMetadata.Primitives)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 적절한 액터 생성
		switch (PrimitiveMeta.Type)
		{
		case EPrimitiveType::Cube:
			NewActor = InLevel->SpawnActor<ACubeActor>();
			break;
		case EPrimitiveType::Sphere:
			NewActor = InLevel->SpawnActor<ASphereActor>();
			break;
		case EPrimitiveType::Triangle:
			NewActor = InLevel->SpawnActor<ATriangleActor>();
			break;
		case EPrimitiveType::Square:
			NewActor = InLevel->SpawnActor<ASquareActor>();
			break;
		default:
			UE_LOG("LevelManager: Unknown Primitive Type: %d", static_cast<int32>(PrimitiveMeta.Type));
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		if (NewActor)
		{
			// Transform 정보 적용
			NewActor->SetActorLocation(PrimitiveMeta.Location);
			NewActor->SetActorRotation(PrimitiveMeta.Rotation);
			NewActor->SetActorScale3D(PrimitiveMeta.Scale);

			UE_LOG("LevelManager: (%.2f, %.2f, %.2f) 지점에 %s (을)를 생성했습니다 ",
			       PrimitiveMeta.Location.X, PrimitiveMeta.Location.Y, PrimitiveMeta.Location.Z,
			       FJsonSerializer::PrimitiveTypeToWideString(PrimitiveMeta.Type).c_str());
		}
		else
		{
			UE_LOG("LevelManager: Actor 생성에 실패했습니다 (Primitive ID: %d)", ID);
		}
	}

	UE_LOG("LevelManager: 레벨이 메타데이터로부터 성공적으로 로드되었습니다");
	return true;
}
