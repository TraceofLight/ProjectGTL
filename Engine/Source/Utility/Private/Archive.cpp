#include "pch.h"
#include "Utility/Public/Archive.h"
#include "Global/CoreTypes.h"
#include <filesystem>

FArchive& FArchive::operator<<(FVertex& Value)
{
	*this << Value.Position;
	*this << Value.Color;
	*this << Value.Normal;
	*this << Value.TextureCoord;
	return *this;
}

FBinaryWriter::FBinaryWriter(const FString& FilePath)
	: File(FilePath, std::ios::binary)
{
}

FBinaryWriter::~FBinaryWriter()
{
	Close();
}

FArchive& FBinaryWriter::operator<<(bool& Value)
{
	WriteData(Value);
	return *this;
}

FArchive& FBinaryWriter::operator<<(uint8& Value)
{
	WriteData(Value);
	return *this;
}

FArchive& FBinaryWriter::operator<<(int32& Value)
{
	WriteData(Value);
	return *this;
}

FArchive& FBinaryWriter::operator<<(uint32& Value)
{
	WriteData(Value);
	return *this;
}

FArchive& FBinaryWriter::operator<<(float& Value)
{
	WriteData(Value);
	return *this;
}

FArchive& FBinaryWriter::operator<<(FString& Value)
{
	uint32 Length = static_cast<uint32>(Value.length());
	WriteData(Length);
	if (Length > 0)
	{
		File.write(Value.c_str(), Length);
	}
	return *this;
}

FArchive& FBinaryWriter::operator<<(FVector& Value)
{
	WriteData(Value.X);
	WriteData(Value.Y);
	WriteData(Value.Z);
	return *this;
}

FArchive& FBinaryWriter::operator<<(FVector2& Value)
{
	WriteData(Value.X);
	WriteData(Value.Y);
	return *this;
}

FArchive& FBinaryWriter::operator<<(FVector4& Value)
{
	WriteData(Value.X);
	WriteData(Value.Y);
	WriteData(Value.Z);
	WriteData(Value.W);
	return *this;
}

// FBinaryReader 구현
FBinaryReader::FBinaryReader(const FString& FilePath)
	: File(FilePath, std::ios::binary)
{
}

FBinaryReader::~FBinaryReader()
{
	Close();
}

FArchive& FBinaryReader::operator<<(bool& Value)
{
	ReadData(Value);
	return *this;
}

FArchive& FBinaryReader::operator<<(uint8& Value)
{
	ReadData(Value);
	return *this;
}

FArchive& FBinaryReader::operator<<(int32& Value)
{
	ReadData(Value);
	return *this;
}

FArchive& FBinaryReader::operator<<(uint32& Value)
{
	ReadData(Value);
	return *this;
}

FArchive& FBinaryReader::operator<<(float& Value)
{
	ReadData(Value);
	return *this;
}

FArchive& FBinaryReader::operator<<(FString& Value)
{
	uint32 Length;
	ReadData(Length);
	if (Length > 0)
	{
		Value.resize(Length);
		File.read(&Value[0], Length);
	}
	else
	{
		Value.clear();
	}
	return *this;
}

FArchive& FBinaryReader::operator<<(FVector& Value)
{
	ReadData(Value.X);
	ReadData(Value.Y);
	ReadData(Value.Z);
	return *this;
}

FArchive& FBinaryReader::operator<<(FVector2& Value)
{
	ReadData(Value.X);
	ReadData(Value.Y);
	return *this;
}

FArchive& FBinaryReader::operator<<(FVector4& Value)
{
	ReadData(Value.X);
	ReadData(Value.Y);
	ReadData(Value.Z);
	ReadData(Value.W);
	return *this;
}

// 헬퍼 함수들
namespace FArchiveHelpers
{
	FString GetBinaryFilePath(const FString& ObjFilePath)
	{
		// FString을 std::string으로 명시적 변환
		std::filesystem::path objPath(static_cast<const std::string&>(ObjFilePath));
		std::filesystem::path binaryPath = objPath;
		binaryPath.replace_extension(".mesh");
		return binaryPath.string();
	}

	bool IsBinaryCacheValid(const FString& ObjFilePath)
	{
		FString BinaryPath = GetBinaryFilePath(ObjFilePath);

		// FString을 std::string으로 명시적 변환
		if (!std::filesystem::exists(static_cast<const std::string&>(BinaryPath)))
		{
			return false;
		}

		try
		{
			auto objTime = std::filesystem::last_write_time(static_cast<const std::string&>(ObjFilePath));
			auto binaryTime = std::filesystem::last_write_time(static_cast<const std::string&>(BinaryPath));
			return binaryTime >= objTime;
		}
		catch (const std::filesystem::filesystem_error&)
		{
			return false;
		}
	}
}
