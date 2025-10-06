#pragma once
#include "Class.h"
#include "Name.h"
#include "ObjectPtr.h"

class UWorld;

UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	// Special Member Function
	UObject();
	explicit UObject(const FName& InName);
	virtual ~UObject();

	bool IsA(TObjectPtr<UClass> InClass) const;

	static void CheckAndCleanupGUObjectArray();
	static void CleanupGUObjectArray();
	static uint32 GetNullObjectCount();

	virtual UObject* Duplicate();
	virtual void DuplicateSubObjects() {}

	// Getter & Setter
	const FName& GetName() const { return Name; }
	TObjectPtr<UObject> GetOuter() const { return Outer; }
	virtual TObjectPtr<UWorld> GetWorld() const { return nullptr; }

	void SetName(const FName& InName) { Name = InName; }
	void SetOuter(UObject* InObject);

	// 기본적으로 외부에서는 생성자 제외 DisplayName만 변경할 수 있도록 처리
	void SetDisplayName(const FString& InName) const { Name.SetDisplayName(InName); }

	uint32 GetUUID() const { return UUID; }
	uint32 GetInternalIndex() const { return InternalIndex; }

private:
	uint32 UUID;
	uint32 InternalIndex;
	FName Name;
	TObjectPtr<UObject> Outer;

	void SetInternalIndex(uint32 InNewIndex) { InternalIndex = InNewIndex; }
};

/**
 * @brief 안전한 타입 캐스팅 함수 (원시 포인터용)
 * UClass::IsChildOf를 사용한 런타임 타입 체크
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return 캐스팅 성공시 T*, 실패시 nullptr
 */
template <typename T>
T* Cast(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (const 원시 포인터용)
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 const 원시 포인터
 * @return 캐스팅 성공시 const T*, 실패시 nullptr
 */
template <typename T>
const T* Cast(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return nullptr;
	}

	// 런타임 타입 체크: InObject가 T 타입인가?
	if (InObject->IsA(T::StaticClass()))
	{
		return static_cast<const T*>(InObject);
	}

	return nullptr;
}

/**
 * @brief 안전한 타입 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return 캐스팅 성공시 TObjectPtr<T>, 실패시 nullptr을 담은 TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> Cast(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "Cast<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "Cast<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = Cast<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 빠른 캐스팅 함수 (런타임 체크 없음, 디버그에서만 체크)
 * 성능이 중요한 상황에서 사용, 타입 안전성을 개발자가 보장해야 함
 * @tparam T 캐스팅할 대상 타입
 * @param InObject 캐스팅할 원시 포인터
 * @return T* (런타임 체크 없이 static_cast)
 */
template <typename T>
T* CastChecked(UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");

#ifdef _DEBUG
	// 디버그 빌드에서는 안전성 체크
	if (InObject && !InObject->IsA(T::StaticClass()))
	{
		// 로그나 assert 추가 가능
		return nullptr;
	}
#endif

	return static_cast<T*>(InObject);
}

/**
 * @brief 빠른 캐스팅 함수 (TObjectPtr용)
 * @tparam T 캐스팅할 대상 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 캐스팅할 TObjectPtr
 * @return TObjectPtr<T>
 */
template <typename T, typename U>
TObjectPtr<T> CastChecked(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "CastChecked<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "CastChecked<T>: U는 UObject를 상속받아야 합니다");

	U* RawPtr = InObjectPtr.Get();
	T* CastedPtr = CastChecked<T>(RawPtr);

	return TObjectPtr<T>(CastedPtr);
}

/**
 * @brief 타입 체크 함수 (캐스팅하지 않고 체크만)
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return InObject가 T 타입이면 true
 */
template <typename T>
bool IsA(const UObject* InObject)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");

	if (!InObject)
	{
		return false;
	}

	return InObject->IsA(T::StaticClass());
}

/**
 * @brief 타입 체크 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return InObjectPtr가 T 타입이면 true
 */
template <typename T, typename U>
bool IsA(const TObjectPtr<U>& InObjectPtr)
{
	static_assert(std::is_base_of_v<UObject, T>, "IsA<T>: T는 UObject를 상속받아야 합니다");
	static_assert(std::is_base_of_v<UObject, U>, "IsA<T>: U는 UObject를 상속받아야 합니다");

	return IsA<T>(InObjectPtr.Get());
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수
 * @tparam T 체크할 타입
 * @param InObject 체크할 객체
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T>
bool IsValid(const UObject* InObject)
{
	return InObject && IsA<T>(InObject);
}

/**
 * @brief 유효성과 타입을 동시에 체크하는 함수 (TObjectPtr용)
 * @tparam T 체크할 타입
 * @tparam U 소스 타입
 * @param InObjectPtr 체크할 TObjectPtr
 * @return 객체가 유효하고 T 타입이면 true
 */
template <typename T, typename U>
bool IsValid(const TObjectPtr<U>& InObjectPtr)
{
	return InObjectPtr && IsA<T>(InObjectPtr);
}


template<typename T>
T* GetTypedOuter(const UObject* InObject)
{
	if (!InObject)
	{
		return nullptr;
	}

	// Outer 체인을 따라 올라가는 루프
	const UObject* CurrentOuter = InObject->GetOuter();
	while (CurrentOuter != nullptr)
	{
		if (CurrentOuter->IsA(T::StaticClass))
		{
			return static_cast<T*>(const_cast<UObject*>(CurrentOuter));
		}

		CurrentOuter = CurrentOuter->GetOuter();
	}

	return nullptr;
}

extern TArray<TObjectPtr<UObject>> GUObjectArray;
