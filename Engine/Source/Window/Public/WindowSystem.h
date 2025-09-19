#pragma once

class SWindow;

namespace WindowSystem
{
    // Set/Get the root window for hit-testing and input routing
    void SetRoot(SWindow* InRoot);
    SWindow* GetRoot();

    // Per-frame, route mouse input from UInputManager to the window tree
    void TickInput();
}

