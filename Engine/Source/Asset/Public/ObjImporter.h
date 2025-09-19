#pragma once
#include "Asset/Public/StaticMeshData.h"
#include "Global/Types.h"

/**
 * @brief FObjImporter: Handles importing and parsing of OBJ files
 * @note Equivalent to Unreal Engine's FBX/OBJ importers
 * Converts raw OBJ data to engine-ready formats
 */
class FObjImporter
{
public:
	FObjImporter() = default;
	~FObjImporter() = default;

	/**
	 * @brief Import an OBJ file and parse it into raw object data
	 * @param FilePath Path to the OBJ file to import
	 * @param OutObjectInfos Array of parsed object information
	 * @return True if import was successful, false otherwise
	 */
	static bool ImportOBJFile(const FString& FilePath, TArray<FObjInfo>& OutObjectInfos);

	/**
	 * @brief Parse material library file (.mtl)
	 * @param MTLFilePath Path to the MTL file
	 * @param OutMaterials Array of parsed material information
	 * @return True if parsing was successful, false otherwise
	 */
	static bool ParseMaterialLibrary(const FString& MTLFilePath, TArray<FObjMaterialInfo>& OutMaterials);

	/**
	 * @brief Convert raw object data to cooked static mesh data
	 * @param ObjectInfos Array of raw object data
	 * @param OutStaticMesh The resulting cooked static mesh data
	 * @return True if conversion was successful, false otherwise
	 */
	static bool ConvertToStaticMesh(const TArray<FObjInfo>& ObjectInfos, FStaticMesh& OutStaticMesh);

	/**
	 * @brief Complete import pipeline: OBJ → Raw Data → Cooked Data
	 * @param FilePath Path to the OBJ file to import
	 * @param OutStaticMesh The resulting cooked static mesh data
	 * @return True if entire pipeline was successful, false otherwise
	 */
	static bool ImportStaticMesh(const FString& FilePath, FStaticMesh& OutStaticMesh);

private:
	/**
	 * @brief Parse a single line from an OBJ file
	 * @param Line The line to parse
	 * @param CurrentObject The object currently being parsed
	 * @param AllObjects All objects being parsed (for group management)
	 * @param GlobalVertices Global vertex list
	 * @param GlobalUVs Global UV list
	 * @param GlobalNormals Global normal list
	 */
	static void ParseOBJLine(const FString& Line,
		FObjInfo& CurrentObject,
		TArray<FObjInfo>& AllObjects,
		TArray<FVector>& GlobalVertices,
		TArray<FVector2>& GlobalUVs,
		TArray<FVector>& GlobalNormals);

	/**
	 * @brief Parse face data from OBJ file
	 * @param FaceData The face data string (e.g., "1/1/1 2/2/2 3/3/3")
	 * @param CurrentObject The object to add face data to
	 */
	static void ParseFaceData(const FString& FaceData, FObjInfo& CurrentObject);

	/**
	 * @brief Convert face indices to triangle list with proper vertex data
	 * @param ObjectInfo Raw object data with separate vertex/UV/normal arrays
	 * @param OutVertices Final vertex array with combined data
	 * @param OutIndices Final index array for triangles
	 */
	static void ConvertToTriangleList(const FObjInfo& ObjectInfo,
		TArray<FVertex>& OutVertices,
		TArray<uint32>& OutIndices);

	/**
	 * @brief Helper function to trim whitespace from strings
	 * @param Str String to trim
	 * @return Trimmed string
	 */
	static FString TrimString(const FString& Str);

	/**
	 * @brief Helper function to split strings by delimiter
	 * @param Str String to split
	 * @param Delimiter Delimiter character
	 * @param OutTokens Array of resulting tokens
	 */
	static void SplitString(const FString& Str, char Delimiter, TArray<FString>& OutTokens);

	/**
	 * @brief Convert OBJ position to UE coordinate system
	 * @param InVector Position vector from OBJ file
	 * @return Position vector in UE coordinate system
	 */
	static FVector PositionToUEBasis(const FVector& InVector);

	/**
	 * @brief Convert OBJ UV coordinates to UE coordinate system
	 * @param InVector UV vector from OBJ file
	 * @return UV vector in UE coordinate system
	 */
	static FVector2 UVToUEBasis(const FVector2& InVector);
};
