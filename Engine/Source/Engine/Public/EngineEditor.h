#pragma once
#include "Core/Public/Object.h"
#include "Subsystem/Public/SubsystemCollection.h"
#include "Subsystem/Public/EditorSubsystem.h"

/**
 * @brief Engine의 Editor 관련 서브시스템 클래스
 * 에디터 서브시스템들을 FSubsystemCollection으로 관리한다
 * 에디터 모드에서만 사용 가능하다 (현재는 Editor가 디폴트)
 * @param bIsInitialized 초기화 여부
 * @param bIsEditorMode 에디터 모드 여부
 * @param EditorSubsystemCollection 엔진 에디터 하의 서브시스템들을 관리하는 컬렉션
 */
UCLASS()
class UEngineEditor :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UEngineEditor, UObject)

public:
	void Initialize();
	void Shutdown();
	void EditorUpdate();

	// Register & Get Subsystem
	template <typename T>
	T* GetEditorSubsystem();

	template <typename T>
	void RegisterEditorSubsystem();

	// Getter & Setter
	bool IsEditorMode() const { return bIsEditorMode; }
	void SetEditorMode(bool bInEditorMode) { bIsEditorMode = bInEditorMode; }

private:
	bool bIsInitialized = false;
	bool bIsEditorMode = false;
	FEditorSubsystemCollection EditorSubsystemCollection;

	void RegisterDefaultEditorSubsystems();
};

// 전역 에디터 인스턴스
extern UEngineEditor* GEditor;

#pragma region EngineEditor template functions

/**
 * @brief 에디터 서브시스템을 가져오는 함수
 * @tparam T UEditorSubsystem을 상속받은 타입
 * @return 요청된 에디터 서브시스템 인스턴스
 */
template <typename T>
T* UEngineEditor::GetEditorSubsystem()
{
	static_assert(std::is_base_of_v<UEditorSubsystem, T>, "T는 반드시 UEditorSubsystem를 상속 받아야 한다");
	return EditorSubsystemCollection.GetSubsystem<T>();
}

/**
 * @brief 에디터 서브시스템 컬렉션에 서브시스템 클래스를 등록하는 함수
 */
template <typename T>
void UEngineEditor::RegisterEditorSubsystem()
{
	EditorSubsystemCollection.RegisterSubsystemClass<T>();
}

#pragma endregion EngineEditor template functions
