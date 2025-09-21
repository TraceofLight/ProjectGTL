#pragma once

/**
 * @brief 오브젝트의 이름을 담당하는 구조체
 * 대소문자 관계 없는 비교 처리와 사용자가 직접 작성한 Display Name을 동시에 사용할 수 있음
 * @param DisplayIndex 표시용 이름 배열에 접근하기 위한 인덱스
 * @param ComparisonIndex 이름 비교를 위한 인덱스
 * @param DisplayNames 사용자가 제공한 실제 출력될 이름을 저장하는 TArray
 * @param NameMap 이름 비교를 위해 LowerString과 Index를 저장하는 TMap
 * @param NextIndex 각 이름의 고유한 인덱스를 부여하기 위한 정적 카운터
 */
struct FName
{
public:
	// DisplayIndex는 사용자가 정의한 대문자명까지 원래 이름대로 표기하기 위한 Index
	// 실제론 동일한 값을 가지나 해당 변수를 사용할 때 목적을 명확하게 사용하기 위해 구분하는 것으로 확인됨
	int32 DisplayIndex;
	int32 ComparisonIndex;

	static const FName None;

	FName(const char* InStringPtr);
	FName(const FString& InString);

	int32 Compare(const FName& InOther) const;
	bool operator==(const FName& InOther) const;
	bool operator!=(const FName& InOther) const;

	const FString& ToString() const;

	// 해시 함수를 위한 GetHash() 메서드
	size_t GetHash() const
	{
		return std::hash<int32>{}(ComparisonIndex);
	}

	// Display 이름 변경 함수
	void SetDisplayName(const FString& InDisplayName) const { DisplayNames[this->DisplayIndex] = InDisplayName; }

private:
	static TArray<FString> DisplayNames;
	static TMap<FString, uint32> NameMap;
	static uint32 NextIndex;

	FName(int32 InComparisonIndex)
	{
		this->ComparisonIndex = InComparisonIndex;
		this->DisplayIndex = this->ComparisonIndex;
	}
};

// std::hash에 대한 FName 특수화
namespace std
{
	template <>
	struct hash<FName>
	{
		size_t operator()(const FName& InName) const noexcept
		{
			return InName.GetHash();
		}
	};
}
