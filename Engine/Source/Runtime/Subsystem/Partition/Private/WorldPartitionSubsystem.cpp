#include "pch.h"
#include "Runtime/Subsystem/Partition/Public/WorldPartitionSubsystem.h"

#include "Physics/Public/BVH.h"

UWorldPartitionSubsystem::UWorldPartitionSubsystem()
{
}

UWorldPartitionSubsystem::~UWorldPartitionSubsystem()
{
}

void UWorldPartitionSubsystem::Initialize()
{
	Super::Initialize();

	BVH = new FBVH();
}

void UWorldPartitionSubsystem::Deinitialize()
{
	Super::Deinitialize();

	delete BVH;
}

void UWorldPartitionSubsystem::RegisterPartitionedComponent(UPrimitiveComponent* Component)
{
	ComponentIndexMap.Add(Component, BVH->Add(Component));
}

void UWorldPartitionSubsystem::UnregisterPartitionedComponent(UPrimitiveComponent* Component)
{
	BVH->Remove(ComponentIndexMap[Component]);
	ComponentIndexMap.Remove(Component);
}

void UWorldPartitionSubsystem::UpdatePartitionedComponent(UPrimitiveComponent* Component)
{
	BVH->Update(ComponentIndexMap[Component]);
}
