#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Subsystem.h"
#include "EngineSubsystem.h"
#include "EditorSubsystem.h"
#include "GameInstanceSubsystem.h"
#include "LocalPlayerSubsystem.h"

/**
 * @brief 특정 타입의 서브시스템들을 담아서 생명주기를 관리하는 컨테이너 클래스
 * UEngine, UGameInstance, UWorld 등 주요 엔진 객체들이 내부적으로 가지고 있으면서 서브시스템을 관리한다
 * 따라서 이 클래스는 직접 사용되지 않는 클래스이다
 * @param SubsystemClasses 서브시스템 클래스들의 등록 정보
 * @param SubsystemInstances 실제 서브시스템 인스턴스들
 * @param InitializationOrder 초기화 순서를 관리하기 위한 의존성 정렬
 */
template <typename TBaseSubsystem>
class FSubsystemCollection
{
	static_assert(std::is_base_of_v<USubsystem, TBaseSubsystem>, "TBaseSubsystem은 USubsystem을 반드시 상속 받아야 한다");

public:
	FSubsystemCollection() = default;
	~FSubsystemCollection() = default;

	void Initialize(TObjectPtr<UObject> InOuter);
	void Deinitialize();

	/**
	 * @brief 특정 타입의 서브시스템 인스턴스를 가져오는 함수
	 * @return 요청된 타입의 서브시스템 인스턴스, 없으면 nullptr
	 */
	template <typename T>
	TObjectPtr<T> GetSubsystem() const
	{
		static_assert(std::is_base_of_v<TBaseSubsystem, T>, "T는 TBaseSubsystem을 반드시 상속 받아야 한다");

		FName ClassName = T::StaticClass()->GetClassTypeName();
		auto Iter = SubsystemInstances.find(ClassName);
		if (Iter != SubsystemInstances.end())
		{
			return Cast<T>(Iter->second);
		}
		return nullptr;
	}

	/**
	 * @brief 서브시스템 클래스를 등록하는 함수
	 */
	template <typename T>
	void RegisterSubsystemClass()
	{
		static_assert(std::is_base_of_v<TBaseSubsystem, T>, "T는 TBaseSubsystem을 반드시 상속 받아야 한다");

		FName ClassName = T::StaticClass()->GetClassTypeName();
		SubsystemClasses[ClassName] = T::StaticClass();
	}

	/**
	 * @brief 등록된 모든 서브시스템들에 대해 특정 함수를 호출하는 함수
	 */
	template <typename FuncType>
	void ForEachSubsystem(FuncType&& InFunction)
	{
		for (auto& Pair : SubsystemInstances)
		{
			if (Pair.second)
			{
				InFunction(Pair.second);
			}
		}
	}

	// Getter
	int32 GetSubsystemCount() const { return static_cast<int32>(SubsystemInstances.size()); }

private:
	TMap<FName, UClass*> SubsystemClasses;
	TMap<FName, TObjectPtr<TBaseSubsystem>> SubsystemInstances;
	TArray<FName> InitializationOrder;

	void CalculateInitializationOrder();
};

using FEngineSubsystemCollection = FSubsystemCollection<UEngineSubsystem>;
using FGameInstanceSubsystemCollection = FSubsystemCollection<UGameInstanceSubsystem>;
using FEditorSubsystemCollection = FSubsystemCollection<UEditorSubsystem>;
using FLocalPlayerSubsystemCollection = FSubsystemCollection<ULocalPlayerSubsystem>;

#pragma region Template Implementation

/**
 * @brief 컬렉션 초기화 함수
 * 모든 등록된 서브시스템 클래스를 찾아서 인스턴스를 생성한다
 * @param InOuter 서브시스템들의 소유자 객체
 */
template <typename TBaseSubsystem>
void FSubsystemCollection<TBaseSubsystem>::Initialize(TObjectPtr<UObject> InOuter)
{
	// 1. 의존성 순서 계산
	CalculateInitializationOrder();

	// 2. 모든 서브시스템 인스턴스 생성
	for (const FName& ClassName : InitializationOrder)
	{
		auto Iter = SubsystemClasses.find(ClassName);
		if (Iter != SubsystemClasses.end() && Iter->second)
		{
			UClass* SubsystemClass = Iter->second;
			TObjectPtr<UObject> NewObject = SubsystemClass->CreateDefaultObject();

			if (NewObject)
			{
				TObjectPtr<TBaseSubsystem> NewSubsystem = Cast<TBaseSubsystem>(NewObject);
				if (NewSubsystem)
				{
					NewSubsystem->SetOuter(InOuter);
					SubsystemInstances[ClassName] = NewSubsystem;
				}
			}
		}
	}

	// 3. 모든 서브시스템 1단계 초기화
	for (const FName& ClassName : InitializationOrder)
	{
		auto Iter = SubsystemInstances.find(ClassName);
		if (Iter != SubsystemInstances.end() && Iter->second)
		{
			Iter->second->Initialize();
			UE_LOG("SubsystemCollection: %s 1단계 초기화 완료", ClassName.ToString().data());
		}
	}

	// 4. 모든 서브시스템 2단계 초기화
	for (const FName& ClassName : InitializationOrder)
	{
		auto Iter = SubsystemInstances.find(ClassName);
		if (Iter != SubsystemInstances.end() && Iter->second)
		{
			Iter->second->PostInitialize();
			UE_LOG("SubsystemCollection: %s 2단계 초기화 완료", ClassName.ToString().data());
		}
	}
}

/**
 * @brief 컬렉션 종료 함수
 * 초기화의 역순으로 모든 서브시스템을 정리하고 소멸한다
 */
template <typename TBaseSubsystem>
void FSubsystemCollection<TBaseSubsystem>::Deinitialize()
{
	for (int32 i = static_cast<int32>(InitializationOrder.size()) - 1; i >= 0; --i)
	{
		FName ClassName = InitializationOrder[i];
		auto it = SubsystemInstances.find(ClassName);

		if (it != SubsystemInstances.end())
		{
			if (it->second)
			{
				it->second->Deinitialize();
			}
		}
	}

	SubsystemInstances.clear();
	InitializationOrder.clear();
}


/**
 * @brief 의존성을 고려한 초기화 순서를 계산하는 함수
 * 현재는 등록 순서대로 초기화를 진행하며, 필요할 경우 Topology Sort 처리할 것
 */
template <typename TBaseSubsystem>
void FSubsystemCollection<TBaseSubsystem>::CalculateInitializationOrder()
{
	InitializationOrder.clear();

	for (const auto& Pair : SubsystemClasses)
	{
		InitializationOrder.push_back(Pair.first);
	}
}

#pragma endregion Template Implementation
