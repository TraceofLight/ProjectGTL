#pragma once
#include "Widget.h"

class UStaticMesh;

UCLASS()
class UPrimitiveSpawnWidget
	:public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveSpawnWidget, UWidget)

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SpawnActors() const;

	// Special Member Function
	UPrimitiveSpawnWidget();
	~UPrimitiveSpawnWidget() override;

private:
	// StaticMesh 목록 관련
	void RefreshStaticMeshList();
	void LoadMeshFromFile();
	TArray<UStaticMesh*> AvailableStaticMeshes;
	TArray<FString> StaticMeshNames;
	int32 SelectedMeshIndex = 0;

	// 스폰 설정
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -5.0f;
	float SpawnRangeMax = 5.0f;
};
