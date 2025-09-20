#include "pch.h"
#include "Engine/Public/GameInstance.h"

UGameInstance* GameInstance = nullptr;

IMPLEMENT_SINGLETON_CLASS(UGameInstance, UObject)

UGameInstance::UGameInstance()
{
	GameInstance = this;
}

UGameInstance::~UGameInstance()
{
	// 소멸자에서 정리 작업
	if (bIsInitialized)
	{
		Shutdown();
	}

	GameInstance = nullptr;
}

/**
 * @brief 게임 인스턴스 초기화 함수
 * 기본 게임 인스턴스 서브시스템 등록 및 서브 시스템 컬렉션 초기화
 */
void UGameInstance::Initialize()
{
	if (!bIsInitialized)
	{
		if (!GameInstance)
		{
			GameInstance = this;
		}

		RegisterDefaultGameInstanceSubsystems();
		GameInstanceSubsystemCollection.Initialize(TObjectPtr(this));

		bIsInitialized = true;
	}
}

/**
 * @brief 게임 인스턴스 종료 함수
 * 게임 종료 이벤트를 전파한 뒤, 남은 자원을 정리한다
 */
void UGameInstance::Shutdown()
{
	if (bIsInitialized)
	{
		OnGameEnd();

		GameInstanceSubsystemCollection.Deinitialize();
		bIsInitialized = false;
	}
}

/**
 * @brief 모든 게임 인스턴스 서브시스템에 게임 시작 이벤트 전달
 */
void UGameInstance::OnGameStart()
{
	if (bIsInitialized && !bIsGameActive)
	{
		bIsGameActive = true;

		GameInstanceSubsystemCollection.ForEachSubsystem([](TObjectPtr<UGameInstanceSubsystem> InSubsystem)
		{
			if (InSubsystem)
			{
				InSubsystem->OnGameStart();
			}
		});
	}
}

/**
 * @brief 모든 게임 인스턴스 서브시스템에 게임 종료 이벤트를 전달하는 함수
 */
void UGameInstance::OnGameEnd()
{
	if (bIsGameActive)
	{
		bIsGameActive = false;

		GameInstanceSubsystemCollection.ForEachSubsystem([](TObjectPtr<UGameInstanceSubsystem> InSubsystem)
		{
			if (InSubsystem)
			{
				InSubsystem->OnGameEnd();
			}
		});
	}
}

/**
 * @brief 모든 게임 인스턴스 서브시스템에 레벨 로드 이벤트를 전달하는 함수
 * @param InLevelName 레벨 이름
 */
void UGameInstance::OnLevelLoaded(const FName& InLevelName)
{
	GameInstanceSubsystemCollection.ForEachSubsystem([&InLevelName](TObjectPtr<UGameInstanceSubsystem> InSubsystem)
	{
		if (InSubsystem)
		{
			InSubsystem->OnLevelLoaded(InLevelName);
		}
	});
}

/**
 * @brief 기본 게임 인스턴스 서브시스템을 등록하는 함수
 */
void UGameInstance::RegisterDefaultGameInstanceSubsystems()
{
	// TODO(KHJ): 필요하면 기본 서브시스템 등록
}
