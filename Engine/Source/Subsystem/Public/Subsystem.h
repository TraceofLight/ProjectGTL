#pragma once
#include "Core/Public/Object.h"

/**
 * @brief Subsystem base class
 */
UCLASS()
class USubsystem :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(USubsystem, UObject)

public:
	// 초기화 & 삭제에 대한 함수 제공
	virtual void Initialize()
	{
	}

	virtual void Deinitialize()
	{
	}

	// Getter
	bool IsInitialized() const { return bIsInitialized; }

	// Special member funtion
	USubsystem();
	~USubsystem() override = default;

protected:
	// Initialized by subsystem only
	void SetInitialized(bool bInInitialized) { bIsInitialized = bInInitialized; }

private:
	bool bIsInitialized;
};
