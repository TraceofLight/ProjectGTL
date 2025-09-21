#pragma once
#include "pch.h"
#include <fstream>

/**
 * @brief 바이너리 직렬화 베이스 클래스
 */
class FArchive
{
public:
	virtual ~FArchive() = default;

	// 기본 타입 직렬화
	virtual FArchive& operator<<(bool& Value) = 0;
	virtual FArchive& operator<<(uint8& Value) = 0;
	virtual FArchive& operator<<(int32& Value) = 0;
	virtual FArchive& operator<<(uint32& Value) = 0;
	virtual FArchive& operator<<(float& Value) = 0;
	virtual FArchive& operator<<(FString& Value) = 0;

	// 벡터 직렬화
	virtual FArchive& operator<<(FVector& Value) = 0;
	virtual FArchive& operator<<(FVector2& Value) = 0;
	virtual FArchive& operator<<(FVector4& Value) = 0;

	// FVertex 직렬화
	FArchive& operator<<(FVertex& Value);

	// 배열은 StaticMesh에서 직접 for문으로 처리

	// 상태 확인
	virtual bool IsLoading() const = 0;
	virtual bool IsSaving() const = 0;
};

/**
 * @brief std::ofstream 기반 바이너리 Writer
 */
class FBinaryWriter : public FArchive
{
public:
	FBinaryWriter(const FString& FilePath);
	~FBinaryWriter();

	FArchive& operator<<(bool& Value) override;
	FArchive& operator<<(uint8& Value) override;
	FArchive& operator<<(int32& Value) override;
	FArchive& operator<<(uint32& Value) override;
	FArchive& operator<<(float& Value) override;
	FArchive& operator<<(FString& Value) override;
	FArchive& operator<<(FVector& Value) override;
	FArchive& operator<<(FVector2& Value) override;
	FArchive& operator<<(FVector4& Value) override;

	bool IsLoading() const override { return false; }
	bool IsSaving() const override { return true; }

	bool IsOpen() const { return File.is_open(); }
	void Close() { if (File.is_open()) File.close(); }

private:
	std::ofstream File;

	template<typename T>
	void WriteData(const T& Value)
	{
		File.write(reinterpret_cast<const char*>(&Value), sizeof(T));
	}
};

/**
 * @brief std::ifstream 기반 바이너리 Reader
 */
class FBinaryReader : public FArchive
{
public:
	FBinaryReader(const FString& FilePath);
	~FBinaryReader();

	FArchive& operator<<(bool& Value) override;
	FArchive& operator<<(uint8& Value) override;
	FArchive& operator<<(int32& Value) override;
	FArchive& operator<<(uint32& Value) override;
	FArchive& operator<<(float& Value) override;
	FArchive& operator<<(FString& Value) override;
	FArchive& operator<<(FVector& Value) override;
	FArchive& operator<<(FVector2& Value) override;
	FArchive& operator<<(FVector4& Value) override;

	bool IsLoading() const override { return true; }
	bool IsSaving() const override { return false; }

	bool IsOpen() const { return File.is_open(); }
	void Close() { if (File.is_open()) File.close(); }

private:
	std::ifstream File;

	template<typename T>
	void ReadData(T& Value)
	{
		File.read(reinterpret_cast<char*>(&Value), sizeof(T));
	}
};

/**
 * @brief 헬퍼 함수들
 */
namespace FArchiveHelpers
{
	FString GetBinaryFilePath(const FString& ObjFilePath);
	bool IsBinaryCacheValid(const FString& ObjFilePath);
}
