#pragma once
#include "Widget.h"

class UStaticMesh;
class UMaterialInterface;

class UTargetActorTransformWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void SetActorProperties();
	void RenderWidget() override;
	void PostProcess() override;

	void UpdateTransformFromActor();
	void ApplyTransformToActor() const;

	// Special Member Function
	UTargetActorTransformWidget();
	~UTargetActorTransformWidget() override;

private:
	TObjectPtr<AActor> SelectedActor;

	FVector EditLocation;
	FVector EditRotation;
	FVector EditScale;
	bool bScaleChanged;
	bool bRotationChanged;
	bool bPositionChanged;
	uint64 LevelMemoryByte;
	uint32 LevelObjectCount;

	// StaticMesh 변경 관련
	void RefreshStaticMeshList();
	void ApplyStaticMeshToActor();
	TArray<UStaticMesh*> AvailableStaticMeshes;
	TArray<FString> StaticMeshNames;
	int32 SelectedMeshIndex = 0;
	bool bMeshChanged = false;

	// 머티리얼 변경 관련
	void RefreshMaterialList();
	TArray<UMaterialInterface*> AvailableMaterials;
	TArray<FString> AvailableMaterialNames;
	int32 SelectedActorMaterialSlotCount; // 선택된 스태틱 메시 액터가 가진 원본 스태틱 메시 애셋의 머티리얼 슬롯 개수
	TArray<int32> SelectedMaterialIndices; // 선택된 액터의 머티리얼 슬롯별로 선택된 머티리얼 인덱스

	// 헬퍼 함수들
	UStaticMesh* ExtractUStaticMeshFromActor(AActor* InActor);
};
