#pragma once
#include "Core/Public/Object.h"
#include "Subsystem/Public/SubsystemCollection.h"
#include "Subsystem/Public/EngineSubsystem.h"

/**
 * @brief 게임에서 가장 핵심이 되는 EngineSubsystem 클래스
 * 엔진 서브시스템들을 컬렉션으로 관리할 수 있다
 * @param bIsInitialized 초기화 여부
 * @param EngineSubsystemCollection 엔진 서브시스템들을 관리하는 컬렉션
 */
UCLASS()
class UEngine :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UEngine, UObject)

public:
	void Initialize();
	void Shutdown();

	// Register & Get Subsystem
	template <typename T>
	T* GetEngineSubsystem();

	template <typename T>
	void RegisterEngineSubsystem();

private:
	bool bIsInitialized = false;
	FEngineSubsystemCollection EngineSubsystemCollection;

	void RegisterDefaultEngineSubsystems();
};

#pragma region Engine template functions

/**
 * @brief 엔진 서브시스템을 가져오는 함수
 * @tparam T UEngineSubsystem을 상속받은 타입
 * @return 요청된 엔진 서브시스템 인스턴스
 */
template <typename T>
T* UEngine::GetEngineSubsystem()
{
	static_assert(std::is_base_of_v<UEngineSubsystem, T>, "T는 반드시 UEngineSubsystem를 상속 받아야 한다");
	return EngineSubsystemCollection.GetSubsystem<T>();
}

/**
 * @brief 엔진 서브시스템 컬렉션에 서브시스템 클래스를 등록하는 함수
 */
template <typename T>
void UEngine::RegisterEngineSubsystem()
{
	EngineSubsystemCollection.RegisterSubsystemClass<T>();
}

#pragma endregion Engine template functions

#pragma region Engine globals

// 전역 엔진 인스턴스
extern UEngine* GEngine;

/**
 * @brief 엔진 서브시스템을 편리하게 가져오기 위한 전역 함수
 */
template <typename T>
T* GetEngineSubsystem()
{
	static_assert(std::is_base_of_v<UEngineSubsystem, T>, "T는 반드시 UEngineSubsystem를 상속 받아야 한다");
	if (GEngine)
	{
		return GEngine->GetEngineSubsystem<T>();
	}
	return nullptr;
}

#pragma endregion Engine globals
