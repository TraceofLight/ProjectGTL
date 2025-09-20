#include "pch.h"
#include "Engine/Public/EngineEditor.h"

UEngineEditor* GEditor = nullptr;

IMPLEMENT_SINGLETON_CLASS(UEngineEditor, UObject)

UEngineEditor::UEngineEditor()
{
	// 생성자에서 에디터 모드 기본 활성화
	bIsEditorMode = true;
}

UEngineEditor::~UEngineEditor()
{
	// 소멸자에서 정리 작업
	if (bIsInitialized)
	{
		Shutdown();
	}
}

/**
 * @brief 엔진 에디터 초기화 함수
 * 전역 에디터 인스턴스 설정 및 에디터 서브시스템 컬렉션 초기화
 */
void UEngineEditor::Initialize()
{
	if (!bIsInitialized && bIsEditorMode)
	{
		// 에디터 인스턴스가 생성되면 에디터 모드로 설정
		if (!GEditor)
		{
			GEditor = this;
			bIsEditorMode = true;
		}

		RegisterDefaultEditorSubsystems();

		EditorSubsystemCollection.Initialize(TObjectPtr(this));
		bIsInitialized = true;
	}
}

/**
 * @brief 엔진 에디터 종료 함수
 * 여기서 엔진 에디터 서브시스템 컬렉션 정리
 */
void UEngineEditor::Shutdown()
{
	if (bIsInitialized)
	{
		EditorSubsystemCollection.Deinitialize();
		bIsInitialized = false;
	}
}

/**
 * @brief Editor의 Tick 함수
 * 모든 에디터 서브시스템에 업데이트 이벤트를 전달한다
 */
void UEngineEditor::EditorUpdate()
{
	if (bIsInitialized && bIsEditorMode)
	{
		EditorSubsystemCollection.ForEachSubsystem([](TObjectPtr<UEditorSubsystem> InSubsystem)
		{
			if (InSubsystem)
			{
				InSubsystem->EditorUpdate();
			}
		});
	}
}

/**
 * @brief 기본 에디터 서브시스템들을 등록하는 함수
 */
void UEngineEditor::RegisterDefaultEditorSubsystems()
{
	// TODO(KHJ): 필요하면 기본 서브시스템 등록
}
