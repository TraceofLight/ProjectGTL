#include "pch.h"
#include "Runtime/Actor/Public/Actor.h"

#include "Runtime/Component/Public/ActorComponent.h"
#include "Runtime/Component/Public/SceneComponent.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

void AActor::EnsureRootComponent()
{
	// RootComponent가 없으면 DefaultSceneRoot 생성
	if (!RootComponent)
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(FName::FName_None);
		RootComponent->SetDisplayName("DefaultSceneRoot");
	}
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.Empty();
}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

const FVector& AActor::GetActorLocation() const
{
	if (!RootComponent)
	{
		static FVector ZeroVector(0.0f, 0.0f, 0.0f);
		UE_LOG_WARNING("AActor::GetActorLocation: %s has no RootComponent", GetName().ToString().c_str());
		return ZeroVector;
	}
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	if (!RootComponent)
	{
		static FVector ZeroVector(0.0f, 0.0f, 0.0f);
		UE_LOG_WARNING("AActor::GetActorRotation: %s has no RootComponent", GetName().ToString().c_str());
		return ZeroVector;
	}
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	if (!RootComponent)
	{
		static FVector OneVector(1.0f, 1.0f, 1.0f);
		UE_LOG_WARNING("AActor::GetActorScale3D: %s has no RootComponent", GetName().ToString().c_str());
		return OneVector;
	}
	return RootComponent->GetRelativeScale3D();
}

void AActor::Tick(float DeltaSeconds)
{
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->TickComponent();
		}
	}
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}

TArray<UPrimitiveComponent*> AActor::GetPrimitiveComponents() const
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	for (const auto& Component : OwnedComponents)
	{
		if (Component)
		{
			// PrimitiveComponent인지 확인
			UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component.Get());
			if (PrimitiveComp)
			{
				PrimitiveComponents.Add(PrimitiveComp);
			}
		}
	}

	return PrimitiveComponents;
}
