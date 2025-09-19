#include "pch.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/SquareActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Asset/Public/StaticMesh.h"

UPrimitiveSpawnWidget::UPrimitiveSpawnWidget()
	: UWidget("Primitive Spawn Widget")
{
}

UPrimitiveSpawnWidget::~UPrimitiveSpawnWidget() = default;

void UPrimitiveSpawnWidget::Initialize()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::Update()
{
	// Do Nothing Here
}

void UPrimitiveSpawnWidget::RenderWidget()
{
	ImGui::Text("Primitive Actor 생성");
	ImGui::Spacing();

	// Primitive 타입 선택 DropDown
	const char* PrimitiveTypes[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"Square",
		"StaticMesh (test.obj)"
	};

	// None을 고려한 Enum 변환 처리
	int TypeNumber = static_cast<int>(SelectedPrimitiveType) - 1;

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &TypeNumber, PrimitiveTypes, 5);

	// ImGui가 받은 값을 반영
	SelectedPrimitiveType = static_cast<EPrimitiveType>(TypeNumber + 1);

	// Spawn 버튼과 개수 입력
	ImGui::Text("Number of Spawn:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##NumberOfSpawn", &NumberOfSpawn);
	NumberOfSpawn = max(1, NumberOfSpawn);
	NumberOfSpawn = min(100, NumberOfSpawn);

	ImGui::SameLine();
	if (ImGui::Button("Spawn Actors"))
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
 * @brief Actor 생성 함수
 * 난수를 활용한 Range, Size, Rotion 값 생성으로 Actor Spawn
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

	UE_LOG("ControlPanel: %s 타입의 Actor를 %d개 생성 시도합니다",
		EnumToString(SelectedPrimitiveType), NumberOfSpawn);

	// 지정된 개수만큼 액터 생성
	for (int32 i = 0; i < NumberOfSpawn; i++)
	{
		AActor* NewActor = nullptr;

		// 타입에 따라 액터 생성
		if (SelectedPrimitiveType == EPrimitiveType::Cube)
		{
			NewActor = CurrentLevel->SpawnActor<ACubeActor>();
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Sphere)
		{
			NewActor = CurrentLevel->SpawnActor<ASphereActor>();
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Triangle)
		{
			NewActor = CurrentLevel->SpawnActor<ATriangleActor>();
		}
		else if (SelectedPrimitiveType == EPrimitiveType::Square)
		{
			NewActor = CurrentLevel->SpawnActor<ASquareActor>();
		}
		else if (SelectedPrimitiveType == EPrimitiveType::StaticMesh)
		{
			AStaticMeshActor* StaticMeshActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
			if (StaticMeshActor)
			{
				// AssetManager에서 test.obj를 가져와서 설정
				UAssetManager& AssetManager = UAssetManager::GetInstance();
				UStaticMesh* TestMesh = AssetManager.GetStaticMesh("Data/Cylinder.obj");
				if (TestMesh)
				{
					StaticMeshActor->SetStaticMesh(TestMesh);
				}
				NewActor = StaticMeshActor;
			}
		}

		if (NewActor)
		{
			// StaticMesh의 경우 0,0,0 위치에 배치, 다른 프리미티브는 랜덤 위치
			if (SelectedPrimitiveType == EPrimitiveType::StaticMesh)
			{
				NewActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
			}
			else
			{
				// 범위 내 랜덤 위치
				float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
				float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
				float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

				NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));
			}

			// StaticMesh의 경우 1,1,1 스케일, 다른 프리미티브는 랜덤 스케일
			if (SelectedPrimitiveType == EPrimitiveType::StaticMesh)
			{
				NewActor->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
			}
			else
			{
				// 임의의 스케일 (0.5 ~ 2.0 범위)
				float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
				NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));
			}

			if (SelectedPrimitiveType == EPrimitiveType::StaticMesh)
			{
				UE_LOG("ControlPanel: (0.0, 0.0, 0.0) 지점에 StaticMesh Actor를 배치했습니다");
			}
			else
			{
				UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 Actor를 배치했습니다",
					NewActor->GetActorLocation().X, NewActor->GetActorLocation().Y, NewActor->GetActorLocation().Z);
			}
		}
		else
		{
			UE_LOG("ControlPanel: Actor 배치에 실패했습니다 %d", i);
		}
	}
}
