#pragma once
#include "Core/Public/Object.h"
#include "Subsystem/Public/SubsystemCollection.h"
#include "Subsystem/Public/LocalPlayerSubsystem.h"

/**
 * @brief 로컬 플레이어 클래스
 * 개별 플레이어에 대한 데이터와 관련 서브시스템을 관리한다
 * @warning 현재는 따로 필요할 거 같은 내용만 채워서 관리, 아직 사용할 수는 없다
 * @brief bIsInitialized 초기화 여부
 * @brief bIsPlayerActive 플레이어 활성화 여부
 * @brief PlayerID 플레이어의 ID
 * @brief LocalPlayerSubsystemCollection 로컬 플레이어 이하의 서브시스템들을 관리하는 컬렉션
 */
UCLASS()
class ULocalPlayer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULocalPlayer, UObject)

public:
	void Initialize();
	void Shutdown();
	void ProcessPlayerInput();

	// Register & Get Subsystem
	template <typename T>
	T* GetSubsystem();

	template <typename T>
	void RegisterLocalPlayerSubsystem();

	// Getter & Setter
	uint32 GetPlayerID() const { return PlayerID; }
	bool IsPlayerActive() const { return bIsPlayerActive; }

	void SetPlayerID(uint32 InPlayerID) { PlayerID = InPlayerID; }
	void SetPlayerActive(bool bInPlayerActive) { bIsPlayerActive = bInPlayerActive; }

private:
	bool bIsInitialized = false;
	bool bIsPlayerActive = false;
	uint32 PlayerID = 0;
	FLocalPlayerSubsystemCollection LocalPlayerSubsystemCollection;

	void RegisterDefaultLocalPlayerSubsystems();
};

#pragma region LocalPlayer template functions

/**
 * @brief 로컬 플레이어 서브시스템을 가져오는 함수
 * @tparam T ULocalPlayerSubsystem을 상속받은 타입
 * @return 요청된 로컬 플레이어 서브시스템 인스턴스
 */
template <typename T>
T* ULocalPlayer::GetSubsystem()
{
	static_assert(std::is_base_of_v<ULocalPlayerSubsystem, T>, "T는 반드시 ULocalPlayerSubsystem를 상속 받아야 한다");
	return LocalPlayerSubsystemCollection.GetSubsystem<T>();
}

/**
 * @brief 로컬 플레이어 서브시스템 컬렉션에 서브시스템 클래스를 등록하는 함수
 */
template <typename T>
void ULocalPlayer::RegisterLocalPlayerSubsystem()
{
	LocalPlayerSubsystemCollection.RegisterSubsystemClass<T>();
}

#pragma endregion LocalPlayer template functions

#pragma region LocalPlayer globals

extern ULocalPlayer* LocalPlayer;

/**
 * @brief 로컬 플레이어 서브시스템을 편리하게 가져오기 위한 전역 함수
 * 여러 플레이어가 있는 경우 첫 번째 플레이어에서 가져옴
 * 지금은 따로 가져오고 있는 정보가 없음
 */
template <typename T>
T* GetLocalPlayerSubsystem()
{
	static_assert(std::is_base_of_v<ULocalPlayerSubsystem, T>, "T는 반드시 ULocalPlayerSubsystem를 상속 받아야 한다");
	return nullptr;
}

#pragma endregion LocalPlayer globals
