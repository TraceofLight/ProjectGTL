#pragma once

#include "RenderCommand.h"

class FRHIDevice;

/**
 * @brief SwapChain Present 명령 (언리얼 스타일)
 */
class FRHIPresentCommand : public IRHICommand
{
public:
    FRHIPresentCommand(FRHIDevice* InRHIDevice);

    void Execute() override;
    ERHICommandType GetCommandType() const override;
    FName GetName() const override { return FName("Present"); }

private:
    FRHIDevice* RHIDevice;
};

/**
 * @brief BackBuffer Surface 접근 명령 (Direct2D 용)
 */
class FRHIGetBackBufferSurfaceCommand : public IRHICommand
{
public:
    FRHIGetBackBufferSurfaceCommand(FRHIDevice* InRHIDevice, IDXGISurface** OutSurface);

    virtual void Execute() override;
    virtual ERHICommandType GetCommandType() const override;
    virtual FName GetName() const override { return FName("GetBackBufferSurface"); }

private:
    FRHIDevice* RHIDevice;
    IDXGISurface** OutputSurface;
};

/**
 * @brief BackBuffer RenderTargetView 접근 명령
 */
class FRHIGetBackBufferRTVCommand : public IRHICommand
{
public:
    FRHIGetBackBufferRTVCommand(FRHIDevice* InRHIDevice, ID3D11RenderTargetView** OutRTV);

    virtual void Execute() override;
    virtual ERHICommandType GetCommandType() const override;
    virtual FName GetName() const override { return FName("GetBackBufferRTV"); }

private:
    FRHIDevice* RHIDevice;
    ID3D11RenderTargetView** OutputRTV;
};
