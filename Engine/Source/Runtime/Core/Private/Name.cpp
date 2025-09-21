#include "pch.h"
#include "Runtime/Core/Public/Name.h"

TArray<FString> FName::DisplayNames = {"None"};
TMap<FString, uint32> FName::NameMap = {{"none", 0}};
uint32 FName::NextIndex = 1;

const FName FName::None(0);

/**
 * @brief FName 생성자
 * @param InString FString 타입의 문자열
 */
FName::FName(const FString& InString)
{
	// 비교는 Lower로 진행
	FString LowerString(InString);
	std::transform(LowerString.begin(), LowerString.end(), LowerString.begin(),
	               [](unsigned char InChar) { return std::tolower(InChar); });

	// 동일 이름 탐색
	auto FindResult = NameMap.find(LowerString);

	// 동일 이름이 존재하지 않는 경우
	if (FindResult == NameMap.end())
	{
		// NameMap에 추가하고 이름 TArray에 각각 추가
		NameMap.insert({LowerString, NextIndex});
		DisplayNames.push_back(InString);

		// 인덱스 제공
		ComparisonIndex = NextIndex++;
		DisplayIndex = ComparisonIndex;
	}
	// 이미 존재하는 경우
	else
	{
		ComparisonIndex = FindResult->second;
		DisplayIndex = ComparisonIndex;
	}
}

/**
 * @brief c-style 문자열을 인자로 받는 FName 생성자
 * @param InStringPtr c-style 문자열
 */
FName::FName(const char* InStringPtr)
	: FName(FString(InStringPtr))
{
}

/**
 * @brief 두 FName을 비교하는 멤버 함수
 * @param InOther 다른 FName
 * @return 둘 사이의 인덱스 거리 차이
 */
int32 FName::Compare(const FName& InOther) const
{
	return this->ComparisonIndex - InOther.ComparisonIndex;
}

/**
 * FName 비교 연산자
 * @param InOther 비교할 다른 FName
 * @return 같은지 여부
 */
bool FName::operator==(const FName& InOther) const
{
	return this->Compare(InOther) == 0;
}

/**
 * FName 비교 연산자 (부등호)
 * @param InOther 비교할 다른 FName
 * @return 다른지 여부
 */
bool FName::operator!=(const FName& InOther) const
{
	return !(*this == InOther);
}

/**
 * @brief 사용자가 제공한 이름을 반환하는 멤버 함수
 * @return DisplayName
 */
const FString& FName::ToString() const
{
	return DisplayNames[DisplayIndex];
}
