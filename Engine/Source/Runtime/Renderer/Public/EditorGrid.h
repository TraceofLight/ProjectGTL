#pragma once

#include "Runtime/Core/Public/Object.h"

/**
 * @brief Editor에서 사용하는 Grid 렌더링 클래스
 * 기존 UGrid를 FEditorGrid로 변경하여 Renderer 모듈로 이동
 */
class FEditorGrid
{
public:
    FEditorGrid();
    ~FEditorGrid();

    void UpdateVerticesBy(float NewCellSize);
    void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);

    uint32 GetNumVertices() const
    {
        return NumVertices;
    }

    float GetCellSize() const
    {
        return CellSize;
    }

    void SetCellSize(const float newCellSize)
    {
        CellSize = newCellSize;
    }

private:
    float CellSize = 1.0f;
    int NumLines = 250;
    TArray<FVector> Vertices;
    uint32 NumVertices;
};