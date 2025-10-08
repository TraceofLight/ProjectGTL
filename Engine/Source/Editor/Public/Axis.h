#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"

class FD3D11RHIModule;

class UAxis :
	public UObject
{
public:
	UAxis(FD3D11RHIModule* InRenderModule);
	~UAxis() override;
	void Render();

private:
	FD3D11RHIModule* RenderModule = nullptr;
	FEditorPrimitive Primitive;
	TArray<FVertex> AxisVertices;
};
