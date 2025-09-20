#include "pch.h"

#include "Manager/Viewport/Public/ViewportManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Core/Public/AppWindow.h"
#include "Window/Public/Window.h"
#include "Window/Public/Splitter.h"
#include "Window/Public/SplitterV.h"
#include "Window/Public/SplitterH.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UViewportManager)

UViewportManager::UViewportManager() = default;
UViewportManager::~UViewportManager() = default;

void UViewportManager::Initialize(FAppWindow* InWindow)
{
    int32 w = 0, h = 0;
    if (InWindow)
        InWindow->GetClientSize(w, h);

    // Start with a single full-viewport window
    SWindow* Single = new SWindow();
    Single->OnResize({ 0, 0, w, h });
    SetRoot(Single);
    bFourSplitActive = false;
}

void UViewportManager::Update()
{
    TickInput();
}

void UViewportManager::SetRoot(SWindow* InRoot)
{
    Root = InRoot;
}

SWindow* UViewportManager::GetRoot()
{
    return Root;
}

void UViewportManager::TickInput()
{
    if (!Root)
    {
        return;
    }

    auto& InputManager = UInputManager::GetInstance();
    const FVector& MousePosition = InputManager.GetMousePosition();
    FPoint P{ LONG(MousePosition.X), LONG(MousePosition.Y) };

    SWindow* Target = Capture ? static_cast<SWindow*>(Capture) : Root->HitTest(P);

	if (!Capture)
	{
		if (auto* S = Cast(Target))
		{
			if (S->IsHandleHover(P)) {}
				//UE_LOG("hover splitter");
		}
	}

    if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
    {
        if (Target && Target->OnMouseDown(P, 0))
        {
			UE_LOG("OnMouseDown act");
            Capture = Target;
        }
    }

    // Mouse move while captured (always forward to allow cross-drag detection)
    if (Capture)
    {
        Capture->OnMouseMove(P);
    }

    if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
    {
        if (Capture)
        {
            Capture->OnMouseUp(P, 0);
            Capture = nullptr;
        }
    }

    if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
    {
        Capture->OnMouseUp(P, /*Button*/0);
        Capture = nullptr;
    }
}

void UViewportManager::RenderOverlay()
{
    if (!Root)
        return;
	//UE_LOG("overlaystart");
    Root->OnPaint();
	//UE_LOG("overlayend");
}

namespace {
    void CollectLeavesRecursive(SWindow* Node, TArray<FRect>& Out)
    {
        if (!Node) return;
        if (auto* Split = Cast(Node))
        {
            CollectLeavesRecursive(Split->SideLT, Out);
            CollectLeavesRecursive(Split->SideRB, Out);
        }
        else
        {
            Out.emplace_back(Node->GetRect());
        }
    }
}

void UViewportManager::GetLeafRects(TArray<FRect>& OutRects)
{
    OutRects.clear();
    if (!Root) return;
    CollectLeavesRecursive(Root, OutRects);
}

void UViewportManager::BuildSingleLayout()
{
    if (!Root)
        return;

    const FRect rect = Root->GetRect();
    Capture = nullptr;

    SWindow* Single = new SWindow();
    Single->OnResize(rect);
    SetRoot(Single);
    bFourSplitActive = false;
}

void UViewportManager::BuildFourSplitLayout()
{
    if (!Root)
        return;

    const FRect rect = Root->GetRect();
    Capture = nullptr;

    // Build 4-way splitter tree
    SSplitter* RootSplit = new SSplitterV();
    RootSplit->Ratio = 0.5f;

    static float SharedY = 0.5f;

    SSplitter* Left = new SSplitterH(); Left->Ratio = 0.5f;
    Left->SetSharedRatio(&SharedY);
    Left->Ratio = SharedY;

    SSplitter* Right = new SSplitterH(); Right->Ratio = 0.5f;
    Right->SetSharedRatio(&SharedY);
    Right->Ratio = SharedY;

    RootSplit->SetChildren(Left, Right);

    SWindow* Top = new SWindow();
    SWindow* Front = new SWindow();
    SWindow* Side = new SWindow();
    SWindow* Persp = new SWindow();
    Left->SetChildren(Top, Front);
    Right->SetChildren(Side, Persp);

    RootSplit->OnResize(rect);
    SetRoot(RootSplit);
    bFourSplitActive = true;
}
