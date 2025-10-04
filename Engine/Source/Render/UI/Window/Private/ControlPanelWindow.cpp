#include "pch.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"

#include "Render/UI/Widget/Public/FPSWidget.h"
#include "Render/UI/Widget/Public/PrimitiveSpawnWidget.h"

/**
 * @brief Control Panel Constructor
 * 적절한 사이즈의 윈도우 제공
 */
UControlPanelWindow::UControlPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Control Panel";
	Config.DefaultSize = ImVec2(350, 270);
	Config.DefaultPosition = ImVec2(10, 56); // 메뉴바 2개만큼 하향 이동
	Config.MinSize = ImVec2(350, 200);
	Config.DockDirection = EUIDockDirection::Left;
	Config.Priority = 15;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<UFPSWidget>());
	AddWidget(NewObject<UPrimitiveSpawnWidget>());
}

/**
 * @brief 초기화 함수
 */
void UControlPanelWindow::Initialize()
{
	UE_LOG("ControlPanelWindow: Initialized");

	// Hidden으로 초기 설정
	SetWindowState(EUIWindowState::Hidden);
}
