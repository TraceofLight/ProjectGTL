#include "pch.h"
#include "Runtime/Component/Public/LineComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/EditorRenderResources.h"

IMPLEMENT_CLASS(ULineComponent, UPrimitiveComponent)

ULineComponent::ULineComponent()
{
	FEditorRenderResources* EditorResources = URenderer::GetInstance().GetEditorResources();
	Type = EPrimitiveType::Line;
	Vertexbuffer = EditorResources->GetGizmoVertexBuffer(Type);
	NumVertices = EditorResources->GetGizmoVertexCount(Type);
	Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::WireFrame;
}
