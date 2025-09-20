#include "pch.h"
#include "Runtime/Actor/Public/SphereActor.h"
#include "Runtime/Component/Mesh/Public/SphereComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetOwner(this);
	SetRootComponent(SphereComponent);
}

