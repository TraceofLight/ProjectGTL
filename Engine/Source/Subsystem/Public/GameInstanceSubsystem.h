#pragma once
#include "Subsystem.h"

/**
 * @brief 게임 인스턴스 서브시스템 기본 클래스
 * 게임 인스턴스 생명주기 동안 지속되는 서브시스템
 */
UCLASS()
class UGameInstanceSubsystem :
	public USubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UGameInstanceSubsystem, USubsystem)

public:
	// USubsystem interface
	void Initialize() override;
	void Deinitialize() override;

	// Game instance life cycle functions
	virtual void OnGameStart()
	{
	}

	virtual void OnGameEnd()
	{
	}

	// 레벨이 로드될 때 호출되는 함수
	virtual void OnLevelLoaded(const FName& InLevelName)
	{
	}

	// Special member function
	UGameInstanceSubsystem();
	~UGameInstanceSubsystem() override;
};
