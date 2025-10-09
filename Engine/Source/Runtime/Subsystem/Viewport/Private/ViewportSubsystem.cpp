#include "pch.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

#include "Factory/Public/NewObject.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Core/Public/AppWindow.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/Config/Public/ConfigSubsystem.h"
#include "Window/Public/Window.h"
#include "Window/Public/Splitter.h"
#include "Window/Public/SplitterV.h"
#include "Window/Public/SplitterH.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/ViewportClient.h"
#include "Runtime/Actor/Public/CameraActor.h"
#include "Runtime/Core/Public/VertexTypes.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/UI/Widget/Public/SceneHierarchyWidget.h"
#include "Runtime/RHI/Public/RHIDevice.h"

IMPLEMENT_CLASS(UViewportSubsystem, UEngineSubsystem)

UViewportSubsystem::UViewportSubsystem() = default;
UViewportSubsystem::~UViewportSubsystem() = default;

static void DestroyTree(SWindow*& Node)
{
	if (!Node)
	{
		return;
	}
	if (auto* Split = dynamic_cast<SSplitter*>(Node))
	{
		SWindow* LeftTop = Split->SideLT;
		SWindow* RightBottom = Split->SideRB;

		Split->SideLT = Split->SideRB = nullptr;
		DestroyTree(LeftTop);
		DestroyTree(RightBottom);
	}
	delete Node;
	Node = nullptr;
}

void UViewportSubsystem::Initialize()
{
	Super::Initialize();
	UE_LOG("ViewportSubsystem: Initialized");
}

void UViewportSubsystem::PostInitialize()
{
	InitializeViewportSystem(GEngine->GetAppWindow());
}

void UViewportSubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG("ViewportSubsystem: Deinitializing...");
	Release();
}

void UViewportSubsystem::InitializeViewportSystem(FAppWindow* InWindow)
{
    // 밖에서 윈도우를 가져와 크기를 가져온다
    int32 Width = 0, Height = 0;
    if (InWindow)
    {
        InWindow->GetClientSize(Width, Height);
    }
    AppWindow = InWindow;

    // 루트 윈도우에 새로운 윈도우를 할당합니다.
    SWindow* Window = new SWindow();
    Window->OnResize({0, 0, Width, Height});
    SetRoot(Window);

    	// 4개의 뷰포트와 클라이언트를 초기화합니다.
        InitializeViewportAndClient();

    	// InitialOffsets 배열 초기화 (카메라는 deprecated지만 InitialOffsets는 필요)
    	InitializeOrthoGraphicCamera();
    	// InitializePerspectiveCamera(); // Perspective 카메라는 필요 없음

    	if (UConfigSubsystem* ConfigSubsystem = GEngine ? GEngine->GetEngineSubsystem<UConfigSubsystem>() : nullptr)	{
		IniSaveSharedV = ConfigSubsystem->GetSplitV();
		IniSaveSharedH = ConfigSubsystem->GetSplitH();
	}
	else
	{
		IniSaveSharedV = 0.5f;
		IniSaveSharedH = 0.5f;
	}

	SplitterValueV = IniSaveSharedV;
	SplitterValueH = IniSaveSharedH;
}

void UViewportSubsystem::Tick(float DeltaSeconds)
{
	// 사용자 입력으로 카메라 조작 시작 시, 모든 포커싱 애니메이션 중단
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	// 우클릭 또는 미들클릭으로 드래그 시작 시
	if (InputSubsystem && (InputSubsystem->IsKeyDown(EKeyInput::MouseRight) || InputSubsystem->IsKeyDown(EKeyInput::MouseMiddle)))
	{
		auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
		TObjectPtr<UWidget> Widget = UISS->FindWidget(FName("Scene Hierarchy Widget"));
		if (USceneHierarchyWidget* SceneWidget = Cast<USceneHierarchyWidget>(Widget))
		{
			if (SceneWidget->IsAnyViewportAnimating())
			{
				SceneWidget->StopAllViewportAnimations();
			}
		}
	}

	if (!Root)
	{
		return;
	}

	int32 Width = 0, Height = 0;

	// AppWindow는 실제 윈도우의 메세지콜이나 크기를 담당합니다
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	const int MenuHeight = static_cast<int>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight());
	const int32 ToolbarH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetToolbarHeight());
	const int32 RightPanelWidth = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;

	if (Width > 0 && Height > 0)
	{
		Root->OnResize(FRect{0, MenuHeight + ToolbarH, max(0, ViewportWidth), max(0, Height - MenuHeight - ToolbarH)});
	}


	// 0) 스플리터 등 윈도우 트리 입력 처리 (캐처/드래그 우선)
	TickInput();

	// 뷰포트 애니메이션 업데이트
	if (ViewportAnimation.bIsAnimating)
	{
		UpdateViewportAnimation();
	}

	// 1) 리프 Rect 변화 동기화(스플리터 드래그 등)
	// 애니메이션 중에도 뷰포트 Rect를 동기화해야 올바른 뷰포트가 렌더링됨
	if (ViewportAnimation.bIsAnimating)
	{
		// 애니메이션 중에는 특별한 동기화 로직 사용
		SyncAnimationRectsToViewports();
	}
	else
	{
		SyncRectsToViewports();
	}

	// 2) 입력 라우팅 (각 뷰포트가 InputManager에서 읽어 로컬 좌표로 변환/전달)
	//    스플리터가 캡처 중이면 충돌 방지를 위해 뷰포트 입력은 건너뜀
	if (Capture == nullptr)
	{
		PumpAllViewportInput();
	}

	// 2.5) 현재 우클릭 중이면 어떤 뷰포트가 카메라 제어 대상인지 결정
	UpdateActiveRmbViewportIndex();

	// 2.7) 직교투영: 마우스가 올라가 있는 뷰포트에 대해 휠로 앞/뒤 도리 이동 처리 (RMB 여부 무관)
	{
		UInputSubsystem* WheelInputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
		float WheelDelta = WheelInputSubsystem ? WheelInputSubsystem->GetMouseWheelDelta() : 0.0f;

		if (WheelDelta != 0.0f && WheelInputSubsystem)
		{
			int32 Index = GetViewportIndexUnderMouse();
			if (Index >= 0 && Index < static_cast<int32>(Clients.Num()))
			{
				FViewportClient* Client = Clients[Index];
				if (Client && Client->IsOrtho())
				{
					// Camera deprecated - 카메라 없이 처리
					// ACameraActor* OrthoCam = GetActiveCameraForViewport(Index);
					// if (OrthoCam)
					// if (true) // 항상 처리
					{
						constexpr float DollyStep = 50.0f; // 튜닝 가능 (10배 증가)
						constexpr float MinFovY = 10.0f; // 최소 FovY 값
						constexpr float MaxFovY = 500.0f; // 최대 FovY 값

						float NewFovY = SharedFovY - (WheelDelta * DollyStep);

						// 갑작스러운 점프 방지를 위해 점진적 제한
						if (NewFovY < MinFovY)
						{
							// 최소값 근처에서는 점점 느려지게 제한
							float DistanceFromMin = SharedFovY - MinFovY;
							if (DistanceFromMin > 0.0f)
							{
								// 최소값에 가까울수록 진행 속도 감소
								float ProgressRatio = min(1.0f, DistanceFromMin / 20.0f); // 20 범위에서 점진적 감소
								SharedFovY = max(MinFovY, SharedFovY - (WheelDelta * DollyStep * ProgressRatio));
							}
							else
							{
								SharedFovY = MinFovY; // 최소값 고정
							}
						}
						else if (NewFovY > MaxFovY)
						{
							SharedFovY = MaxFovY; // 최대값 제한
						}
						else
						{
							SharedFovY = NewFovY; // 정상 범위 내에서 업데이트
						}

						WheelInputSubsystem->SetMouseWheelDelta(0.0f);
					}
				}
				else if (Client && !Client->IsOrtho())
				{
					// 2.8) 원근투영: 우클릭 드래그 중에만 휠로 카메라 이동 속도 조절
					if (WheelInputSubsystem->IsKeyDown(EKeyInput::MouseRight))
					{
						constexpr float SpeedStep = 5.0f;
						constexpr float MinSpeed = 15.0f;
						constexpr float MaxSpeed = 500.0f;

						float NewSpeed = PerspectiveMoveSpeed + (WheelDelta * SpeedStep);
						PerspectiveMoveSpeed = std::clamp(NewSpeed, MinSpeed, MaxSpeed);

						WheelInputSubsystem->SetMouseWheelDelta(0.0f);
					}
				}
			}
		}
	}

	UpdateOrthoGraphicCameraPoint();

	UpdateOrthoGraphicCameraFov();

	// Perspective 카메라 입력 처리
	UpdatePerspectiveCamera();

	// Camera deprecated - 직교 카메라 배열 사용 안 함
	// FovY 업데이트 후 모든 직교 카메라의 행렬 갱신
	// for (ACameraActor* Camera : OrthoGraphicCameras)
	// {
	// 	if (Camera && Camera->GetCameraComponent())
	// 	{
	// 		Camera->GetCameraComponent()->UpdateMatrixByOrth();
	// 	}
	// }

	//ApplySharedOrthoCenterToAllCameras();

	// 3) 카메라 업데이트 (공유 오쉰 1회, 퍼스폐티브는 각자)
	TickCameras();
}

void UViewportSubsystem::BuildSingleLayout(int32 PromoteIdx)
{
	if (!Root)
	{
		return;
	}

	Capture = nullptr;
	DestroyTree(Root);

	if (PromoteIdx < 0 || PromoteIdx >= static_cast<int32>(Viewports.Num()))
	{
		// 우클릭 드래그 중인 뷰가 있으면 그걸 우선, 없으면 0번 유지
		PromoteIdx = (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < static_cast<int32>(Viewports.Num()))
			             ? ActiveRmbViewportIdx
			             : 0;
	}

	// 0.5) 선택된 인덱스를 0번으로 올리기(뷰포트/클라이언트 페어 동기 스왑)
	if (PromoteIdx != 0)
	{
		std::swap(Viewports[0], Viewports[PromoteIdx]);
		std::swap(Clients[0], Clients[PromoteIdx]);
		LastPromotedIdx = PromoteIdx;
	}
	else
	{
		LastPromotedIdx = -1;
	}

	// 1) 윈도우 크기 읽기 및 오른쪽 패널 영역 제외
	int32 Width = 0, Height = 0;
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	const int32 MenuH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight());
	const int32 ToolbarH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetToolbarHeight());
	const int32 BottomH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetBottomBarHeight());
	const int32 RightPanelWidth = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;
	const int32 ViewportHeight = Height - MenuH - ToolbarH - BottomH;

	const FRect Rect{0, MenuH + ToolbarH, max(0, ViewportWidth), max(0, ViewportHeight)};

	// 2) 기존 캡처 해제 (안전)
	Capture = nullptr;

	// 3) 새 루트 구성 (싱글)
	SWindow* Single = new SWindow();
	Single->OnResize(Rect);
	SetRoot(Single);
}


// 싱글->쿼드로 전환 시, 스플리터V를 할당합니다. 스플리터V는 트리구조로 스플리터H를 2개 자식으로 가지고 최종적으로 4개의 Swindow를 가집니다.
void UViewportSubsystem::BuildFourSplitLayout()
{
	if (!Root)
	{
		return;
	}

	if (LastPromotedIdx > 0 && LastPromotedIdx < (int32)Viewports.Num())
	{
		std::swap(Viewports[0], Viewports[LastPromotedIdx]);
		std::swap(Clients[0], Clients[LastPromotedIdx]);
	}
	LastPromotedIdx = -1;

	Capture = nullptr;
	DestroyTree(Root);

	int32 Width = 0, Height = 0;

	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	const int MenuHeight = static_cast<int>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight());
	const int32 ToolbarH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetToolbarHeight());
	const int32 BottomH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetBottomBarHeight());
	const int32 RightPanelWidth = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;
	const int32 ViewportHeight = Height - MenuHeight - ToolbarH - BottomH;

	const FRect Rect{0, MenuHeight + ToolbarH, max(0, ViewportWidth), max(0, ViewportHeight)};

	// 기존 캡처 해제 (안전)
	Capture = nullptr;

	// 4-way splitter tree
	// Unreal Layout: [0]Top(좌상), [1]Front(좌하), [2]Perspective(우상), [3]Right(우하)
	SSplitter* RootSplit = new SSplitterV();
	RootSplit->Ratio = IniSaveSharedV;

	//SharedY = 0.5f;
	SSplitter* LeftSplit = new SSplitterH();
	SSplitter* RightSplit = new SSplitterH();

	LeftSplit->Ratio = IniSaveSharedH;
	RightSplit->Ratio = IniSaveSharedH;

	LeftSplit->SetSharedRatio(&IniSaveSharedH);
	RightSplit->SetSharedRatio(&IniSaveSharedH);

	RootSplit->SetChildren(LeftSplit, RightSplit);

	// 재귀 순회 순서: Left의 LT(0), Left의 RB(1), Right의 LT(2), Right의 RB(3)
	// 따라서: 0=좌상, 1=좌하, 2=우상, 3=우하
	SWindow* TopLeft = new SWindow();      // 0: Perspective
	SWindow* BottomLeft = new SWindow();   // 1: Front
	SWindow* TopRight = new SWindow();     // 2: Top
	SWindow* BottomRight = new SWindow();  // 3: Right

	// Left splitter: 0=Perspective(좌상), 1=Front(좌하)
	LeftSplit->SetChildren(TopLeft, BottomLeft);

	// Right splitter: 2=Top(우상), 3=Right(우하)
	RightSplit->SetChildren(TopRight, BottomRight);

	// splitterV를 root에 set합니다
	RootSplit->OnResize(Rect);
	SetRoot(RootSplit);

	ForceRefreshOrthoViewsAfterLayoutChange();
}

void UViewportSubsystem::GetLeafRects(TArray<FRect>& OutRects) const
{
	OutRects.Empty();
	if (!Root)
	{
		return;
	}

	// 재귀 수집
	struct Local
	{
		static void Collect(SWindow* Node, TArray<FRect>& Out)
		{
			if (auto* Split = Cast(Node))
			{
				Collect(Split->SideLT, Out);
				Collect(Split->SideRB, Out);
			}
			else
			{
				Out.Emplace(Node->GetRect());
			}
		}
	};
	Local::Collect(Root, OutRects);
}


void UViewportSubsystem::SyncRectsToViewports() const
{
	TArray<FRect> Leaves;
	GetLeafRects(Leaves);

	// 싱글 모드: 리프가 1개, 4분할: 리프가 4개
	const int32 N = min<int32>(static_cast<int32>(Leaves.Num()), static_cast<int32>(Viewports.Num()));
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->SetRect(Leaves[i]);
		Viewports[i]->SetToolbarHeight(28);

		const int32 renderH = max<LONG>(0, Leaves[i].H - Viewports[i]->GetToolbarHeight());
		Clients[i]->OnResize({Leaves[i].W, renderH});

		// DEBUG: 모든 뷰포트 Rect 출력 (한번만)
		static bool bPrinted = false;
		if (!bPrinted && N == 4)
		{
			UE_LOG("Viewport[%d] Rect: X=%d, Y=%d, W=%d, H=%d", i, Leaves[i].X, Leaves[i].Y, Leaves[i].W, Leaves[i].H);
			if (i == 3) bPrinted = true;
		}
	}
}

void UViewportSubsystem::PumpAllViewportInput() const
{
	// 각 뷰포트가 스스로 InputManager에서 읽어
	// 로컬 좌표로 변환 → 각자의 Client로 MouseMove/CapturedMouseMove 전달
	const int32 N = static_cast<int32>(Viewports.Num());
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->PumpMouseFromInputManager();
	}
}

// 오른쪽 마우스를 누르고 있는 뷰포트 인덱스를 셋 해줍니다.
void UViewportSubsystem::UpdateActiveRmbViewportIndex()
{
	ActiveRmbViewportIdx = -1;
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem || !InputSubsystem->IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	const FVector& MousePosition = InputSubsystem->GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositioinY = static_cast<LONG>(MousePosition.Y);

	const int32 N = static_cast<int32>(Viewports.Num());
	for (int32 i = 0; i < N; ++i)
	{
		const FRect& Rect = Viewports[i]->GetRect();
		const int32 ToolbarHeight = Viewports[i]->GetToolbarHeight();

		// Toolbar 높이를 고려한 실제 렌더링 영역에서만 활성화
		const LONG RenderAreaTop = Rect.Y + ToolbarHeight;
		const LONG RenderAreaBottom = Rect.Y + Rect.H;

		if (MousePositionX >= Rect.X && MousePositionX < Rect.X + Rect.W &&
			MousePositioinY >= RenderAreaTop && MousePositioinY < RenderAreaBottom)
		{
			ActiveRmbViewportIdx = i;
			break;
		}
	}
}

void UViewportSubsystem::TickCameras() const
{
	// Camera deprecated - 더 이상 카메라를 틱하지 않음
	// 우클릭이 눌린 상태라면 해당 뷰포트의 카메라만 입력 반영(Update)
	// 그렇지 않다면(비상호작용) 카메라 이동 입력은 생략하여 타 뷰포트에 영향이 가지 않도록 함
	// UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	// const bool bRmbDown = InputSubsystem && InputSubsystem->IsKeyDown(EKeyInput::MouseRight);
	// if (bRmbDown)
	// {
	// 	if (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < static_cast<int32>(Clients.Num()))
	// 	{
	// 		ACameraActor* ActiveCamera = GetActiveCameraForViewport(ActiveRmbViewportIdx);
	// 		if(ActiveCamera)
	// 		{
	// 			ActiveCamera->Tick(DT);
	// 		}
	// 	}
	// }
}

void UViewportSubsystem::RenderOverlay()
{
	if (!Root)
	{
		return;
	}
	// 스플리터 선만 렌더링 (나머지 UI는 ViewportControlWidget에서 처리)
	Root->OnPaint();
}

void UViewportSubsystem::RenderViewports()
{
	// 렌더링 전 준비 작업
	if (!GDynamicRHI || !GDynamicRHI->IsInitialized())
	{
		return;
	}

	// 프레임 시작
	if (!GDynamicRHI->BeginFrame())
	{
		return;
	}

	// 메인 렌더 타겟 설정
	GDynamicRHI->SetMainRenderTarget();

	// 백버퍼 클리어
	constexpr float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	GDynamicRHI->ClearRenderTarget(ClearColor);

	// 각 뷰포트 렌더링 (각자 내부에서 CommandList 생성하고 관리)
	for (FViewport* Viewport : Viewports)
	{
		if (Viewport && Viewport->ShouldRender())
		{
			if (Viewport->GetViewportClient())
			{
				Viewport->GetViewportClient()->Draw(Viewport);
			}
		}
	}


	// 프레임 종료 및 Present
	GDynamicRHI->EndFrame();
}

void UViewportSubsystem::Release()
{
	Capture = nullptr;
	ActiveRmbViewportIdx = -1;

	for (FViewportClient* Client : Clients)
	{
		delete Client;
	}

	for (FViewport* Viewport : Viewports)
	{
		delete Viewport;
	}


	// Camera deprecated - 카메라 배열 삭제 안 함
	// for (ACameraActor* Cam : OrthoGraphicCameras)
	// {
	// 	delete Cam;
	// }


	// for (ACameraActor* Cam : PerspectiveCameras)
	// {
	// 	delete Cam;
	// }

	if (Root)
	{
		delete Root;
		Root = nullptr;
	}

	AppWindow = nullptr;
}

void UViewportSubsystem::InitializeViewportAndClient()
{
    for (int i = 0; i < 4; i++)
    {
        FViewport* Viewport = new FViewport;
        FViewportClient* ViewportClient = new FViewportClient;
        Viewport->SetViewportClient(ViewportClient);
        Clients.Add(ViewportClient);
        Viewports.Add(Viewport);
    }

    // 뷰포트 트리 구조에 맞춤 매핑:
    // Left splitter:  0=Perspective(좌상), 1=Front(좌하)
    // Right splitter: 2=Top(우상), 3=Right(우하)

    // [0] Perspective view (좌상)
    Clients[0]->SetViewType(EViewType::Perspective);
    Clients[0]->SetViewMode(EViewMode::Unlit);
    Clients[0]->SetViewLocation(FVector(-30.0f, 30.0f, 30.0f));
    // Pitch 이제 음수 = 아래를 봄 (언리얼 표준)
    Clients[0]->SetViewRotation(FVector(-40.0f, -45.0f, 0.0f));
    Clients[0]->SetFovY(90.0f);

    // [1] Front view - X축 방향을 봄 (+X 방향) (좌하)
    // Forward=(1,0,0), Up=(0,0,1) → Right=(0,-1,0)
    Clients[1]->SetViewType(EViewType::OrthoFront);
    Clients[1]->SetViewMode(EViewMode::WireFrame);
    // OrthoGraphicCamerapoint(0,0,0) + InitialOffsets[4] = (-100, 0, 0)
    Clients[1]->SetViewLocation(FVector(-100.0f, 0.0f, 0.0f));
    Clients[1]->SetViewRotation(FVector(0.0f, 0.0f, 0.0f));
    Clients[1]->SetOrthoWidth(100.0f);

    // [2] Top view - 위에서 아래를 봄 (-Z 방향) (우상)
    // Forward=(0,0,-1), Up=(0,1,0) → Right=(1,0,0)
    Clients[2]->SetViewType(EViewType::OrthoTop);
    Clients[2]->SetViewMode(EViewMode::WireFrame);
    // OrthoGraphicCamerapoint(0,0,0) + InitialOffsets[0] = (0, 0, 100)
    Clients[2]->SetViewLocation(FVector(0.0f, 0.0f, 100.0f));
    Clients[2]->SetViewRotation(FVector(0.0f, 0.0f, 0.0f));
    Clients[2]->SetOrthoWidth(100.0f);

    // [3] Right view - Y축 방향을 봄 (+Y 방향) (우하)
    // Forward=(0,1,0), Up=(0,0,1) → Right=(-1,0,0)
    Clients[3]->SetViewType(EViewType::OrthoRight);
    Clients[3]->SetViewMode(EViewMode::WireFrame);
    // OrthoGraphicCamerapoint(0,0,0) + InitialOffsets[3] = (0, -100, 0)
    Clients[3]->SetViewLocation(FVector(0.0f, -100.0f, 0.0f));
    Clients[3]->SetViewRotation(FVector(0.0f, 0.0f, 0.0f));
    Clients[3]->SetOrthoWidth(100.0f);
}

void UViewportSubsystem::InitializeOrthoGraphicCamera()
{
	// Camera deprecated - 더 이상 카메라를 초기화하지 않음
	// for (int i = 0; i < 6; ++i)
	// {
	// 	ACameraActor* Camera = NewObject<ACameraActor>();
	// 	Camera->SetDisplayName("Camera_" + to_string(ACameraActor::GetNextGenNumber()));
	// 	OrthoGraphicCameras.Add(Camera);
	// }

	// InitialOffsets는 여전히 필요할 수 있으므로 유지
	InitialOffsets.Emplace(0.0f, 0.0f, 100.0f);
	InitialOffsets.Emplace(0.0f, 0.0f, -100.0f);
	InitialOffsets.Emplace(0.0f, 100.0f, 0.0f);
	InitialOffsets.Emplace(0.0f, -100.0f, 0.0f);
	InitialOffsets.Emplace(100.0f, 0.0f, 0.0f);
	InitialOffsets.Emplace(-100.0f, 0.0f, 0.0f);
}

void UViewportSubsystem::InitializePerspectiveCamera()
{
	// Camera deprecated - 더 이상 카메라를 초기화하지 않음
	// for (int i = 0; i < 4; ++i)
	// {
	// 	ACameraActor* Camera = NewObject<ACameraActor>();
	// 	Camera->SetDisplayName("Camera_" + to_string(ACameraActor::GetNextGenNumber()));

	// 	// 초기 위치 설정
	// 	Camera->SetActorLocation(FVector(-25.0f, -20.0f, 18.0f));
	// 	Camera->SetActorRotation(FVector(0, 45.0f, 35.0f));

	// 	PerspectiveCameras.Add(Camera);
	// }
}

void UViewportSubsystem::UpdateOrthoGraphicCameraPoint()
{
	// 인풋서브시스템을 가져옵니다.
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 오른쪽 클릭이 아니라면 리턴합니다
	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	// 오른쪽 클릭을 했을 때, 인덱스를 가져옵니다.
	const int32 Index = ActiveRmbViewportIdx;

	// 인덱스가 범위 밖이라면 리턴합니다
	if (Index < 0 || Index >= Clients.Num())
	{
		return;
	}

	// n번째 뷰포트 클라이언트를 가져옵니다.
	FViewportClient* ViewportClient = Clients[Index];

	// 클라이언트가 원근투영 상태라면 리턴합니다.
	if (!ViewportClient || !ViewportClient->IsOrtho())
	{
		return;
	}

	// n번째 뷰포트를 가져옵니다
	FViewport* Viewport = Viewports[Index];
	if (!Viewport)
	{
		return;
	}

	const FRect& Rect = Viewport->GetRect();
	const int32 ToolH = Viewport->GetToolbarHeight(); // 델타에는 영향 X, 높이 > 폭 계산용
	const float Width = static_cast<float>(Rect.W);
	const float Height = static_cast<float>(max<LONG>(0, Rect.H - ToolH));

	if (Width <= 0.0f || Height <= 0.0f)
	{
		return;
	}

	// Camera deprecated - 카메라 없이 처리
	// ACameraActor* OrthoCam = GetActiveCameraForViewport(Index);
	const EViewType ViewType = ViewportClient->GetViewType();

	// 드래그는 ViewportClient.cpp의 GetViewMatrix() Right/Up 벡터의 반대 방향으로 설정
	// (마우스 오른쪽 드래그 = 카메라가 오른쪽으로 이동하는 것처럼 보임 = 월드를 왼쪽으로 이동)
	FVector Right, Up;
	switch (ViewType)
	{
	case EViewType::OrthoTop:
		// Top: ViewMatrix Right=-Y, Up=-X → 드래그는 반대
		Right = FVector(0.0f, 1.0f, 0.0f);      // +Y (반대)
		Up = FVector(1.0f, 0.0f, 0.0f);         // +X (반대)
		break;
	case EViewType::OrthoBottom:
		// Bottom: ViewMatrix Right=+Y, Up=+X → 드래그는 반대
		Right = FVector(0.0f, -1.0f, 0.0f);     // -Y (반대)
		Up = FVector(-1.0f, 0.0f, 0.0f);        // -X (반대)
		break;
	case EViewType::OrthoFront:
		// Front: ViewMatrix Right=-Y, Up=+Z → 드래그는 반대
		Right = FVector(0.0f, 1.0f, 0.0f);      // +Y (반대)
		Up = FVector(0.0f, 0.0f, -1.0f);        // -Z (반대)
		break;
	case EViewType::OrthoBack:
		// Back: ViewMatrix Right=+Y, Up=+Z → 드래그는 반대
		Right = FVector(0.0f, -1.0f, 0.0f);     // -Y (반대)
		Up = FVector(0.0f, 0.0f, -1.0f);        // -Z (반대)
		break;
	case EViewType::OrthoLeft:
		// Left: ViewMatrix Right=-X, Up=+Z → 드래그는 반대
		Right = FVector(1.0f, 0.0f, 0.0f);      // +X (반대)
		Up = FVector(0.0f, 0.0f, -1.0f);        // -Z (반대)
		break;
	case EViewType::OrthoRight:
		// Right: ViewMatrix Right=-X, Up=+Z → 드래그는 반대
		Right = FVector(1.0f, 0.0f, 0.0f);      // +X (반대)
		Up = FVector(0.0f, 0.0f, -1.0f);        // -Z (반대)
		break;
	default: return; // 퍼스펙티브면 여기선 스킵
	}

	// 마우스 델타(px) → NDC 델타 → 월드 델타
	const FVector& Delta = InputSubsystem->GetMouseDelta();
	if (Delta.X == 0.0f && Delta.Y == 0.0f)
	{
		return;
	}

	const float Aspect = (Height > 0.f) ? (Width / Height) : 1.0f;
	// Orthographic 투영에서는 SharedFovY가 실제로는 OrthoWidth로 사용됨
	const float OrthoWidth = SharedFovY;  // SharedFovY는 Ortho에서는 월드 단위 크기
	const float OrthoHeight = (Aspect > 0.0f) ? (OrthoWidth / Aspect) : OrthoWidth;

	// NDC 델타 (마우스 픽셀 → -1..+1) - 감도 5배
	const float Sensitivity = 1.0f;
	// 마우스 오른쪽 드래그 = Right 방향 이동
	// 마우스 위쪽 드래그 = Up 방향 이동
	const float NdcDX = (Delta.X / Width) * 2.0f * Sensitivity;
	const float NdcDY = -(Delta.Y / Height) * 2.0f * Sensitivity;  // Y축 반전 (화면 Y는 위가 0)
	const FVector WorldDelta = Right * (NdcDX * (OrthoWidth * 0.5f)) + Up * (NdcDY * (OrthoHeight * 0.5f));

	// 공유 중심 누적 이동
	OrthoGraphicCamerapoint += WorldDelta;

	// 같은 ViewType을 가진 모든 직교 클라이언트를 업데이트 (같은 뷰는 같은 카메라를 공유)
	for (int32 i = 0; i < Clients.Num(); ++i)
	{
		if (Clients[i] && Clients[i]->IsOrtho())
		{
			// 각 직교 뷰는 공유 중심점 + 초기 오프셋을 사용
			int32 OrthoIdx = -1;
			switch (Clients[i]->GetViewType())
			{
			case EViewType::OrthoTop: OrthoIdx = 0; break;
			case EViewType::OrthoBottom: OrthoIdx = 1; break;
			case EViewType::OrthoLeft: OrthoIdx = 2; break;
			case EViewType::OrthoRight: OrthoIdx = 3; break;
			case EViewType::OrthoFront: OrthoIdx = 4; break;
			case EViewType::OrthoBack: OrthoIdx = 5; break;
			default: break;
			}

			if (OrthoIdx >= 0 && OrthoIdx < static_cast<int32>(InitialOffsets.Num()))
			{
				FVector NewLocation = OrthoGraphicCamerapoint + InitialOffsets[OrthoIdx];
				Clients[i]->SetViewLocation(NewLocation);
				// 같은 ViewType을 가진 다른 모든 클라이언트도 동일한 위치로 설정
				for (int32 j = i + 1; j < Clients.Num(); ++j)
				{
					if (Clients[j] && Clients[j]->GetViewType() == Clients[i]->GetViewType())
					{
						Clients[j]->SetViewLocation(NewLocation);
					}
				}
			}
		}
	}
}

void UViewportSubsystem::UpdateOrthoGraphicCameraFov() const
{
	// Camera deprecated - ViewportClient에 직접 FovY 설정
	// 모든 직교 뷰포트에 공유 FovY를 적용 (모든 Ortho 뷰가 같은 줌 레벨을 공유)
	for (FViewportClient* Client : Clients)
	{
		if (Client && Client->IsOrtho())
		{
			Client->SetOrthoWidth(SharedFovY);
		}
	}
}

void UViewportSubsystem::UpdatePerspectiveCamera()
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 오른쪽 클릭이 아니라면 리턴
	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	const int32 Index = ActiveRmbViewportIdx;

	// 인덱스가 범위 밖이라면 리턴
	if (Index < 0 || Index >= Clients.Num())
	{
		return;
	}

	FViewportClient* ViewportClient = Clients[Index];

	// 클라이언트가 직교투영 상태라면 리턴 (Perspective만 처리)
	if (!ViewportClient || ViewportClient->IsOrtho())
	{
		return;
	}

	// n번째 뷰포트를 가져옵니다
	FViewport* Viewport = Viewports[Index];
	if (!Viewport)
	{
		return;
	}

	FVector CurrentRotation = ViewportClient->GetViewRotation();

	// 1. 회전 처리 (마우스 드래그)
	const FVector& MouseDelta = InputSubsystem->GetMouseDelta();
	if (MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f)
	{
		constexpr float RotationSpeed = 0.25f;
		CurrentRotation.Y += MouseDelta.X * RotationSpeed; // Yaw
		CurrentRotation.X -= MouseDelta.Y * RotationSpeed; // Pitch (마우스 위 = 카메라 위)

		// Pitch 제한 (-89 ~ 89도)
		CurrentRotation.X = max(-89.0f, min(89.0f, CurrentRotation.X));

		ViewportClient->SetViewRotation(CurrentRotation);
	}

	// 2. 이동 처리 (WASD + QE)
	const float MoveSpeed = PerspectiveMoveSpeed; // ViewportSubsystem의 속도 사용
	FVector CurrentLocation = ViewportClient->GetViewLocation();
	bool bHasMovement = false;

	// WASD: 카메라 회전에 따른 이동 (로컬 공간)
	FVector HorizontalMoveDelta(0.0f, 0.0f, 0.0f);

	if (InputSubsystem->IsKeyDown(EKeyInput::W))
	{
		HorizontalMoveDelta += FVector(1.0f, 0.0f, 0.0f); // Forward
		bHasMovement = true;
	}
	if (InputSubsystem->IsKeyDown(EKeyInput::S))
	{
		HorizontalMoveDelta += FVector(-1.0f, 0.0f, 0.0f); // Backward
		bHasMovement = true;
	}
	if (InputSubsystem->IsKeyDown(EKeyInput::D))
	{
		HorizontalMoveDelta += FVector(0.0f, 1.0f, 0.0f); // Right
		bHasMovement = true;
	}
	if (InputSubsystem->IsKeyDown(EKeyInput::A))
	{
		HorizontalMoveDelta += FVector(0.0f, -1.0f, 0.0f); // Left
		bHasMovement = true;
	}

	// WASD 이동을 카메라 회전에 맞춰 변환
	if (HorizontalMoveDelta.LengthSquared() > 0.0f)
	{
		FVector Radians = FVector::GetDegreeToRadian(CurrentRotation);
		FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
		FVector WorldMoveDelta = FMatrix::VectorMultiply(HorizontalMoveDelta, RotationMatrix);
		WorldMoveDelta.Normalize();
		CurrentLocation += WorldMoveDelta * MoveSpeed * DT;
	}

	// QE: 절대 월드 Z축 상하 이동 (회전과 무관)
	if (InputSubsystem->IsKeyDown(EKeyInput::E))
	{
		CurrentLocation.Z += MoveSpeed * DT; // Up
		bHasMovement = true;
	}
	if (InputSubsystem->IsKeyDown(EKeyInput::Q))
	{
		CurrentLocation.Z -= MoveSpeed * DT; // Down
		bHasMovement = true;
	}

	// 위치 업데이트
	if (bHasMovement)
	{
		ViewportClient->SetViewLocation(CurrentLocation);
	}
}

void UViewportSubsystem::BindOrthoGraphicCameraToClient() const
{
	Clients[1]->SetViewType(EViewType::OrthoLeft);

	Clients[2]->SetViewType(EViewType::OrthoTop);

	Clients[3]->SetViewType(EViewType::OrthoRight);
}

void UViewportSubsystem::ForceRefreshOrthoViewsAfterLayoutChange()
{
	// 1) 현재 리프 Rect들을 뷰포트에 즉시 반영 (전환 직후 크기 확정)
	SyncRectsToViewports();

	// 2) 각 클라이언트에 카메라 재바인딩 + Update + OnResize
	const int32 N = static_cast<int32>(Clients.Num());
	for (int32 i = 0; i < N; ++i)
	{
		FViewportClient* Client = Clients[i];
		if (!Client) continue;

		const EViewType Viewport = Client->GetViewType();
		if (Viewport == EViewType::Perspective)
		{
			// Camera deprecated
			// if (i < static_cast<int32>(PerspectiveCameras.Num()) && PerspectiveCameras[i])
			// {
			// 	PerspectiveCameras[i]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
			// }
		}
		else
		{
			// 오쏘: ViewType → 6방향 인덱스 매핑
			int OrthoIdx = -1;
			switch (Viewport)
			{
			case EViewType::OrthoTop: OrthoIdx = 0;
				break;
			case EViewType::OrthoBottom: OrthoIdx = 1;
				break;
			case EViewType::OrthoLeft: OrthoIdx = 2;
				break;
			case EViewType::OrthoRight: OrthoIdx = 3;
				break;
			case EViewType::OrthoFront: OrthoIdx = 4;
				break;
			case EViewType::OrthoBack: OrthoIdx = 5;
				break;
			default: break;
			}

			// Camera deprecated
			// if (OrthoIdx >= 0 && OrthoIdx < static_cast<int>(OrthoGraphicCameras.Num()))
			// {
			// 	ACameraActor* Camera = OrthoGraphicCameras[OrthoIdx];
			// }
		}

		// 크기 반영(툴바 높이 포함) — 깜빡임/툴바 좌표 안정화
		const FRect& Rect = Viewports[i]->GetRect();
		Viewports[i]->SetToolbarHeight(28);
		Client->OnResize({Rect.W, Rect.H});
	}

	// 3) 같은 ViewType을 가진 Ortho 뷰들이 같은 카메라 위치를 공유하도록 동기화
	for (int32 i = 0; i < N; ++i)
	{
		FViewportClient* Client = Clients[i];
		if (!Client || !Client->IsOrtho()) continue;

		EViewType ViewType = Client->GetViewType();
		int32 OrthoIdx = -1;
		switch (ViewType)
		{
		case EViewType::OrthoTop: OrthoIdx = 0; break;
		case EViewType::OrthoBottom: OrthoIdx = 1; break;
		case EViewType::OrthoLeft: OrthoIdx = 2; break;
		case EViewType::OrthoRight: OrthoIdx = 3; break;
		case EViewType::OrthoFront: OrthoIdx = 4; break;
		case EViewType::OrthoBack: OrthoIdx = 5; break;
		default: continue;
		}

		if (OrthoIdx >= 0 && OrthoIdx < static_cast<int32>(InitialOffsets.Num()))
		{
			FVector SharedLocation = OrthoGraphicCamerapoint + InitialOffsets[OrthoIdx];
			// 같은 ViewType을 가진 모든 클라이언트에 동일한 위치 적용
			for (int32 j = 0; j < N; ++j)
			{
				if (Clients[j] && Clients[j]->GetViewType() == ViewType)
				{
					Clients[j]->SetViewLocation(SharedLocation);
				}
			}
		}
	}

	// 4) 드래그 대상 초기화
	ActiveRmbViewportIdx = -1;
}

void UViewportSubsystem::ApplySharedOrthoCenterToAllCameras() const
{
	// Camera deprecated - 이 함수는 더 이상 사용하지 않음
	// OrthoGraphicCamerapoint 기준으로 6개 오쉐 카메라 위치 갱신
	// for (ACameraActor* Camera : OrthoGraphicCameras)
	// {
	// ...
	// }
}

void UViewportSubsystem::PersistSplitterRatios()
{
	if (!Root)
	{
		return;
	}

	// 세로
	IniSaveSharedV = static_cast<SSplitter*>(Root)->Ratio;

	if (UConfigSubsystem* ConfigSubsystem = GEngine ? GEngine->GetEngineSubsystem<UConfigSubsystem>() : nullptr)
	{
		ConfigSubsystem->SetSplitV(IniSaveSharedV);
		ConfigSubsystem->SetSplitH(IniSaveSharedH);
		ConfigSubsystem->SaveEditorSetting();
	}
}

void UViewportSubsystem::TickInput()
{
	if (!Root)
	{
		return;
	}

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	const FVector& MousePosition = InputSubsystem->GetMousePosition();
	FPoint Point{static_cast<LONG>(MousePosition.X), static_cast<LONG>(MousePosition.Y)};

	SWindow* Target = nullptr;

	if (Capture != nullptr)
	{
		// 드래그 캡처 중이면, 히트 테스트 없이 캡처된 위젯으로 고정
		Target = static_cast<SWindow*>(Capture);
	}
	else
	{
		// 캡처가 아니면 화면 좌표로 히트 테스트
		Target = (Root != nullptr) ? Root->HitTest(Point) : nullptr;
	}

	if (InputSubsystem->IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputSubsystem->IsKeyDown(EKeyInput::MouseLeft)))
	{
		if (Target && Target->OnMouseDown(Point, 0))
		{
			Capture = Target;
		}
	}

	const FVector& Delta = InputSubsystem->GetMouseDelta();
	if ((Delta.X != 0.0f || Delta.Y != 0.0f) && Capture)
	{
		Capture->OnMouseMove(Point);
	}

	if (InputSubsystem->IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Capture)
		{
			Capture->OnMouseUp(Point, 0);
			Capture = nullptr;
		}
	}

	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseLeft) && Capture)
	{
		Capture->OnMouseUp(Point, 0);
		Capture = nullptr;
	}
}

/**
 * @brief 마우스가 컨트롤 중인 Viewport의 인덱스를 반환하는 함수
 * @return 기본적으로는 index를 반환, 아무 것도 만지지 않았다면 -1
 */
int32 UViewportSubsystem::GetViewportIndexUnderMouse() const
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return -1;
	}

	const FVector& MousePosition = InputSubsystem->GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositionY = static_cast<LONG>(MousePosition.Y);

	for (int32 i = 0; i < static_cast<int8>(Viewports.Num()); ++i)
	{
		const FRect& Rect = Viewports[i]->GetRect();
		const int32 ToolbarHeight = Viewports[i]->GetToolbarHeight();

		const LONG RenderAreaTop = Rect.Y + ToolbarHeight;
		const LONG RenderAreaBottom = Rect.Y + Rect.H;

		if (MousePositionX >= Rect.X && MousePositionX < Rect.X + Rect.W &&
			MousePositionY >= RenderAreaTop && MousePositionY < RenderAreaBottom)
		{
			return i;
		}
	}
	return -1;
}

bool UViewportSubsystem::ComputeLocalNDCForViewport(int32 InIdx, float& OutNdcX, float& OutNdcY) const
{
	if (InIdx < 0 || InIdx >= static_cast<int32>(Viewports.Num())) return false;
	const FRect& Rect = Viewports[InIdx]->GetRect();
	if (Rect.W <= 0 || Rect.H <= 0) return false;

	// 툴바 높이를 고려한 실제 렌더링 영역 기준으로 NDC 계산
	const int32 ToolbarHeight = Viewports[InIdx]->GetToolbarHeight();
	const float RenderAreaY = static_cast<float>(Rect.Y + ToolbarHeight);
	const float RenderAreaHeight = static_cast<float>(Rect.H - ToolbarHeight);

	if (RenderAreaHeight <= 0)
	{
		return false;
	}

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return false;
	}

	const FVector& MousePosition = InputSubsystem->GetMousePosition();
	const float LocalX = (MousePosition.X - static_cast<float>(Rect.X));
	const float LocalY = (MousePosition.Y - RenderAreaY);
	const float Width = static_cast<float>(Rect.W);
	const float Height = RenderAreaHeight;

	OutNdcX = (LocalX / Width) * 2.0f - 1.0f;
	OutNdcY = 1.0f - (LocalY / Height) * 2.0f;
	return true;
}

void UViewportSubsystem::SetOrthoGraphicCenter(const FVector& NewCenter)
{
	OrthoGraphicCamerapoint = NewCenter;
}

#pragma region viewport animation

void UViewportSubsystem::StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex)
{
	StartViewportAnimation(bSingleToQuad, ViewportIndex);
}

void UViewportSubsystem::StartViewportAnimation(bool bSingleToQuad, int32 PromoteIdx)
{
	// 현재 애니메이션이 진행 중이면 중단
	if (ViewportAnimation.bIsAnimating)
	{
		RestoreOriginalLayout();
	}

	// 애니메이션 설정 초기화
	ViewportAnimation.bSingleToQuad = bSingleToQuad;
	ViewportAnimation.AnimationTime = 0.0f;
	ViewportAnimation.PromotedViewportIndex = (PromoteIdx >= 0) ? PromoteIdx : ActiveRmbViewportIdx;

	// 현재 스플리터 비율 저장 (기존 비율 유지)
	ViewportAnimation.CurrentVerticalRatio = IniSaveSharedV;
	ViewportAnimation.CurrentHorizontalRatio = IniSaveSharedH;

	// 현재 루트 백업
	ViewportAnimation.BackupRoot = Root;
	Root = nullptr; // 임시로 비우기

	// 애니메이션용 SWindow 트리 생성
	CreateAnimationSplitters();

	// 애니메이션 시작
	ViewportAnimation.bIsAnimating = true;

	// 디버깅 로그
	UE_LOG("ViewportSubsystem: SWindow 애니메이션 시작: %s, Duration: %.2f",
	       bSingleToQuad ? "SingleToQuad" : "QuadToSingle",
	       ViewportAnimation.AnimationDuration);
}

void UViewportSubsystem::UpdateViewportAnimation()
{
	if (!ViewportAnimation.bIsAnimating)
	{
		return;
	}

	// 애니메이션 시간 진행 (DT 매크로 사용)
	ViewportAnimation.AnimationTime += DT;
	float Progress = ViewportAnimation.AnimationTime / ViewportAnimation.AnimationDuration;

	// 애니메이션 완료 체크
	if (Progress >= 1.0f)
	{
		Progress = 1.0f;

		// 마지막 이징 프레임 적용 (깜빡임 방지)
		float FinalEasedProgress = EaseInOutCubic(Progress);
		AnimateSplitterTransition(FinalEasedProgress);

		// 애니메이션 완료 처리 - 백업 루트 복원 없이 직접 전환
		if (!ViewportAnimation.bSingleToQuad) // QuadToSingle
		{
			// 현재 애니메이션 루트를 직접 단일 레이아웃으로 변경
			FinalizeSingleLayoutFromAnimation();
		}
		else // SingleToQuad
		{
			// 현재 애니메이션 루트를 직접 4분할 레이아웃으로 완료
			FinalizeFourSplitLayoutFromAnimation();
		}

		// 정리
		ViewportAnimation.bIsAnimating = false;
		ViewportAnimation.BackupRoot = nullptr; // 백업은 사용안함
		ViewportAnimation.AnimationRoot = nullptr;

		UE_LOG_SUCCESS("ViewportSubsystem: SWindow 애니메이션 완료");
		return;
	}

	// 이징 함수 적용 후 SWindow 전환 애니메이션
	float EasedProgress = EaseInOutCubic(Progress);
	AnimateSplitterTransition(EasedProgress);
}

float UViewportSubsystem::EaseInOutCubic(float t) const
{
	// 부드러운 3차 이징 함수 (가속 → 등속 → 감속)
	if (t < 0.5f)
	{
		return 4.0f * t * t * t;
	}
	else
	{
		float f = (2.0f * t - 2.0f);
		return 1.0f + f * f * f * 0.5f;
	}
}

void UViewportSubsystem::SyncAnimationRectsToViewports() const
{
	TArray<FRect> Leaves;
	GetLeafRects(Leaves);

	if (Leaves.IsEmpty()) return;

	// 애니메이션 유형에 따라 올바른 뷰포트 매핑
	if (!ViewportAnimation.bSingleToQuad) // QuadToSingle
	{
		// Quad → Single: 선택된 뷰포트만 첫 번째 위치에 렌더링
		int32 PromotedIdx = ViewportAnimation.PromotedViewportIndex;
		if (PromotedIdx >= 0 && PromotedIdx < static_cast<int32>(Viewports.Num()) && !Leaves.IsEmpty())
		{
			// 선택된 뷰포트의 데이터를 0번 인덱스에 임시 배치
			// 이렇게 하면 전체 화면에 올바른 뷰포트가 보인다
			FViewport* TargetViewport = Viewports[PromotedIdx];
			FViewportClient* TargetClient = Clients[PromotedIdx];

			// 첫 번째 리프 영역에 선택된 뷰포트 배치 (전체 화면)
			TargetViewport->SetRect(Leaves[0]);
			TargetViewport->SetToolbarHeight(24);

			const int32 renderH = max<LONG>(0, Leaves[0].H - TargetViewport->GetToolbarHeight());
			TargetClient->OnResize({Leaves[0].W, renderH});

			// 다른 뷰포트들은 최소 크기로 숨김 (비가시)
			for (int32 i = 0; i < static_cast<int32>(Viewports.Num()); ++i)
			{
				if (i != PromotedIdx)
				{
					Viewports[i]->SetRect(FRect{0, 0, 0, 0}); // 비가시
					Clients[i]->OnResize({0, 0});
				}
			}
		}
	}
	else // SingleToQuad
	{
		// Single → Quad: 모든 뷰포트를 올바른 영역에 매핑
		// 애니메이션 중에는 원래 배치대로 복원
		const int32 N = min<int32>(static_cast<int32>(Leaves.Num()), static_cast<int32>(Viewports.Num()));

		// 기본 매핑: 원래 뷰포트 순서대로 (0=Top, 1=Front, 2=Side, 3=Persp)
		for (int32 i = 0; i < N; ++i)
		{
			Viewports[i]->SetRect(Leaves[i]);
			Viewports[i]->SetToolbarHeight(24);

			const int32 renderH = max<LONG>(0, Leaves[i].H - Viewports[i]->GetToolbarHeight());
			Clients[i]->OnResize({Leaves[i].W, renderH});
		}
	}
}

void UViewportSubsystem::CreateAnimationSplitters()
{
	// 현재 창 크기 얻기
	int32 Width = 0, Height = 0;
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	const int MenuHeight = static_cast<int>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight());
	const int32 ToolbarH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetToolbarHeight());
	const int32 RightPanelWidth = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;
	const FRect ViewportRect{0, MenuHeight + ToolbarH, max(0, ViewportWidth), max(0, Height - MenuHeight - ToolbarH)};

	if (ViewportAnimation.bSingleToQuad) // SingleToQuad
	{
		// 싱글 → 쿼드: 4분할 구조로 생성하지만 선택된 뷰포트가 전체 차지하도록 설정
		// 4분할 스플리터 구조 생성
		SSplitter* RootSplit = new SSplitterV();
		SSplitter* Left = new SSplitterH();
		SSplitter* Right = new SSplitterH();

		// 현재 선택된 뷰포트(BuildSingleLayout에서 0번으로 스왈됨)가 어떤 위치에 있었는지 결정
		// LastPromotedIdx가 원래 위치를 나타냄
		int32 OriginalViewportIndex = (LastPromotedIdx >= 0) ? LastPromotedIdx : 0;

		// 원래 위치에 따라 시작 비율 설정
		switch (OriginalViewportIndex)
		{
		case 0: // Top (Top-Left): 왼쪽 위쪽에서 시작
			RootSplit->Ratio = 1.0f; // 왼쪽 전체
			Left->Ratio = 1.0f; // Top 전체
			Right->Ratio = 1.0f; // 사용안됨
			break;
		case 1: // Front (Bottom-Left): 왼쪽 아래쪽에서 시작
			RootSplit->Ratio = 1.0f; // 왼쪽 전체
			Left->Ratio = 0.0f; // Front 전체
			Right->Ratio = 1.0f; // 사용안됨
			break;
		case 2: // Side (Top-Right): 오른쪽 위쪽에서 시작
			RootSplit->Ratio = 0.0f; // 오른쪽 전체
			Left->Ratio = 1.0f; // 사용안됨
			Right->Ratio = 1.0f; // Side 전체
			break;
		case 3: // Persp (Bottom-Right): 오른쪽 아래쪽에서 시작
			RootSplit->Ratio = 0.0f; // 오른쪽 전체
			Left->Ratio = 1.0f; // 사용안됨
			Right->Ratio = 0.0f; // Persp 전체
			break;
		default:
			// 기본값 (0번과 동일)
			RootSplit->Ratio = 1.0f;
			Left->Ratio = 1.0f;
			Right->Ratio = 1.0f;
			break;
		}

		RootSplit->SetChildren(Left, Right);

		SWindow* Top = new SWindow();
		SWindow* Front = new SWindow();
		SWindow* Side = new SWindow();
		SWindow* Persp = new SWindow();

		Left->SetChildren(Top, Front);
		Right->SetChildren(Side, Persp);

		RootSplit->OnResize(ViewportRect);
		ViewportAnimation.AnimationRoot = RootSplit;
		ViewportAnimation.StartRatio = 1.0f; // 선택된 뷰포트가 전체 차지
		ViewportAnimation.TargetRatio = ViewportAnimation.CurrentVerticalRatio; // 목표: 기존 비율
	}
	else // QuadToSingle
	{
		// 쿼드 → 싱글: 쿼드 스플리터로 시작해서 점진적으로 싱글로 바꿲
		// 4분할 스플리터 구조 생성
		SSplitter* RootSplit = new SSplitterV();
		RootSplit->Ratio = IniSaveSharedV;

		SSplitter* Left = new SSplitterH();
		SSplitter* Right = new SSplitterH();
		Left->Ratio = IniSaveSharedH;
		Right->Ratio = IniSaveSharedH;

		RootSplit->SetChildren(Left, Right);

		SWindow* Top = new SWindow();
		SWindow* Front = new SWindow();
		SWindow* Side = new SWindow();
		SWindow* Persp = new SWindow();

		Left->SetChildren(Top, Front);
		Right->SetChildren(Side, Persp);

		RootSplit->OnResize(ViewportRect);
		ViewportAnimation.AnimationRoot = RootSplit;
		ViewportAnimation.StartRatio = IniSaveSharedV;
		ViewportAnimation.TargetRatio = 1.0f; // 전체 화면으로 (싱글 상태)
	}

	// 애니메이션 루트를 현재 루트로 설정
	Root = ViewportAnimation.AnimationRoot;
}

void UViewportSubsystem::AnimateSplitterTransition(float Progress)
{
	if (!ViewportAnimation.AnimationRoot) return;

	// 진행률에 따른 스플리터 비율 계산
	float CurrentRatio = ViewportAnimation.StartRatio + (ViewportAnimation.TargetRatio - ViewportAnimation.StartRatio) *
		Progress;

	if (ViewportAnimation.bSingleToQuad) // SingleToQuad
	{
		// 싱글 → 쿼드: 원래 위치에서 기존 비율로 단계적 축소
		if (SSplitter* RootSplitter = Cast(ViewportAnimation.AnimationRoot))
		{
			int32 OriginalViewportIndex = (LastPromotedIdx >= 0) ? LastPromotedIdx : 0;

			// 원래 위치에 따라 애니메이션 다르게 처리
			switch (OriginalViewportIndex)
			{
			case 0: // Top (Top-Left)
				// 수직: 1.0 → 기존비율, 수평: 1.0 → 기존비율
				RootSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentVerticalRatio - 1.0f) * Progress;
				if (SSplitter* LeftSplitter = Cast(RootSplitter->SideLT))
				{
					LeftSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentHorizontalRatio - 1.0f) * Progress;
				}
				break;
			case 1: // Front (Bottom-Left)
				// 수직: 1.0 → 기존비율, 수평: 0.0 → 기존비율
				RootSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentVerticalRatio - 1.0f) * Progress;
				if (SSplitter* LeftSplitter = Cast(RootSplitter->SideLT))
				{
					LeftSplitter->Ratio = 0.0f + (ViewportAnimation.CurrentHorizontalRatio - 0.0f) * Progress;
				}
				break;
			case 2: // Side (Top-Right)
				// 수직: 0.0 → 기존비율, 수평: 1.0 → 기존비율
				RootSplitter->Ratio = 0.0f + (ViewportAnimation.CurrentVerticalRatio - 0.0f) * Progress;
				if (SSplitter* RightSplitter = Cast(RootSplitter->SideRB))
				{
					RightSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentHorizontalRatio - 1.0f) * Progress;
				}
				break;
			case 3: // Persp (Bottom-Right)
				// 수직: 0.0 → 기존비율, 수평: 0.0 → 기존비율
				RootSplitter->Ratio = 0.0f + (ViewportAnimation.CurrentVerticalRatio - 0.0f) * Progress;
				if (SSplitter* RightSplitter = Cast(RootSplitter->SideRB))
				{
					RightSplitter->Ratio = 0.0f + (ViewportAnimation.CurrentHorizontalRatio - 0.0f) * Progress;
				}
				break;
			default:
				// 기본값 (0번과 동일)
				RootSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentVerticalRatio - 1.0f) * Progress;
				if (SSplitter* LeftSplitter = Cast(RootSplitter->SideLT))
				{
					LeftSplitter->Ratio = 1.0f + (ViewportAnimation.CurrentHorizontalRatio - 1.0f) * Progress;
				}
				break;
			}

			RootSplitter->LayoutChildren();
		}
	}
	else // QuadToSingle
	{
		// 쿼드 → 싱글: 선택된 뷰포트 위치에 따라 확장 방향 결정
		if (SSplitter* RootSplitter = Cast(ViewportAnimation.AnimationRoot))
		{
			// 먼저 수직 스플리터(V) 비율 조정
			// 레이아웃: 0=Top(TL), 1=Front(BL), 2=Side(TR), 3=Persp(BR)
			// 선택된 뷰포트가 점진적으로 전체 화면으로 확장
			switch (ViewportAnimation.PromotedViewportIndex)
			{
			case 0: // Top (Top-Left): 왼쪽 영역 확장
			case 1: // Front (Bottom-Left): 왼쪽 영역 확장
				// 시작 0.5에서 1.0으로 확장
				RootSplitter->Ratio = IniSaveSharedV + (1.0f - IniSaveSharedV) * Progress;
				break;
			case 2: // Side (Top-Right): 오른쪽 영역 확장
			case 3: // Persp (Bottom-Right): 오른쪽 영역 확장
				// 시작 0.5에서 0.0으로 축소 (오른쪽이 확장됨)
				RootSplitter->Ratio = IniSaveSharedV * (1.0f - Progress);
				break;
			default:
				break;
			}

			// 수평 스플리터(H)들의 비율도 조정
			if (RootSplitter->SideLT && RootSplitter->SideRB)
			{
				SSplitter* LeftSplitter = Cast(RootSplitter->SideLT);
				SSplitter* RightSplitter = Cast(RootSplitter->SideRB);

				if (LeftSplitter && RightSplitter)
				{
					// 수평 스플리터에서도 선택된 뷰포트가 점진적으로 확장
					switch (ViewportAnimation.PromotedViewportIndex)
					{
					case 0: // Top (Top-Left): 왼쪽 수평 스플리터에서 Top 확장
						// 시작 0.5에서 1.0으로 확장
						LeftSplitter->Ratio = IniSaveSharedH + (1.0f - IniSaveSharedH) * Progress;
						break;
					case 1: // Front (Bottom-Left): 왼쪽 수평 스플리터에서 Front 확장
						// 시작 0.5에서 0.0으로 축소 (Front가 확장됨)
						LeftSplitter->Ratio = IniSaveSharedH * (1.0f - Progress);
						break;
					case 2: // Side (Top-Right): 오른쪽 수평 스플리터에서 Side 확장
						// 시작 0.5에서 1.0으로 확장
						RightSplitter->Ratio = IniSaveSharedH + (1.0f - IniSaveSharedH) * Progress;
						break;
					case 3: // Persp (Bottom-Right): 오른쪽 수평 스플리터에서 Persp 확장
						// 시작 0.5에서 0.0으로 축소 (Persp가 확장됨)
						RightSplitter->Ratio = IniSaveSharedH * (1.0f - Progress);
						break;
					default:
						break;
					}
				}
			}

			RootSplitter->LayoutChildren();
		}
	}
}

void UViewportSubsystem::RestoreOriginalLayout()
{
	if (ViewportAnimation.AnimationRoot)
	{
		delete ViewportAnimation.AnimationRoot;
		ViewportAnimation.AnimationRoot = nullptr;
	}

	if (ViewportAnimation.BackupRoot)
	{
		Root = ViewportAnimation.BackupRoot;
		ViewportAnimation.BackupRoot = nullptr;
	}

	ViewportAnimation.bIsAnimating = false;
}

void UViewportSubsystem::FinalizeSingleLayoutFromAnimation()
{
	// 현재 애니메이션 루트를 삭제하고 단일 레이아웃으로 직접 전환
	if (ViewportAnimation.AnimationRoot)
	{
		delete ViewportAnimation.AnimationRoot;
		ViewportAnimation.AnimationRoot = nullptr;
	}

	if (ViewportAnimation.BackupRoot)
	{
		delete ViewportAnimation.BackupRoot;
		ViewportAnimation.BackupRoot = nullptr;
	}

	// 애니메이션에서 선택된 뷰포트 인덱스 사용 (그대로 유지)
	int32 PromoteIdx = ViewportAnimation.PromotedViewportIndex;

	// 단일 레이아웃 직접 구축 - 선택된 뷰포트만 0번에 나타나게 함
	// 하지만 원래 뷰포트 배열은 그대로 유지해서 나중에 원위치 복원 가능하게
	int32 Width = 0, Height = 0;
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	const int32 MenuH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight());
	const int32 ToolbarH = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetToolbarHeight());
	const int32 RightPanelWidth = static_cast<int32>(GEngine->GetEngineSubsystem<UUISubsystem>()->GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;

	const FRect Rect{0, MenuH + ToolbarH, max(0, ViewportWidth), max(0, Height - MenuH - ToolbarH)};

	// 새 루트 구성 (싱글)
	SWindow* Single = new SWindow();
	Single->OnResize(Rect);
	SetRoot(Single);

	// 선택된 뷰포트를 0번 위치에 임시 오는 대신, 애니메이션 목적에 맞게
	// PromotedViewportIndex만 기록하고 실제 배열 순서는 그대로 유지
	if (PromoteIdx >= 0 && PromoteIdx < static_cast<int32>(Viewports.Num()))
	{
		// 단일 레이아웃에서는 선택된 뷰포트만 보이도록 조정
		// ViewportChange가 Single이므로 RenderOverlay에서 N=1로 처리됨
		// 0번 인덱스에 선택된 뷰포트를 임시 배치해야 데이터 접근에 문제가 없음
		if (PromoteIdx != 0)
		{
			std::swap(Viewports[0], Viewports[PromoteIdx]);
			std::swap(Clients[0], Clients[PromoteIdx]);
			LastPromotedIdx = PromoteIdx; // 나중에 원위치 복원용
		}
		else
		{
			LastPromotedIdx = -1;
		}
	}
	else
	{
		LastPromotedIdx = -1;
	}
}

void UViewportSubsystem::FinalizeFourSplitLayoutFromAnimation()
{
	// 현재 애니메이션 루트의 구조를 그대로 유지하되 비율을 최종값으로 설정
	if (!ViewportAnimation.AnimationRoot) return;

	// 애니메이션 루트를 그대로 사용하되, 최종 비율로 고정
	if (SSplitter* RootSplitter = Cast(ViewportAnimation.AnimationRoot))
	{
		// 최종 비율 적용
		RootSplitter->Ratio = ViewportAnimation.CurrentVerticalRatio;

		if (SSplitter* LeftSplitter = Cast(RootSplitter->SideLT))
		{
			LeftSplitter->Ratio = ViewportAnimation.CurrentHorizontalRatio;
			LeftSplitter->SetSharedRatio(&IniSaveSharedH);
		}

		if (SSplitter* RightSplitter = Cast(RootSplitter->SideRB))
		{
			RightSplitter->Ratio = ViewportAnimation.CurrentHorizontalRatio;
			RightSplitter->SetSharedRatio(&IniSaveSharedH);
		}

		RootSplitter->LayoutChildren();
	}

	// 애니메이션 루트를 영구 루트로 이전 (삭제하지 않음)
	Root = ViewportAnimation.AnimationRoot;
	ViewportAnimation.AnimationRoot = nullptr;

	// 백업 정리
	if (ViewportAnimation.BackupRoot)
	{
		delete ViewportAnimation.BackupRoot;
		ViewportAnimation.BackupRoot = nullptr;
	}

	// 뷰포트 배열 원위치 복원: SingleToQuad 애니메이션 후 뷰포트들이 원래 자리로 돌아가도록
	if (LastPromotedIdx > 0 && LastPromotedIdx < static_cast<int32>(Viewports.Num()))
	{
		std::swap(Viewports[0], Viewports[LastPromotedIdx]);
		std::swap(Clients[0], Clients[LastPromotedIdx]);
	}
	LastPromotedIdx = -1;

	ForceRefreshOrthoViewsAfterLayoutChange();
}

ACameraActor* UViewportSubsystem::GetActiveCameraForViewport(int32 InViewportIndex) const
{
    // Camera deprecated - 더 이상 카메라를 반환하지 않음
    return nullptr;
}

TArray<ACameraActor*> UViewportSubsystem::GetAllCameras() const
{
    // Camera deprecated - 빈 배열 반환
    TArray<ACameraActor*> AllCameras;
    return AllCameras;
}

void UViewportSubsystem::SetViewportViewType(int32 InViewportIndex, EViewType InNewType)
{
    if (InViewportIndex < 0 || InViewportIndex >= Clients.Num())
    {
        return;
    }

    FViewportClient* Client = Clients[InViewportIndex];
    if (Client)
    {
        EViewType OldType = Client->GetViewType();
        Client->SetViewType(InNewType);

        // 뷰 타입 변경 후 즉시 카메라 상태 강제 업데이트
        // Ortho ↔ Perspective 변환 시 화면이 즉시 동일하게 보이도록
        bool bWasOrtho = (OldType >= EViewType::OrthoTop && OldType <= EViewType::OrthoBack);
        bool bIsOrtho = (InNewType >= EViewType::OrthoTop && InNewType <= EViewType::OrthoBack);

        if (bWasOrtho != bIsOrtho)
        {
            // Perspective → Ortho 또는 Ortho → Perspective 변환 시
            if (bIsOrtho)
            {
                // Ortho로 변경: 공유 중심점 및 초기 오프셋 적용
                int32 OrthoIdx = -1;
                switch (InNewType)
                {
                case EViewType::OrthoTop: OrthoIdx = 0; break;
                case EViewType::OrthoBottom: OrthoIdx = 1; break;
                case EViewType::OrthoLeft: OrthoIdx = 2; break;
                case EViewType::OrthoRight: OrthoIdx = 3; break;
                case EViewType::OrthoFront: OrthoIdx = 4; break;
                case EViewType::OrthoBack: OrthoIdx = 5; break;
                default: break;
                }

                if (OrthoIdx >= 0 && OrthoIdx < static_cast<int32>(InitialOffsets.Num()))
                {
                    FVector NewLocation = OrthoGraphicCamerapoint + InitialOffsets[OrthoIdx];
                    Client->SetViewLocation(NewLocation);
                    Client->SetOrthoWidth(SharedFovY);
                }
            }
            // Perspective로 변경시는 현재 위치를 유지
        }
        else if (bIsOrtho)
        {
            // Ortho → Ortho (다른 방향) 변환 시
            int32 OrthoIdx = -1;
            switch (InNewType)
            {
            case EViewType::OrthoTop: OrthoIdx = 0; break;
            case EViewType::OrthoBottom: OrthoIdx = 1; break;
            case EViewType::OrthoLeft: OrthoIdx = 2; break;
            case EViewType::OrthoRight: OrthoIdx = 3; break;
            case EViewType::OrthoFront: OrthoIdx = 4; break;
            case EViewType::OrthoBack: OrthoIdx = 5; break;
            default: break;
            }

            if (OrthoIdx >= 0 && OrthoIdx < static_cast<int32>(InitialOffsets.Num()))
            {
                FVector NewLocation = OrthoGraphicCamerapoint + InitialOffsets[OrthoIdx];
                Client->SetViewLocation(NewLocation);
                Client->SetOrthoWidth(SharedFovY);
            }
        }

        UE_LOG("ViewportSubsystem: Viewport %d type changed to %d", InViewportIndex, (int32)InNewType);
    }
}

#pragma endregion viewport animation
