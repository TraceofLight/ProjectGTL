#pragma once
#include "Widget.h"

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
	EPrimitiveType SelectedPrimitiveType = EPrimitiveType::Sphere;
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -5.0f;
	float SpawnRangeMax = 5.0f;
};
