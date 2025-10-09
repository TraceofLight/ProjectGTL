#include "pch.h"
#include "Runtime/Renderer/Public/SetBlendStateCommand.h"

#include "Runtime/RHI/Public/RHIDevice.h"

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
