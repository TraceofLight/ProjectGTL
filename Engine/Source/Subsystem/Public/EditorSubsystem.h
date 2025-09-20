#pragma once
#include "Subsystem.h"

/**
 * @brief Editor subsystem base class
 * Editor 모드에서만 동작하는 서브 시스템
 */
UCLASS()
class UEditorSubsystem :
	public USubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UEditorSubsystem, USubsystem)

	// Editor의 Tick 함수
	// 기즈모, 에디터 카메라, 오브젝트 피킹 등을 여기서 처리
	virtual void EditorUpdate()
	{
	}

	virtual bool IsEditorOnly() const { return true; }
};
