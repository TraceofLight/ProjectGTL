#include "pch.h"
#include "Subsystem/Public/GameInstanceSubsystem.h"

IMPLEMENT_CLASS(UGameInstanceSubsystem, USubsystem)

UGameInstanceSubsystem::UGameInstanceSubsystem() = default;

UGameInstanceSubsystem::~UGameInstanceSubsystem() = default;

/**
 * @brief 게임 인스턴스 서브시스템별 초기화 함수
 */
void UGameInstanceSubsystem::Initialize()
{
	Super::Initialize();
}

/**
 * @brief 게임 인스턴스 서브시스템별 정리 함수
 */
void UGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
