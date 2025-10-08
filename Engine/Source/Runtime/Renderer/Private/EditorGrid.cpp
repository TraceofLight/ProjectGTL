#include "pch.h"
#include "Runtime/Renderer/Public/EditorGrid.h"

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

    if (Vertices.Num() < static_cast<int32>(NumVertices))
    {
        Vertices.SetNum(static_cast<int32>(NumVertices));
    }

    uint32 vertexIndex = 0;
    // Y축 방향 라인들 (X축을 따라 이동) - 바닥면(Z=0)
    for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
    {
        float xPos = static_cast<float>(LineCount) * NewCellSize;
        Vertices[vertexIndex++] = { xPos, -LineLength, 0.0f };
        Vertices[vertexIndex++] = { xPos, LineLength, 0.0f };
    }

    // X축 방향 라인들 (Y축을 따라 이동) - 바닥면(Z=0)
    for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
    {
        float yPos = static_cast<float>(LineCount) * NewCellSize;
        Vertices[vertexIndex++] = { -LineLength, yPos, 0.0f };
        Vertices[vertexIndex++] = { LineLength, yPos, 0.0f };
    }
}

void FEditorGrid::MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex)
{
    // 인덱스 범위 보정
	insertStartIndex = min(static_cast<int32>(insertStartIndex), destVertices.Num());

    // 미리 메모리 확보
    destVertices.Reserve(
    	static_cast<int32>(destVertices.Num() + std::distance(Vertices.begin(), Vertices.end())));

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
