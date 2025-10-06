#include "pch.h"
#include "Editor/Public/Axis.h"
#include "Render/Renderer/Public/Renderer.h"

UAxis::UAxis()
{
	URenderer& Renderer = URenderer::GetInstance();

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
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(AxisVertices.GetData(), Primitive.NumVertices * sizeof(FVertex));
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Color = FVector4(1, 1, 1, 0);
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
}

UAxis::~UAxis()
{
	URenderer::ReleaseVertexBuffer(Primitive.Vertexbuffer);
}

void UAxis::Render()
{
	URenderer& Renderer = URenderer::GetInstance();
	Renderer.RenderPrimitive(Primitive, Primitive.RenderState);
}
