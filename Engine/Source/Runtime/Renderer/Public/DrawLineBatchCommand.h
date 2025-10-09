#pragma once

#include "RenderCommand.h"
#include "Global/CoreTypes.h"
#include <d3d11.h>

class FRHIDevice;

/**
 * @brief Line Batch 렌더링 Command
 * LineBatcher에서 생성한 버퍼를 사용하여 라인을 렌더링
 */
class FRHIDrawLineBatchCommand : public IRHICommand
{
public:
	FRHIDrawLineBatchCommand(
		FRHIDevice* InRHIDevice,
		ID3D11Buffer* InVertexBuffer,
		ID3D11Buffer* InIndexBuffer,
		uint32 InIndexCount,
		const FMatrix& InModelMatrix,
		const FMatrix& InViewMatrix,
		const FMatrix& InProjMatrix,
		const FRect* InScissorRect
	);

	~FRHIDrawLineBatchCommand() override;

	void Execute() override;

	ERHICommandType GetCommandType() const override
	{
		return ERHICommandType::DrawIndexedPrimitives;
	}

private:
	FRHIDevice* RHIDevice;
	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;
	uint32 IndexCount;
	FMatrix ModelMatrix;
	FMatrix ViewMatrix;
	FMatrix ProjMatrix;
	FRect ScissorRect;
	bool bHasScissorRect;
};
