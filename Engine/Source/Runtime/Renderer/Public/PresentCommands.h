#pragma once

#include "RenderCommand.h"

class URHIDevice;

/**
 * @brief SwapChain Present 명령 (언리얼 스타일)
 */
class FRHIPresentCommand : public IRHICommand
{
public:
    FRHIPresentCommand(URHIDevice* InRHIDevice);
    
    virtual void Execute() override;
    virtual ERHICommandType GetCommandType() const override;
    virtual FName GetName() const override { return FName("Present"); }

private:
    URHIDevice* RHIDevice;
};

/**
 * @brief BackBuffer Surface 접근 명령 (Direct2D 용)
 */
class FRHIGetBackBufferSurfaceCommand : public IRHICommand
{
public:
    FRHIGetBackBufferSurfaceCommand(URHIDevice* InRHIDevice, IDXGISurface** OutSurface);
    
    virtual void Execute() override;
    virtual ERHICommandType GetCommandType() const override;
    virtual FName GetName() const override { return FName("GetBackBufferSurface"); }

private:
    URHIDevice* RHIDevice;
    IDXGISurface** OutputSurface;
};

/**
 * @brief BackBuffer RenderTargetView 접근 명령
 */
class FRHIGetBackBufferRTVCommand : public IRHICommand
{
public:
    FRHIGetBackBufferRTVCommand(URHIDevice* InRHIDevice, ID3D11RenderTargetView** OutRTV);
    
    virtual void Execute() override;
    virtual ERHICommandType GetCommandType() const override;
    virtual FName GetName() const override { return FName("GetBackBufferRTV"); }

private:
    URHIDevice* RHIDevice;
    ID3D11RenderTargetView** OutputRTV;
};