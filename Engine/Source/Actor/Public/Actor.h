#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/ObjectPtr.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Factory/Public/NewObject.h"

class UBillBoardComponent;
/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()

class AActor : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	~AActor() override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FVector& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;

	template <typename T>
	TObjectPtr<T> CreateDefaultSubobject(const FName& InName);

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick();

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent.Get(); }
	const TArray<TObjectPtr<UActorComponent>>& GetOwnedComponents() const { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	// PrimitiveComponent 관련 함수들
	TArray<class UPrimitiveComponent*> GetPrimitiveComponents() const;

private:
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	TObjectPtr<UBillBoardComponent> BillBoardComponent = nullptr;
	TArray<TObjectPtr<UActorComponent>> OwnedComponents;
};

template <typename T>
TObjectPtr<T> AActor::CreateDefaultSubobject(const FName& InName)
{
	static_assert(std::is_base_of_v<UActorComponent, T>, "CreateDefaultSubobject can only be used with UActorComponent derivatives");

	TObjectPtr<T> NewComponent = TObjectPtr<T>(new T());

	if (NewComponent)
	{
		// Component 기본 설정
		if (InName != FName::None)
		{
			NewComponent->SetName(InName);
		}

		NewComponent->SetOuter(TObjectPtr<UObject>(this));

		// Component에 Owner 설정
		NewComponent->SetOwner(this);
		OwnedComponents.push_back(NewComponent);
	}

	return NewComponent;
}
