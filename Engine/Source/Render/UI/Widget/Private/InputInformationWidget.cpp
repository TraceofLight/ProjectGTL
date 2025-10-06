#include "pch.h"
#include "Render/UI/Widget/Public/InputInformationWidget.h"

#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"

constexpr uint8 MaxKeyHistory = 10;

UInputInformationWidget::UInputInformationWidget()
	: UWidget("Input Information Widget")
{
}

UInputInformationWidget::~UInputInformationWidget() = default;

void UInputInformationWidget::Initialize()
{
}

void UInputInformationWidget::Update()
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 마우스 위치 업데이트
	FVector CurrentMousePosition = InputSubsystem->GetMousePosition();
	MouseDelta = CurrentMousePosition - LastMousePosition;
	LastMousePosition = CurrentMousePosition;

	// 새로운 키 입력 확인 및 히스토리에 추가
	auto& PressedKeys = InputSubsystem->GetPressedKeys();
	for (const auto& Key : PressedKeys)
	{
		FString KeyName = UInputSubsystem::KeyInputToString(Key);

		// 키 통계 업데이트
		if (!KeyPressCount.contains(KeyName))
		{
			KeyPressCount[KeyName] = 0;
		}
		++KeyPressCount[KeyName];

		// 히스토리에 추가 (중복 방지)
		if (RecentKeyPresses.IsEmpty() || RecentKeyPresses.Top() != KeyName)
		{
			AddKeyToHistory(KeyName);
		}
	}
}

void UInputInformationWidget::RenderWidget()
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	auto& PressedKeys = InputSubsystem->GetPressedKeys();

	// 탭으로 구분
	if (ImGui::BeginTabBar("InputTabs"))
	{
		// 현재 입력 탭
		if (ImGui::BeginTabItem("Current Input"))
		{
			RenderKeyList(PressedKeys);
			ImGui::EndTabItem();
		}

		// 마우스 정보 탭
		if (ImGui::BeginTabItem("Mouse Info"))
		{
			RenderMouseInfo();
			ImGui::EndTabItem();
		}

		// 키 히스토리 탭
		if (ImGui::BeginTabItem("Key History"))
		{
			ImGui::Text("Recent Key Presses:");
			ImGui::Separator();

			for (int i = RecentKeyPresses.Num() - 1; i >= 0; --i)
			{
				ImGui::Text("- %s", RecentKeyPresses[i].c_str());
			}

			if (ImGui::Button("Clear History"))
			{
				RecentKeyPresses.Empty();
			}

			ImGui::EndTabItem();
		}

		// 통계 탭
		if (ImGui::BeginTabItem("Statistics"))
		{
			RenderKeyStatistics();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void UInputInformationWidget::AddKeyToHistory(const FString& InKeyName)
{
	RecentKeyPresses.Add(InKeyName);

	// 최대 히스토리 크기 제한
	if (RecentKeyPresses.Num() > MaxKeyHistory)
	{
		RecentKeyPresses.RemoveAt(0);
	}
}

void UInputInformationWidget::RenderKeyList(const TArray<EKeyInput>& InPressedKeys)
{
	ImGui::Text("Pressed Keys:");
	ImGui::Separator();

	if (InPressedKeys.IsEmpty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(No Input)");
	}
	else
	{
		UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
		if (!InputSubsystem)
		{
			return;
		}

		for (const EKeyInput& Key : InPressedKeys)
		{
			FString KeyString = UInputSubsystem::KeyInputToString(Key);
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "- %s", KeyString.c_str());
		}

		ImGui::Separator();
		ImGui::Text("Total Keys: %d", static_cast<int>(InPressedKeys.Num()));
	}
}

void UInputInformationWidget::RenderMouseInfo() const
{
	ImGui::Text("Mouse Position: (%.0f, %.0f)", LastMousePosition.X, LastMousePosition.Y);
	ImGui::Text("Mouse Delta: (%.2f, %.2f)", MouseDelta.X, MouseDelta.Y);
	ImGui::Text("Mouse Speed: %.2f", MouseDelta.Length());

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 마우스 위치를 작은 그래프로 표시
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 CanvasPosition = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize = ImVec2(200, 100);

	// BackGround
	DrawList->AddRectFilled(CanvasPosition,
	                        ImVec2(CanvasPosition.x + CanvasSize.x, CanvasPosition.y + CanvasSize.y),
	                        IM_COL32(50, 50, 50, 255));

	// 마우스 위치에 Dot 표시
	FVector MouseNDC = InputSubsystem->GetMouseNDCPosition();

	// [-1, 1] 범위를 [0, 1] 범위로 변환
	float NormalizedX = (MouseNDC.X + 1.0f) * 0.5f;
	float NormalizedY = 1 - (MouseNDC.Y + 1.0f) * 0.5f;

	// 마우스 위치에 Dot 표시
	ImVec2 MousePosNormalized = ImVec2(NormalizedX * CanvasSize.x, NormalizedY * CanvasSize.y);

	DrawList->AddCircleFilled(ImVec2(CanvasPosition.x + MousePosNormalized.x, CanvasPosition.y + MousePosNormalized.y),
	                          3.0f, IM_COL32(255, 0, 0, 255));

	ImGui::Dummy(CanvasSize);
}

void UInputInformationWidget::RenderKeyStatistics()
{
	ImGui::Text("Key Press Statistics:");
	ImGui::Separator();

	if (KeyPressCount.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No statistics yet");
	}
	else
	{
		// 통계를 카운트 순으로 정렬
		TArray<TPair<FString, uint32>> SortedStats;
		for (const auto& Pair : KeyPressCount)
		{
			SortedStats.Add(Pair);
		}

		std::ranges::sort(SortedStats,
		                  [](const auto& A, const auto& B) { return A.second > B.second; });

		for (const auto& [Key, Count] : SortedStats)
		{
			ImGui::Text("%s: %u times", Key.c_str(), Count);
		}

		if (ImGui::Button("Clear Statistics"))
		{
			KeyPressCount.clear();
		}
	}
}
