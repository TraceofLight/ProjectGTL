#include "pch.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Public/StaticMeshComponent.h"
#include "Asset/Public/StaticMesh.h"
#include <algorithm>

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
	: AActor()
{
	// StaticMeshComponent 생성 및 RootComponent로 설정
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(StaticMeshComponent.Get());
}

AStaticMeshActor::AStaticMeshActor(UObject* InOuter)
	: AActor(InOuter)
{
	// StaticMeshComponent 생성 및 RootComponent로 설정
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(StaticMeshComponent.Get());
}

AStaticMeshActor::~AStaticMeshActor()
{
}

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

void AStaticMeshActor::Tick()
{
	Super::Tick();

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

FAABB AStaticMeshActor::GetLocalAABB() const
{
	UStaticMesh* StaticMesh = GetStaticMesh();
	if (StaticMesh)
	{
		return StaticMesh->GetLocalAABB();
	}

	// StaticMesh가 없으면 기본 AABB 반환
	return FAABB(FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
}

FAABB AStaticMeshActor::GetWorldAABB() const
{
	FAABB LocalAABB = GetLocalAABB();

	// 액터의 변환 행렬 가져오기
	const FVector& Location = GetActorLocation();
	const FVector& Scale = GetActorScale3D();

	// 로컬 AABB의 Min/Max 점을 월드 좌표로 변환
	FVector LocalMin = LocalAABB.Min;
	FVector LocalMax = LocalAABB.Max;

	// 스케일 적용
	LocalMin = FVector(LocalMin.X * Scale.X, LocalMin.Y * Scale.Y, LocalMin.Z * Scale.Z);
	LocalMax = FVector(LocalMax.X * Scale.X, LocalMax.Y * Scale.Y, LocalMax.Z * Scale.Z);

	// 위치 변환 적용
	FVector WorldMin = LocalMin + Location;
	FVector WorldMax = LocalMax + Location;

	// Min/Max가 뒤바뀔 수 있으므로 정리
	FVector FinalMin(
		std::min(WorldMin.X, WorldMax.X),
		std::min(WorldMin.Y, WorldMax.Y),
		std::min(WorldMin.Z, WorldMax.Z)
	);

	FVector FinalMax(
		std::max(WorldMin.X, WorldMax.X),
		std::max(WorldMin.Y, WorldMax.Y),
		std::max(WorldMin.Z, WorldMax.Z)
	);

	return FAABB(FinalMin, FinalMax);
}
