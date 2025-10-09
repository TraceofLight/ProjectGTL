#pragma once
#include "Runtime/Actor/Public/Actor.h"
#include "Physics/Public/AABB.h"

class UStaticMeshComponent;

/**
 * @brief StaticMesh를 렌더링하는 액터 클래스
 * OBJ 파일에서 로드된 3D 모델을 화면에 표시하는 역할
 */
UCLASS()
class AStaticMeshActor :
	public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AStaticMeshActor, AActor)

public:
	AStaticMeshActor();
	AStaticMeshActor(UObject* InOuter);
	~AStaticMeshActor() override;

	void BeginPlay() override;
	void EndPlay() override;
	void Tick(float DeltaSeconds) override;

	// StaticMesh 관련 함수들
	void SetStaticMesh(class UStaticMesh* InStaticMesh);
	UStaticMesh* GetStaticMesh() const;

	// StaticMeshComponent 접근자
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent.Get(); }


	FAABB GetWorldAABB() const;


	FAABB GetLocalAABB() const;

protected:
	// StaticMesh를 렌더링할 컴포넌트
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
};
