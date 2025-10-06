#include "pch.h"
#include "Renderer/RenderCommand/Public/SetDepthStencilStateCommand.h"

#include "Render/RHI/Public/RHIDevice.h"

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
