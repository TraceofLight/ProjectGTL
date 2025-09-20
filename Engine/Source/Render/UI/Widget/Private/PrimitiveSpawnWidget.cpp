#include "pch.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
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
		"Torus",
		"Cylinder",
		"Cone"
	};

	// 콤보박스 인덱스와 EPrimitiveType 매핑
	static const EPrimitiveType IndexToPrimitiveType[] = {
		EPrimitiveType::Sphere,
		EPrimitiveType::Cube,
		EPrimitiveType::Triangle,
		EPrimitiveType::Square,
		EPrimitiveType::Torus,
		EPrimitiveType::Cylinder,
		EPrimitiveType::Cone
	};

	// 현재 선택된 타입을 인덱스로 변환
	int TypeNumber = 0;
	for (int i = 0; i < 7; i++)
	{
		if (IndexToPrimitiveType[i] == SelectedPrimitiveType)
		{
			TypeNumber = i;
			break;
		}
	}

	ImGui::Text("Primitive Type:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120);
	ImGui::Combo("##PrimitiveType", &TypeNumber, PrimitiveTypes, 7);

	// ImGui가 받은 값을 반영
	if (TypeNumber >= 0 && TypeNumber < 7)
	{
		SelectedPrimitiveType = IndexToPrimitiveType[TypeNumber];
	}

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
		AStaticMeshActor* StaticMeshActor = CurrentLevel->SpawnActor<AStaticMeshActor>("StaticMeshActor");
		AActor* NewActor = nullptr;

		if (StaticMeshActor)
		{
			// AssetManager에서 프리미티브 타입에 맞는 StaticMesh 가져오기
			UAssetManager& AssetManager = UAssetManager::GetInstance();
			UStaticMesh* PrimitiveMesh = AssetManager.GetPrimitiveStaticMesh(SelectedPrimitiveType);

			if (PrimitiveMesh)
			{
				StaticMeshActor->SetStaticMesh(PrimitiveMesh);
				NewActor = StaticMeshActor;
			}
			else
			{
				// StaticMesh 로드 실패 시 액터 삭제
				CurrentLevel->DestroyActor(StaticMeshActor);
				UE_LOG_ERROR("프리미티브 StaticMesh를 찾을 수 없습니다: %s", EnumToString(SelectedPrimitiveType));
				continue;
			}
		}

		if (NewActor)
		{
			// 범위 내 랜덤 위치 생성
			float RandomX = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomY = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);
			float RandomZ = SpawnRangeMin + (static_cast<float>(rand()) / RAND_MAX) * (SpawnRangeMax - SpawnRangeMin);

			NewActor->SetActorLocation(FVector(RandomX, RandomY, RandomZ));

			// 임의의 스케일 (0.5 ~ 2.0 범위)
			float RandomScale = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 1.5f;
			NewActor->SetActorScale3D(FVector(RandomScale, RandomScale, RandomScale));

			UE_LOG("ControlPanel: (%.2f, %.2f, %.2f) 지점에 %s Actor를 배치했습니다",
				NewActor->GetActorLocation().X, NewActor->GetActorLocation().Y, NewActor->GetActorLocation().Z,
				EnumToString(SelectedPrimitiveType));
		}
		else
		{
			UE_LOG("ControlPanel: Actor 배치에 실패했습니다 %d", i);
		}
	}
}
