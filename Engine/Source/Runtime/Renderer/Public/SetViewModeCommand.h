#pragma once
#include "RenderCommand.h"
#include "Editor/Public/Editor.h"

class URHIDevice;

/**
 * @brief View Mode Set Command Class
 */
class FRHISetViewModeCommand :
    public IRHICommand
{
public:
    FRHISetViewModeCommand(URHIDevice* InRHIDevice, EViewMode InViewMode)
        : RHIDevice(InRHIDevice), ViewMode(InViewMode)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URHIDevice* RHIDevice;
    EViewMode ViewMode;
};
