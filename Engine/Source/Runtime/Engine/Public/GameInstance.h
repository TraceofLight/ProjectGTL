#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Subsystem/Public/SubsystemCollection.h"
#include "Runtime/Subsystem/Public/GameInstanceSubsystem.h"

/**
 * @brief GameInstance Class
 * 게임의 전체 생명주기 동안 지속되는 인스턴스
 * @param bIsInitialized 초기화 여부
 * @param bIsGameActive 게임 액티브 여부
 * @param GameInstanceSubsystemCollection 게임 인스턴스 하의 서브시스템들을 관리하는 컬렉션
 */
UCLASS()
class UGameInstance :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UGameInstance, UObject)

public:
	void Initialize();
	void Shutdown();
	void OnGameStart();
	void OnGameEnd();
	void OnLevelLoaded(const FName& InLevelName);

	// Register & Get Subsystem
	template <typename T>
	void RegisterGameInstanceSubsystem();

	template <typename T>
	T* GetSubsystem();

	// Getter
	bool IsGameActive() const { return bIsGameActive; }

private:
	bool bIsInitialized = false;
	bool bIsGameActive = false;
	FGameInstanceSubsystemCollection GameInstanceSubsystemCollection;

	void RegisterDefaultGameInstanceSubsystems();
};

// 전역 게임 인스턴스
extern UGameInstance* GameInstance;

#pragma region GameInstance template functions

/**
 * @brief 게임 인스턴스 서브시스템을 가져오는 함수
 * @tparam T UGameInstanceSubsystem을 상속받은 타입
 * @return 요청된 게임 인스턴스 서브시스템 인스턴스
 */
template <typename T>
T* UGameInstance::GetSubsystem()
{
	static_assert(std::is_base_of_v<UGameInstanceSubsystem, T>, "T는 반드시 UGameInstanceSubsystem를 상속 받아야 한다");
	return GameInstanceSubsystemCollection.GetSubsystem<T>();
}

/**
 * @brief 게임 인스턴스 서브시스템 컬렉션에 서브시스템 클래스를 등록하는 함수
 */
template <typename T>
void UGameInstance::RegisterGameInstanceSubsystem()
{
	GameInstanceSubsystemCollection.RegisterSubsystemClass<T>();
}

#pragma endregion GameInstance template functions
