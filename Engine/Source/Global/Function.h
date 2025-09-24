#pragma once

using std::string;
using std::wstring;

template <typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}

/**
 * @brief wstring을 멀티바이트 FString으로 변환합니다.
 * @param InString 변환할 FString
 * @return 변환된 wstring
 */
static FString WideStringToString(const wstring& InString)
{
	int32 ByteNumber = WideCharToMultiByte(CP_UTF8, 0,
	                                       InString.c_str(), -1, nullptr, 0, nullptr, nullptr);

	FString OutString(ByteNumber, 0);

	WideCharToMultiByte(CP_UTF8, 0,
	                    InString.c_str(), -1, OutString.data(), ByteNumber, nullptr, nullptr);

	return OutString;
}

/**
 * @brief 멀티바이트 UTF-8 FString을 와이드 스트링(wstring)으로 변환합니다.
 * @param InString 변환할 FString (UTF-8)
 * @return 변환된 wstring
 */
static wstring StringToWideString(const FString& InString)
{
	// 필요한 와이드 문자의 개수를 계산
	int32 WideCharNumber = MultiByteToWideChar(CP_UTF8, 0,
	                                           InString.c_str(), -1, nullptr, 0);

	// 계산된 크기만큼 wstring 버퍼를 할당
	wstring OutString(WideCharNumber, 0);

	// 실제 변환을 수행
	MultiByteToWideChar(CP_UTF8, 0,
	                    InString.c_str(), -1, OutString.data(), WideCharNumber);

	// null 문자 제거
	OutString.resize(WideCharNumber - 1);

	return OutString;
}

/**
 * @brief CP949 (ANSI) 타입의 문자열을 UTF-8 타입으로 변환하는 함수
 * CP949 To UTF-8 직변환 함수가 제공되지 않아 UTF-16을 브릿지로 사용한다
 * @param InANSIString ANSI 문자열
 * @return UTF-8 문자열
 */
static string ConvertCP949ToUTF8(const char* InANSIString)
{
	if (InANSIString == nullptr || InANSIString[0] == '\0')
	{
		return "";
	}

	// CP949 -> UTF-8 변환
	int WideCharacterSize = MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, nullptr, 0);
	if (WideCharacterSize == 0)
	{
		return "";
	}

	wstring WideString(WideCharacterSize, 0);
	MultiByteToWideChar(CP_ACP, 0, InANSIString, -1, WideString.data(), WideCharacterSize);

	// UTF-16 -> UTF-8 변환
	int UTF8Size = WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (UTF8Size == 0)
	{
		return "";
	}

	string UTF8String(UTF8Size, 0);
	WideCharToMultiByte(CP_UTF8, 0, WideString.c_str(), -1, UTF8String.data(), UTF8Size, nullptr, nullptr);

	if (!UTF8String.empty() && UTF8String.back() == '\0')
	{
		UTF8String.pop_back();
	}

	return UTF8String;
}

/**
 * @brief 문자 유니코드 값으로 반환하는 함수
 * @param InUTF8String 입력되는 UTF-8 문자열
 * @param InIndex 문자열에서 내가 원하는 문자 위치
 * @return 해당 글자의 Unicode 값
 */
static uint32 UTF8ToUnicode(const string& InUTF8String, uint32 InIndex)
{
	// 인덱스가 문자열 범위를 벗어나는지 확인
	if (InIndex >= InUTF8String.length())
	{
		return 0;
	}

	uint32_t CodePoint = 0;

	// char가 signed일 경우를 대비해 unsigned char*로 캐스팅하여 비트 연산 오류 방지
	const unsigned char* Ptr = reinterpret_cast<const unsigned char*>(InUTF8String.data()) + InIndex;
	const unsigned char FirstByte = *Ptr;

	// 첫 바이트를 확인, UTF-8 문자를 구성하는 바이트 수를 결정 및 디코딩
	// 1바이트 문자: 0xxxxxxx
	if (FirstByte < 0x80)
	{
		CodePoint = FirstByte;
	}
	// 2바이트 문자: 110xxxxx 10xxxxxx
	else if ((FirstByte & 0xE0) == 0xC0)
	{
		// 문자열 끝을 넘어서는지 확인
		if (InIndex + 1 >= InUTF8String.length()) return 0;

		const unsigned char SecondByte = *(Ptr + 1);
		CodePoint = ((FirstByte & 0x1F) << 6) | (SecondByte & 0x3F);
	}
	// 3바이트 문자: 1110xxxx 10xxxxxx 10xxxxxx
	else if ((FirstByte & 0xF0) == 0xE0)
	{
		if (InIndex + 2 >= InUTF8String.length()) return 0;

		const unsigned char SecondByte = *(Ptr + 1);
		const unsigned char ThirdByte = *(Ptr + 2);
		CodePoint = ((FirstByte & 0x0F) << 12) | ((SecondByte & 0x3F) << 6) | (ThirdByte & 0x3F);
	}
	// 4바이트 문자: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((FirstByte & 0xF8) == 0xF0)
	{
		if (InIndex + 3 >= InUTF8String.length()) return 0;

		const unsigned char SecondByte = *(Ptr + 1);
		const unsigned char ThirdByte = *(Ptr + 2);
		const unsigned char FourthByte = *(Ptr + 3);
		CodePoint = ((FirstByte & 0x07) << 18) | ((SecondByte & 0x3F) << 12) | ((ThirdByte & 0x3F) << 6) | (FourthByte & 0x3F);
	}

	// 그 외의 경우는 잘못된 UTF-8 시작이므로 CodePoint는 0으로 유지, 도출된 CodePoint 반환
	return CodePoint;
}
