#include "pch.h"
#include "Factory/Public/FactorySystem.h"

#include "Factory/Public/Factory.h"
#include "Factory/Actor/Public/StaticMeshActorFactory.h"

bool FFactorySystem::bIsInitialized = false;

/**
 * @brief Factory 시스템을 초기화하는 함수
 * 모든 Factory 클래스의 인스턴스를 생성하고 등록한다
 */
void FFactorySystem::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG("FactorySystem: Already initialized");
		return;
	}

	UE_LOG_SYSTEM("FactorySystem: Factory System을 초기화합니다...");

	// Factory 인스턴스 생성 (생성자에서 자동 등록)
	static UStaticMeshActorFactory StaticMeshFactory;

	bIsInitialized = true;

	UE_LOG_SUCCESS("FactorySystem: Factory System 초기화가 성공적으로 완료되었습니다");
	PrintAllFactories();
}

/**
 * @brief Factory 시스템을 정리하는 함수
 * 정적 객체가 많아 따로 정리하는 내용은 없음
 */
void FFactorySystem::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	UE_LOG_SYSTEM("FactorySystem: Factory System을 종료합니다...");

	// Factory들은 정적 객체이므로 별도 정리 불필요
	bIsInitialized = false;

	UE_LOG_SUCCESS("FactorySystem: Factory System 종료가 성공적으로 완료되었습니다");
}

/**
 * @brief 등록된 모든 Factory 정보를 출력하는 함수
 */
void FFactorySystem::PrintAllFactories()
{
	TArray<TObjectPtr<UFactory>>& FactoryList = UFactory::GetFactoryList();

	UE_LOG("=== Factory System: Registered Factories (%d) ===", FactoryList.Num());

	for (int32 i = 0; i < FactoryList.Num(); ++i)
	{
		const UFactory* Factory = FactoryList[i];
		if (Factory)
		{
			UE_LOG("[%d] %s -> %s",
			       i,
			       Factory->GetDescription().data(),
			       Factory->GetSupportedClass() ?
			       Factory->GetSupportedClass()->GetClassTypeName().ToString().data() :"Unknown");
		}
	}

	UE_LOG("=======================================");
}
