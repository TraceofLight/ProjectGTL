#pragma once
#include "Runtime/Core/Public/Object.h"

class FEditorRenderResources;

UCLASS()
class UAxis :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UAxis, UObject)

public:
	UAxis();
	~UAxis() override;

	static void AddAxisLinesToBatch(FEditorRenderResources* EditorResources, float AxisLength = 50000.0f);
};
