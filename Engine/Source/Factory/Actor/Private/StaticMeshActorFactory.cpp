#include "pch.h"
#include "Factory/Actor/Public/StaticMeshActorFactory.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Asset/Public/StaticMesh.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"

IMPLEMENT_CLASS(UStaticMeshActorFactory, UActorFactory)

UStaticMeshActorFactory::UStaticMeshActorFactory()
	: UActorFactory()
{
	// 기본 설정
	CurrentStaticMesh = nullptr;

	// Support class setting
	SupportedClass = AStaticMeshActor::StaticClass();
	Description = "StaticMeshActorFactory";

	// Register self
	RegisterFactory(TObjectPtr<UFactory>(this));
}

TObjectPtr<AStaticMeshActor> UStaticMeshActorFactory::CreateStaticMeshActor(
	TObjectPtr<UObject> InWorld,
	TObjectPtr<ULevel> InLevel,
	UStaticMesh* InStaticMesh,
	const FTransform& InTransform,
	uint32 InObjectFlags)
{
	if (!InStaticMesh)
	{
		UE_LOG_ERROR("UStaticMeshActorFactory::CreateStaticMeshActor - StaticMesh가 null입니다.");
		return nullptr;
	}

	// 현재 StaticMesh 설정
	CurrentStaticMesh = InStaticMesh;

	// Actor 생성
	TObjectPtr<AActor> NewActor = CreateActor(InWorld, InLevel, InTransform, InObjectFlags);

	// StaticMeshActor로 캐스팅
	TObjectPtr<AStaticMeshActor> StaticMeshActor = Cast<AStaticMeshActor>(NewActor);

	if (StaticMeshActor)
	{
		UE_LOG_SUCCESS("StaticMeshActor 생성 성공: %s", InStaticMesh->GetAssetPathFileName().c_str());
	}
	else
	{
		UE_LOG_ERROR("StaticMeshActor 캐스팅 실패");
	}

	// 임시 변수 정리
	CurrentStaticMesh = nullptr;

	return StaticMeshActor;
}

TObjectPtr<AStaticMeshActor> UStaticMeshActorFactory::CreateStaticMeshActorFromFile(
	TObjectPtr<UObject> InWorld,
	TObjectPtr<ULevel> InLevel,
	const FString& InObjFilePath,
	const FTransform& InTransform,
	uint32 InObjectFlags)
{
	// AssetSubsystem을 통해 StaticMesh 로드
	UAssetSubsystem* AssetSubsystem = GEngine->GetEngineSubsystem<UAssetSubsystem>();
	UStaticMesh* StaticMesh = nullptr;
	if(AssetSubsystem)
	{
		StaticMesh = AssetSubsystem->LoadStaticMesh(InObjFilePath);
	}

	if (!StaticMesh)
	{
		UE_LOG_ERROR("UStaticMeshActorFactory::CreateStaticMeshActorFromFile - StaticMesh 로드 실패: %s",
		             InObjFilePath.c_str());
		return nullptr;
	}

	// StaticMesh를 사용하여 Actor 생성
	return CreateStaticMeshActor(InWorld, InLevel, StaticMesh, InTransform, InObjectFlags);
}

TObjectPtr<AActor> UStaticMeshActorFactory::CreateNewActor()
{
	TObjectPtr<AStaticMeshActor> NewStaticMeshActor =
		Cast<AStaticMeshActor>(AStaticMeshActor::CreateDefaultObjectAStaticMeshActor());

	NewStaticMeshActor->SetDisplayName("StaticMeshActor_" + to_string(AStaticMeshActor::GetNextGenNumber()));

	if (NewStaticMeshActor && CurrentStaticMesh)
	{
		// StaticMesh 설정
		NewStaticMeshActor->SetStaticMesh(CurrentStaticMesh);
	}

	return Cast<AActor>(NewStaticMeshActor);
}

void UStaticMeshActorFactory::PostCreateActor(AActor* InActor, const FTransform& InTransform)
{
	UActorFactory::PostCreateActor(InActor, InTransform);

	// 추가적인 StaticMeshActor 초기화 로직
	AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(InActor);
	if (StaticMeshActor)
	{
		// 위치, 회전, 스케일 설정
		StaticMeshActor->SetActorLocation(InTransform.Location);
		StaticMeshActor->SetActorRotation(InTransform.Rotation);
		StaticMeshActor->SetActorScale3D(InTransform.Scale);

		if (InTransform == FTransform())
		{
			UE_LOG_SUCCESS("StaticMeshActorFactory: StaticMeshActor 초기화 완료");
		}
		else
		{
			UE_LOG_SUCCESS("StaticMeshActor 초기화 완료 - 위치: (%f, %f, %f)",
						InTransform.Location.X, InTransform.Location.Y, InTransform.Location.Z);
		}
	}
}
