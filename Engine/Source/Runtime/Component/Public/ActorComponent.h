#pragma once
#include "Runtime/Core/Public/Object.h"

class AActor;

UCLASS()
class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorComponent, UObject)

public:
	UActorComponent();
	~UActorComponent();
	// Legacy render function removed

	virtual void BeginPlay();
	virtual void TickComponent();
	virtual void EndPlay();

	EComponentType GetComponentType() { return ComponentType; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const {return Owner;}

	EComponentType GetComponentType() const { return ComponentType; }

	// Component Active State
	bool IsActive() const { return bIsActive; }
	void SetActive(bool bInActive) { bIsActive = bInActive; }
	virtual void Activate() { bIsActive = true; }
	virtual void Deactivate() { bIsActive = false; }

protected:
	EComponentType ComponentType;

private:
	AActor* Owner;
	bool bIsActive = true; // 기본값으로 활성 상태
};
