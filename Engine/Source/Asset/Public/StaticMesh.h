#pragma once
#include "../../Core/Public/Object.h"
#include "Asset/Public/StaticMeshData.h"

/**
 * @brief UStaticMesh: Asset class that holds static mesh data
 * @note Equivalent to Unreal Engine's UStaticMesh
 * Contains the cooked mesh data for rendering
 */
UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();

	/**
	 * @brief Get the static mesh data
	 * @return Reference to the FStaticMesh data structure
	 */
	const FStaticMesh& GetStaticMeshData() const { return StaticMeshData; }

	/**
	 * @brief Set the static mesh data
	 * @param InStaticMeshData The mesh data to set
	 */
	void SetStaticMeshData(const FStaticMesh& InStaticMeshData);

	/**
	 * @brief Get the source file path
	 * @return Path to the source mesh file
	 */
	const FString& GetSourceFilePath() const { return StaticMeshData.PathFileName; }

	/**
	 * @brief Get vertex data for rendering
	 * @return Array of vertices
	 */
	const TArray<FVertex>& GetVertices() const { return StaticMeshData.Vertices; }

	/**
	 * @brief Get index data for rendering
	 * @return Array of indices
	 */
	const TArray<uint32>& GetIndices() const { return StaticMeshData.Indices; }

	/**
	 * @brief Check if the mesh has valid data
	 * @return True if mesh has vertices and indices
	 */
	bool IsValidMesh() const;

protected:
	/** The actual mesh data */
	FStaticMesh StaticMeshData;
};
