#include "pch.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"
#include "Window/Public/ViewportClient.h"
#include "Utility/Public/JsonSerializer.h"
#include "Utility/Public/Metadata.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Editor/Public/Editor.h"
#include "Asset/Public/StaticMesh.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Window/Public/Viewport.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Runtime/Subsystem/Public/PathSubsystem.h"

IMPLEMENT_CLASS(UWorldSubsystem, UEngineSubsystem)

// ===== UEngineSubsystem 인터페이스 구현 =====

void UWorldSubsystem::Initialize()
{
	Super::Initialize();
	UE_LOG("WorldSubsystem: Initialized");
}

void UWorldSubsystem::PostInitialize()
{
	// 2단계 초기화: 모든 서브시스템이 생성된 후, 의존성을 갖는 초기화
	Editor = MakeUnique<UEditor>();
	if (Editor)
	{
		// 렌더링 리소스가 필요한 모든 멤버를 초기화
		Editor->Initialize();
	}

	CreateAndRegisterLevel();
}

void UWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG("WorldSubsystem: Deinitializing...");

	Shutdown();

	// TUniquePtr 해제
	Editor.Reset();
}

bool UWorldSubsystem::IsTickable() const
{
	return true; // 매 프레임 업데이트가 필요하므로 true
}

void UWorldSubsystem::Tick()
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

void UWorldSubsystem::RegisterLevel(const FName& InName, TObjectPtr<ULevel> InLevel)
{
	Levels[InName] = InLevel;
	if (!CurrentLevel)
	{
		CurrentLevel = InLevel;
	}
}

void UWorldSubsystem::LoadLevel(const FName& InName)
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

/**
 * @brief WorldSubsystem의 정리 함수
 * Outer 끊어주면 GC에서 수거하는 방향을 고려하여 구현됨
 * GC는 현재 없으나 마지막 Engine 종료에서 UClass 객체 전부 지워주고 있으므로 오류 없이 모든 Level이 정상 제거됨
 */
void UWorldSubsystem::Shutdown()
{
	for (auto& Level : Levels)
	{
		if (Level.second)
		{
			Level.second->Release();
		}
	}

	Levels.clear();
	CurrentLevel = nullptr;
}

/**
 * @brief 레벨을 생성하고 등록까지 처리하는 함수
 */
void UWorldSubsystem::CreateAndRegisterLevel()
{
	TObjectPtr<ULevel> Level = NewObject<ULevel>();
	FName InternalName = "Level_" + to_string(Level->GetClass()->GetNextGenNumber());
	Level->SetName(InternalName);
	Level->SetDisplayName("New Level");

	// WorldSubsystem을 Outer로 설정하여 생명주기 관리
	Level->SetOuter(this);

	RegisterLevel(InternalName, Level);

	UE_LOG_SUCCESS("WorldSubsystem: 새로운 레벨 Level_%d 을(를) 생성했습니다", Level->GetClass()->GetNextGenNumber());
}


/**
 * @brief 현재 레벨을 지정된 경로에 저장
 */
bool UWorldSubsystem::SaveCurrentLevel(const FString& InFilePath) const
{
	if (!CurrentLevel)
	{
		UE_LOG("WorldSubsystem: No Current Level To Save");
		return false;
	}

	// 기본 파일 경로 생성
	path FilePath = InFilePath;
	if (FilePath.empty())
	{
		// 기본 파일명은 Level 이름으로 세팅
		FilePath = GenerateLevelFilePath(CurrentLevel->GetName() == FName::FName_None
			                                 ? "Untitled"
			                                 : CurrentLevel->GetName().ToString());
	}

	UE_LOG("WorldSubsystem: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

	// LevelSerializer를 사용하여 저장
	try
	{
		// 현재 레벨의 메타데이터 생성
		FLevelMetadata Metadata = ConvertLevelToMetadata(CurrentLevel);

		// 0번 ViewPort의 PerspectiveCamera 세팅을 Metadata에 포함
		UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
		TArray<FViewportClient*>& Clients = ViewportSS->GetClients();
		UE_LOG("WorldSubsystem: SaveCurrentLevel - Saving camera from Client[0]");
		if (!Clients.empty() && Clients[0])
		{
			ACameraActor* PerspectiveCamera = Clients[0]->GetPerspectiveCamera();
			if (PerspectiveCamera)
			{
				UCameraComponent* CamComp = PerspectiveCamera->GetCameraComponent();
				if (CamComp)
				{
					Metadata.PerspectiveCamera.FarClip = CamComp->GetFarZ();
					Metadata.PerspectiveCamera.NearClip = CamComp->GetNearZ();
					Metadata.PerspectiveCamera.FOV = CamComp->GetFovY();
					Metadata.PerspectiveCamera.Location = PerspectiveCamera->GetActorLocation();
					Metadata.PerspectiveCamera.Rotation = PerspectiveCamera->GetActorRotation();

					UE_LOG(
						"WorldSubsystem: Camera saved - Loc:(%.2f,%.2f,%.2f) Rot:(%.2f,%.2f,%.2f) FOV:%.2f Near:%.2f Far:%.2f",
						Metadata.PerspectiveCamera.Location.X, Metadata.PerspectiveCamera.Location.Y,
						Metadata.PerspectiveCamera.Location.Z,
						Metadata.PerspectiveCamera.Rotation.X, Metadata.PerspectiveCamera.Rotation.Y,
						Metadata.PerspectiveCamera.Rotation.Z,
						Metadata.PerspectiveCamera.FOV, Metadata.PerspectiveCamera.NearClip,
						Metadata.PerspectiveCamera.FarClip);
				}
				else
				{
					UE_LOG("WorldSubsystem: WARNING - PerspectiveCamera has no CameraComponent, camera data not saved!");
				}
			}
			else
			{
				UE_LOG("WorldSubsystem: WARNING - Client[0] has no PerspectiveCamera, camera data not saved!");
			}
		}
		else
		{
			UE_LOG("WorldSubsystem: WARNING - No clients available, camera data not saved!");
		}

		bool bSuccess = FJsonSerializer::SaveLevelToFile(Metadata, FilePath.string());

		if (bSuccess)
		{
			UE_LOG("WorldSubsystem: 레벨이 성공적으로 저장되었습니다");
		}
		else
		{
			UE_LOG("WorldSubsystem: 레벨을 저장하는 데에 실패했습니다");
		}

		return bSuccess;
	}
	catch (const exception& Exception)
	{
		UE_LOG("WorldSubsystem: 저장 과정에서 Exception 발생: %s", Exception.what());
		return false;
	}
}

/**
 * @brief 지정된 파일로부터 Level Load & Register
 */
bool UWorldSubsystem::LoadLevel(const FString& InLevelName, const FString& InFilePath)
{
	UE_LOG("WorldSubsystem: Loading Level '%s' From: %s", InLevelName.data(), InFilePath.data());

	// 기존 레벨이 있다면 먼저 정리
	ClearCurrentLevel();

	// Make New Level
	TObjectPtr<ULevel> NewLevel = NewObject<ULevel>();
	NewLevel->SetDisplayName(InLevelName);
	// WorldSubsystem을 Outer로 설정하여 생명주기 관리
	NewLevel->SetOuter(this);
	FLevelMetadata Metadata;

	// 직접 LevelSerializer를 사용하여 로드
	try
	{
		bool bLoadSuccess = FJsonSerializer::LoadLevelFromFile(Metadata, InFilePath);
		if (!bLoadSuccess)
		{
			UE_LOG("WorldSubsystem: Failed To Load Level From: %s", InFilePath.c_str());
			NewLevel->Release();
			return false;
		}

		// 유효성 검사
		FString ErrorMessage;
		if (!FJsonSerializer::ValidateLevelData(Metadata, ErrorMessage))
		{
			UE_LOG("WorldSubsystem: Level Validation Failed: %s", ErrorMessage.c_str());
			NewLevel->Release();
			return false;
		}

		// 메타데이터로부터 Level 생성
		bool bSuccess = LoadLevelFromMetadata(NewLevel, Metadata);

		if (!bSuccess)
		{
			UE_LOG("WorldSubsystem: Failed To Create Level From Metadata");
			NewLevel->Release();
			return false;
		}
	}
	catch (const exception& InException)
	{
		UE_LOG("WorldSubsystem: Exception During Load: %s", InException.what());
		// Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
		NewLevel->Release();
		return false;
	}

	// 위에서 이미 로드 완료했으므로 Success 처리
	bool bSuccess = true;

	if (bSuccess)
	{
		FName LevelName(InLevelName);

		// 새 레벨 등록 및 활성화
		RegisterLevel(LevelName, NewLevel);

		// 현재 레벨을 로드된 레벨로 전환
		if (CurrentLevel && CurrentLevel != NewLevel)
		{
			CurrentLevel->Cleanup();
		}

		CurrentLevel = NewLevel;
		CurrentLevel->Init();

		// 카메라 정보 복원
		RestoreCameraFromMetadata(Metadata.PerspectiveCamera);

		UE_LOG("WorldSubsystem: Level이 성공적으로 로드되어 Level '%s' (으)로 레벨을 교체 완료했습니다", InLevelName.c_str());
	}
	else
	{
		NewLevel->Release();
		UE_LOG("WorldSubsystem: 파일로부터 Level을 로드하는 데에 실패했습니다");
	}

	return bSuccess;
}

/**
 * @brief 새로운 빈 레벨을 생성해주는 함수
 * Display Name을 항상동일하게 NewLevel로 표기한다
 */
bool UWorldSubsystem::CreateNewLevel()
{
	UE_LOG("WorldSubsystem: 새로운 레벨을 생성 중...");

	// 기존 레벨이 있다면 먼저 정리 (Asset 포함)
	UE_LOG("WorldSubsystem: 기존 레벨 정리를 시도합니다");
	ClearCurrentLevel();

	// 새 레벨에서 처리
	CreateAndRegisterLevel();
	CurrentLevel = GetCurrentLevel();
	CurrentLevel->Init();

	UE_LOG_SUCCESS("WorldSubsystem: 새로운 레벨이 성공적으로 생성되었습니다");

	return true;
}

/**
 * @brief 레벨 저장 디렉토리 경로 반환
 */
path UWorldSubsystem::GetLevelDirectory()
{
	UPathSubsystem* PathSubsystem = GEngine->GetEngineSubsystem<UPathSubsystem>();
	return PathSubsystem->GetWorldPath();
}

/**
 * @brief 레벨 이름을 바탕으로 전체 파일 경로 생성
 */
path UWorldSubsystem::GenerateLevelFilePath(const FString& InLevelName)
{
	path LevelDirectory = GetLevelDirectory();
	path FileName = InLevelName + ".json";
	path FullPath = LevelDirectory / FileName;
	return FullPath;
}

/**
 * @brief ULevel을 FLevelMetadata로 변환
 */
FLevelMetadata UWorldSubsystem::ConvertLevelToMetadata(TObjectPtr<ULevel> InLevel)
{
	FLevelMetadata Metadata;
	Metadata.Version = 1;
	Metadata.NextUUID = 1;

	if (!InLevel)
	{
		UE_LOG("WorldSubsystem: ConvertLevelToMetadata: Level Is Null");
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
		PrimitiveMeta.Type = EPrimitiveType::StaticMeshComp;

		// StaticMeshActor에서 OBJ 파일 경로 가져오기
		if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor))
		{
			if (UStaticMeshComponent* MeshComp = StaticMeshActor->GetStaticMeshComponent())
			{
				if (UStaticMesh* StaticMesh = MeshComp->GetStaticMesh())
				{
					PrimitiveMeta.ObjStaticMeshAsset = StaticMesh->GetAssetPathFileName();
				}
			}
		}

		// ObjStaticMeshAsset이 비어있으면 기본값 설정
		if (PrimitiveMeta.ObjStaticMeshAsset.empty())
		{
			PrimitiveMeta.ObjStaticMeshAsset = "Data/DefaultMesh.obj";
		}

		Metadata.Primitives[PrimitiveMeta.ID] = PrimitiveMeta;
	}

	Metadata.NextUUID = CurrentID;

	UE_LOG("WorldSubsystem: Converted %zu Actors To Metadata", Metadata.Primitives.size());
	return Metadata;
}

/**
 * @brief FLevelMetadata로부터 ULevel에 Actor Load
 */
bool UWorldSubsystem::LoadLevelFromMetadata(TObjectPtr<ULevel> InLevel, const FLevelMetadata& InMetadata)
{
	if (!InLevel)
	{
		UE_LOG("WorldSubsystem: LoadLevelFromMetadata: InLevel Is Null");
		return false;
	}

	UE_LOG("WorldSubsystem: Loading %zu Primitives From Metadata", InMetadata.Primitives.size());

	// Metadata의 각 Primitive를 Actor로 생성
	for (const auto& [ID, PrimitiveMeta] : InMetadata.Primitives)
	{
		TObjectPtr<AActor> NewActor = nullptr;

		// 타입에 따라 적절한 액터 생성
		switch (PrimitiveMeta.Type)
		{
		case EPrimitiveType::StaticMeshComp:
			NewActor = InLevel->SpawnActor<AStaticMeshActor>("StaticMeshActor");
			break;
		default:
			UE_LOG("WorldSubsystem: Unknown Primitive Type: %d", static_cast<int32>(PrimitiveMeta.Type));
			assert(!"고려하지 않은 Actor 타입");
			continue;
		}

		if (NewActor)
		{
			// StaticMeshActor의 경우 OBJ 파일 로드
			if (PrimitiveMeta.Type == EPrimitiveType::StaticMeshComp)
			{
				if (TObjectPtr<AStaticMeshActor> StaticMeshActor = Cast<AStaticMeshActor>(NewActor))
				{
					UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
					UStaticMesh* StaticMesh = nullptr;
					if (AssetSubsystem)
					{
						StaticMesh = AssetSubsystem->LoadStaticMesh(PrimitiveMeta.ObjStaticMeshAsset);
					}
					if (StaticMesh)
					{
						StaticMeshActor->SetStaticMesh(StaticMesh);
					}
					else
					{
						UE_LOG("WorldSubsystem: Failed To Load StaticMesh From: %s",
						       PrimitiveMeta.ObjStaticMeshAsset.c_str());
						InLevel->DestroyActor(StaticMeshActor);
					}
				}
			}

			// Transform 정보 적용
			NewActor->SetActorLocation(PrimitiveMeta.Location);
			NewActor->SetActorRotation(PrimitiveMeta.Rotation);
			NewActor->SetActorScale3D(PrimitiveMeta.Scale);

			UE_LOG("WorldSubsystem: (%.2f, %.2f, %.2f) 지점에 %s (을)를 생성했습니다 ",
			       PrimitiveMeta.Location.X, PrimitiveMeta.Location.Y, PrimitiveMeta.Location.Z,
			       FJsonSerializer::PrimitiveTypeToWideString(PrimitiveMeta.Type).c_str());
		}
		else
		{
			UE_LOG("WorldSubsystem: Actor 생성에 실패했습니다 (Primitive ID: %d)", ID);
		}
	}

	UE_LOG("WorldSubsystem: 레벨이 메타데이터로부터 성공적으로 로드되었습니다");
	return true;
}

void UWorldSubsystem::ClearCurrentLevel()
{
	if (CurrentLevel)
	{
		// Outer 관계를 끊어서 GC에서 자연스럽게 정리되도록 함
		CurrentLevel->Release();
		CurrentLevel = nullptr;
		UE_LOG("WorldSubsystem: Current level cleared successfully");
	}
}

/**
 * @brief 메타데이터에서 카메라 정보를 복원
 */
void UWorldSubsystem::RestoreCameraFromMetadata(const FCameraMetadata& InCameraMetadata)
{
	UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	TArray<FViewportClient*>& Clients = ViewportSS->GetClients();

	// FOV/Near/Far가 0이면 저장된 데이터가 손상되었으므로 기본값 사용
	bool bUseDefaults = (InCameraMetadata.FOV <= 0.0f || InCameraMetadata.NearClip <= 0.0f ||
		InCameraMetadata.FarClip <= 0.0f);
	if (bUseDefaults)
	{
		UE_LOG_WARNING("WorldSubsystem: 경고: 카메라 메타데이터 손상, 기본값을 사용합니다");
	}

	// 모든 Perspective 카메라에 동일한 설정 적용
	// 각 Viewport의 실제 크기를 기반으로 Aspect 설정
	TArray<FViewport*>& Viewports = ViewportSS->GetViewports();

	for (size_t i = 0; i < Clients.size(); ++i)
	{
		UE_LOG("WorldSubsystem: Processing Client[%d]", static_cast<int>(i));

		FViewportClient* Client = Clients[i];
		if (!Client)
		{
			UE_LOG("WorldSubsystem: Client[%d] is NULL", static_cast<int>(i));
			continue;
		}

		ACameraActor* PerspectiveCamera = Client->GetPerspectiveCamera();
		if (!PerspectiveCamera)
		{
			UE_LOG("WorldSubsystem: Client[%d]는 PerspectiveCamera가 없습니다", static_cast<int>(i));
			continue;
		}

		UCameraComponent* CameraComponent = PerspectiveCamera->GetCameraComponent();
		if (!CameraComponent)
		{
			UE_LOG_WARNING("WorldSubsystem: Client[%d]의 PerspectiveCamera가 CameraComponent가 없습니다", static_cast<int>(i));
			continue;
		}

		UE_LOG("WorldSubsystem: Client[%d]: 카메라 속성을 설정합니다", static_cast<int>(i));

		// 카메라 설정 복원
		PerspectiveCamera->SetActorLocation(InCameraMetadata.Location);
		PerspectiveCamera->SetActorRotation(InCameraMetadata.Rotation);

		// FOV/Near/Far는 유효성 검사 후 설정
		if (!bUseDefaults)
		{
			CameraComponent->SetFovY(InCameraMetadata.FOV);
			CameraComponent->SetNearZ(InCameraMetadata.NearClip);
			CameraComponent->SetFarZ(InCameraMetadata.FarClip);
		}
		else
		{
			UE_LOG("WorldSubsystem: Client[%d]: FOV/Near/Far 기본값을 사용합니다", static_cast<int>(i));
		}

		UE_LOG("WorldSubsystem: Client[%d] - Camera set to Location:(%.2f,%.2f,%.2f) Rotation:(%.2f,%.2f,%.2f)",
		       static_cast<int>(i),
		       InCameraMetadata.Location.X, InCameraMetadata.Location.Y, InCameraMetadata.Location.Z,
		       InCameraMetadata.Rotation.X, InCameraMetadata.Rotation.Y, InCameraMetadata.Rotation.Z);

		// 각 Viewport의 실제 크기를 기반으로 Aspect 계산
		if (i < Viewports.size() && Viewports[i])
		{
			const FRect& ViewportRect = Viewports[i]->GetRect();
			const int32 ToolbarHeight = Viewports[i]->GetToolbarHeight();
			const float ViewportWidth = static_cast<float>(ViewportRect.W);
			const float ViewportHeight = static_cast<float>(max<LONG>(0, ViewportRect.H - ToolbarHeight));

			UE_LOG("WorldSubsystem: Client[%d] Viewport: Rect:(%ld,%ld,%ld,%ld) Toolbar:%d RenderSize:(%.2fx%.2f)",
			       static_cast<int>(i),
			       ViewportRect.X, ViewportRect.Y, ViewportRect.W, ViewportRect.H,
			       ToolbarHeight, ViewportWidth, ViewportHeight);

			if (ViewportHeight > 0.0f && ViewportWidth > 0.0f)
			{
				float Aspect = ViewportWidth / ViewportHeight;
				CameraComponent->SetAspect(Aspect);
				UE_LOG("WorldSubsystem: Client[%d]: Aspect 설정: %.4f", static_cast<int>(i), Aspect);
			}
			else
			{
				UE_LOG("WorldSubsystem: Client[%d]: 비정상적인 viewport size, aspect를 스킵합니다", static_cast<int>(i));
			}
		}
		else
		{
			UE_LOG("WorldSubsystem: Client[%d]: Viewport[%d]이 NULL이거나 범위 바깥입니다",
			       static_cast<int>(i), static_cast<int>(i));
		}
	}
}
