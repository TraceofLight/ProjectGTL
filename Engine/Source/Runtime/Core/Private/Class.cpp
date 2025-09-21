#include "pch.h"
#include "Runtime/Core/Public/Class.h"

#include "Runtime/Core/Public/Object.h"

using std::stringstream;

// 전역 클래스 레지스트리 초기화
TArray<TObjectPtr<UClass>> UClass::AllClasses;

/**
 * @brief UClass Constructor
 * @param InName Class 이름
 * @param InSuperClass Parent Class
 * @param InClassSize Class Size
 * @param InConstructor 생성자 함수 포인터
 */
UClass::UClass(const FName& InName, TObjectPtr<UClass> InSuperClass, size_t InClassSize,
               ClassConstructorType InConstructor)
	: ClassName(InName), SuperClass(InSuperClass), ClassSize(InClassSize), Constructor(InConstructor)
{
	UE_LOG("UClass: 클래스 등록: %s", ClassName.ToString().data());
}

/**
 * @brief 이 클래스가 지정된 클래스의 하위 클래스인지 확인
 * @param InClass 확인할 클래스
 * @return 하위 클래스이거나 같은 클래스면 true
 */
bool UClass::IsChildOf(const TObjectPtr<UClass> InClass) const
{
	if (!InClass)
	{
		return false;
	}

	// 자기 자신과 같은 클래스인 경우
	if (this == InClass)
	{
		return true;
	}

	// 부모 클래스들을 거슬러 올라가면서 확인
	const UClass* CurrentClass = this;
	while (CurrentClass)
	{
		if (CurrentClass == InClass)
		{
			return true;
		}

		CurrentClass = CurrentClass->SuperClass.Get();
	}

	return false;
}

/**
 * @brief 새로운 인스턴스 생성
 * @return 생성된 객체 포인터
 */
TObjectPtr<UObject> UClass::CreateDefaultObject()
{
	if (Constructor)
	{
		++NextGenNumber;
		return Constructor();
	}

	return nullptr;
}

/**
 * @brief 클래스 이름으로 UClass 찾기
 * @param InClassName 찾을 클래스 이름
 * @return 찾은 UClass 포인터 (없으면 nullptr)
 */
TObjectPtr<UClass> UClass::FindClass(const FString& InClassName)
{
	for (TObjectPtr<UClass> Class : AllClasses)
	{
		if (Class && Class->GetClassTypeName() == InClassName)
		{
			return Class;
		}
	}

	return nullptr;
}

/**
 * @brief 모든 UClass 등록
 * @param InClass 등록할 UClass
 */
void UClass::SignUpClass(TObjectPtr<UClass> InClass)
{
	if (InClass)
	{
		AllClasses.emplace_back(InClass);
		UE_LOG("UClass: Class registered: %s (Total: %llu)", InClass->GetClassTypeName().ToString().data(), AllClasses.size());
	}
}

/**
 * @brief 등록된 모든 클래스 출력
 * For Debugging
 */
void UClass::PrintAllClasses()
{
	UE_LOG("=== Registered Classes (%llu) ===", AllClasses.size());

	for (size_t i = 0; i < AllClasses.size(); ++i)
	{
		UClass* Class = AllClasses[i];

		stringstream ss;
		ss << Class->GetClassTypeName().ToString();

		if (Class)
		{
			ss << "[" << i << "] " << Class->GetClassTypeName().ToString()
				<< " (Size: " << Class->GetClassSize() << " bytes)";

			if (Class->GetSuperClass())
			{
				ss << " -> " << Class->GetSuperClass()->GetClassTypeName().ToString();
			}
			else
			{
				ss << " (Base Class)";
			}
			UE_LOG("%s", ss.str().c_str());
		}
	}

	UE_LOG("================================");
}

/**
 * @brief 안전하게 종료 전 ClassObject에 할당한 메모리를 free하는 함수
 */
bool UClass::Shutdown()
{
	for (TObjectPtr<UClass>& ClassObject : AllClasses)
	{
		if (ClassObject.Get())
		{
			FString ClassName = ClassObject->GetClassTypeName().ToString();
			UE_LOG_WARNING("System: GC: %s에 해제되지 않은 메모리가 있습니다", ClassName.data());
			delete ClassObject.Get();
			UE_LOG_SUCCESS("System: GC: %s에 할당한 메모리를 해제했습니다", ClassName.data());
		}
	}

	return AllClasses.empty();
}
