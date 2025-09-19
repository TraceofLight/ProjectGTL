#pragma once
#include "Component/Public/MeshComponent.h"
#include "Asset/Public/StaticMesh.h"

/**
 * @brief UStaticMeshComponent: Component for rendering static meshes
 * @note Equivalent to Unreal Engine's UStaticMeshComponent
 * Holds a reference to a UStaticMesh asset and renders it
 */
UCLASS()
class UStaticMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
	UStaticMeshComponent();

	/**
	 * @brief Set the static mesh asset to render
	 * @param InStaticMesh Pointer to the static mesh asset
	 */
	void SetStaticMesh(UStaticMesh* InStaticMesh);

	/**
	 * @brief Get the current static mesh asset
	 * @return Pointer to the static mesh asset, or nullptr if none set
	 */
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }

	// Override from UMeshComponent
	virtual bool HasValidMeshData() const override;
	virtual uint32 GetNumVertices() const override;
	virtual uint32 GetNumTriangles() const override;

protected:
	// Override from UMeshComponent
	virtual void InitializeMeshRenderData() override;
	virtual void UpdateMeshBounds() override;

	/**
	 * @brief Update vertex and index buffers from static mesh data
	 */
	void UpdateRenderData();

private:
	/** The static mesh asset to render */
	UStaticMesh* StaticMesh = nullptr;
};
