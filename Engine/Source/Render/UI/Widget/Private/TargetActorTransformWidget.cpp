#include "pch.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Asset/Public/StaticMesh.h"
#include "Runtime/Core/Public/ObjectIterator.h"
#include "Material/Public/Material.h"

UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Tranform Widget")
{
}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::Initialize()
{
	// Do Nothing Here
}

void UTargetActorTransformWidget::Update()
{
	// Level memory는 현재 따로 출력하지 않음
	// LevelMemoryByte = CurrentLevel->GetAllocatedBytes();
	// LevelObjectCount = CurrentLevel->GetAllocatedCount();
}

/**
 * @brief Actor에 대한 속성값을 로드하는 함수
 * 원래 Update() 함수에 존재했으나, Update와 Render 시점 차이에 따른 Actor 상태 차이로 인한 문제가 발생
 * 최대한 Render 직전에 업데이트 할 수 있도록 호출 지점을 조절한 상태
 */
void UTargetActorTransformWidget::SetActorProperties()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	TObjectPtr<ULevel> CurrentLevel = WorldSS->GetCurrentLevel();

	if (CurrentLevel)
	{
		UEditor* Editor = WorldSS->GetEditor();
		if (!Editor)
		{
			return;
		}

		TObjectPtr<AActor> CurrentSelectedActor = TObjectPtr(Editor->GetSelectedActor());

		// Update Current Selected Actor
		if (SelectedActor != CurrentSelectedActor)
		{
			SelectedActor = CurrentSelectedActor;
		}

		// Get Current Selected Actor Information
		if (SelectedActor)
		{
			UpdateTransformFromActor();
		}

		RefreshStaticMeshList();
		RefreshMaterialList();
	}
}

void UTargetActorTransformWidget::RenderWidget()
{
	SetActorProperties();

	if (SelectedActor)
	{
		ImGui::Separator();
		ImGui::Text("Transform");

		ImGui::Spacing();

		bPositionChanged |= ImGui::DragFloat3("Location", &EditLocation.X, 0.1f);
		bRotationChanged |= ImGui::DragFloat3("Rotation", &EditRotation.X, 0.1f);

		// Uniform Scale 옵션
		bool bUniformScale = SelectedActor->IsUniformScale();
		if (bUniformScale)
		{
			float UniformScale = EditScale.X;

			if (ImGui::DragFloat("Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
			{
				EditScale = FVector(UniformScale, UniformScale, UniformScale);
				bScaleChanged = true;
			}
		}
		else
		{
			bScaleChanged |= ImGui::DragFloat3("Scale", &EditScale.X, 0.1f);
		}

		ImGui::Checkbox("Uniform Scale", &bUniformScale);

		SelectedActor->SetUniformScale(bUniformScale);

		// StaticMesh 변경 UI (StaticMeshActor인 경우에만 표시)
		if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(SelectedActor))
		{
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Text("StaticMesh:");
			ImGui::SameLine();

			if (!AvailableStaticMeshes.IsEmpty())
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200);

				// StaticMesh 이름들을 const char* 배열로 변환
				TArray<const char*> MeshNamesCStr;
				for (const FString& Name : StaticMeshNames)
				{
					MeshNamesCStr.Add(Name.c_str());
				}

				if (ImGui::Combo("##StaticMeshSelect", &SelectedMeshIndex, MeshNamesCStr.GetData(),
				                 static_cast<int>(MeshNamesCStr.Num())))
				{
					bMeshChanged = true;
				}

				// 인덱스 범위 검사
				SelectedMeshIndex = max(0, min(SelectedMeshIndex, AvailableStaticMeshes.Num() - 1));

				ImGui::Separator();
				ImGui::Text("Materials");

				if (!AvailableMaterials.IsEmpty())
				{
					// 머티리얼 이름들을 const char* 배열로 변환 ("None" 옵션 추가)
					TArray<const char*> MaterialNamesCStr;
					MaterialNamesCStr.Add("None"); // 기본 옵션
					for (const FString& Name : AvailableMaterialNames)
					{
						MaterialNamesCStr.Add(Name.c_str());
					}

					// 각 머티리얼 슬롯에 대해 콤보박스 생성
					for (int32 SlotIndex = 0; SlotIndex < SelectedActorMaterialSlotCount; ++SlotIndex)
					{
						ImGui::Text("Slot %d:", SlotIndex);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(200);

						// 인덱스를 +1 해서 "None"을 0으로 처리
						int32 CurrentSelection = SelectedMaterialIndices[SlotIndex] + 1;

						// 콤보박스 ID를 고유하게 만들기 위해 SlotIndex 사용
						FString ComboID = "##MaterialSlot" + std::to_string(SlotIndex);
						if (ImGui::Combo(ComboID.c_str(), &CurrentSelection, MaterialNamesCStr.GetData(),
						                 MaterialNamesCStr.Num()))
						{
							// 선택이 변경되면 MaterialOverrideMap 업데이트
							UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
							if (StaticMeshComponent)
							{
								if (CurrentSelection == 0) // "None" 선택
								{
									// 오버라이드 제거 (기본 머티리얼 사용)
									StaticMeshComponent->RemoveMaterialOverride(SlotIndex);
									SelectedMaterialIndices[SlotIndex] = -1;
								}
								else
								{
									// 새로운 머티리얼로 오버라이드
									int32 MaterialIndex = CurrentSelection - 1;
									if (MaterialIndex >= 0 && MaterialIndex < AvailableMaterials.Num())
									{
										UMaterialInterface* SelectedMaterial = AvailableMaterials[MaterialIndex];
										StaticMeshComponent->SetMaterialOverride(SlotIndex, SelectedMaterial);
										SelectedMaterialIndices[SlotIndex] = MaterialIndex;
									}
								}
							}
						}
					}
				}
			}
			else
			{
				ImGui::Text("No StaticMesh found!");
			}
		}
	}

	ImGui::Separator();
}

/**
 * @brief Render에서 체크된 내용으로 인해 이후 변경되어야 할 내용이 있다면 Change 처리
 */
void UTargetActorTransformWidget::PostProcess()
{
	if (bPositionChanged || bRotationChanged || bScaleChanged)
	{
		ApplyTransformToActor();
	}

	if (bMeshChanged)
	{
		ApplyStaticMeshToActor();
		bMeshChanged = false;
	}
}

void UTargetActorTransformWidget::UpdateTransformFromActor()
{
	if (SelectedActor)
	{
		EditLocation = SelectedActor->GetActorLocation();
		EditRotation = SelectedActor->GetActorRotation();
		EditScale = SelectedActor->GetActorScale3D();
	}
}

void UTargetActorTransformWidget::ApplyTransformToActor() const
{
	if (SelectedActor)
	{
		SelectedActor->SetActorLocation(EditLocation);
		SelectedActor->SetActorRotation(EditRotation);
		SelectedActor->SetActorScale3D(EditScale);
	}
}

/**
 * @brief ObjectIterator를 사용하여 모든 StaticMesh 수집
 */
void UTargetActorTransformWidget::RefreshStaticMeshList()
{
	AvailableStaticMeshes.Empty();
	StaticMeshNames.Empty();

	// ObjectIterator를 사용해 모든 UStaticMesh 오브젝트 순회
	for (UStaticMesh* StaticMesh : MakeObjectRange<UStaticMesh>())
	{
		if (StaticMesh && StaticMesh->IsValidMesh())
		{
			AvailableStaticMeshes.Add(StaticMesh);

			// StaticMesh 이름 생성 (경로나 에셋 이름 사용)
			FString MeshName = StaticMesh->GetAssetPathFileName();

			// 경로에서 파일명만 추출
			size_t LastSlash = MeshName.find_last_of("/\\");
			if (LastSlash != std::string::npos)
			{
				MeshName = MeshName.substr(LastSlash + 1);
			}

			StaticMeshNames.Add(MeshName);
		}
	}

	// 현재 선택된 액터의 StaticMesh에 해당하는 인덱스 찾기
	SelectedMeshIndex = 0; // 기본값
	if (SelectedActor)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(SelectedActor);
		if (StaticMeshActor)
		{
			UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
			if (StaticMeshComponent)
			{
				UStaticMesh* CurrentMesh = StaticMeshComponent->GetStaticMesh();
				if (CurrentMesh)
				{
					// 현재 메시와 일치하는 인덱스 찾기
					for (int32 i = 0; i < static_cast<int32>(AvailableStaticMeshes.Num()); ++i)
					{
						if (AvailableStaticMeshes[i] == CurrentMesh)
						{
							SelectedMeshIndex = i;
							break;
						}
					}
				}
			}
		}
	}

	// 선택된 인덱스 범위 검사
	if (SelectedMeshIndex >= static_cast<int32>(AvailableStaticMeshes.Num()))
	{
		SelectedMeshIndex = 0;
	}
}

/**
 * @brief 선택된 StaticMesh를 현재 액터에 적용
 */
void UTargetActorTransformWidget::ApplyStaticMeshToActor()
{
	if (!SelectedActor || AvailableStaticMeshes.IsEmpty() || SelectedMeshIndex < 0 ||
		SelectedMeshIndex >= AvailableStaticMeshes.Num())
	{
		return;
	}

	AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(SelectedActor);
	if (!StaticMeshActor)
	{
		return;
	}

	UStaticMesh* SelectedMesh = AvailableStaticMeshes[SelectedMeshIndex];
	const FString& MeshName = StaticMeshNames[SelectedMeshIndex];

	StaticMeshActor->SetStaticMesh(SelectedMesh);

	UE_LOG("TargetActorTransformWidget: Applied StaticMesh '%s' to actor", MeshName.c_str());
}

void UTargetActorTransformWidget::RefreshMaterialList()
{
	AvailableMaterials.Empty();
	AvailableMaterialNames.Empty();
	// ObjectIterator를 사용해 모든 UMaterial 및 Material Name 수집
	for (UMaterial* Material : MakeObjectRange<UMaterial>())
	{
		if (Material)
		{
			AvailableMaterials.Add(Material);
			FString MaterialName = Material->GetName().ToString();
			AvailableMaterialNames.Add(MaterialName);
		}
	}

	// 선택된 액터의 머티리얼 슬롯 개수 및 인덱스 초기화
	SelectedActorMaterialSlotCount = 0;
	SelectedMaterialIndices.Empty();
	if (UStaticMesh* CurrentMesh = ExtractUStaticMeshFromActor(SelectedActor))
	{
		const TArray<UMaterialInterface*>& MaterialSlots = CurrentMesh->GetMaterialSlots();
		SelectedActorMaterialSlotCount = static_cast<int32>(MaterialSlots.Num());
		SelectedMaterialIndices.SetNum(SelectedActorMaterialSlotCount, -1); // -1로 초기화 (None 선택)

		// 각 머티리얼 슬롯에 대해 현재 적용된 머티리얼의 인덱스 찾기
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(SelectedActor);
		UStaticMeshComponent* StaticMeshComponent = StaticMeshActor
			                                            ? StaticMeshActor->GetStaticMeshComponent()
			                                            : nullptr;

		for (int32 SlotIndex = 0; SlotIndex < SelectedActorMaterialSlotCount; ++SlotIndex)
		{
			UMaterialInterface* CurrentMaterial = nullptr;
			if (StaticMeshComponent)
			{
				// 먼저 오버라이드된 머티리얼 체크
				CurrentMaterial = StaticMeshComponent->GetMaterialOverride(SlotIndex);
			}

			// 오버라이드가 없으면 기본 머티리얼 사용
			if (!CurrentMaterial)
			{
				CurrentMaterial = MaterialSlots[SlotIndex];
			}

			// 현재 머티리얼에 대응하는 인덱스 찾기
			if (CurrentMaterial)
			{
				bool bFoundMaterial = false;
				for (int32 MatIndex = 0; MatIndex < static_cast<int32>(AvailableMaterials.Num()); ++MatIndex)
				{
					if (AvailableMaterials[MatIndex] == CurrentMaterial)
					{
						SelectedMaterialIndices[SlotIndex] = MatIndex;
						bFoundMaterial = true;
						break;
					}
				}
				// 머티리얼을 찾지 못한 경우 -1로 설정 (None)
				if (!bFoundMaterial)
				{
					SelectedMaterialIndices[SlotIndex] = -1;
				}
			}
			else
			{
				// 머티리얼이 없는 경우 -1로 설정 (None)
				SelectedMaterialIndices[SlotIndex] = -1;
			}
		}
	}
}

UStaticMesh* UTargetActorTransformWidget::ExtractUStaticMeshFromActor(AActor* InActor)
{
	if (!InActor)
	{
		return nullptr;
	}
	AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(InActor);
	if (!StaticMeshActor)
	{
		return nullptr;
	}
	UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
	if (!StaticMeshComponent)
	{
		return nullptr;
	}
	return StaticMeshComponent->GetStaticMesh();
}
