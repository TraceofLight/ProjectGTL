#pragma once
#include <d3d11.h>
#include "Global/Vector.h"

struct FEditorPrimitive
{
	ID3D11Buffer* Vertexbuffer;
	uint32 NumVertices;
	D3D11_PRIMITIVE_TOPOLOGY Topology;
	FVector4 Color;
	FVector Location;
	FVector Rotation;
	FVector Scale;
	FRenderState RenderState;
	bool bShouldAlwaysVisible = false;
};
