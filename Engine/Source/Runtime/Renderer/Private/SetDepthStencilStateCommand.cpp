#include "pch.h"
#include "Runtime/Renderer/Public/SetDepthStencilStateCommand.h"

#include "Runtime/RHI/Public/RHIDevice.h"

/**
 * @brief Depth Stencil State 설정을 실행하는 함수
 */
void FRHISetDepthStencilStateCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    RHIDevice->OmSetDepthStencilState(CompareFunction);
}
