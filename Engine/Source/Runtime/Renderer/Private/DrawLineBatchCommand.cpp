#include "pch.h"
#include "Runtime/Renderer/Public/DrawLineBatchCommand.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Core/Public/VertexTypes.h"

FRHIDrawLineBatchCommand::FRHIDrawLineBatchCommand(
	FRHIDevice* InRHIDevice,
	ID3D11Buffer* InVertexBuffer,
	ID3D11Buffer* InIndexBuffer,
	uint32 InIndexCount,
	const FMatrix& InModelMatrix,
	const FMatrix& InViewMatrix,
	const FMatrix& InProjMatrix,
	const FRect* InScissorRect
)
	: RHIDevice(InRHIDevice)
	, VertexBuffer(InVertexBuffer)
	, IndexBuffer(InIndexBuffer)
	, IndexCount(InIndexCount)
	, ModelMatrix(InModelMatrix)
	, ViewMatrix(InViewMatrix)
	, ProjMatrix(InProjMatrix)
	, bHasScissorRect(InScissorRect != nullptr)
{
	if (InScissorRect)
	{
		ScissorRect = *InScissorRect;
	}
}

FRHIDrawLineBatchCommand::~FRHIDrawLineBatchCommand()
{
	// 버퍼 정리
	if (RHIDevice)
	{
		if (VertexBuffer)
		{
			RHIDevice->ReleaseVertexBuffer(VertexBuffer);
		}
		if (IndexBuffer)
		{
			RHIDevice->ReleaseIndexBuffer(IndexBuffer);
		}
	}
}

void FRHIDrawLineBatchCommand::Execute()
{
	if (!RHIDevice || !VertexBuffer || !IndexBuffer || IndexCount == 0)
	{
		return;
	}

	ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
	if (!DeviceContext)
	{
		return;
	}

	// 상수 버퍼 업데이트
	RHIDevice->UpdateConstantBuffers(ModelMatrix, ViewMatrix, ProjMatrix);

	// Depth Test 비활성화 (라인은 항상 보여야 함)
	D3D11_DEPTH_STENCIL_DESC DepthDesc = {};
	DepthDesc.DepthEnable = FALSE;
	DepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthDesc.StencilEnable = FALSE;

	ID3D11DepthStencilState* DepthState = nullptr;
	RHIDevice->GetDevice()->CreateDepthStencilState(&DepthDesc, &DepthState);
	if (DepthState)
	{
		DeviceContext->OMSetDepthStencilState(DepthState, 0);
	}

	// Blend State 설정
	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = FALSE;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* BlendState = nullptr;
	RHIDevice->GetDevice()->CreateBlendState(&BlendDesc, &BlendState);
	if (BlendState)
	{
		DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
	}

	// Rasterizer State 설정
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = D3D11_FILL_SOLID;
	RasterizerDesc.CullMode = D3D11_CULL_NONE;
	RasterizerDesc.FrontCounterClockwise = FALSE;
	RasterizerDesc.DepthClipEnable = TRUE;
	RasterizerDesc.ScissorEnable = bHasScissorRect ? TRUE : FALSE;

	ID3D11RasterizerState* RastState = nullptr;
	RHIDevice->GetDevice()->CreateRasterizerState(&RasterizerDesc, &RastState);
	if (RastState)
	{
		DeviceContext->RSSetState(RastState);
	}

	// Scissor Rect 설정
	if (bHasScissorRect)
	{
		D3D11_RECT D3DScissorRect;
		D3DScissorRect.left = static_cast<LONG>(ScissorRect.X);
		D3DScissorRect.top = static_cast<LONG>(ScissorRect.Y);
		D3DScissorRect.right = static_cast<LONG>(ScissorRect.X + ScissorRect.W);
		D3DScissorRect.bottom = static_cast<LONG>(ScissorRect.Y + ScissorRect.H);
		DeviceContext->RSSetScissorRects(1, &D3DScissorRect);
	}

	// 셔이더 설정 (BatchLine 셔이더 사용)
	// D3D11RHIModule을 통해 EditorResources 접근
	FD3D11RHIModule& RHIModule = FD3D11RHIModule::GetInstance();
	FEditorRenderResources* EditorResources = RHIModule.GetEditorResources();
	if (!EditorResources)
	{
		return;
	}

	ID3D11VertexShader* VS = EditorResources->GetEditorVertexShader(EShaderType::BatchLine);
	ID3D11PixelShader* PS = EditorResources->GetEditorPixelShader(EShaderType::BatchLine);
	ID3D11InputLayout* IL = EditorResources->GetEditorInputLayout(EShaderType::BatchLine);

	DeviceContext->VSSetShader(VS, nullptr, 0);
	DeviceContext->PSSetShader(PS, nullptr, 0);
	DeviceContext->IASetInputLayout(IL);

	// 버퍼 바인딩
	UINT Stride = sizeof(FVertexSimple);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 렌더링
	DeviceContext->DrawIndexed(IndexCount, 0, 0);

	// State 해제
	if (DepthState)
	{
		DepthState->Release();
	}
	if (BlendState)
	{
		BlendState->Release();
	}
	if (RastState)
	{
		RastState->Release();
	}
}
