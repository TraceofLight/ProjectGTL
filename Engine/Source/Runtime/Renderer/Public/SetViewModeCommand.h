#pragma once
#include "RenderCommand.h"
#include "Editor/Public/Editor.h"

class FRHIDevice;

/**
 * @brief View Mode Set Command Class
 */
class FRHISetViewModeCommand :
    public IRHICommand
{
public:
    FRHISetViewModeCommand(FRHIDevice* InRHIDevice, EViewMode InViewMode)
        : RHIDevice(InRHIDevice), ViewMode(InViewMode)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    FRHIDevice* RHIDevice;
    EViewMode ViewMode;
};
