#pragma once
#include "Runtime/UI/Widget/Public/Widget.h"

/**
 * @brief 하단 바 위젯
 * 화면 하단에 고정되는 도구 모음
 */
UCLASS()
class UBottomBarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UBottomBarWidget, UWidget)

public:
	UBottomBarWidget();
	virtual ~UBottomBarWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	float GetBarHeight() const { return BarHeight; }
	void SetShowConsole(bool bShow) { bShowConsole = bShow; }
	void ToggleConsole();

private:
	float BarHeight = 0.0f;
	bool bShowConsole = false;
};
