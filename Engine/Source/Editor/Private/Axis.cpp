#include "pch.h"
#include "Editor/Public/Axis.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

IMPLEMENT_CLASS(UAxis, UObject)

UAxis::UAxis() = default;

UAxis::~UAxis() = default;

/**
 * @brief EditorRenderResources의 Line Batch에 축 라인을 추가하는 함수
 * @param EditorResources Line Batch를 지원하는 EditorRenderResources
 * @param AxisLength 축의 길이
 */
void UAxis::AddAxisLinesToBatch(FEditorRenderResources* EditorResources, float AxisLength)
{
	if (!EditorResources)
	{
		return;
	}

	// X축 (Forward) - 빨간색
	EditorResources->AddLine(
		FVector(0.0f, 0.0f, 0.0f),
		FVector(AxisLength, 0.0f, 0.0f),
		FVector4(1.0f, 0.0f, 0.0f, 1.0f)
	);

	// Y축 (Right) - 초록색
	EditorResources->AddLine(
		FVector(0.0f, 0.0f, 0.0f),
		FVector(0.0f, AxisLength, 0.0f),
		FVector4(0.0f, 1.0f, 0.0f, 1.0f)
	);

	// Z축 (Up) - 파란색
	EditorResources->AddLine(
		FVector(0.0f, 0.0f, 0.0f),
		FVector(0.0f, 0.0f, AxisLength),
		FVector4(0.0f, 0.0f, 1.0f, 1.0f)
	);
}
