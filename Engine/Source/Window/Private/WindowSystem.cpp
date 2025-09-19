#include "pch.h"
#include "Window/Public/WindowSystem.h"
#include "Window/Public/Window.h"
#include "Manager/Input/Public/InputManager.h"
#include "Window/Public/Splitter.h"

namespace WindowSystem
{
    static SWindow* GRoot = nullptr;
    static SWindow* GCapture = nullptr;

    void SetRoot(SWindow* InRoot)
    {
        GRoot = InRoot;
    }

    SWindow* GetRoot()
    {
        return GRoot;
    }

    void TickInput()
    {
        if (!GRoot)
        {
	        return;
        }

        auto& InputManager = UInputManager::GetInstance();
        const FVector& MousePosition = InputManager.GetMousePosition();
        FPoint P{ LONG(MousePosition.X), LONG(MousePosition.Y) };
		//UE_LOG("%d %d", P.X, P.Y);

        SWindow* Target = GCapture ? GCapture : GRoot->HitTest(P);


    	//UE_LOG("before Button Pressed");
        // Left mouse: start capture on Pressed; if timing misses Pressed (message processed previous loop), allow when Down and no capture
        if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!GCapture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
        {
            if (Target && Target->OnMouseDown(P, 0))
            {
                GCapture = Target;
            }
        	//UE_LOG_DEBUG("Left Button Pressed");
        }

        // Mouse move (only if any delta)
        const FVector& d = InputManager.GetMouseDelta();
        if ((d.X != 0.0f || d.Y != 0.0f) && Target)
        {
            Target->OnMouseMove(P);
        }

        if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
        {
            if (Target)
            {
                Target->OnMouseUp(P, 0);
            }
            GCapture = nullptr;
        }
    }
}

namespace WindowSystem
{
    void RenderOverlay()
    {
        if (!GRoot)
            return;
        // Let the window tree paint itself (uses ImGui draw lists)
        GRoot->OnPaint();
    }
}

namespace {
    void CollectLeavesRecursive(SWindow* Node, TArray<FRect>& Out)
    {
        if (!Node) return;
        if (auto* Split = dynamic_cast<SSplitter*>(Node))
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

namespace WindowSystem
{
    void GetLeafRects(TArray<FRect>& OutRects)
    {
        OutRects.clear();
        if (!GRoot) return;
        CollectLeavesRecursive(GRoot, OutRects);
    }
}
