#include "pch.h"
#include "Runtime/Actor/Public/SquareActor.h"
#include "Runtime/Component/Mesh/Public/SquareComponent.h"

IMPLEMENT_CLASS(ASquareActor, AActor)

ASquareActor::ASquareActor()
{
	SquareComponent = CreateDefaultSubobject<USquareComponent>("SquareComponent");
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SquareComponent->SetOwner(this);
	SetRootComponent(SquareComponent);
}
