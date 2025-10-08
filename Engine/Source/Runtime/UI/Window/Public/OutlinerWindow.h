#pragma once
#include "UIWindow.h"

class ULevelManager;

/**
 * @brief Outliner 역할을 제공할 Window
 * Actor Transform UI를 제공한다
 */
class UOutlinerWindow
	: public UUIWindow
{
public:
    UOutlinerWindow();
	void Initialize() override;
};
