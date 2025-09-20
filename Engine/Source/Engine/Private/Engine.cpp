#include "pch.h"
#include "Engine/Public/Engine.h"
#include "Subsystem/Public/PathSubsystem.h"

UEngine* GEngine = nullptr;

IMPLEMENT_SINGLETON_CLASS(UEngine, UObject)

UEngine::UEngine()
{
	GEngine = this;
}

UEngine::~UEngine()
{
	// 소멸자에서 정리 작업
	if (bIsInitialized)
	{
		Shutdown();
	}

	GEngine = nullptr;
}

/**
 * @brief 엔진 초기화 함수
 * 기본 엔진 서브시스템들 등록 및 엔진 서브시스템 컬렉션 초기화
 */
void UEngine::Initialize()
{
	if (!bIsInitialized)
	{
		RegisterDefaultEngineSubsystems();

		EngineSubsystemCollection.Initialize(TObjectPtr(this));
		bIsInitialized = true;
	}
}

/**
 * @brief 엔진 종료 함수
 * 여기서 엔진 서브시스템 컬렉션 정리
 */
void UEngine::Shutdown()
{
	if (bIsInitialized)
	{
		EngineSubsystemCollection.Deinitialize();
		bIsInitialized = false;
	}
}

/**
 * @brief 기본 엔진 서브시스템을 등록하는 함수
 */
void UEngine::RegisterDefaultEngineSubsystems()
{
	// 기본 엔진 서브시스템 등록
	RegisterEngineSubsystem<UPathSubsystem>();
}
