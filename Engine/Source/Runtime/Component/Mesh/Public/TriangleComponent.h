#pragma once
#include "Runtime/Component/Public/PrimitiveComponent.h"

UCLASS()
class UTriangleComponent :
public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTriangleComponent, UPrimitiveComponent)

public:
	UTriangleComponent();
};
