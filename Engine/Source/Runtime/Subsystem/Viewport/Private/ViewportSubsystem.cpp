#include "pch.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

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
#include "Runtime/Component/Public/CameraComponent.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

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

void UViewportSubsystem::Tick(float DeltaSeconds)
{
	Update();
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

	// 4개의 뷰포트와 클라이언트를 할당받습니다.
	InitializeViewportAndClient();

	// orthographic camera 6개를 초기화합니다.
	InitializeOrthoGraphicCamera();

	// perspective camera 4개를 초기화합니다.
	InitializePerspectiveCamera();

	// 직교 카메라 중점을 업데이트 합니다
	UpdateOrthoGraphicCameraPoint();

	// 기본 직교카메라를 클라이언트에게 셋합니다.
	BindOrthoGraphicCameraToClient();

	if (UConfigSubsystem* ConfigSubsystem = GEngine ? GEngine->GetEngineSubsystem<UConfigSubsystem>() : nullptr)
	{
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
	SSplitter* RootSplit = new SSplitterV();
	RootSplit->Ratio = IniSaveSharedV;

	//SharedY = 0.5f;
	SSplitter* Left = new SSplitterH();
	SSplitter* Right = new SSplitterH();

	Left->Ratio = IniSaveSharedH;
	Right->Ratio = IniSaveSharedH;

	Left->SetSharedRatio(&IniSaveSharedH);
	Right->SetSharedRatio(&IniSaveSharedH);

	RootSplit->SetChildren(Left, Right);

	SWindow* Top = new SWindow();
	SWindow* Front = new SWindow();
	SWindow* Side = new SWindow();
	SWindow* Persp = new SWindow();

	// 0,1
	Left->SetChildren(Top, Front);

	// 2,3
	Right->SetChildren(Side, Persp);

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
		if (MousePositionX >= Rect.X && MousePositionX < Rect.X + Rect.W &&
			MousePositioinY >= Rect.Y && MousePositioinY < Rect.Y + Rect.H)
		{
			ActiveRmbViewportIdx = i;
			//UE_LOG("%d", i);
			break;
		}
	}
}

void UViewportSubsystem::TickCameras() const
{
	// 우클릭이 눌린 상태라면 해당 뷰포트의 카메라만 입력 반영(Update)
	// 그렇지 않다면(비상호작용) 카메라 이동 입력은 생략하여 타 뷰포트에 영향이 가지 않도록 함
	const int32 N = static_cast<int32>(Clients.Num());
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	const bool bRmbDown = InputSubsystem && InputSubsystem->IsKeyDown(EKeyInput::MouseRight);
	if (bRmbDown)
	{
		if (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < N)
		{
			Clients[ActiveRmbViewportIdx]->Tick(DT);
		}
	}
}

void UViewportSubsystem::Update()
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
			if (SceneWidget->IsAnyCameraAnimating())
			{
				SceneWidget->StopAllCameraAnimations();
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
					if (Client->GetOrthoCamera())
					{
						constexpr float DollyStep = 5.0f; // 튤닝 가능
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
			}
		}
	}

	UpdateOrthoGraphicCameraPoint();

	UpdateOrthoGraphicCameraFov();

	// FovY 업데이트 후 모든 직교 카메라의 행렬 갱신
	for (ACameraActor* Camera : OrthoGraphicCameras)
	{
		if (Camera && Camera->GetCameraComponent())
		{
			Camera->GetCameraComponent()->UpdateMatrixByOrth();
		}
	}

	//ApplySharedOrthoCenterToAllCameras();

	// 3) 카메라 업데이트 (공유 오쏘 1회, 퍼스펙티브는 각자)
	TickCameras();

	// 4) 드로우(3D) — 실제 렌더러 루프에서 Viewport 적용 후 호출해도 됨
	//    여기서는 뷰/클라 페어 순회만 보여줌. (RS 바인딩은 네 렌더러 Update에서 수행 중)
	const int32 N = static_cast<int32>(Clients.Num());
	for (int32 i = 0; i < N; ++i)
	{
		Clients[i]->Draw(Viewports[i]);
	}
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


	for (ACameraActor* Cam : OrthoGraphicCameras)
	{
		delete Cam;
	}


	for (ACameraActor* Cam : PerspectiveCameras)
	{
		delete Cam;
	}

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
		FViewport* Viewport = new FViewport();
		FViewportClient* ViewportClient = new FViewportClient();

		Viewport->SetViewportClient(ViewportClient);

		Clients.Add(ViewportClient);
		Viewports.Add(Viewport);
	}

	Clients[0]->SetViewType(EViewType::Perspective);
	Clients[0]->SetViewMode(EViewMode::Unlit);

	Clients[1]->SetViewType(EViewType::OrthoLeft);
	Clients[1]->SetViewMode(EViewMode::WireFrame);

	Clients[2]->SetViewType(EViewType::OrthoTop);
	Clients[2]->SetViewMode(EViewMode::WireFrame);

	Clients[3]->SetViewType(EViewType::OrthoRight);
	Clients[3]->SetViewMode(EViewMode::WireFrame);
}

void UViewportSubsystem::InitializeOrthoGraphicCamera()
{
	for (int i = 0; i < 6; ++i)
	{
		ACameraActor* Camera = NewObject<ACameraActor>();
		Camera->SetDisplayName("Camera_" + to_string(ACameraActor::GetNextGenNumber()));
		OrthoGraphicCameras.Add(Camera);
	}

	// TODO(KHJ): 좌표축 이슈 처리 시 해결해야 함
	// 상단
	OrthoGraphicCameras[0]->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
	OrthoGraphicCameras[0]->SetActorRotation(FVector(0.0f, 90.0f, 180.0f));
	OrthoGraphicCameras[0]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[0]->GetCameraComponent()->SetFarZ(150.0f);

	// 하단
	OrthoGraphicCameras[1]->SetActorLocation(FVector(0.0f, 0.0f, -100.0f));
	OrthoGraphicCameras[1]->SetActorRotation(FVector(0.0f, -90.0f, 0.0f));
	OrthoGraphicCameras[1]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[1]->GetCameraComponent()->SetFarZ(150.0f);

	// 왼쪽
	OrthoGraphicCameras[2]->SetActorLocation(FVector(0.0f, 100.0f, 0.0f));
	OrthoGraphicCameras[2]->SetActorRotation(FVector(0.0f, 0.0f, -90.0f));
	OrthoGraphicCameras[2]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[2]->GetCameraComponent()->SetFarZ(150.0f);

	// 오른쪽
	OrthoGraphicCameras[3]->SetActorLocation(FVector(0.0f, -100.0f, 0.0f));
	OrthoGraphicCameras[3]->SetActorRotation(FVector(0.0f, 0.0f, 90.0f));
	OrthoGraphicCameras[3]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[3]->GetCameraComponent()->SetFarZ(150.0f);

	// 정면
	OrthoGraphicCameras[4]->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[4]->SetActorRotation(FVector(180.0f, 180.0f, 0.0f));
	OrthoGraphicCameras[4]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[4]->GetCameraComponent()->SetFarZ(150.0f);

	// 후면
	OrthoGraphicCameras[5]->SetActorLocation(FVector(-100.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[5]->SetActorRotation(FVector(0.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[5]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[5]->GetCameraComponent()->SetFarZ(150.0f);

	InitialOffsets.Emplace(0.0f, 0.0f, 100.0f);
	InitialOffsets.Emplace(0.0f, 0.0f, -100.0f);
	InitialOffsets.Emplace(0.0f, 100.0f, 0.0f);
	InitialOffsets.Emplace(0.0f, -100.0f, 0.0f);
	InitialOffsets.Emplace(100.0f, 0.0f, 0.0f);
	InitialOffsets.Emplace(-100.0f, 0.0f, 0.0f);
}

void UViewportSubsystem::InitializePerspectiveCamera()
{
	for (int i = 0; i < 4; ++i)
	{
		ACameraActor* Camera = NewObject<ACameraActor>();
		Camera->SetDisplayName("Camera_" + to_string(ACameraActor::GetNextGenNumber()));

		// 극적인 앵글로 초기 위치 설정
		Camera->SetActorLocation(FVector(-25.0f, -20.0f, 18.0f));
		Camera->SetActorRotation(FVector(0, 45.0f, 35.0f));

		PerspectiveCameras.Add(Camera);
		Clients[i]->SetPerspectiveCamera(Camera);
	}
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
	if (Index < 0 || Index >= static_cast<int32>(Clients.Num()))
	{
		return;
	}

	// n번째 뷰포트 클라이언트를 가져옵니다.
	FViewportClient* ViewportClient = Clients[Index];

	// 클라이언트가 원근투영 상태라면 리턴합니다.
	if (!ViewportClient || !ViewportClient->IsOrtho()) return;

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

	ACameraActor* OrthoCam = ViewportClient->GetOrthoCamera();
	const EViewType ViewType = ViewportClient->GetViewType();

	FVector OrthoCameraFoward;
	switch (ViewType)
	{
	case EViewType::OrthoTop:
		OrthoCameraFoward = FVector(0, 0, -1);
		break;
	case EViewType::OrthoBottom:
		OrthoCameraFoward = FVector(0, 0, 1);
		break;
	case EViewType::OrthoFront:
		OrthoCameraFoward = FVector(-1, 0, 0);
		break;
	case EViewType::OrthoBack:
		OrthoCameraFoward = FVector(1, 0, 0);
		break;
	case EViewType::OrthoRight:
		OrthoCameraFoward = FVector(0, 1, 0);
		break;
	case EViewType::OrthoLeft:
		OrthoCameraFoward = FVector(0, -1, 0);
		break;
	default: return; // 퍼스펙티브면 여기선 스킵
	}


	FVector UpRef(0, 0, 1);
	// 직교 탑 타입이거나 바텀 타입이면
	if (ViewType == EViewType::OrthoTop || ViewType == EViewType::OrthoBottom)
	{
		UpRef = {-1, 0, 0};
	}

	FVector Right = OrthoCameraFoward.Cross(UpRef);
	Right.Normalize();
	FVector Up = Right.Cross(OrthoCameraFoward);

	// 마우스 델타(px) → NDC 델타 → 월드 델타
	const FVector& Delta = InputSubsystem->GetMouseDelta();
	if (Delta.X == 0.0f && Delta.Y == 0.0f) return;

	const float Aspect = (Height > 0.f) ? (Width / Height) : 1.0f;
	const float Rad = FVector::GetDegreeToRadian(OrthoCam->GetCameraComponent()->GetFovY());
	const float OrthoWidth = 2.0f * std::tanf(Rad * 0.5f);
	const float OrthoHeight = (Aspect > 0.0f) ? (OrthoWidth / Aspect) : OrthoWidth;

	// 화면 기준 드래그 비율(NDC 스케일) 기본 부호
	float sX = -1.0f;
	float sY = 1.0f;

	// NDC 델타 (마우스 픽셀 → -1..+1)
	const float NdcDX = sX * (Delta.X / Width) * 2.0f;
	const float NdcDY = sY * (Delta.Y / Height) * 2.0f;
	const FVector WorldDelta = Right * (NdcDX * (OrthoWidth * 0.5f)) + Up * (NdcDY * (OrthoHeight * 0.5f));

	// 공유 중심 누적 이동
	OrthoGraphicCamerapoint += WorldDelta;

	for (int32 i = 0; i < static_cast<int32>(OrthoGraphicCameras.Num()); ++i)
	{
		if (ACameraActor* Cam = OrthoGraphicCameras[i])
		{
			Cam->SetActorLocation(OrthoGraphicCamerapoint + InitialOffsets[i]);
		}
	}
}

void UViewportSubsystem::UpdateOrthoGraphicCameraFov() const
{
	for (int i = 0; i < 6; ++i)
	{
		OrthoGraphicCameras[i]->GetCameraComponent()->SetFovY(SharedFovY);
	}
}

void UViewportSubsystem::BindOrthoGraphicCameraToClient() const
{
	Clients[1]->SetViewType(EViewType::OrthoLeft);
	Clients[1]->SetOrthoCamera(OrthoGraphicCameras[2]);

	Clients[2]->SetViewType(EViewType::OrthoTop);
	Clients[2]->SetOrthoCamera(OrthoGraphicCameras[0]);

	Clients[3]->SetViewType(EViewType::OrthoRight);
	Clients[3]->SetOrthoCamera(OrthoGraphicCameras[3]);
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
			// 퍼스펙티브는 자신 카메라 보장
			if (i < static_cast<int32>(PerspectiveCameras.Num()) && PerspectiveCameras[i])
			{
				Client->SetPerspectiveCamera(PerspectiveCameras[i]);
				PerspectiveCameras[i]->GetCameraComponent()->SetCameraType(ECameraType::ECT_Perspective);
			}
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

			if (OrthoIdx >= 0 && OrthoIdx < static_cast<int>(OrthoGraphicCameras.Num()))
			{
				ACameraActor* Camera = OrthoGraphicCameras[OrthoIdx];
				Client->SetOrthoCamera(Camera);
			}
		}

		// 크기 반영(툴바 높이 포함) — 깜빡임/툴바 좌표 안정화
		const FRect& Rect = Viewports[i]->GetRect();
		Viewports[i]->SetToolbarHeight(28);
		Client->OnResize({Rect.W, Rect.H});
	}

	// 3) 오쏘 6방향 카메라를 공유 중심점에 맞춰 정렬만 수행(이동 입력 없이)
	auto Reposition = [&](int InCamIndex, const FVector& InCamFwd)
	{
		if (InCamIndex < 0 || InCamIndex >= static_cast<int>(OrthoGraphicCameras.Num())) return;
		if (ACameraActor* Camera = OrthoGraphicCameras[InCamIndex])
		{
			const FVector Loc = Camera->GetActorLocation();
			const FVector ToC = Loc - OrthoGraphicCamerapoint;
			const float Dist = std::fabs(ToC.Dot(InCamFwd));
			Camera->SetActorLocation(OrthoGraphicCamerapoint - InCamFwd * Dist);
		}
	};

	Reposition(0, FVector(0, 0, -1)); // Top
	Reposition(1, FVector(0, 0, 1)); // Bottom
	Reposition(2, FVector(0, -1, 0)); // Left
	Reposition(3, FVector(0, 1, 0)); // Right
	Reposition(4, FVector(-1, 0, 0)); // Front
	Reposition(5, FVector(1, 0, 0)); // Back

	// 4) 드래그 대상 초기화
	ActiveRmbViewportIdx = -1;
}

void UViewportSubsystem::ApplySharedOrthoCenterToAllCameras() const
{
	// OrthoGraphicCamerapoint 기준으로 6개 오쏘 카메라 위치 갱신
	for (ACameraActor* Camera : OrthoGraphicCameras)
	{
		if (!Camera) continue;

		// 카메라 '전방'을 "센터 - 카메라 위치"로 잡으면, Top/Right/... 어떤 회전이라도 일관되게 작동
		FVector Forward = (OrthoGraphicCamerapoint - Camera->GetActorLocation()).Normalized();

		// 특이 케이스 보호: fwd가 0이면 월드 Z를 피벗으로 대체
		if (Forward.IsNearlyZero())
		{
			Forward = FVector(0, 0, -1); // 아무거나 기본값
		}

		// 현재 센터까지의 전방 거리 유지
		const FVector toC = Camera->GetActorLocation() - OrthoGraphicCamerapoint;
		const float Dist = std::fabs(toC.Dot(Forward));

		Camera->SetActorLocation(OrthoGraphicCamerapoint - Forward * Dist);
	}
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

#pragma endregion viewport animation
