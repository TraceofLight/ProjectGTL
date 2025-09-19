#include "pch.h"
#include "Window/Public/WindowSystem.h"
#include "Window/Public/Window.h"
#include "Manager/Input/Public/InputManager.h"

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
            return;

        auto& IM = UInputManager::GetInstance();
        const FVector& mpos = IM.GetMousePosition();
        FPoint P{ LONG(mpos.X), LONG(mpos.Y) };

        SWindow* Target = GCapture ? GCapture : GRoot->HitTest(P);

        // Left mouse
        if (IM.IsKeyPressed(EKeyInput::MouseLeft))
        {
            if (Target && Target->OnMouseDown(P, 0))
            {
                GCapture = Target;
            }
        	UE_LOG_DEBUG("Left Button Pressed");
        }

        // Mouse move (only if any delta)
        const FVector& d = IM.GetMouseDelta();
        if ((d.X != 0.0f || d.Y != 0.0f) && Target)
        {
            Target->OnMouseMove(P);
        }

        if (IM.IsKeyReleased(EKeyInput::MouseLeft))
        {
            if (Target)
            {
                Target->OnMouseUp(P, 0);
            }
            GCapture = nullptr;
        }
    }
}

