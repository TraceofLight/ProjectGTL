#include "pch.h"
#include "Render/UI/Window/Public/DetailWindow.h"

#include "Render/UI/Widget/Public/ActorDetailWidget.h"
#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

/**
 * @brief Detail Window Constructor
 * Selected된 Actor의 관리를 위한 적절한 크기의 윈도우 제공
 */
UDetailWindow::UDetailWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Details";
	Config.DefaultSize = ImVec2(330, 440);
	Config.DefaultPosition = ImVec2(1565, 590);
	Config.MinSize = ImVec2(250, 300);
	Config.DockDirection = EUIDockDirection::Right;
	Config.Priority = 20;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<UActorDetailWidget>());
	AddWidget(NewObject<UTargetActorTransformWidget>());
	AddWidget(NewObject<UActorTerminationWidget>());
}

/**
 * @brief 초기화 함수
 */
void UDetailWindow::Initialize()
{
	UE_LOG("DetailWindow: Initialized");
}
