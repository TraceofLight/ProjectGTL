#include "pch.h"
#include "Source/Window/Public/Viewport.h"
#include "Source/Window/Public/ViewportClient.h"
// InputManager를 통해 전역 마우스 상태를 조회
#include "Source/Manager/Input/Public/InputManager.h"

FViewport::FViewport()
{
}

FViewport::~FViewport() = default;


bool FViewport::HandleMouseDown(int32 InButton, int32 InLocalX, int32 InLocalY)
{
    LastMousePos = { InLocalX, InLocalY };
    // 캡처 시작 조건: 좌클릭(0)만 우선 지원
    if (InButton == 0)
    {
        bCapturing = true;
    }
    // ViewportClient의 명시적 버튼 콜백이 아직 없다면 여기서는 true만 반환
    return true;
}

bool FViewport::HandleMouseUp(int32 InButton, int32 InLocalX, int32 InLocalY)
{
    LastMousePos = { InLocalX, InLocalY };
    if (InButton == 0)
    {
        bCapturing = false;
    }
    return true;
}

bool FViewport::HandleMouseMove(int32 InLocalX, int32 InLocalY)
{
    LastMousePos = { InLocalX, InLocalY };
    if (ViewportClient)
    {
        ViewportClient->MouseMove(this, InLocalX, InLocalY);
        return true;
    }
    return false;
}

bool FViewport::HandleCapturedMouseMove(int32 InLocalX, int32 InLocalY)
{
    LastMousePos = { InLocalX, InLocalY };
    if (ViewportClient)
    {
        ViewportClient->CapturedMouseMove(this, InLocalX, InLocalY);
        return true;
    }
    return false;
}

void FViewport::PumpMouseFromInputManager()
{
    auto& Input = UInputManager::GetInstance();

    const FVector& MousePosWS = Input.GetMousePosition(); // window space (screen)
    const FPoint   Screen{ (LONG)MousePosWS.X, (LONG)MousePosWS.Y };

    // 로컬 좌표계 변환
    const FPoint Local = ToLocal(Screen);

    const bool inside = (Screen.X >= Rect.X) && (Screen.X < Rect.X + Rect.W) &&
                        (Screen.Y >= Rect.Y) && (Screen.Y < Rect.Y + Rect.H);

    // 버튼 상태 변화 감지 후 down/up 처리 (좌클릭 우선)
    if (Input.IsKeyPressed(EKeyInput::MouseLeft) && inside)
    {
        HandleMouseDown(/*Button*/0, Local.X, Local.Y);
    }
    if (Input.IsKeyReleased(EKeyInput::MouseLeft))
    {
        HandleMouseUp(/*Button*/0, Local.X, Local.Y);
    }

    // 이동 라우팅: 캡처 중이면 Captured, 아니면 일반 Move
    const FVector& d = Input.GetMouseDelta();
    const bool moved = (d.X != 0.0f || d.Y != 0.0f);
    if (moved)
    {
        if (bCapturing)
        {
            HandleCapturedMouseMove(Local.X, Local.Y);
        }
        else if (inside)
        {
            HandleMouseMove(Local.X, Local.Y);
        }
    }
}

