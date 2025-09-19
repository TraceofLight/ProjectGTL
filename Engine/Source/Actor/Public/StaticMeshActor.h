#pragma once
#include "Actor/Public/Actor.h"

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
	void Tick() override;

	// StaticMesh 관련 함수들
	void SetStaticMesh(class UStaticMesh* InStaticMesh);
	class UStaticMesh* GetStaticMesh() const;

	// StaticMeshComponent 접근자
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent.Get(); }

protected:
	// StaticMesh를 렌더링할 컴포넌트
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
};