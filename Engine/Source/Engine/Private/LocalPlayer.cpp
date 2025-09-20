#include "pch.h"
#include "Engine/Public/LocalPlayer.h"

ULocalPlayer* LocalPlayer = nullptr;

IMPLEMENT_SINGLETON_CLASS(ULocalPlayer, UObject)

ULocalPlayer::ULocalPlayer()
{
	// 기본 초기화
}

ULocalPlayer::~ULocalPlayer()
{
	// 소멸자에서 정리 작업
	if (bIsInitialized)
	{
		Shutdown();
	}
}

/**
 * @brief 로컬 플레이어 초기화 함수
 * 기본 서브시스템 등록 및 에디터 서브시스템 컬렉션 초기화
 */
void ULocalPlayer::Initialize()
{
	if (!bIsInitialized)
	{
		RegisterDefaultLocalPlayerSubsystems();

		LocalPlayerSubsystemCollection.Initialize(TObjectPtr(this));
		bIsInitialized = true;
		bIsPlayerActive = true;
	}
}

/**
 * @brief 로컬 플레이어 종료 함수
 * 여기서 로컬 플레이어 서브시스템 컬렉션 정리
 */
void ULocalPlayer::Shutdown()
{
	if (bIsInitialized)
	{
		bIsPlayerActive = false;

		LocalPlayerSubsystemCollection.Deinitialize();
		bIsInitialized = false;
	}
}

/**
 * @brief 모든 로컬 플레이어 서브시스템에 입력 처리 이벤트를 전달하는 함수
 */
void ULocalPlayer::ProcessPlayerInput()
{
	if (bIsInitialized && bIsPlayerActive)
	{
		LocalPlayerSubsystemCollection.ForEachSubsystem([](TObjectPtr<ULocalPlayerSubsystem> InSubsystem)
		{
			if (InSubsystem)
			{
				// TODO(KHJ): 필요하면 입력 처리 로직 추가
			}
		});
	}
}

/**
 * @brief 기본 로컬 플레이어 서브시스템들을 등록하는 함수
 */
void ULocalPlayer::RegisterDefaultLocalPlayerSubsystems()
{
	// TODO(KHJ): 필요하면 기본 서브시스템 등록
}
