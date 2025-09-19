#pragma once

class SWindow
{
public:
    virtual ~SWindow() = default;

    // Geometry
    virtual void OnResize(const FRect& InRect); // sets Rect then relayout
    virtual void LayoutChildren();              // for containers
    const FRect& GetRect() const { return Rect; }

    // Painting
    virtual void OnPaint();

    // Input (return true if handled)
    virtual bool OnMouseDown(FPoint Coord, int Button);
    virtual bool OnMouseUp(FPoint Coord, int Button);
    virtual bool OnMouseMove(FPoint Coord);

    // Hit testing (default: self/children-less)
    virtual SWindow* HitTest(FPoint ScreenCoord);

public:
    bool IsHover(FPoint coord) const;

protected:
    FRect Rect;
};
