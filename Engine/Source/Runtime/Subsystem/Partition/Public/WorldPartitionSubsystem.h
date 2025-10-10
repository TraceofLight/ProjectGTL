#pragma once

#include "Runtime/Subsystem/Public/EngineSubsystem.h"

class UPrimitiveComponent;
class FBVH;

UCLASS()
class UWorldPartitionSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UWorldPartitionSubsystem, UEngineSubsystem)

public:
	UWorldPartitionSubsystem();
	virtual ~UWorldPartitionSubsystem() override;

	// USubsystem interface
	virtual void Initialize() override;
	virtual void Deinitialize() override;

	// World Partitioning specific functions
	void RegisterPartitionedComponent(UPrimitiveComponent* Component);
	void UnregisterPartitionedComponent(UPrimitiveComponent* Component);
	void UpdatePartitionedComponent(UPrimitiveComponent* Component);

	// TODO: Add more functions for loading/unloading cells, streaming, etc.

private:
	TMap<UPrimitiveComponent*, int32> ComponentIndexMap;

	// TODO: Parition Structure Interface
	FBVH* BVH;
};
