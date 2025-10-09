#pragma once
#include "RenderCommand.h"

class FRHIDevice;
class FSceneView;

/**
 * @brief RenderTarget 설정 Command를 정의한 클래스
 */
class FRHISetRenderTargetCommand :
    public IRHICommand
{
public:
    FRHISetRenderTargetCommand(FRHIDevice* InRHIDevice, const FSceneView* InView)
        : RHIDevice(InRHIDevice), View(InView)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetRenderTarget;
    }

private:
    FRHIDevice* RHIDevice;
    const FSceneView* View;
};
