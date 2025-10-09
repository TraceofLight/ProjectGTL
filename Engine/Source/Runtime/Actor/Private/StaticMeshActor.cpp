#include "pch.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Asset/Public/StaticMesh.h"
#include "Runtime/Component/Public/BillBoardComponent.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
	AStaticMeshActor::GetClass()->IncrementGenNumber();

	// StaticMeshComponent 생성 및 RootComponent로 설정
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(FName::FName_None);
	StaticMeshComponent->SetDisplayName("StaticMeshComponent_" + to_string(UStaticMeshComponent::GetNextGenNumber()));

	SetRootComponent(StaticMeshComponent);
}

AStaticMeshActor::AStaticMeshActor(UObject* InOuter)
	: AActor(InOuter)
{
	AStaticMeshActor::GetClass()->IncrementGenNumber();

	// StaticMeshComponent 생성 및 RootComponent로 설정
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(
		"StaticMeshComponent_" + to_string(UStaticMeshComponent::GetNextGenNumber()));

	SetRootComponent(StaticMeshComponent);
}

// XXX(KHJ): 이거 기본 소멸자 해도 되나? 멤버들 스마트 포인터라 안 지워줘도 알아서 날아가나..
AStaticMeshActor::~AStaticMeshActor() = default;

void AStaticMeshActor::BeginPlay()
{
	Super::BeginPlay();

	// StaticMeshComponent 초기화
	if (StaticMeshComponent)
	{
		// 추가 초기화 로직이 필요하면 여기에 구현
	}
}

void AStaticMeshActor::EndPlay()
{
	Super::EndPlay();
}

void AStaticMeshActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// StaticMeshActor는 일반적으로 정적이므로 특별한 업데이트 로직이 필요하지 않음
	// 필요시 여기에 추가
}

void AStaticMeshActor::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(InStaticMesh);
	}
}

UStaticMesh* AStaticMeshActor::GetStaticMesh() const
{
	if (StaticMeshComponent)
	{
		return StaticMeshComponent->GetStaticMesh();
	}
	return nullptr;
}
