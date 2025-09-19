#pragma once
#include "Global/Types.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"

/**
 * @brief Material information for objects in a static mesh
 * Contains all material properties including texture paths and lighting parameters.
 */
struct FObjectMaterialInfo
{
	FString MaterialName;
	FString DiffuseTexturePath;
	FString NormalTexturePath;
	FString SpecularTexturePath;
	FVector AmbientColorScalar = FVector(0.2f, 0.2f, 0.2f);
	FVector DiffuseColorScalar = FVector(0.8f, 0.8f, 0.8f);
	FVector SpecularColorScalar = FVector(1.0f, 1.0f, 1.0f);
	float ShininessScalar = 32.0f;
	float TransparencyScalar = 1.0f;

	FObjectMaterialInfo() = default;

	/**
	 * @brief Constructor with material name
	 * @param InMaterialName Name of the material to create
	 */
	FObjectMaterialInfo(const FString& InMaterialName)
		: MaterialName(InMaterialName)
	{
	}
};

/**
 * @brief Raw data structure for a single object within a static mesh
 *
 * Contains separate arrays for each type of data as parsed from OBJ files.
 * This structure holds raw OBJ data before it's processed into final vertex format.
 * Similar to intermediate parsing structures in OBJ loaders.
 */
struct FObjectInfo
{
	FString ObjectName;							// Name of the object (from OBJ 'o' or 'g' command)
	TArray<FVector> VertexList;					// Raw vertex positions from OBJ 'v' commands
	TArray<FVector2> UVList;					// Raw UV coordinates from OBJ 'vt' commands
	TArray<FVector> NormalList;					// Raw normal vectors from OBJ 'vn' commands
	TArray<uint32> VertexIndexList;				// Vertex indices from OBJ 'f' commands (references VertexList)
	TArray<uint32> UVIndexList;					// UV indices from OBJ 'f' commands (references UVList)
	TArray<uint32> NormalIndexList;				// Normal indices from OBJ 'f' commands (references NormalList)
	TArray<FObjectMaterialInfo> MaterialList;	// List of materials used by this object
	TArray<FString> TextureList;				// List of texture file paths used by this object

	FObjectInfo() = default;

	/**
	 * @brief Constructor with object name
	 * @param InObjectName Name of the object to create
	 */
	FObjectInfo(const FString& InObjectName)
		: ObjectName(InObjectName)
	{
	}
};

/**
 * @brief Static Mesh Data Structure (Unreal: `FStaticMeshLODResources`)
 * @note This structure holds the final processed data for rendering.
 * Contains cooked vertex data and indices for GPU buffer creation.
 */
struct FStaticMesh
{
	FString PathFileName;			// Path to the source file (e.g., "Assets/Models/House.obj")
	TArray<FVertex> Vertices;		// Final processed vertices (position, normal, UV combined)
	TArray<uint32> Indices;			// Index buffer for rendering triangles

	FStaticMesh() = default;

	/**
	 * @brief Constructor with file path
	 * @param InPathFileName Path to the source mesh file
	 */
	FStaticMesh(const FString& InPathFileName)
		: PathFileName(InPathFileName)
	{
	}
};
