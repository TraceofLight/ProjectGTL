#include "pch.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"

FSceneViewFamily::FSceneViewFamily()
{
	// 기본 설정으로 초기화
	FeatureLevel = ERenderingFeatureLevel::SM5;
	SceneRenderTargetsMode = ESceneRenderTargetsMode::SetTextures;
	bRealtimeUpdate = true;
}

FSceneViewFamily::~FSceneViewFamily()
{
	Views.Empty();
}

void FSceneViewFamily::AddView(FSceneView* InView)
{
	if (InView && !Views.Contains(InView))
	{
		Views.Add(InView);
	}
}

void FSceneViewFamily::RemoveView(FSceneView* InView)
{
	if (InView)
	{
		Views.Remove(InView);
	}
}

void FSceneViewFamily::ClearViews()
{
	Views.Empty();
}

FSceneView* FSceneViewFamily::GetView(int32 InIndex) const
{
	if (InIndex >= 0 && InIndex < Views.Num())
	{
		return Views[InIndex];
	}
	return nullptr;
}

bool FSceneViewFamily::IsValid() const
{
	// 최소 하나의 유효한 뷰가 있어야 함
	if (Views.Num() == 0)
	{
		return false;
	}

	// 모든 뷰가 유효한지 확인
	for (const FSceneView* View : Views)
	{
		if (!View)
		{
			return false;
		}
	}

	return true;
}

void FSceneViewFamily::Reset()
{
	Views.Empty();
	RenderTarget = nullptr;
	CurrentTime = 0.0f;
	DeltaWorldTime = 0.0f;

	// 기본값으로 재설정
	FeatureLevel = ERenderingFeatureLevel::SM5;
	SceneRenderTargetsMode = ESceneRenderTargetsMode::SetTextures;
	bRealtimeUpdate = true;
}
