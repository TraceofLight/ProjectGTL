#include "pch.h"
#include "Runtime/Core/Public/AppWindow.h"

#include <dwmapi.h>

#include "resource.h"
#include "ImGui/imgui.h"

#include "Window/Public/Window.h"
#include "Manager/UI/Public/UIManager.h"
#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"
#include "Render/Renderer/Public/Renderer.h"

FAppWindow::FAppWindow(FEngineLoop* InOwner)
	: Owner(InOwner), InstanceHandle(nullptr), MainWindowHandle(nullptr)
{
}

FAppWindow::~FAppWindow() = default;

bool FAppWindow::Init(HINSTANCE InInstance, int InCmdShow)
{
	InstanceHandle = InInstance;

	WCHAR WindowClass[] = L"BrandNewEngineWindowClass";

	// 아이콘 로드
	HICON hIcon = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));
	HICON hIconSm = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));

	WNDCLASSW wndclass;
	wndclass.style = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = InInstance;
	wndclass.hIcon = hIcon;
	wndclass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndclass.hbrBackground = nullptr;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = WindowClass;

	RegisterClassW(&wndclass);

	MainWindowHandle = CreateWindowExW(0, WindowClass, L"",
	                                   WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
	                                   CW_USEDEFAULT, CW_USEDEFAULT,
	                                   Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT,
	                                   nullptr, nullptr, InInstance, this);

	if (!MainWindowHandle)
	{
		return false;
	}

	// DWM을 사용하여 클라이언트 영역을 non-client 영역까지 확장
	MARGINS Margins = {1, 1, 1, 1};
	DwmExtendFrameIntoClientArea(MainWindowHandle, &Margins);

	if (hIcon)
	{
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSm));
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	}

	ShowWindow(MainWindowHandle, InCmdShow);
	UpdateWindow(MainWindowHandle);
	SetNewTitle(L"BrandNew Engine");

	return true;
}

/**
 * @brief Initialize Console & Redirect IO
 * 현재 ImGui로 기능을 넘기면서 사용은 하지 않으나 코드는 유지
 */
void FAppWindow::InitializeConsole()
{
	// Error Handle
	if (!AllocConsole())
	{
		MessageBoxW(nullptr, L"콘솔 생성 실패", L"Error", 0);
	}

	// Console 출력 지정
	FILE* FilePtr;
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stdout);
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stderr);
	(void)freopen_s(&FilePtr, "CONIN$", "r", stdin);

	// Console Menu Setting
	HWND ConsoleWindow = GetConsoleWindow();
	HMENU MenuHandle = GetSystemMenu(ConsoleWindow, FALSE);
	if (MenuHandle != nullptr)
	{
		EnableMenuItem(MenuHandle, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		DeleteMenu(MenuHandle, SC_CLOSE, MF_BYCOMMAND);
	}
}

FAppWindow* FAppWindow::GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam)
{
	if (InMessage == WM_NCCREATE)
	{
		CREATESTRUCT* CreateStruct = reinterpret_cast<CREATESTRUCT*>(InLParam);
		FAppWindow* WindowInstance = static_cast<FAppWindow*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(InWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowInstance));

		return WindowInstance;
	}

	return reinterpret_cast<FAppWindow*>(GetWindowLongPtr(InWindowHandle, GWLP_USERDATA));
}

LRESULT CALLBACK FAppWindow::WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam,
                                     LPARAM InLParam)
{
	// WndProc 최상단에서 Window Instance 포인터를 가져오거나, WM_NCCREATE 메시지에서 설정
	// 이 작업은 다른 모든 메시지 처리보다 반드시 먼저 수행되어야 함
	FAppWindow* WindowInstance = GetWindowInstance(InWindowHandle, InMessage, InLParam);

	if (UUIManager::WndProcHandler(InWindowHandle, InMessage, InWParam, InLParam))
	{
		if (ImGui::GetIO().WantCaptureMouse)
		{
			return true;
		}
	}

	switch (InMessage)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		if (WindowInstance)
		{
			WindowInstance->EnqueueInputMessage(InMessage, InWParam, InLParam);
		}
		return 0;
	}

	// 엔진 서브시스템에 의존하는 메시지들
	const bool bRequiresEngineSubsystem =
		InMessage == WM_SIZE ||
		InMessage == WM_EXITSIZEMOVE;

	if (WindowInstance && bRequiresEngineSubsystem)
	{
		if (WindowInstance->bIsEngineInitialized)
		{
			// 엔진 초기화 완료: 즉시 처리
			WindowInstance->ProcessWindowMessage({InWindowHandle, InMessage, InWParam, InLParam});
		}
		else
		{
			// 엔진 초기화 미완료: 큐에 보관
			lock_guard Lock(WindowInstance->WindowEventMutex);
			WindowInstance->PendingWindowEvents.push_back({InWindowHandle, InMessage, InWParam, InLParam});
		}
		return 0;
	}

	// 나머지 메시지들은 기존 방식으로 처리
	switch (InMessage)
	{
	case WM_NCCALCSIZE:
		{
			// non-client 영역을 완전히 제거
			if (InWParam == TRUE)
			{
				return 0;
			}
			break;
		}

	case WM_NCHITTEST:
		{
			// 기본 hit test 수행
			LRESULT Hit = DefWindowProc(InWindowHandle, InMessage, InWParam, InLParam);

			// 클라이언트 영역이고 상단 메뉴바 영역이면 드래그 가능하도록 설정
			if (Hit == HTCLIENT)
			{
				POINT ScreenPoint;
				ScreenPoint.x = static_cast<int16>(LOWORD(InLParam));
				ScreenPoint.y = static_cast<int16>(HIWORD(InLParam));
				ScreenToClient(InWindowHandle, &ScreenPoint);

				// 메뉴바 높이만큼만 드래그 가능
				if (ScreenPoint.y >= 0 && ScreenPoint.y <= 30)
				{
					if (ImGui::GetIO().WantCaptureMouse && ImGui::IsAnyItemHovered())
					{
						// ImGui 요소 위에 있으면 클라이언트 영역으로 처리
						return HTCLIENT;
					}

					// 빈 공간이면 타이틀바처럼 동작
					return HTCAPTION;
				}
			}
			return Hit;
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ENTERSIZEMOVE: //드래그 시작
		URenderer::GetInstance().SetIsResizing(true);
		break;

	case WM_ACTIVATE:
		if (LOWORD(InWParam) == WA_INACTIVE)
		{
			if (WindowInstance)
			{
				WindowInstance->EnqueueInputMessage(WM_KILLFOCUS, 0, 0);
			}
		}
		else
		{
			if (WindowInstance)
			{
				WindowInstance->EnqueueInputMessage(WM_SETFOCUS, 0, 0);
			}
			UUIManager::GetInstance().OnWindowRestored();
		}
		break;

	default:
		return DefWindowProc(InWindowHandle, InMessage, InWParam, InLParam);
	}

	return 0;
}

void FAppWindow::SetNewTitle(const wstring& InNewTitle) const
{
	SetWindowTextW(MainWindowHandle, InNewTitle.c_str());
}

void FAppWindow::GetClientSize(int32& OutWidth, int32& OutHeight) const
{
	RECT ClientRectangle;
	GetClientRect(MainWindowHandle, &ClientRectangle);
	OutWidth = ClientRectangle.right - ClientRectangle.left;
	OutHeight = ClientRectangle.bottom - ClientRectangle.top;
}

/**
 * @brief 입력 메시지를 큐에 추가
 */
void FAppWindow::EnqueueInputMessage(uint32 InMessage, WPARAM InWParam, LPARAM InLParam)
{
	std::lock_guard Lock(InputQueueMutex);
	InputMessageQueue.push({InMessage, InWParam, InLParam});
}

/**
 * @brief 큐에 쌓인 입력 메시지를 InputSubsystem에 전달하여 처리
 */
void FAppWindow::ProcessPendingInputMessages()
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		UE_LOG_ERROR("ProcessPendingInputMessages: InputSubsystem이 존재하지 않습니다");
		return;
	}

	lock_guard Lock(InputQueueMutex);

	while (!InputMessageQueue.empty())
	{
		const FInputMessage& Message = InputMessageQueue.front();

		// Focus 메시지 처리
		if (Message.Message == WM_SETFOCUS)
		{
			InputSubsystem->SetWindowFocus(true);
		}
		else if (Message.Message == WM_KILLFOCUS)
		{
			InputSubsystem->SetWindowFocus(false);
		}
		else
		{
			// 일반 입력 메시지 처리
			InputSubsystem->ProcessKeyMessage(Message.Message, Message.WParam, Message.LParam);
		}

		InputMessageQueue.pop();
	}
}

/**
 * @brief 윈도우 이벤트 메시지를 실제로 처리하는 내부 함수
 */
void FAppWindow::ProcessWindowMessage(const FQueuedWindowEvent& Event)
{
	switch (Event.Message)
	{
	case WM_EXITSIZEMOVE: // 드래그 종료
		URenderer::GetInstance().SetIsResizing(false);
		URenderer::GetInstance().OnResize();
		UUIManager::GetInstance().RepositionImGuiWindows();
		if (auto* ViewportSubsystem = GEngine->GetEngineSubsystem<UViewportSubsystem>())
		{
			if (auto* Root = ViewportSubsystem->GetRoot())
			{
				RECT Rect;
				GetClientRect(Event.hWnd, &Rect);
				Root->OnResize({
					0, 0, static_cast<int32>(Rect.right - Rect.left), static_cast<int32>(Rect.bottom - Rect.top)
				});
			}
		}
		break;

	case WM_SIZE:
		if (Event.WParam != SIZE_MINIMIZED)
		{
			if (!URenderer::GetInstance().GetIsResizing())
			{
				// 드래그 X 일때 추가 처리
				URenderer::GetInstance().OnResize(LOWORD(Event.LParam), HIWORD(Event.LParam));
				UUIManager::GetInstance().RepositionImGuiWindows();
				if (auto* ViewportSubsystem = GEngine->GetEngineSubsystem<UViewportSubsystem>())
				{
					if (auto* Root = ViewportSubsystem->GetRoot())
					{
						Root->OnResize({0, 0, static_cast<int32>(LOWORD(Event.LParam)), static_cast<int32>(HIWORD(Event.LParam))});
					}
				}
			}
		}
		else // SIZE_MINIMIZED
		{
			EnqueueInputMessage(WM_KILLFOCUS, 0, 0);
			UUIManager::GetInstance().OnWindowMinimized();
		}
		break;
	}
}

/**
 * @brief 큐에 쌓인 윈도우 이벤트를 처리
 */
void FAppWindow::ProcessPendingWindowEvents()
{
	lock_guard Lock(WindowEventMutex);

	for (const auto& Event : PendingWindowEvents)
	{
		ProcessWindowMessage(Event);
	}

	PendingWindowEvents.clear();
}
