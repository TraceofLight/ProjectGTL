#include "pch.h"
#include "Engine/Public/Engine.h"
#include "Subsystem/Public/EngineEditorSubsystem.h"

UEngine* GEngine = nullptr;

IMPLEMENT_SINGLETON_CLASS_BASE(UEngine)

/**
 * @brief 엔진 초기화 함수
 * 기본 엔진 서브시스템들 등록 및 엔진 서브시스템 컬렉션 초기화
 */
void UEngine::Initialize()
{
	if (!bIsInitialized)
	{
		RegisterDefaultEngineSubsystems();

		EngineSubsystemCollection.Initialize(this);
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
	// TODO(KHJ): 필요하면 기본 서브시스템 등록
}
