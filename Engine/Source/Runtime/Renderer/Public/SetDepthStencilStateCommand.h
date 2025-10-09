#pragma once
#include "RenderCommand.h"

class FRHIDevice;

/**
 * @brief Depth Stencil State 설정 Command Class
 */
class FRHISetDepthStencilStateCommand :
    public IRHICommand
{
public:
    FRHISetDepthStencilStateCommand(FRHIDevice* InRHIDevice, EComparisonFunc InCompareFunc)
        : RHIDevice(InRHIDevice), CompareFunction(InCompareFunc)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    FRHIDevice* RHIDevice;
    EComparisonFunc CompareFunction;
};
