#include "pch.h"
#include "Render/UI/Widget/Public/BottomBarWidget.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"

IMPLEMENT_CLASS(UBottomBarWidget, UWidget)

UBottomBarWidget::UBottomBarWidget() = default;

void UBottomBarWidget::Initialize()
{
	// 초기화 로직
}

void UBottomBarWidget::Update()
{
	// 업데이트 로직
}

void UBottomBarWidget::RenderWidget()
{
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const float ScreenWidth = Viewport->Size.x;
	const float ScreenHeight = Viewport->Size.y;

	// Alt + ` 단축키로 Console 토글
	ImGuiIO& IO = ImGui::GetIO();
	if (IO.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_GraveAccent))
	{
		ToggleConsole();
	}

	// 윈도우 플래그
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// 하단바 위치 설정
	if (BarHeight > 0)
	{
		ImGui::SetNextWindowPos(ImVec2(0, ScreenHeight - BarHeight));
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, BarHeight));
	}
	else
	{
		ImGui::SetNextWindowPos(ImVec2(0, ScreenHeight - 23.0f));
		ImGui::SetNextWindowSize(ImVec2(ScreenWidth, 23.0f));
	}

	if (ImGui::Begin("##BottomBar", nullptr, windowFlags))
	{
		if (ImGui::BeginMenuBar())
		{
			// Console 토글 버튼
			if (ImGui::MenuItem(bShowConsole ? "Hide Console" : "Show Console"))
			{
				ToggleConsole();
			}

			ImGui::Text("|");
			ImGui::Text("Ready");

			// 높이 업데이트
			BarHeight = ImGui::GetWindowSize().y;

			ImGui::EndMenuBar();
		}
	}
	ImGui::End();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void UBottomBarWidget::ToggleConsole()
{
	bShowConsole = !bShowConsole;

	// Console 윈도우 토글
	auto& UIManager = UUIManager::GetInstance();
	if (auto ConsoleWindow = UIManager.FindUIWindow(FName("Console")))
	{
		if (bShowConsole)
		{
			ConsoleWindow->SetWindowState(EUIWindowState::Visible);
			UIManager.ArrangeConsolePanel();
		}
		else
		{
			ConsoleWindow->SetWindowState(EUIWindowState::Hidden);
		}
	}
}
