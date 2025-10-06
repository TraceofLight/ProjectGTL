#pragma once
#include "RenderCommand.h"

class URHIDevice;

/**
 * @brief Depth Stencil State 설정 Command Class
 */
class FRHISetDepthStencilStateCommand :
    public IRHICommand
{
public:
    FRHISetDepthStencilStateCommand(URHIDevice* InRHIDevice, EComparisonFunc InCompareFunc)
        : RHIDevice(InRHIDevice), CompareFunction(InCompareFunc)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URHIDevice* RHIDevice;
    EComparisonFunc CompareFunction;
};
