#include "pch.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

#include "Runtime/Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Asset/Public/StaticMesh.h"
#include "Runtime/Core/Public/ObjectIterator.h"

UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Tranform Widget")
{
	RefreshStaticMeshList();
}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::Initialize()
{
	// Do Nothing Here
}

void UTargetActorTransformWidget::Update()
{
	// 매 프레임 Level의 선택된 Actor를 확인해서 정보 반영
	// TODO(KHJ): 적절한 위치를 찾을 것
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	LevelMemoryByte = CurrentLevel->GetAllocatedBytes();
	LevelObjectCount = CurrentLevel->GetAllocatedCount();

	if (CurrentLevel)
	{
		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();

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
		else if (CurrentSelectedActor)
		{
			SelectedActor = nullptr;
		}
	}
}

void UTargetActorTransformWidget::RenderWidget()
{
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

			if (!AvailableStaticMeshes.empty())
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200);

				// StaticMesh 이름들을 const char* 배열로 변환
				TArray<const char*> MeshNamesCStr;
				for (const FString& Name : StaticMeshNames)
				{
					MeshNamesCStr.push_back(Name.c_str());
				}

				if (ImGui::Combo("##StaticMeshSelect", &SelectedMeshIndex, MeshNamesCStr.data(), static_cast<int>(MeshNamesCStr.size())))
				{
					bMeshChanged = true;
				}

				// 인덱스 범위 검사
				SelectedMeshIndex = max(0, min(SelectedMeshIndex, static_cast<int>(AvailableStaticMeshes.size()) - 1));
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
		RefreshStaticMeshList();
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
 * @brief 선택된 StaticMesh를 현재 액터에 적용
 */
void UTargetActorTransformWidget::ApplyStaticMeshToActor()
{
	if (!SelectedActor || AvailableStaticMeshes.empty() || SelectedMeshIndex < 0 || SelectedMeshIndex >= static_cast<int32>(AvailableStaticMeshes.size()))
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
