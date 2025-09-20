#pragma once
#include "Component/Public/SceneComponent.h"
#include "Physics/Public/BoundingVolume.h"

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	const TArray<FVertex>* GetVerticesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	const FRenderState& GetRenderState() const { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

	EPrimitiveType GetPrimitiveType() const { return Type; }

	// 공통 렌더링 인터페이스
	virtual bool HasRenderData() const;
	virtual ID3D11Buffer* GetRenderVertexBuffer() const;
	virtual ID3D11Buffer* GetRenderIndexBuffer() const;
	virtual uint32 GetRenderVertexCount() const;
	virtual uint32 GetRenderIndexCount() const;
	virtual uint32 GetRenderVertexStride() const;
	virtual bool UseIndexedRendering() const;
	virtual EShaderType GetShaderType() const;

	// BoundingVolume 관련 함수들
	/**
	 * @brief 월드 좌표계 AABB를 가져옴
	 * @param OutMin 최소 좌표 (출력)
	 * @param OutMax 최대 좌표 (출력)
	 */
	virtual void GetWorldAABB(FVector& OutMin, FVector& OutMax) const;

	/**
	 * @brief BoundingVolume을 가져옴
	 */
	IBoundingVolume* GetBoundingVolume() const { return BoundingVolume; }

protected:
	// 기즈모 때문에 지우면 안될듯..?
	const TArray<FVertex>* Vertices = nullptr;
	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };
	ID3D11Buffer* Vertexbuffer = nullptr;
	uint32 NumVertices = 0;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRenderState RenderState = {};
	EPrimitiveType Type = EPrimitiveType::Cube;
	//

	bool bVisible = true;

	IBoundingVolume* BoundingVolume = nullptr;
};
