#pragma once
#include "RenderCommand.h"

class FRHIDevice;

/**
* @brief Blend State 설정 Command 클래스
 */
class FRHISetBlendStateCommand :
    public IRHICommand
{
public:
    FRHISetBlendStateCommand(FRHIDevice* InRHIDevice, bool bInEnableBlend)
        : RHIDevice(InRHIDevice), bEnableBlend(bInEnableBlend)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    FRHIDevice* RHIDevice;
    bool bEnableBlend;
};
