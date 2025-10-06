#pragma once
#include "Runtime/Actor/Public/Actor.h"
#include "Physics/Public/AABB.h"

class UStaticMeshComponent;

/**
 * @brief StaticMesh를 렌더링하는 액터 클래스
 * OBJ 파일에서 로드된 3D 모델을 화면에 표시하는 역할
 */
UCLASS()
class AStaticMeshActor : public AActor
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
	class UStaticMesh* GetStaticMesh() const;

	// StaticMeshComponent 접근자
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent.Get(); }

	// AABB 관련 함수들
	/**
	 * @brief 액터의 월드 좌표계 AABB를 가져옴
	 * @return 변환된 월드 AABB
	 */
	FAABB GetWorldAABB() const;

	/**
	 * @brief 액터의 로컬 좌표계 AABB를 가져옴
	 * @return 로컬 AABB
	 */
	FAABB GetLocalAABB() const;

protected:
	// StaticMesh를 렌더링할 컴포넌트
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
};
