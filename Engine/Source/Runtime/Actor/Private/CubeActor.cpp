#include "pch.h"
#include "Runtime/Actor/Public/CubeActor.h"

#include "Runtime/Component/Mesh/Public/CubeComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	CubeComponent->SetOwner(this);
	SetRootComponent(CubeComponent);
}
