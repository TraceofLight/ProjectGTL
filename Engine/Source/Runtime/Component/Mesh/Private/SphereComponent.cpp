#include "pch.h"
#include "Runtime/Component/Mesh/Public/SphereComponent.h"

#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"

IMPLEMENT_CLASS(USphereComponent, UPrimitiveComponent)

USphereComponent::USphereComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Sphere;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	BoundingVolume = nullptr;
}

