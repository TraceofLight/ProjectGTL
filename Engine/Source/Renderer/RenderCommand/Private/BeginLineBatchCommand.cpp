#include "pch.h"
#include "Renderer/RenderCommand/Public/BeginLineBatchCommand.h"

/**
 * @brief Line Batch 시작 Command 실행 함수
 */
void FRHIBeginLineBatchCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    // TODO: RHI를 통한 라인 배치 시작 로직 구현
    // 예: 라인 버퍼 초기화, 셰이더 설정 등
}
