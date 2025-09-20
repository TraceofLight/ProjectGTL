#include "pch.h"
#include "Runtime/Component/Public/SceneComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	//부모의 조상중에 내 자식이 있으면 순환참조 -> 스택오버플로우 일어남.
	for (USceneComponent* Ancester = NewParent; NewParent; Ancester = NewParent->ParentAttachment)
	{
		if (NewParent == this) //조상중에 내 자식이 있다면 조상중에 내가 있을 것임.
			return;
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	ParentAttachment = NewParent;

	MarkAsDirty();

}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	Children.erase(std::remove(Children.begin(), Children.end(), this), Children.end());
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	OnTransformChanged();

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}

