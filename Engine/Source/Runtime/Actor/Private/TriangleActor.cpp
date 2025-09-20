#include "pch.h"
#include "Runtime/Actor/Public/TriangleActor.h"
#include "Runtime/Component/Mesh/Public/TriangleComponent.h"

IMPLEMENT_CLASS(ATriangleActor, AActor)

ATriangleActor::ATriangleActor()
{
	TriangleComponent = CreateDefaultSubobject<UTriangleComponent>("TriangleComponent");
	TriangleComponent->SetRelativeRotation({ 90, 0, 0 });
	TriangleComponent->SetOwner(this);
	SetRootComponent(TriangleComponent);
}
