#pragma once

namespace json
{
	class JSON;
}

using JSON = json::JSON;

enum class EPrimitiveType : uint8_t;
struct FLevelMetadata;
struct FPrimitiveMetadata;
struct FCameraMetadata;

// FontRenderer를 위한 문자 메트릭 구조체
struct CharacterMetric
{
	int x = 0, y = 0; // 아틀라스에서의 위치
	int width = 6, height = 16; // 문자 크기
	int left = 0, top = 16; // 오프셋
	int advance_x = 6; // 다음 문자와의 간격
};

/**
 * @brief Level 직렬화에 관여하는 클래스
 * JSON 기반으로 레벨의 데이터를 Save / Load 처리
 */
class FJsonSerializer
{
public:
	static JSON VectorToJson(const FVector& InVector);
	static FVector JsonToVector(const JSON& InJsonData);
	static FString PrimitiveTypeToWideString(EPrimitiveType InType);
	static EPrimitiveType StringToPrimitiveType(const FString& InTypeString);
	static JSON PrimitiveMetadataToJson(const FPrimitiveMetadata& InPrimitive);
	static FPrimitiveMetadata JsonToPrimitive(const JSON& InJsonData, uint32 InID);
	static JSON CameraMetadataToJson(const FCameraMetadata& InCamera);
	static FCameraMetadata JsonToCameraMetadata(const JSON& InJsonData);
	static JSON LevelToJson(const FLevelMetadata& InLevelData);
	static FLevelMetadata JsonToLevel(JSON& InJsonData);
	static bool SaveLevelToFile(const FLevelMetadata& InLevelData, const FString& InFilePath);
	static bool LoadLevelFromFile(FLevelMetadata& OutLevelData, const FString& InFilePath);
	static FString FormatJsonString(const JSON& JsonData, int Indent = 2);
	static bool ValidateLevelData(const FLevelMetadata& InLevelData, FString& OutErrorMessage);
	static FLevelMetadata MergeLevelData(const FLevelMetadata& InBaseLevel,
	                                     const FLevelMetadata& InMergeLevel);

	static TArray<FPrimitiveMetadata>
	FilterPrimitivesByType(const FLevelMetadata& InLevelData, EPrimitiveType InType);

	struct FLevelStats
	{
		uint32 TotalPrimitives = 0;
		TMap<EPrimitiveType, uint32> PrimitiveCountByType;
		// FVector BoundingBoxMin;
		// FVector BoundingBoxMax;
	};

	static FLevelStats GenerateLevelStats(const FLevelMetadata& InLevelData);
	static void PrintLevelInfo(const FLevelMetadata& InLevelData);

	// 폰트 메트릭 처리 함수들
	static bool LoadFontMetrics(TMap<uint32, CharacterMetric>& OutFontMetrics,
	                            float& OutAtlasWidth, float& OutAtlasHeight, const path& InFilePath);
	static CharacterMetric JsonToCharacterMetric(const JSON& InJsonData);
	static CharacterMetric GetDefaultCharacterMetric();

private:
	static bool HandleJsonError(const exception& InException, const FString& InContext,
	                            FString& OutErrorMessage);
};
