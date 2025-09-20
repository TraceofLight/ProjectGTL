#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"

class UAxis : public UObject
{
public:
	UAxis();
	~UAxis() override;
	void Render();

private:
	FEditorPrimitive Primitive;
	TArray<FVertex> AxisVertices;
};
