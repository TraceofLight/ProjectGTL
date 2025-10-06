#pragma once
#include "RenderCommand.h"

class URHIDevice;

/**
* @brief Blend State 설정 Command 클래스
 */
class FRHISetBlendStateCommand :
    public IRHICommand
{
public:
    FRHISetBlendStateCommand(URHIDevice* InRHIDevice, bool bInEnableBlend)
        : RHIDevice(InRHIDevice), bEnableBlend(bInEnableBlend)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URHIDevice* RHIDevice;
    bool bEnableBlend;
};
