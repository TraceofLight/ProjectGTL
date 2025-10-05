#include "pch.h"
#include "Runtime/Core/Public/EngineLoop.h"

#include "Runtime/Core/Public/AppWindow.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Engine/Public/EngineEditor.h"
#include "Runtime/Engine/Public/GameInstance.h"
#include "Runtime/Engine/Public/LocalPlayer.h"

#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Viewport/Public/ViewportManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Runtime/Subsystem/Public/OverlayManagerSubsystem.h"

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
	// CRITICAL: UClass 시스템 초기화는 다른 모든 시스템보다 먼저 수행되어야 함
	// 언리얼 엔진에서는 CoreInit에서 처리되지만, 여기서는 명시적으로 초기화
	InitializeClassSystem();

	// Initialize Core Subsystem
	UEngine::GetInstance();
	GEngine->SetAppWindow(Window);

	// CRITICAL: 렌더러는 서브시스템들이 초기화되기 전에 먼저 준비되어야 함
	// TODO(KHJ): 렌더러 제거
	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	GEngine->Initialize();

#ifdef EDITOR_MODE
	UEngineEditor::GetInstance();
	GEditor->Initialize();
#endif

	UGameInstance::GetInstance();
	GameInstance->Initialize();

	ULocalPlayer::GetInstance();
	LocalPlayer->Initialize();

	// UIManager Initialize
	auto& UIManger = UUIManager::GetInstance();
	UIManger.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();
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

	// Process input task
    if (auto* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>())
    {
        InputSubsystem->PrepareNewFrame();
    	Window->ProcessPendingInputMessages();
    }

	// Engine tick
    GEngine->TickEngineSubsystems();

	// Editor tick
#ifdef EDITOR_MODE
    GEditor->EditorUpdate();
#endif

    UUIManager::GetInstance().Update();
    UViewportManager::GetInstance().Update();

    URenderer::GetInstance().Update();
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리하는 함수
 */
void FEngineLoop::Exit() const
{
	LocalPlayer->Shutdown();
	GameInstance->Shutdown();

#ifdef EDITOR_MODE
	GEditor->Shutdown();
#endif

	GEngine->Shutdown();

	URenderer::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();
	UViewportManager::GetInstance().Release();

	// Release되지 않은 UObject의 메모리 할당 해제
	// TODO(KHJ): 추후 GC가 처리할 것
	UClass::Shutdown();

	delete Window;
}

/**
 * @brief UClass 시스템 초기화 함수
 * 언리얼 엔진의 CoreInit 단계에서 수행되는 작업을 대체하며, 명시적으로 Class가 우선적으로 초기화될 수 있도록 한다
 */
void FEngineLoop::InitializeClassSystem()
{
	UE_LOG_SYSTEM("System: UClass: Reflection 시스템 초기화 시작...");

	// 기본 클래스들 등록
	UObject::StaticClass();
	UClass::PrintAllClasses();

	UE_LOG_SYSTEM("System: UClass: Reflection 시스템 초기화 완료");
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
