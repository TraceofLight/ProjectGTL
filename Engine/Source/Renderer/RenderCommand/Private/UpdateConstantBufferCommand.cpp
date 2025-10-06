#include "pch.h"
#include "Renderer/RenderCommand/Public/UpdateConstantBufferCommand.h"

#include "Render/RHI/Public/RHIDevice.h"

/**
 * @brief Constant Buffer를 업데이트하는 Command 실행 함수
 */
void FRHIUpdateConstantBufferCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    RHIDevice->UpdateConstantBuffers(ModelMatrix, ViewMatrix, ProjMatrix);
}

