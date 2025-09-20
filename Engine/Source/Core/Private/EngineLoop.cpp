#include "pch.h"
#include "Core/Public/EngineLoop.h"

#include "Core/Public/AppWindow.h"

#include "Engine/Public/Engine.h"
#include "Engine/Public/EngineEditor.h"
#include "Engine/Public/GameInstance.h"
#include "Engine/Public/LocalPlayer.h"

#include "Manager/Input/Public/InputManager.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Viewport/Public/ViewportManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"

#define EDITOR_MODE 1

// 전역 DeltaTime 변수 정의
float GDeltaTime = 0.0f;

FEngineLoop::FEngineLoop()
{
	// 시간 초기화
	CurrentTime = high_resolution_clock::now();
	PreviousTime = CurrentTime;
}

FEngineLoop::~FEngineLoop() = default;

/**
 * @brief Loop main runtime function
 * 엔진 초기화, Main Loop 실행을 통한 전체 cycle
 * @return Program termination code
 */
int FEngineLoop::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Make prerequisite for engine initializing
	PreInit(InInstanceHandle, InCmdShow);

	// Initialize base system
	Init();

	// Execute main loop
	MainLoop();

	// Termination process
	Exit();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief 엔진이 Initialize 하기 전, 처리해야 하는 다양한 작업들을 실행하는 함수
 * @param InInstanceHandle Process instance handle
 * @param InCmdShow Window display method
 */
void FEngineLoop::PreInit(HINSTANCE InInstanceHandle, int InCmdShow)
{
	// Memory leak detection & report
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window object initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
	}

	// Create Console
	// #ifdef _DEBUG
	// 	Window->InitializeConsole();
	// #endif

	// Keyboard Accelerator Table Setting
	// AcceleratorTable = LoadAccelerators(InInstanceHandle, MAKEINTRESOURCE(IDC_CLIENT));
}

/**
 * @brief Initialize System For Game Execution
 */
void FEngineLoop::Init() const
{
	// Initialize Core Subsystem
	auto& CoreEngine = UEngine::GetInstance();
	CoreEngine.Initialize();

#ifdef EDITOR_MODE
	auto& CoreEditor = UEngineEditor::GetInstance();
	CoreEditor.Initialize();
#endif

	auto& CoreGameInstance = UGameInstance::GetInstance();
	CoreGameInstance.Initialize();

	auto& CoreLocalPlayer = ULocalPlayer::GetInstance();
	CoreLocalPlayer.Initialize();

	// Initialize By Get Instance
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
	UViewportManager::GetInstance().Initialize(Window);
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FEngineLoop::MainLoop()
{
	// 메시지를 우선적으로 다 처리하기 전까지 Tick이 진행되지 않음
	// Tick에 비해서 매우 많은 메시지가 발생하기 때문인데 if - else 없이 if만 사용하고 항상 Tick을 호출하게 될 경우,
	// 메시지마다 Tick이 호출되므로 성능저하가 발생하게 됨
	while (true)
	{
		// Async message process
		if (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process termination
			if (MainMessage.message == WM_QUIT)
			{
				break;
			}
			// Shortcut key processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		// Game system update
		else
		{
			Tick();
		}
	}
}

/**
 * @brief Update system while game processing
 */
void FEngineLoop::Tick()
{
	UpdateDeltaTime();

	// 일단 Editor만 Tick 처리
	// 나머지는 필요하면 추가할 것
#ifdef EDITOR_MODE
	auto& CoreEditor = UEngineEditor::GetInstance();
	CoreEditor.EditorUpdate();
#endif

	auto& InputManager = UInputManager::GetInstance();
	auto& UIManager = UUIManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	auto& LevelManager = ULevelManager::GetInstance();
	auto& ViewportManager = UViewportManager::GetInstance();

	LevelManager.Update();
	InputManager.Update(Window);
	UIManager.Update();
	ViewportManager.Update();
	Renderer.Update();
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리하는 함수
 */
void FEngineLoop::Exit() const
{
	auto& CoreEngine = UEngine::GetInstance();
	auto& CoreEditor = UEngineEditor::GetInstance();
	auto& CoreGameInstance = UGameInstance::GetInstance();
	auto& CoreLocalPlayer = ULocalPlayer::GetInstance();

	CoreLocalPlayer.Shutdown();
	CoreGameInstance.Shutdown();
	CoreEditor.Shutdown();
	CoreEngine.Shutdown();

	URenderer::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();
	ULevelManager::GetInstance().Shutdown();
	UAssetManager::GetInstance().Release();

	// Release되지 않은 UObject의 메모리 할당 해제
	// TODO(KHJ): 추후 GC가 처리할 것
	UClass::Shutdown();

	delete Window;
}

/**
 * @brief 이전 시간과 비교하여 DeltaTime 계산하는 함수
 * 여기서 GDeltaTime 전역 변수에 반영한다
 */
void FEngineLoop::UpdateDeltaTime()
{
	// 이전 시간 업데이트
	PreviousTime = CurrentTime;
	CurrentTime = high_resolution_clock::now();

	// DeltaTime 계산
	auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - PreviousTime);
	GDeltaTime = Duration.count() / 1000000.0f;
}
