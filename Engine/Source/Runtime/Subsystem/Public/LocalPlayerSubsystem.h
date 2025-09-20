#pragma once
#include "Subsystem.h"

/**
 * @brief Local subsystem base class
 * 개별 플레이어에 대한 서브 시스템을 처리하는 클래스
 * 현재 용도는 디테일하게 파악하지 못했으나, 코어 시스템으로 보여 미리 구축
 */
UCLASS()
class ULocalPlayerSubsystem :
	public USubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(ULocalPlayerSubsystem, USubsystem)

public:
	virtual void ProcessPlayerInput()
	{
	}

	virtual void PlayerTick()
	{
	}
};
