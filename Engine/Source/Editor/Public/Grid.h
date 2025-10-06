#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"

class UGrid :
	public UObject
{
public:
	UGrid();
	~UGrid() override;

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
	//FEditorPrimitive Primitive;
	TArray<FVector> Vertices;
	uint32 NumVertices;
};
