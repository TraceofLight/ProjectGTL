#pragma once
#include "Widget.h"

class UStaticMesh;

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
};
