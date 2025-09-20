#pragma once
#include "Factory/Actor/Public/ActorFactory.h"

class AStaticMeshActor;
class UStaticMesh;

/**
 * @brief StaticMeshActor 생성을 위한 팩토리 클래스
 * StaticMesh 애셋을 사용하여 StaticMeshActor를 생성하고 초기화
 */
UCLASS()
class UStaticMeshActorFactory : public UActorFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshActorFactory, UActorFactory)

public:
	UStaticMeshActorFactory();
	~UStaticMeshActorFactory() override = default;

	// StaticMesh를 사용하여 Actor 생성
	TObjectPtr<AStaticMeshActor> CreateStaticMeshActor(
		TObjectPtr<UObject> InWorld,
		TObjectPtr<ULevel> InLevel,
		UStaticMesh* InStaticMesh,
		const FTransform& InTransform = FTransform(),
		uint32 InObjectFlags = 0);

	// OBJ 파일 경로를 사용하여 Actor 생성
	TObjectPtr<AStaticMeshActor> CreateStaticMeshActorFromFile(
		TObjectPtr<UObject> InWorld,
		TObjectPtr<ULevel> InLevel,
		const FString& InObjFilePath,
		const FTransform& InTransform = FTransform(),
		uint32 InObjectFlags = 0);

protected:
	TObjectPtr<AActor> CreateNewActor() override;
	void PostCreateActor(AActor* InActor, const FTransform& InTransform) override;

private:
	// 현재 설정된 StaticMesh (팩토리에서 사용)
	UStaticMesh* CurrentStaticMesh = nullptr;
};
