#include "pch.h"
#include "Runtime/Renderer/Public/RenderGridCommand.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

void FRHIRenderGridCommand::Execute()
{
    if (EditorResources)
    {
        EditorResources->RenderGrid(ViewMatrix, ProjectionMatrix, CellSize);
    }
}
