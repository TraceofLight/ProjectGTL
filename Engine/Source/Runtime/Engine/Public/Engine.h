#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Subsystem/Public/SubsystemCollection.h"
#include "Runtime/Subsystem/Public/EngineSubsystem.h"

class FAppWindow;
class FRHIDevice;

/**
 * @brief 게임에서 가장 핵심이 되는 EngineSubsystem 클래스
 * 엔진 서브시스템들을 컬렉션으로 관리할 수 있다
 * @param bIsInitialized 초기화 여부
 * @param EngineSubsystemCollection 엔진 서브시스템들을 관리하는 컬렉션
 */
UCLASS()
class UEngine :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UEngine, UObject)

public:
	void Initialize();
	void Shutdown();

	// Register & Get Subsystem
	template <typename T>
	T* GetEngineSubsystem();

	template <typename T>
	void RegisterEngineSubsystem();

	void SetAppWindow(FAppWindow* InWindow) { AppWindow = InWindow; }
	FAppWindow* GetAppWindow() const { return AppWindow; }

	UWorld* GetEditorWorld() const { return EditorWorld; }
	void SetEditorWorld(TObjectPtr<UWorld> InWorld) { EditorWorld = InWorld; }

	UWorld* GetPlayWorld() const { return PlayWorld; }
	void SetPlayWorld(TObjectPtr<UWorld> InWorld) { PlayWorld = InWorld; }

	TObjectPtr<UWorld> GetWorld() const override;

	// Tick engine subsystem
	void TickEngineSubsystems(FAppWindow* InWindow = nullptr);

private:
	bool bIsInitialized = false;
	FEngineSubsystemCollection EngineSubsystemCollection;
	FAppWindow* AppWindow = nullptr;

	TObjectPtr<UWorld> EditorWorld;
	TObjectPtr<UWorld> PlayWorld;

	void RegisterDefaultEngineSubsystems();
};

#pragma region Engine template functions

/**
 * @brief 엔진 서브시스템을 가져오는 함수
 * @tparam T UEngineSubsystem을 상속받은 타입
 * @return 요청된 엔진 서브시스템 인스턴스
 */
template <typename T>
T* UEngine::GetEngineSubsystem()
{
	static_assert(std::is_base_of_v<UEngineSubsystem, T>, "T는 반드시 UEngineSubsystem를 상속 받아야 한다");
	return EngineSubsystemCollection.GetSubsystem<T>();
}

/**
 * @brief 엔진 서브시스템 컬렉션에 서브시스템 클래스를 등록하는 함수
 */
template <typename T>
void UEngine::RegisterEngineSubsystem()
{
	EngineSubsystemCollection.RegisterSubsystemClass<T>();
}

// 전역 엔진 인스턴스
extern UEngine* GEngine;

#pragma endregion Engine template functions
