#include "pch.h"
#include "Window/Public/Splitter.h"

void SSplitter::SetChildren(SWindow* InLT, SWindow* InRB)
{
    SideLT = InLT;
    SideRB = InRB;
}

FRect SSplitter::GetHandleRect() const
{
    if (Orientation == EOrientation::Vertical)
    {
        int32 splitX = Rect.X + int32(std::lround(Rect.W * Ratio));
        int32 half = Thickness / 2;
        int32 hx = splitX - half;
        return FRect{ hx, Rect.Y, Thickness, Rect.H };
    }
    else
    {
        int32 splitY = Rect.Y + int32(std::lround(Rect.H * Ratio));
        int32 half = Thickness / 2;
        int32 hy = splitY - half;
        return FRect{ Rect.X, hy, Rect.W, Thickness };
    }
}

bool SSplitter::IsHandleHover(FPoint Coord) const
{
    const FRect h = GetHandleRect();
    return (Coord.X >= h.X) && (Coord.X < h.X + h.W) &&
           (Coord.Y >= h.Y) && (Coord.Y < h.Y + h.H);
}

void SSplitter::LayoutChildren()
{
    if (!SideLT || !SideRB)
        return;

    const FRect h = GetHandleRect();
    if (Orientation == EOrientation::Vertical)
    {
        FRect left{ Rect.X, Rect.Y, max(0L, h.X - Rect.X), Rect.H };
        int32 rightX = h.X + h.W;
        FRect right{ rightX, Rect.Y, max(0L, (Rect.X + Rect.W) - rightX), Rect.H };
        SideLT->OnResize(left);
        SideRB->OnResize(right);
    }
    else
    {
        FRect top{ Rect.X, Rect.Y, Rect.W, max(0L, h.Y - Rect.Y) };
        int32 bottomY = h.Y + h.H;
        FRect bottom{ Rect.X, bottomY, Rect.W, max(0L, (Rect.Y + Rect.H) - bottomY) };
        SideLT->OnResize(top);
        SideRB->OnResize(bottom);
    }
}

bool SSplitter::OnMouseDown(FPoint Coord, int Button)
{
    if (Button == 0 && IsHandleHover(Coord))
    {
        bDragging = true;
        return true; // handled
    }
    return false;
}

bool SSplitter::OnMouseMove(FPoint Coord)
{
    if (!bDragging)
    {
        // return true on hover to allow cursor change upstream if needed
        return IsHandleHover(Coord);
    }

    if (Orientation == EOrientation::Vertical)
    {
        const int32 span = std::max(1L, Rect.W);
        float r = float(Coord.X - Rect.X) / float(span);
        float limit = float(MinChildSize) / float(span);
        Ratio = std::clamp(r, limit, 1.0f - limit);
    }
    else
    {
        const int32 span = std::max(1L, Rect.H);
        float r = float(Coord.Y - Rect.Y) / float(span);
        float limit = float(MinChildSize) / float(span);
        Ratio = std::clamp(r, limit, 1.0f - limit);
    }

    LayoutChildren();
    return true;
}

bool SSplitter::OnMouseUp(FPoint, int Button)
{
    if (Button == 0 && bDragging)
    {
        bDragging = false;
        return true;
    }
    return false;
}

void SSplitter::OnPaint()
{
    if (SideLT) SideLT->OnPaint();
    if (SideRB) SideRB->OnPaint();
    // Draw handle line for visual feedback (ImGui overlay)
    const FRect h = GetHandleRect();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 p0{ (float)h.X, (float)h.Y };
    ImVec2 p1{ (float)(h.X + h.W), (float)(h.Y + h.H) };
    dl->AddRectFilled(p0, p1, IM_COL32(80,80,80,160));
}

SWindow* SSplitter::HitTest(FPoint ScreenCoord)
{
    // Handle has priority so splitter can capture drag
    if (IsHandleHover(ScreenCoord))
        return this;

    // Delegate to children based on rect containment
    if (SideLT)
    {
        const FRect& r = SideLT->GetRect();
        if (ScreenCoord.X >= r.X && ScreenCoord.X < r.X + r.W &&
            ScreenCoord.Y >= r.Y && ScreenCoord.Y < r.Y + r.H)
        {
            if (auto* hit = SideLT->HitTest(ScreenCoord)) return hit;
            return SideLT;
        }
    }
    if (SideRB)
    {
        const FRect& r = SideRB->GetRect();
        if (ScreenCoord.X >= r.X && ScreenCoord.X < r.X + r.W &&
            ScreenCoord.Y >= r.Y && ScreenCoord.Y < r.Y + r.H)
        {
            if (auto* hit = SideRB->HitTest(ScreenCoord)) return hit;
            return SideRB;
        }
    }
    return SWindow::HitTest(ScreenCoord);
}
