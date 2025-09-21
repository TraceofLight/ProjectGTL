#pragma once
#include "Widget.h"

class UCamera;

UCLASS()
class UCameraControlWidget
	: public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraControlWidget, UWidget)

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SyncFromCamera();
	void PushToCamera();

	// Special Member Function
	UCameraControlWidget();
	~UCameraControlWidget() override;

private:
	TObjectPtr<UCamera> Camera = nullptr;
	float UiFovY = 80.f;
	float UiNearZ = 0.1f;
	float UiFarZ = 10000.f;
	int   CameraModeIndex = 0;
};
