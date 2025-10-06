#pragma once
#include "Widget.h"

/**
 * @brief Frame과 관련된 내용을 제공하는 UI Widget
 */
UCLASS()
class UFPSWidget :
	public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UFPSWidget, UWidget)

public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Setter

	// Special member function
	UFPSWidget();
	~UFPSWidget() override;

private:
	float FrameTimeHistory[60] = {};
	int32 FrameTimeIndex = 0;
	float AverageFrameTime = 0.0f;

	// FPS 계산 관련 변수
	float CurrentFPS = 0.0f;
	float MinFPS = static_cast<float>(INT_MAX);
	float MaxFPS = static_cast<float>(INT_MIN);

	// FPS 계산을 위한 샘플링
	static constexpr int32 FPS_SAMPLE_COUNT = 60;
	float FrameSpeedSamples[FPS_SAMPLE_COUNT] = {};
	int32 FrameSpeedSampleIndex = 0;

	float TotalGameTime = 0.0f;
	float CurrentDeltaTime = 0.0f;

	float CellSize = 1.0f;

	// 출력을 위한 변수
	float PreviousTime = 0.0f;
	float PrintFPS = 0.0f;
	float PrintDeltaTime = 0.0f;
	bool bShowGraph = false;

	void CalculateFPS();
	static ImVec4 GetFPSColor(float InFPS);
};
