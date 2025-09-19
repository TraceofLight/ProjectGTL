#pragma once
#include "Component/Public/PrimitiveComponent.h"

/**
 * @brief UMeshComponent: Base class for all mesh-based components
 * @note Equivalent to Unreal Engine's UMeshComponent
 * Provides common functionality for components that render meshes
 */
UCLASS()
class UMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)

public:
	UMeshComponent();

	/**
	 * @brief Check if the component has valid mesh data for rendering
	 * @return True if mesh data is available and valid
	 */
	virtual bool HasValidMeshData() const;

	/**
	 * @brief Get the number of vertices in the mesh
	 * @return Number of vertices
	 */
	virtual uint32 GetNumVertices() const;

	/**
	 * @brief Get the number of triangles in the mesh
	 * @return Number of triangles
	 */
	virtual uint32 GetNumTriangles() const;

protected:
	/**
	 * @brief Initialize mesh-specific rendering data
	 * Called when mesh data changes
	 */
	virtual void InitializeMeshRenderData();

	/**
	 * @brief Update mesh bounds for culling
	 */
	virtual void UpdateMeshBounds();
};
