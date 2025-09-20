#include "pch.h"
#include "Core/Public/ClientApp.h"

#include "Editor/Public/Editor.h"
#include "Core/Public/AppWindow.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Time/Public/TimeManager.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Manager/Viewport/Public/ViewportManager.h"


FClientApp::FClientApp() = default;

FClientApp::~FClientApp() = default;
/**
 * @brief Client Main Runtime Function
 * App 초기화, Main Loop 실행을 통한 전체 Cycle
 *
 * @param InInstanceHandle Process Instance Handle
 * @param InCmdShow Window Display Method
 *
 *
 * @return Program Termination Code
 */
int FClientApp::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Memory Leak Detection & Report
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
		return 0;
	}

	// Create Console
	// #ifdef _DEBUG
	// 	Window->InitializeConsole();
	// #endif

	// Keyboard Accelerator Table Setting
	// AcceleratorTable = LoadAccelerators(InInstanceHandle, MAKEINTRESOURCE(IDC_CLIENT));

	// Initialize Base System
	int InitResult = InitializeSystem();
	if (InitResult != S_OK)
	{
		assert(!"Initialize Failed");
		return 0;
	}

	// Execute Main Loop
	MainLoop();

	// Termination Process
	ShutdownSystem();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem() const
{
	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance();

	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	// UIManager Initialize
	auto& UIManger = UUIManager::GetInstance();
	UIManger.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();

	UAssetManager::GetInstance().Initialize();

	// Create Default Level
	// TODO(KHJ): 나중에 Init에서 처리하도록 하는 게 맞을 듯
	ULevelManager::GetInstance().CreateDefaultLevel();

    // Initialize viewport manager (build root layout)
    UViewportManager::GetInstance().Initialize(Window);


	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::UpdateSystem() const
{
	auto& TimeManager = UTimeManager::GetInstance();
	auto& InputManager = UInputManager::GetInstance();
	auto& UIManager = UUIManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	auto& LevelManager = ULevelManager::GetInstance();
	auto& ViewportManager = UViewportManager::GetInstance();

	//Editor->Update();
	LevelManager.Update();
	TimeManager.Update();
	InputManager.Update(Window);
	UIManager.Update();
	ViewportManager.Update();
    Renderer.Update();
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FClientApp::MainLoop()
{
	while (true)
	{
		// Async Message Process
		if (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				break;
			}
			// Shortcut Key Processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		// Game System Update
		else
		{
			UpdateSystem();
		}
	}
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리합니다.
 */
void FClientApp::ShutdownSystem() const
{
	URenderer::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();
	ULevelManager::GetInstance().Shutdown();
	UAssetManager::GetInstance().Release();

	// Release되지 않은 UObject의 메모리 할당 해제
	// 추후 GC가 처리할 것
	UClass::Shutdown();

	delete Window;
}

