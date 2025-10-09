#pragma once

class FString : public std::string
{
public:
	// 기본 생성자들
	FString() : std::string() {}
	FString(const std::string& str) : std::string(str) {}
	FString(const char* str) : std::string(str) {}
	FString(size_t n, char c) : std::string(n, c) {}

	// 언리얼 스타일 메서드들
	bool IsEmpty() const { return empty(); }
	size_t Len() const { return length(); }

	// std::string 호환을 위한 캐스팅
	operator const std::string&() const { return *this; }
	operator std::string&() { return *this; }

	// 대입 연산자
	FString& operator=(const std::string& str) {
		std::string::operator=(str);
		return *this;
	}
	FString& operator=(const char* str) {
		std::string::operator=(str);
		return *this;
	}
	
	// 해시 함수를 위한 GetHash() 메서드
	size_t GetHash() const
	{
		return std::hash<std::string>{}(static_cast<const std::string&>(*this));
	}
};

// std::hash에 대한 FString 특수화
namespace std
{
	template <>
	struct hash<FString>
	{
		size_t operator()(const FString& InString) const noexcept
		{
			return InString.GetHash();
		}
	};
}
