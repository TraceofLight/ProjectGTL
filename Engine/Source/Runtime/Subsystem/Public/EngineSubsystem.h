#pragma once
#include "Subsystem.h"

/**
 * @brief Engine subsystem base class
 * 엔진의 생명 주기동안 지속되는 시스템 클래스
 */
UCLASS()
class UEngineSubsystem :
	public USubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UEngineSubsystem, USubsystem)

public:
	virtual void Tick(float DeltaSeconds)
	{
	}

	// Tick을 쓰지 않는 함수의 Tick 호출을 방지하기 위한 기본값 설정
	virtual bool IsTickable() const { return false; }
};
