#include "pch.h"
#include "Window/Public/WindowSystem.h"
#include "Window/Public/Window.h"
#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
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

		UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
		if (!InputSubsystem)
		{
			return;
		}

		const FVector& MousePosition = InputSubsystem->GetMousePosition();
		FPoint P = FPoint(static_cast<LONG>(MousePosition.X), static_cast<LONG>(MousePosition.Y));

		SWindow* Target = GCapture ? GCapture : GRoot->HitTest(P);

		//UE_LOG("before Button Pressed");
		// --- 1) Left Pressed: 토글 동작 구현 -------------------------------------
		if (!GCapture && InputSubsystem->IsKeyPressed(EKeyInput::MouseLeft))
		{
			if (Target && Target->OnMouseDown(P, 0))
			{
				GCapture = Target;
			}
		}

		// Mouse move (only if any delta)
		const FVector& d = InputSubsystem->GetMouseDelta();
		if ((d.X != 0.0f || d.Y != 0.0f) && GCapture)
		{
			GCapture->OnMouseMove(P);
		}

		if (InputSubsystem->IsKeyReleased(EKeyInput::MouseLeft))
		{
			if (GCapture)
			{
				GCapture->OnMouseUp(P, 0);
				GCapture = nullptr;
			}
		}

		if (!InputSubsystem->IsKeyDown(EKeyInput::MouseLeft) && GCapture)
		{
			GCapture->OnMouseUp(P, /*Button*/0);
			GCapture = nullptr;
		}
	}
}

namespace WindowSystem
{
	void RenderOverlay()
	{
		if (!GRoot)
		{
			return;
		}
		// Let the window tree paint itself (uses ImGui draw lists)
		GRoot->OnPaint();
	}
}

namespace
{
	void CollectLeavesRecursive(SWindow* Node, TArray<FRect>& Out)
	{
		if (!Node)
		{
			return;
		}
		if (auto* Split = dynamic_cast<SSplitter*>(Node))
		{
			CollectLeavesRecursive(Split->SideLT, Out);
			CollectLeavesRecursive(Split->SideRB, Out);
		}
		else
		{
			Out.Emplace(Node->GetRect());
		}
	}
}

namespace WindowSystem
{
	void GetLeafRects(TArray<FRect>& OutRects)
	{
		OutRects.Empty();
		if (!GRoot)
		{
			return;
		}
		CollectLeavesRecursive(GRoot, OutRects);
	}
}
