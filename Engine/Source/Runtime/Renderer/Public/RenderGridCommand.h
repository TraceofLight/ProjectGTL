#pragma once

#include "Runtime/Renderer/Public/RenderCommand.h"
#include "Global/Matrix.h"

class FEditorRenderResources;

/**
 * @brief 그리드를 렌더링하는 RHI Command
 */
class FRHIRenderGridCommand : public FRHICommand
{
public:
    FRHIRenderGridCommand(FEditorRenderResources* InEditorResources, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix, float InCellSize)
        : EditorResources(InEditorResources)
        , ViewMatrix(InViewMatrix)
        , ProjectionMatrix(InProjectionMatrix)
        , CellSize(InCellSize)
    {
    }

    virtual void Execute() override;

private:
    FEditorRenderResources* EditorResources;
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    float CellSize;
};
