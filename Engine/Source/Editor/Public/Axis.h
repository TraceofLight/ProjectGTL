#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"

class FRendererModule;

class UAxis :
	public UObject
{
public:
	UAxis(FRendererModule* InRenderModule);
	~UAxis() override;
	void Render();

private:
	FRendererModule* RenderModule = nullptr;
	FEditorPrimitive Primitive;
	TArray<FVertex> AxisVertices;
};
