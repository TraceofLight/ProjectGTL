#include "pch.h"
#include "Renderer/RenderCommand/Public/SetRenderTargetCommand.h"

#include "Render/RHI/Public/RHIDevice.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Window/Public/Viewport.h"

void FRHISetRenderTargetCommand::Execute()
{
    if (!View)
        return;

    // 뷰포트의 렌더 타겟을 설정
    FViewport* Viewport = View->GetViewport();
    if (Viewport)
    {
        URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();

        // D3D11 뷰포트 설정
        const FRect& viewportRect = Viewport->GetRect();
        D3D11_VIEWPORT D3DViewport = {};
        D3DViewport.Width = static_cast<float>(Viewport->GetSizeX());
        D3DViewport.Height = static_cast<float>(Viewport->GetSizeY());
        D3DViewport.MinDepth = 0.0f;
        D3DViewport.MaxDepth = 1.0f;
        D3DViewport.TopLeftX = static_cast<float>(viewportRect.X);
        D3DViewport.TopLeftY = static_cast<float>(viewportRect.Y);

        RHI->GetDeviceContext()->RSSetViewports(1, &D3DViewport);

        // TODO: 렌더 타겟과 깊이 버퍼 설정 - DeviceResources에서 가져와야 함
        // ID3D11RenderTargetView* RenderTargetView = DeviceResources->GetRenderTargetView();
        // ID3D11DepthStencilView* DepthStencilView = DeviceResources->GetDepthStencilView();
        // 
        // if (RenderTargetView && DepthStencilView)
        // {
        //     RHI->GetDeviceContext()->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
        // }
    }
}
