#include "pch.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"

IMPLEMENT_CLASS(UPrimitiveSpawnWidget, UWidget)

#include "Runtime/Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Asset/Public/StaticMesh.h"
#include "Runtime/Core/Public/ObjectIterator.h"

#include <shobjidl.h>

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
	: UWidget("StaticMesh Spawn Widget")
{
	RefreshStaticMeshList();
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// 이거 호출 안됨
	RefreshStaticMeshList();
}

void UPrimitiveSpawnWidget::Update()
{
	// 필요시 정기적으로 StaticMesh 목록 갱신
	//RefreshStaticMeshList();
}

void UPrimitiveSpawnWidget::RenderWidget()
{
	ImGui::Text("StaticMesh Actor 생성");
	ImGui::Spacing();

	// StaticMesh 목록 새로고침 버튼
	if (ImGui::Button("Refresh Mesh List"))
	{
		RefreshStaticMeshList();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Mesh"))
	{
		LoadMeshFromFile();
	}
	ImGui::SameLine();
	ImGui::Text("Total Meshes: %d", static_cast<int>(AvailableStaticMeshes.size()));

	// StaticMesh 선택 콤보박스
	if (!AvailableStaticMeshes.empty())
	{
		ImGui::Text("StaticMesh:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200);

		// StaticMesh 이름들을 const char* 배열로 변환
		TArray<const char*> MeshNamesCStr;
		for (const FString& Name : StaticMeshNames)
		{
			MeshNamesCStr.push_back(Name.c_str());
		}

		ImGui::Combo("##StaticMeshType", &SelectedMeshIndex, MeshNamesCStr.data(),
		             static_cast<int>(MeshNamesCStr.size()));

		// 인덱스 범위 검사
		SelectedMeshIndex = max(0, min(SelectedMeshIndex, static_cast<int>(AvailableStaticMeshes.size()) - 1));
	}
	else
	{
		ImGui::Text("No StaticMesh found!");
	}

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(100, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors") && !AvailableStaticMeshes.empty())
	{
		SpawnActors();
	}

	// 스폰 범위 설정
	ImGui::Text("Spawn Range:");
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Min##SpawnRange", &SpawnRangeMin, 0.1f, -50.0f, SpawnRangeMax - 0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Max##SpawnRange", &SpawnRangeMax, 0.1f, SpawnRangeMin + 0.1f, 50.0f);

	ImGui::Separator();
}

/**
 * @brief ObjectIterator를 사용하여 모든 StaticMesh 수집
 */
void UPrimitiveSpawnWidget::RefreshStaticMeshList()
{
	AvailableStaticMeshes.clear();
	StaticMeshNames.clear();

	// ObjectIterator를 사용해 모든 UStaticMesh 오브젝트 순회
	for (UStaticMesh* StaticMesh : MakeObjectRange<UStaticMesh>())
	{
		if (StaticMesh && StaticMesh->IsValidMesh())
		{
			AvailableStaticMeshes.push_back(StaticMesh);

			// StaticMesh 이름 생성 (경로나 에셋 이름 사용)
			FString MeshName = StaticMesh->GetAssetPathFileName();

			// 경로에서 파일명만 추출
			size_t LastSlash = MeshName.find_last_of("/\\");
			if (LastSlash != std::string::npos)
			{
				MeshName = MeshName.substr(LastSlash + 1);
			}

			StaticMeshNames.push_back(MeshName);
		}
	}

	// 선택된 인덱스 범위 검사
	if (SelectedMeshIndex >= static_cast<int32>(AvailableStaticMeshes.size()))
	{
		SelectedMeshIndex = 0;
	}
}

/**
 * @brief 선택된 StaticMesh로 Actor 생성
 */
void UPrimitiveSpawnWidget::SpawnActors() const
{
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ControlPanel: Actor를 생성할 레벨이 존재하지 않습니다");
		return;
	}

	if (AvailableStaticMeshes.empty() || SelectedMeshIndex < 0 || SelectedMeshIndex >= static_cast<int32>(
		AvailableStaticMeshes.size()))
	{
		UE_LOG("ControlPanel: 유효한 StaticMesh가 선택되지 않았습니다");
		return;
	}

	UStaticMesh* SelectedMesh = AvailableStaticMeshes[SelectedMeshIndex];
	const FString& MeshName = StaticMeshNames[SelectedMeshIndex];

	UE_LOG("ControlPanel: %s StaticMesh로 Actor를 %d개 생성 시도합니다", MeshName.c_str(), NumberOfSpawn);

	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
	{
		AStaticMeshActor* StaticMeshActor = CurrentLevel->SpawnActor<AStaticMeshActor>("StaticMeshActor");

		if (StaticMeshActor)
		{
			// 선택된 StaticMesh 설정
			StaticMeshActor->SetStaticMesh(SelectedMesh);

			// 범위 내 랜덤 위치 생성
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			StaticMeshActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			StaticMeshActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 %s Actor를 배치했습니다",
			       StaticMeshActor->GetActorLocation().X, StaticMeshActor->GetActorLocation().Y,
			       StaticMeshActor->GetActorLocation().Z,
			       MeshName.c_str());
		}
		else
		{
			UE_LOG("ControlPanel: Actor 배치에 실패했습니다 %d", i);
		}
	}
}

/**
 * @brief 파일 대화상자를 통해 OBJ 파일을 선택하고 로드
 */
void UPrimitiveSpawnWidget::LoadMeshFromFile()
{
	// Windows 파일 대화상자 열기
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;

		// FileOpenDialog 생성
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		                      IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// OBJ 파일 필터 설정
			COMDLG_FILTERSPEC rgSpec[] =
			{
				{L"OBJ Files", L"*.obj"},
				{L"All Files", L"*.*"}
			};
			pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
			pFileOpen->SetDefaultExtension(L"obj");

			// 대화상자 표시
			hr = pFileOpen->Show(NULL);

			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					if (SUCCEEDED(hr))
					{
						// Wide string을 일반 string으로 변환
						int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
						FString filePath(size_needed - 1, '\0');
						WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &filePath[0], size_needed, NULL, NULL);

						// 프로젝트 루트 경로 찾기 (Engine 폴더 포함)
						size_t enginePos = filePath.find("\\Engine\\");
						if (enginePos != std::string::npos)
						{
							// Engine\\ 다음부터 상대 경로로 변환
							filePath = filePath.substr(enginePos + 8); // "\\Engine\\" 길이만큼 제거
						}

						// 이미 로드된 메시인지 확인
						bool bAlreadyExists = false;
						for (const UStaticMesh* ExistingMesh : AvailableStaticMeshes)
						{
							if (ExistingMesh && ExistingMesh->GetAssetPathFileName() == filePath)
							{
								bAlreadyExists = true;
								UE_LOG("PrimitiveSpawnWidget: Mesh already loaded: %s", filePath.c_str());

								// 이미 존재하는 메시를 선택하도록 설정
								for (int32 i = 0; i < AvailableStaticMeshes.size(); ++i)
								{
									if (AvailableStaticMeshes[i] == ExistingMesh)
									{
										SelectedMeshIndex = i;
										break;
									}
								}
								break;
							}
						}

						if (!bAlreadyExists)
						{
							// AssetSubsystem을 통해 StaticMesh 로드
							UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
							UStaticMesh* LoadedMesh = nullptr;
							if (AssetSubsystem)
							{
								LoadedMesh = AssetSubsystem->LoadStaticMesh(filePath);
							}

							if (LoadedMesh)
							{
								UE_LOG("PrimitiveSpawnWidget: Successfully loaded mesh from: %s", filePath.data());

								// 메시 목록 갱신
								RefreshStaticMeshList();

								// 새로 로드된 메시를 선택하도록 설정
								for (int32 i = 0; i < AvailableStaticMeshes.size(); ++i)
								{
									if (AvailableStaticMeshes[i] == LoadedMesh)
									{
										SelectedMeshIndex = i;
										break;
									}
								}
							}
							else
							{
								UE_LOG("PrimitiveSpawnWidget: Failed to load mesh from: %s", filePath.c_str());
							}
						}

						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
}
