#pragma once
#include "Runtime/Core/Public/Object.h"

class SWindow;
class SSplitter;
class FAppWindow;

UCLASS()
class UViewportManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UViewportManager, UObject)

public:
    // Lifecycle
    void Initialize(FAppWindow* InWindow);
    void Update();

    // Layout controls
    void BuildSingleLayout();
    void BuildFourSplitLayout();

    // Set/Get the root window for hit-testing and input routing
    void SetRoot(SWindow* InRoot);
    SWindow* GetRoot();

	// Per-frame, route mouse input from UInputManager to the window tree
	void TickInput();

	// Render overlay (calls root->OnPaint), for prototype using ImGui draw list
	void RenderOverlay();

    // Collect current leaf rectangles from the splitter tree (in window space)
    void GetLeafRects(TArray<FRect>& OutRects);

private:
    SWindow* Root = nullptr;
    SWindow* Capture = nullptr;
    bool bFourSplitActive = false;
};
