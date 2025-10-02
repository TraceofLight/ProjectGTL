#include "pch.h"
#include "Render/UI/Window/Public/MainMenuWindow.h"

#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Render/UI/Widget/Public/MainBarWidget.h"
#include "Render/UI/Widget/Public/BottomBarWidget.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"

IMPLEMENT_SINGLETON_CLASS(UMainMenuWindow, UUIWindow)

UMainMenuWindow::UMainMenuWindow()
{
	// 메인 메뉴 전용 설정
	SetupMainMenuConfig();
}

UMainMenuWindow::~UMainMenuWindow() = default;

void UMainMenuWindow::Initialize()
{
	// MainBarWidget 생성 및 초기화
	MainBarWidget = new UMainBarWidget;
	if (MainBarWidget)
	{
		MainBarWidget->Initialize();
		AddWidget(MainBarWidget);
		UE_LOG("MainMenuWindow: MainBarWidget이 생성되고 초기화되었습니다");
	}
	else
	{
		UE_LOG("MainMenuWindow: Error: MainBarWidget 생성에 실패했습니다!");
		return;
	}

	// BottomBarWidget 생성 및 초기화
	BottomBarWidget = new UBottomBarWidget;
	if (BottomBarWidget)
	{
		BottomBarWidget->Initialize();
		AddWidget(BottomBarWidget);
		UE_LOG("MainMenuWindow: BottomBarWidget이 생성되고 초기화되었습니다");
	}
	else
	{
		UE_LOG("MainMenuWindow: Error: BottomBarWidget 생성에 실패했습니다!");
	}

	UE_LOG("MainMenuWindow: 메인 메뉴 윈도우가 초기화되었습니다");

	// TODO(KHJ): 어디에 붙어있는 것이 적합한지 아직은 모르겠어서 MainMenu로 이관
	// 필요 시 적절한 위치로 위젯 내부의 기능을 배치할 것
	AddWidget(NewObject<UActorTerminationWidget>());
	AddWidget(NewObject<UViewportControlWidget>());
}

void UMainMenuWindow::Cleanup()
{
	if (MainBarWidget && !MainBarWidget->IsSingleton())
	{
		SafeDelete(MainBarWidget);
	}

	if (BottomBarWidget && !BottomBarWidget->IsSingleton())
	{
		SafeDelete(BottomBarWidget);
	}

	UUIWindow::Cleanup();
	UE_LOG("MainMenuWindow: 메인 메뉴 윈도우가 정리되었습니다");
}

/**
 * @brief 메뉴바 Config 세팅 함수
 */
void UMainMenuWindow::SetupMainMenuConfig()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "MainMenuBar";

	// 메인 메뉴바는 화면 상단에 고정되어야 함
	Config.DefaultPosition = ImVec2(0, 0);
	Config.DefaultSize = ImVec2(0, 0);

	// 메인 메뉴바는 이동하거나 크기 조절할 수 없어야 함
	Config.bMovable = false;
	Config.bResizable = false;
	Config.bCollapsible = false;

	// 항상 표시되어야 함
	Config.InitialState = EUIWindowState::Visible;

	// 우선순위 설정
	// TODO(KHJ): Config 자체가 잘 작동하지 않는 부분이 있는데 이 우선순위는 의미가 없어보임. 개선 필요
	Config.Priority = 1000;

	// 메뉴바만 보이도록 하기 위해 전체적으로 숨김 처리
	Config.WindowFlags = ImGuiWindowFlags_NoTitleBar |
	                     ImGuiWindowFlags_NoResize |
	                     ImGuiWindowFlags_NoMove |
	                     ImGuiWindowFlags_NoCollapse |
	                     ImGuiWindowFlags_NoScrollbar |
	                     ImGuiWindowFlags_NoBackground |
	                     ImGuiWindowFlags_NoBringToFrontOnFocus |
	                     ImGuiWindowFlags_NoNavFocus |
	                     ImGuiWindowFlags_NoDecoration;

	SetConfig(Config);
}
