#include "pch.h"
#include "Runtime/UI/Window/Public/OutlinerWindow.h"

#include "Runtime/UI/Widget/Public/SceneHierarchyWidget.h"

UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner";
	Config.DefaultSize = ImVec2(340, 520); // 오른편 초기화에서 맞춰주므로 상관 없는 값
	Config.DefaultPosition = ImVec2(1565, 56); // 메뉴바만큼 하향 이동
	Config.MinSize = ImVec2(200, 50);
	Config.bResizable = true;
	Config.bMovable = false;
	Config.bCollapsible = false;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<USceneHierarchyWidget>());
}

void UOutlinerWindow::Initialize()
{
	UE_LOG("OutlinerWindow: Window가 성공적으로 생성되었습니다");
}
