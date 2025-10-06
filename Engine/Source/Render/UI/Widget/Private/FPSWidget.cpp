#include "pch.h"
#include "Render/UI/Widget/Public/FPSWidget.h"

IMPLEMENT_CLASS(UFPSWidget, UWidget)

constexpr float REFRESH_INTERVAL = 0.1f;

UFPSWidget::UFPSWidget()
{
	SetDisplayName("FPS Widget");
}

UFPSWidget::~UFPSWidget() = default;

void UFPSWidget::Initialize()
{
	// 히스토리 초기화
	for (int i = 0; i < 60; ++i)
	{
		FrameTimeHistory[i] = 0.0f;
	}

	// FPS 계산 샘플 배열 초기화
	for (int i = 0; i < FPS_SAMPLE_COUNT; ++i)
	{
		FrameSpeedSamples[i] = 0.0f;
	}

	FrameTimeIndex = 0;
	FrameSpeedSampleIndex = 0;
	AverageFrameTime = 0.0f;
	CurrentFPS = 0.0f;
	MinFPS = 999.0f;
	MaxFPS = 0.0f;
	TotalGameTime = 0.0f;

	UE_LOG("FPSWidget: Successfully Initialized");
}

void UFPSWidget::Update()
{
	CurrentDeltaTime = DT;
	TotalGameTime += DT;

	CalculateFPS();

	// FPS 통계 업데이트
	MaxFPS = max(CurrentFPS, MaxFPS);
	MinFPS = min(CurrentFPS, MinFPS);

	// 프레임 시간 히스토리 업데이트
	FrameTimeHistory[FrameTimeIndex] = DT * 1000.0f;
	FrameTimeIndex = (FrameTimeIndex + 1) % 60;

	// 평균 프레임 시간 계산
	float Total = 0.0f;
	for (int i = 0; i < 60; ++i)
	{
		Total += FrameTimeHistory[i];
	}

	AverageFrameTime = Total / 60.0f;
}

void UFPSWidget::RenderWidget()
{
	// 러프한 일정 간격으로 FPS 및 Delta Time 정보 출력
	if (TotalGameTime - PreviousTime > REFRESH_INTERVAL)
	{
		PrintFPS = CurrentFPS;
		PrintDeltaTime = CurrentDeltaTime * 1000.0f;
		PreviousTime = TotalGameTime;
	}

	ImVec4 FPSColor = GetFPSColor(CurrentFPS);
	ImGui::TextColored(FPSColor, "FPS: %.1f (%.2f ms)", PrintFPS, PrintDeltaTime);

	// Game Time 출력
	ImGui::Text("Game Time: %.1f s", TotalGameTime);
	ImGui::Checkbox("Show Details", &bShowGraph);

	// Details
	if (bShowGraph)
	{
		ImGui::Text("동적 할당된 메모리 정보");
		ImGui::Text("Overall Object Count: %u", TotalAllocationCount);
		ImGui::Text("Overall Memory: %.3f KB", static_cast<float>(TotalAllocationBytes) / KILO);
		ImGui::Separator();

		ImGui::Text("Frame Time History:");
		ImGui::PlotLines("##FrameTime", FrameTimeHistory, 60, FrameTimeIndex,
		                 ("Average: " + to_string(AverageFrameTime) + " ms").c_str(),
		                 0.0f, 50.0f, ImVec2(0, 80));

		ImGui::Text("Statistics:");
		ImGui::Text("  Min FPS: %.1f", MinFPS);
		ImGui::Text("  Max FPS: %.1f", MaxFPS);
		ImGui::Text("  Average Frame Time: %.2f ms", AverageFrameTime);

		if (ImGui::Button("Reset Statistics"))
		{
			MinFPS = static_cast<float>(INT_MAX);
			MaxFPS = static_cast<float>(INT_MIN);
			for (int i = 0; i < 60; ++i)
			{
				FrameTimeHistory[i] = 0.0f;
			}

			FrameSpeedSampleIndex = 0;
			for (int i = 0; i < FPS_SAMPLE_COUNT; ++i)
			{
				FrameSpeedSamples[i] = 0.0f;
			}
		}
	}

	ImGui::Separator();
}

ImVec4 UFPSWidget::GetFPSColor(float InFPS)
{
	if (InFPS >= 60.0f)
	{
		return {0.0f, 1.0f, 0.0f, 1.0f}; // 녹색 (우수)
	}
	else if (InFPS >= 30.0f)
	{
		return {1.0f, 1.0f, 0.0f, 1.0f}; // 노란색 (보통)
	}
	else
	{
		return {1.0f, 0.0f, 0.0f, 1.0f}; // 빨간색 (주의)
	}
}

/**
 * @brief FPS를 계산하는 함수
 */
void UFPSWidget::CalculateFPS()
{
	if (GDeltaTime > 0.0f)
	{
		FrameSpeedSamples[FrameSpeedSampleIndex] = 1.0f / GDeltaTime;
		FrameSpeedSampleIndex = (FrameSpeedSampleIndex + 1) % FPS_SAMPLE_COUNT;
	}

	float FPSSum = 0.0f;
	int ValidSampleCount = 0;

	for (int i = 0; i < FPS_SAMPLE_COUNT; ++i)
	{
		if (FrameSpeedSamples[i] > 0.0f)
		{
			FPSSum += FrameSpeedSamples[i];
			++ValidSampleCount;
		}
	}

	if (ValidSampleCount > 0)
	{
		CurrentFPS = FPSSum / ValidSampleCount;
	}
}
