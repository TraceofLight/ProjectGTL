#include "pch.h"
#include "Renderer/Public/EditorGrid.h"

FEditorGrid::FEditorGrid()
    : Vertices(TArray<FVector>())
    , NumLines(250)
    , CellSize(0) // 아래 UpdateVerticesBy에 넣어주는 값과 달라야 함
{
    NumVertices = NumLines * 4;
    Vertices.Reserve(NumVertices);
    
    // 기본 셀 크기로 초기화
    UpdateVerticesBy(1.0f);
}

FEditorGrid::~FEditorGrid()
{
}

void FEditorGrid::UpdateVerticesBy(float NewCellSize)
{
    // 중복 삽입 방지
    if (CellSize == NewCellSize)
    {
        return;
    }

    CellSize = NewCellSize; // 필요하다면 멤버 변수도 갱신

    float LineLength = NewCellSize * static_cast<float>(NumLines) / 2.f;

    if (Vertices.Num() < NumVertices)
    {
        Vertices.SetNum(NumVertices);
    }

    uint32 vertexIndex = 0;
    // z축 라인 업데이트
    for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
    {
        if (LineCount == 0)
        {
            Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, -LineLength, 0.0f };
            Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, 0.f, 0.f };
        }
        else
        {
            Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, -LineLength, 0.0f };
            Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, LineLength, 0.0f };
        }
    }

    // x축 라인 업데이트
    for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
    {
        if (LineCount == 0)
        {
            Vertices[vertexIndex++] = { -LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
            Vertices[vertexIndex++] = { 0.f, static_cast<float>(LineCount) * NewCellSize, 0.0f };
        }
        else
        {
            Vertices[vertexIndex++] = { -LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
            Vertices[vertexIndex++] = { LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
        }
    }
}

void FEditorGrid::MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex)
{
    // 인덱스 범위 보정
    if (insertStartIndex > destVertices.Num())
        insertStartIndex = destVertices.Num();

    // 미리 메모리 확보
    destVertices.Reserve(destVertices.Num() + std::distance(Vertices.begin(), Vertices.end()));

    // 덮어쓸 수 있는 개수 계산
    size_t overwriteCount = std::min(
        Vertices.Num(),
        destVertices.Num() - static_cast<int32>(insertStartIndex)
    );

    // 기존 요소 덮어쓰기
    std::copy(
        Vertices.begin(),
        Vertices.begin() + overwriteCount,
        destVertices.begin() + insertStartIndex
    );
}