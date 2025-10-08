#include "pch.h"
#include "Editor/Public/Axis.h"

#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"

UAxis::UAxis(FD3D11RHIModule* InRenderModule)
	: RenderModule(InRenderModule)
{
	// UE x(forward)
	AxisVertices.Add({ { 0.0f,0.0f,50000.0f }, { 1,0,0,1 } });
	AxisVertices.Add({ { 0.0f,0.0f,0.0f }, { 1,0,0,1 } });

	// UE y(right)
	AxisVertices.Add({ { 50000.0f,0.0f,0.0f }, { 0,1,0,1 } });
	AxisVertices.Add({ { 0.0f,0.0f,0.0f }, { 0,1,0,1 } });

	// UE z(up)
	AxisVertices.Add({ { 0.0f,50000.0f,0.0f }, { 0,0,1,1 } });
	AxisVertices.Add({ { 0.0f,0.0f,0.0f }, { 0,0,1,1 } });

	Primitive.NumVertices = AxisVertices.Num();
	if (FEditorRenderResources* EditorResources = RenderModule->GetEditorResources())
	{
		Primitive.Vertexbuffer = EditorResources->CreateVertexBuffer(AxisVertices.GetData(), Primitive.NumVertices * sizeof(FVertex));
	}
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Color = FVector4(1, 1, 1, 0);
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
}

UAxis::~UAxis()
{
	if(RenderModule)
	{
		if (FEditorRenderResources* EditorResources = RenderModule->GetEditorResources())
		{
			EditorResources->ReleaseVertexBuffer(Primitive.Vertexbuffer);
		}
	}
}

void UAxis::Render()
{
	if(!RenderModule) return;
	if (FEditorRenderResources* EditorResources = RenderModule->GetEditorResources())
	{
		EditorResources->RenderPrimitiveIndexed(Primitive, Primitive.RenderState, false, sizeof(FVertex), 0);
	}
}
