#pragma once

class FEditorRenderResources;

/**
 * @brief Editor에서 사용하는 Grid 렌더링 클래스
 */
class FEditorGrid
{
public:
	FEditorGrid();
	~FEditorGrid();

	static void AddGridLinesToBatch(
		FEditorRenderResources* EditorResources,
		float CellSize = 1.0f,
		int32 NumLines = 250,
		const FVector4& Color = FVector4(0.5f, 0.5f, 0.5f, 1.0f)
	);

	void UpdateVerticesBy(float InCellSize);
	void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);

	uint32 GetNumVertices() const { return NumVertices; }
	static float GetCellSize() { return CellSize; }
	const TArray<FVector>& GetVertices() const { return Vertices; }
	int32 GetNumLines() const { return NumLines; }

	static void SetCellSize(const float InCellSize) { CellSize = InCellSize; }

private:
	static float CellSize;
	int NumLines = 250;
	TArray<FVector> Vertices;
	int32 NumVertices;
};
