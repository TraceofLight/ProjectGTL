#include "pch.h"
#include "Renderer/RenderPass/Public/DepthPrePass.h"

#include "Render/RHI/Public/RHIDevice.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Runtime/Engine/Public/World.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Window/Public/Viewport.h"

void FDepthPrePass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View || !SceneRenderer) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;

    // Depth Pre-Pass 설정
    // TODO: Depth Write/Color Write 설정 구현 필요
    RHI->OMSetDepthWriteEnabled(true);
    RHI->OMSetColorWriteEnabled(false);

    // 모든 액터들의 깊이 정보만 렌더링
    const TArray<TObjectPtr<AActor>>& Actors = World->GetLevel()->GetLevelActors();
    for (AActor* Actor : Actors)
    {
        if (Actor && !Actor->GetActorHiddenInGame())
        {
            RenderActorDepth(Actor, View, SceneRenderer);
        }
    }
}

void FDepthPrePass::RenderActorDepth(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!Actor || !View || !SceneRenderer) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 액터의 프리미티브 컴포넌트들을 깊이만 렌더링
    for (USceneComponent* Component : Actor->GetComponents<USceneComponent>())
    {
        if (!Component) continue;

        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }

        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            // 깊이만 렌더링하도록 설정된 상태에서 프리미티브 렌더링
            // TODO: Primitive 컴포넌트의 깊이 전용 렌더링 구현
            // Primitive->RenderDepthOnly(RHI, ViewMatrix, ProjectionMatrix);
        }
    }
}
