#include "pch.h"
#include "Runtime/Component/Public/SceneComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

USceneComponent::~USceneComponent()
{
	// 부모에서 자신을 제거
	if (ParentAttachment != nullptr)
	{
		ParentAttachment->RemoveChild(this);
		ParentAttachment = nullptr;
	}

	// 자식 컴포넌트들의 부모 참조 해제 및 삭제
	for (USceneComponent* child : Children)
	{
		if (child != nullptr)
		{
			child->ParentAttachment = nullptr; // 부모 참조 해제
			delete child;
		}
	}
	Children.clear();
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
	// 버그 수정: this가 아닌 ChildDeleted를 제거해야 함
	Children.erase(std::remove(Children.begin(), Children.end(), ChildDeleted), Children.end());
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

