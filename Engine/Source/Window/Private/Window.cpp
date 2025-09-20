#include "pch.h"
#include "Window/Public/Window.h"

void SWindow::OnResize(const FRect& InRect)
{
    Rect = InRect;
    LayoutChildren();
}

void SWindow::LayoutChildren()
{
    // Base: no children
}

void SWindow::OnPaint()
{
    // Base: no-op
}

bool SWindow::OnMouseDown(FPoint, int)
{
    return false;
}

bool SWindow::OnMouseUp(FPoint, int)
{
    return false;
}

bool SWindow::OnMouseMove(FPoint)
{
    return false;
}

SWindow* SWindow::HitTest(FPoint ScreenCoord)
{
    return IsHover(ScreenCoord) ? this : nullptr;
}

bool SWindow::IsHover(FPoint coord) const
{
    return (coord.X >= Rect.X) && (coord.X < Rect.X + Rect.W) &&
           (coord.Y >= Rect.Y) && (coord.Y < Rect.Y + Rect.H);
}
