#include "pch.h"
#include "Runtime/Component/Public/ActorComponent.h"

IMPLEMENT_CLASS(UActorComponent, UObject)

UActorComponent::UActorComponent()
{
	ComponentType = EComponentType::Actor;
}

UActorComponent::~UActorComponent()
{
	SetOuter(nullptr);
}

void UActorComponent::BeginPlay()
{

}

void UActorComponent::TickComponent()
{
	// 비활성 상태일 때는 Tick 실행하지 않음
	if (!bIsActive)
	{
		return;
	}
	
	// 하위 클래스에서 구현
}

void UActorComponent::EndPlay()
{

}
