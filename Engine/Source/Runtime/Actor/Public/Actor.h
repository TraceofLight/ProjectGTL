#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/ObjectPtr.h"
#include "Runtime/Component/Public/ActorComponent.h"
#include "Runtime/Component/Public/SceneComponent.h"

class ULevel;
class UPrimitiveComponent;
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

	// RootComponent가 없을 때 DefaultSceneRoot 생성
	void EnsureRootComponent();

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FVector& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);

	bool IsUniformScale() const;

	template <typename T>
	TObjectPtr<T> CreateDefaultSubobject(const FName& InName);
	
	// std::string 오버로드 (편의성을 위해)
	template <typename T>
	TObjectPtr<T> CreateDefaultSubobject(const std::string& InName)
	{
		return CreateDefaultSubobject<T>(FName(InName));
	}

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick(float DeltaSeconds);

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent.Get(); }
	const TArray<TObjectPtr<UActorComponent>>& GetOwnedComponents() const { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	const FVector& GetActorLocation() const;
	const FVector& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	// PrimitiveComponent 관련 함수들
	TArray<UPrimitiveComponent*> GetPrimitiveComponents() const;
	TObjectPtr<UWorld> GetWorld() const override;
	TObjectPtr<ULevel> GetLevel() const;

	bool IsActorTickEnabled() const { return bTickInEditor; }

	// 언리얼 엔진 호환성을 위한 GetComponents 템플릿 함수
	template<typename T>
	TArray<T*> GetComponents() const;

	// Hidden 관련 함수들 (언리얼 엔진 호환)
	bool GetActorHiddenInGame() const { return bHidden; }
	void SetActorHiddenInGame(bool bNewHidden) { bHidden = bNewHidden; }

private:
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	TArray<TObjectPtr<UActorComponent>> OwnedComponents;
	bool bTickInEditor = false;
	bool bHidden = false; // 언리얼 엔진 호환성을 위한 Hidden 상태
};

template <typename T>
TObjectPtr<T> AActor::CreateDefaultSubobject(const FName& InName)
{
	static_assert(std::is_base_of_v<UActorComponent, T>, "CreateDefaultSubobject는 UActorComponent를 상속 받아야 합니다");

	TObjectPtr<T> NewComponent = TObjectPtr<T>(new T());

	if (NewComponent)
	{
		// Component 기본 설정
		if (InName != FName::FName_None)
		{
			NewComponent->SetName(InName);
		}

		NewComponent->SetOuter(TObjectPtr<UObject>(this));

		// Component에 Owner 설정
		NewComponent->SetOwner(this);
		OwnedComponents.Add(NewComponent);
	}

	return NewComponent;
}

// GetComponents 템플릿 함수 구현
template<typename T>
TArray<T*> AActor::GetComponents() const
{
	static_assert(std::is_base_of_v<UActorComponent, T>, "GetComponents는 UActorComponent를 상속받은 클래스만 사용 가능합니다");
	
	TArray<T*> Result;
	for (const auto& Component : OwnedComponents)
	{
		if (T* CastedComponent = Cast<T>(Component))
		{
			Result.Add(CastedComponent);
		}
	}
	return Result;
}
