#include "pch.h"
#include "Runtime/Renderer/Public/EditorGrid.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Renderer/Public/LineBatcher.h"

float FEditorGrid::CellSize = 5.0f;

FEditorGrid::FEditorGrid()
{
	NumVertices = NumLines * 4;
	Vertices.Reserve(NumVertices);
}

FEditorGrid::~FEditorGrid() = default;

void FEditorGrid::UpdateVerticesBy(float InCellSize)
{
	// 중복 삽입 방지
	if (CellSize == InCellSize)
	{
		return;
	}

	CellSize = InCellSize; // 필요하다면 멤버 변수도 갱신

	float LineLength = InCellSize * static_cast<float>(NumLines) / 2.f;

	if (Vertices.Num() < static_cast<int32>(NumVertices))
	{
		Vertices.SetNum(static_cast<int32>(NumVertices));
	}

	uint32 vertexIndex = 0;
	// Y축 방향 라인들 (X축을 따라 이동) - 바닥면(Z=0)
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		float xPos = static_cast<float>(LineCount) * InCellSize;
		Vertices[vertexIndex++] = {xPos, -LineLength, 0.0f};
		Vertices[vertexIndex++] = {xPos, LineLength, 0.0f};
	}

	// X축 방향 라인들 (Y축을 따라 이동) - 바닥면(Z=0)
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		float yPos = static_cast<float>(LineCount) * InCellSize;
		Vertices[vertexIndex++] = {-LineLength, yPos, 0.0f};
		Vertices[vertexIndex++] = {LineLength, yPos, 0.0f};
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
	size_t overwriteCount = min(
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

/**
 * @brief EditorRenderResources의 Line Batch에 그리드 라인을 추가
 * @param EditorResources Line Batch를 지원하는 EditorRenderResources
 * @param CellSize 셀 크기
 * @param NumLines 라인 개수
 * @param Color 그리드 색상
 * @note XYZ 양수 방향은 Axis와 겹치므로 제외됨
 */
void FEditorGrid::AddGridLinesToBatch(
	FEditorRenderResources* EditorResources,
	float CellSize,
	int32 NumLines,
	const FVector4& Color)
{
	if (!EditorResources)
	{
		return;
	}

	FLineBatcher* LineBatcher = EditorResources->GetLineBatcher();
	if (!LineBatcher)
	{
		return;
	}

	const float LineLength = CellSize * static_cast<float>(NumLines) / 2.0f;
	const int32 HalfLines = NumLines / 2;

	// Y축 방향 라인들 (X축을 따라 이동) - 바닥면(Z=0)
	for (int32 LineCount = -HalfLines; LineCount < HalfLines; ++LineCount)
	{
		if (LineCount == 0)
		{
			// X=0 라인: 음수 방향만 (양수는 Y축 Axis와 겹침)
			LineBatcher->AddLine(
				FVector(0.0f, -LineLength, 0.0f),
				FVector(0.0f, 0.0f, 0.0f),
				Color
			);
		}
		else
		{
			// X!=0 라인: 전체 길이
			float xPos = static_cast<float>(LineCount) * CellSize;
			LineBatcher->AddLine(
				FVector(xPos, -LineLength, 0.0f),
				FVector(xPos, LineLength, 0.0f),
				Color
			);
		}
	}

	// X축 방향 라인들 (Y축을 따라 이동) - 바닥면(Z=0)
	for (int32 LineCount = -HalfLines; LineCount < HalfLines; ++LineCount)
	{
		if (LineCount == 0)
		{
			// Y=0 라인: 음수 방향만 (양수는 X축 Axis와 겹침)
			LineBatcher->AddLine(
				FVector(-LineLength, 0.0f, 0.0f),
				FVector(0.0f, 0.0f, 0.0f),
				Color
			);
		}
		else
		{
			// Y!=0 라인: 전체 길이
			float yPos = static_cast<float>(LineCount) * CellSize;
			LineBatcher->AddLine(
				FVector(-LineLength, yPos, 0.0f),
				FVector(LineLength, yPos, 0.0f),
				Color
			);
		}
	}
}
