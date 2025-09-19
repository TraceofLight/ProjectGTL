#include "pch.h"
#include "Core/Public/ClientApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    FEngineLoop Client;
    return Client.Run(hInstance, nShowCmd);
}
