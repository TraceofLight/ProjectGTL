#include "pch.h"
#include "Runtime/Renderer/Public/PresentCommands.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"

//=============================================================================
// FRHIPresentCommand
//=============================================================================

FRHIPresentCommand::FRHIPresentCommand(FRHIDevice* InRHIDevice)
    : RHIDevice(InRHIDevice)
{
}

void FRHIPresentCommand::Execute()
{
    if (!RHIDevice)
    {
        UE_LOG_ERROR("FRHIPresentCommand: RHIDevice가 null입니다");
        return;
    }

    IDXGISwapChain* SwapChain = RHIDevice->GetSwapChain();
    if (!SwapChain)
    {
        UE_LOG_ERROR("FRHIPresentCommand: SwapChain을 찾을 수 없습니다");
        return;
    }

    // Present 실행 (VSync Off)
    HRESULT hr = SwapChain->Present(0, 0);
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            HRESULT reason = RHIDevice->GetDevice() ? RHIDevice->GetDevice()->GetDeviceRemovedReason() : hr;
            UE_LOG_ERROR("FRHIPresentCommand: DEVICE LOST during Present (hr=0x%08lX, reason=0x%08lX)", hr, reason);
            FD3D11RHIModule::GetInstance().RecreateAfterDeviceLost();
        }
        else
        {
            UE_LOG_ERROR("FRHIPresentCommand: Present 실패 (HRESULT: 0x%08lX)", hr);
        }
    }
}

ERHICommandType FRHIPresentCommand::GetCommandType() const
{
    return ERHICommandType::Present;
}

//=============================================================================
// FRHIGetBackBufferSurfaceCommand
//=============================================================================

FRHIGetBackBufferSurfaceCommand::FRHIGetBackBufferSurfaceCommand(FRHIDevice* InRHIDevice, IDXGISurface** OutSurface)
    : RHIDevice(InRHIDevice), OutputSurface(OutSurface)
{
}

void FRHIGetBackBufferSurfaceCommand::Execute()
{
    if (!RHIDevice || !OutputSurface)
    {
        UE_LOG_ERROR("FRHIGetBackBufferSurfaceCommand: 잘못된 파라미터");
        return;
    }

    IDXGISwapChain* SwapChain = RHIDevice->GetSwapChain();
    if (!SwapChain)
    {
        UE_LOG_ERROR("FRHIGetBackBufferSurfaceCommand: SwapChain을 찾을 수 없습니다");
        return;
    }

    // BackBuffer Surface 가져오기
    HRESULT hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(OutputSurface));
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FRHIGetBackBufferSurfaceCommand: GetBuffer 실패 (HRESULT: 0x%08lX)", hr);
        *OutputSurface = nullptr;
    }
}

ERHICommandType FRHIGetBackBufferSurfaceCommand::GetCommandType() const
{
    return ERHICommandType::GetBackBufferSurface;
}

//=============================================================================
// FRHIGetBackBufferRTVCommand
//=============================================================================

FRHIGetBackBufferRTVCommand::FRHIGetBackBufferRTVCommand(FRHIDevice* InRHIDevice, ID3D11RenderTargetView** OutRTV)
    : RHIDevice(InRHIDevice), OutputRTV(OutRTV)
{
}

void FRHIGetBackBufferRTVCommand::Execute()
{
    if (!RHIDevice || !OutputRTV)
    {
        UE_LOG_ERROR("FRHIGetBackBufferRTVCommand: 잘못된 파라미터");
        return;
    }

    IDXGISwapChain* SwapChain = RHIDevice->GetSwapChain();
    if (!SwapChain)
    {
        UE_LOG_ERROR("FRHIGetBackBufferRTVCommand: SwapChain을 찾을 수 없습니다");
        return;
    }

    // BackBuffer Texture2D 가져오기
    ID3D11Texture2D* BackBuffer = nullptr;
    HRESULT hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
    if (FAILED(hr))
    {
        UE_LOG_ERROR("FRHIGetBackBufferRTVCommand: GetBuffer 실패 (HRESULT: 0x%08lX)", hr);
        return;
    }

    // RenderTargetView 생성
    hr = RHIDevice->GetDevice()->CreateRenderTargetView(BackBuffer, nullptr, OutputRTV);
    BackBuffer->Release();

    if (FAILED(hr))
    {
        UE_LOG_ERROR("FRHIGetBackBufferRTVCommand: CreateRenderTargetView 실패 (HRESULT: 0x%08lX)", hr);
        *OutputRTV = nullptr;
    }
}

ERHICommandType FRHIGetBackBufferRTVCommand::GetCommandType() const
{
    return ERHICommandType::GetBackBufferRTV;
}
