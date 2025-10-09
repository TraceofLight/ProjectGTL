#include "pch.h"
#include "Runtime/Renderer/Public/EndLineBatchCommand.h"

/**
 * @brief Line Batch 종료 Command 실행 함수
 */
void FRHIEndLineBatchCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    // TODO: RHI를 통한 라인 배치 종료 및 렌더링 로직 구현
    // 예: 상수 버퍼 업데이트, 라인 드로우 콜 실행 등
    // RHIDevice->UpdateConstantBuffers(ModelMatrix, ViewMatrix, ProjMatrix);
    // RHIDevice->DrawLines();
}
