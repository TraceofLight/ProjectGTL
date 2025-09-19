#pragma once
#include "Core/Public/Object.h"
#include "Subsystem.h"
#include "EngineSubsystem.h"
#include "LocalPlayerSubsystem.h"

class UGameInstanceSubsystem;
class UEditorSubsystem;

using std::is_base_of_v;

/**
 * @brief 모든 서브시스템을 관리하는 매니저 클래스
 */
UCLASS()
class USubsystemManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(USubsystemManager, UObject)

public:
	void Initialize();
	void Shutdown();
	void Tick();

	template <typename T>
	void RegisterSubsystem(TObjectPtr<T> InSubsystem);

	template <typename T>
	TObjectPtr<T> GetEngineSubsystem();

	template <typename T>
	TObjectPtr<T> GetEditorSubsystem();

	template <typename T>
	TObjectPtr<T> GetGameInstanceSubsystem();

	template <typename T>
	TObjectPtr<T> GetLocalPlayerSubsystem();

	void OnGameStart();
	void OnGameEnd();
	void OnLevelLoaded(const FName& InLevelName);
	void ProcessPlayerInput();
	void EditorUpdate();

private:
	TMap<FName, TObjectPtr<USubsystem>> AllSubsystems;
	TMap<FName, TObjectPtr<UEngineSubsystem>> EngineSubsystems;
	TMap<FName, TObjectPtr<UEditorSubsystem>> EditorSubsystems;
	TMap<FName, TObjectPtr<UGameInstanceSubsystem>> GameInstanceSubsystems;
	TMap<FName, TObjectPtr<ULocalPlayerSubsystem>> LocalPlayerSubsystems;

	template <typename T>
	TObjectPtr<T> GetSubsystem();

	void InitializeAllSubsystems();
	void DeinitializeAllSubsystems();
};

#pragma region Subsystem global function

template <typename T>
TObjectPtr<T> GetEngineSubsystem()
{
	static_assert(is_base_of_v<UEngineSubsystem, T>, "T는 UEngineSubsystem을 상속 받아야 합니다");
	const TObjectPtr<USubsystemManager> SubsystemManager = USubsystemManager::GetInstance();
	if (SubsystemManager)
	{
		return SubsystemManager->GetEngineSubsystem<T>();
	}

	return nullptr;
}

template <typename T>
TObjectPtr<T> GetEditorSubsystem()
{
	static_assert(is_base_of_v<UEditorSubsystem, T>, "T는 UEditorSubsystem을 상속 받아야 합니다");
	const TObjectPtr<USubsystemManager> SubsystemManager = USubsystemManager::GetInstance();
	if (SubsystemManager)
	{
		return SubsystemManager->GetEditorSubsystem<T>();
	}

	return nullptr;
}

template <typename T>
TObjectPtr<T> GetGameInstanceSubsystem()
{
	static_assert(is_base_of_v<UGameInstanceSubsystem, T>, "T는 UGameInstanceSubsystem을 상속 받아야 합니다");

	const TObjectPtr<USubsystemManager> SubsystemManager = USubsystemManager::GetInstance();
	if (SubsystemManager)
	{
		return SubsystemManager->GetGameInstanceSubsystem<T>();
	}

	return nullptr;
}

template <typename T>
TObjectPtr<T> GetLocalPlayerSubsystem()
{
	static_assert(is_base_of_v<ULocalPlayerSubsystem, T>, "T는 ULocalPlayerSubsystem을 상속 받아야 합니다");

	const TObjectPtr<USubsystemManager> SubsystemManager = USubsystemManager::GetInstance();
	if (SubsystemManager)
	{
		return SubsystemManager->GetLocalPlayerSubsystem<T>();
	}

	return nullptr;
}

#pragma endregion Subsystem global function

#pragma region Subsystem implementation functions

/**
 * @brief 서브시스템 등록 함수
 * @param InSubsystem 등록할 서브시스템
 */
template <typename T>
void USubsystemManager::RegisterSubsystem(TObjectPtr<T> InSubsystem)
{
	static_assert(is_base_of_v<USubsystem, T>, "T는 USubsystem을 상속 받아야 합니다");

	// 타입에 따라 적절한 컨테이너에 저장
	// 사용자 친화적인 탐색을 위해 FName 키로 사용
	if (InSubsystem)
	{
		FName ClassName = T::StaticClass()->GetClassTypeName();

		// 타입에 따라 적절한 컨테이너에 저장
		if (std::is_base_of_v<UEngineSubsystem, T>)
		{
			EngineSubsystems.insert(ClassName, Cast<UEngineSubsystem>(InSubsystem));
		}
		else if (std::is_base_of_v<UEditorSubsystem, T>)
		{
			EditorSubsystems.insert(ClassName, Cast<UEditorSubsystem>(InSubsystem));
		}
		else if (std::is_base_of_v<UGameInstanceSubsystem, T>)
		{
			GameInstanceSubsystems.insert(ClassName, Cast<UGameInstanceSubsystem>(InSubsystem));
		}
		else if (std::is_base_of_v<ULocalPlayerSubsystem, T>)
		{
			LocalPlayerSubsystems.insert(ClassName, Cast<ULocalPlayerSubsystem>(InSubsystem));
		}

		// 전역 서브시스템 맵 사용
		AllSubsystems.insert(ClassName, InSubsystem);
	}
}

/**
 * @brief 서브시스템 가져오는 Internal 함수
 * @return 요청된 타입의 서브시스템 인스턴스
 */
template <typename T>
TObjectPtr<T> USubsystemManager::GetSubsystem()
{
	static_assert(is_base_of_v<USubsystem, T>, "T는 USubsystem을 상속 받아야 합니다");

	FString ClassName = T::StaticClass()->GetClassTypeName();

	if (TObjectPtr<T>* FoundSubsystem = AllSubsystems.find(ClassName))
	{
		return Cast<T>(*FoundSubsystem);
	}

	return nullptr;
}

/**
 * @brief 엔진의 서브시스템을 가져오는 함수
 */
template <typename T>
TObjectPtr<T> USubsystemManager::GetEngineSubsystem()
{
	static_assert(is_base_of_v<UEngineSubsystem, T>, "T는 UEngineSubsystem을 상속 받아야 합니다");
	return GetSubsystem<T>();
}

/**
 * @brief 에디터의 서브시스템을 가져오는 함수
 */
template <typename T>
TObjectPtr<T> USubsystemManager::GetEditorSubsystem()
{
	static_assert(is_base_of_v<UEditorSubsystem, T>, "T는 UEditorSubsystem을 상속 받아야 합니다");
	return GetSubsystem<T>();
}

/**
 * @brief 게임 인스턴스의 서브시스템을 가져오는 함수
 */
template <typename T>
TObjectPtr<T> USubsystemManager::GetGameInstanceSubsystem()
{
	static_assert(is_base_of_v<UGameInstanceSubsystem, T>, "T는 UGameInstanceSubsystem을 상속 받아야 합니다");
	return GetSubsystem<T>();
}

/**
 * @brief 로컬 플레이어 서브시스템을 가져오는 함수
 * 현재 사용할 일은 없으나, 언리얼의 구조를 차용한다고 생각하면서 필요할 때까지 사용 보류
 */
template <typename T>
TObjectPtr<T> USubsystemManager::GetLocalPlayerSubsystem()
{
	static_assert(is_base_of_v<ULocalPlayerSubsystem, T>, "T는 ULocalPlayerSubsystem을 상속 받아야 합니다");
	return GetSubsystem<T>();
}

#pragma endregion Subsystem implementation functions
