#include "pch.h"
#include "Renderer/RenderCommand/Public/SetBlendStateCommand.h"

#include "Render/RHI/Public/RHIDevice.h"

/**
 * @brief Blend State 설정 실행 함수
 */
void FRHISetBlendStateCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    RHIDevice->OMSetBlendState(bEnableBlend);
}
