#include "pch.h"
#include "Runtime/Engine/Public/Engine.h"

#include "Runtime/Core/Public/ModuleManager.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Subsystem/Public/DebugRenderingSubsystem.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"
#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/Subsystem/Config/Public/ConfigSubsystem.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"

UEngine* GEngine = nullptr;

IMPLEMENT_SINGLETON_CLASS(UEngine, UObject)

UEngine::UEngine()
{
	GEngine = this;
}

UEngine::~UEngine()
{
	// 소멸자에서 정리 작업
	if (bIsInitialized)
	{
		Shutdown();
	}

	GEngine = nullptr;
}

/**
 * @brief UObject 인터페이스의 GetWorld 구현
 * @return PIE일 경우 PlayWorld를, 그 외에는 EditorWorld를 반환
 */
TObjectPtr<UWorld> UEngine::GetWorld() const
{
	// PIE일 경우 PlayWorld 반환
	if (PlayWorld)
	{
		return PlayWorld;
	}

	// 일반 편집 시에는 EditorWorld 반환
	if (EditorWorld)
	{
		return EditorWorld;
	}

	return nullptr;
}

/**
 * @brief 엔진 초기화 함수
 * 기본 엔진 서브시스템들 등록 및 엔진 서브시스템 컬렉션 초기화
 */
void UEngine::Initialize()
{
	if (!bIsInitialized)
	{
		// RHI는 이미 PreInit에서 초기화되었으므로 별도 처리 불필요

		RegisterDefaultEngineSubsystems();

		EngineSubsystemCollection.Initialize(TObjectPtr(this));
		bIsInitialized = true;

		UE_LOG("UEngine: 초기화 완료");
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

		// ModuleManager를 통한 모듈 정리
		FModuleManager::GetInstance().ShutdownAllModules();

		bIsInitialized = false;

		UE_LOG("UEngine: 종료 완료");
	}
}

// GetRendererModule 제거됨 - 이제 GDynamicRHI 직접 사용

/**
 * @brief 모든 Tickable 엔진 서브시스템의 Tick 함수를 우선순위 순서대로 호출
 */
void UEngine::TickEngineSubsystems(FAppWindow* InWindow)
{
	// Tickable 서브시스템만 모아서 정렬
	// TODO(KHJ): Tickable 서브시스템 목록을 캐싱하고, 서브시스템 추가, 제거 시에만 재정렬하도록 해야 함
	TArray<UEngineSubsystem*> TickableSubsystems;

	EngineSubsystemCollection.ForEachSubsystem([&TickableSubsystems](UEngineSubsystem* InSubsystem)
	{
		if (InSubsystem && InSubsystem->IsTickable())
		{
			TickableSubsystems.Add(InSubsystem);
		}
	});

	// 정렬된 순서대로 Tick 호출
	for (UEngineSubsystem* Subsystem : TickableSubsystems)
	{
		Subsystem->Tick(DT);
	}
}

/**
 * @brief 기본 엔진 서브시스템을 등록하는 함수
 */
void UEngine::RegisterDefaultEngineSubsystems()
{
	// 기본 엔진 서브시스템 등록
	RegisterEngineSubsystem<UConfigSubsystem>();
	RegisterEngineSubsystem<UAssetSubsystem>();
	RegisterEngineSubsystem<UInputSubsystem>();
	RegisterEngineSubsystem<UViewportSubsystem>();
	RegisterEngineSubsystem<UUISubsystem>();
	RegisterEngineSubsystem<UWorldSubsystem>();
	RegisterEngineSubsystem<UDebugRenderingSubsystem>();
}
