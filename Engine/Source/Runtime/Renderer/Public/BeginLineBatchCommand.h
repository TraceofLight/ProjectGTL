#pragma once
#include "RenderCommand.h"

class FRHIDevice;

/**
* @brief Line Batch 처리 Command Class
 */
class FRHIBeginLineBatchCommand :
    public IRHICommand
{
public:
    FRHIBeginLineBatchCommand(FRHIDevice* InRHIDevice) : RHIDevice(InRHIDevice)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    FRHIDevice* RHIDevice;
};
