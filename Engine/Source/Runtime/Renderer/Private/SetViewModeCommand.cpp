#include "pch.h"
#include "Runtime/Renderer/Public/SetViewModeCommand.h"

#include "Runtime/RHI/Public/RHIDevice.h"

/**
 * @brief ViewMode 변경 처리를 실행하는 함수
 */
void FRHISetViewModeCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    RHIDevice->RSSetState(ViewMode);
}
