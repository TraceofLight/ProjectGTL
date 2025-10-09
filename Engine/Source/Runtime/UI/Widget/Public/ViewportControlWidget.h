#pragma once
#include "Widget.h"

class UViewportManager;
class FViewportClient;
enum class EViewType : uint8;
enum class EViewportChange : uint8;

/**
 * @brief 뷰포트 제어를 담당하는 위젯
 * ViewMode, ViewType 변경 및 레이아웃 전환 기능을 제공
 */
class UViewportControlWidget : public UWidget
{
public:
	UViewportControlWidget();
	~UViewportControlWidget() override;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

private:
	void RenderViewportToolbar(int32 ViewportIndex);
	static void RenderSplitterLines();
	static void RenderCameraSpeedControl(int32 ViewportIndex);
	static void RenderGridSizeControl();
	static void HandleCameraBinding(int32 ViewportIndex, EViewType NewType, int32 TypeIndex);

private:
	// ViewMode와 ViewType 라벨들
	static const char* ViewModeLabels[];
	static const char* ViewTypeLabels[];

	// 현재 레이아웃 상태
	enum class ELayout : uint8 { Single, Quad };
	ELayout CurrentLayout = ELayout::Single;
};
