#include "pch.h"

#include "Manager/Viewport/Public/ViewportManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Core/Public/AppWindow.h"
#include "Window/Public/Window.h"
#include "Window/Public/Splitter.h"
#include "Window/Public/SplitterV.h"
#include "Window/Public/SplitterH.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UViewportManager)

UViewportManager::UViewportManager() = default;
UViewportManager::~UViewportManager() = default;

void UViewportManager::Initialize(FAppWindow* InWindow)
{
    int32 Width = 0, Height = 0;
	if (InWindow)
	{
		InWindow->GetClientSize(Width, Height);
	}

	OrthographicCamera = new UCamera;
	OrthographicCamera->SetCameraType(ECameraType::ECT_Orthographic);

    // Start with a single full-viewport window
    SWindow* Single = new SWindow();
    Single->OnResize({ 0, 0, Width, Height });
    SetRoot(Single);
    bFourSplitActive = false;

	// 싱글 모드용 Viewport/Client 1쌍
	CreateViewportsAndClients(/*InCount=*/1);

	LastTime = UTimeManager::GetInstance().GetGameTime(); // 있다면
}

void UViewportManager::SetRoot(SWindow* InRoot)
{
	Root = InRoot;
}

SWindow* UViewportManager::GetRoot()
{
	return Root;
}

void UViewportManager::BuildSingleLayout()
{
	if (!Root) return;
	const FRect Rect = Root->GetRect();

	// 새 루트 구성
	SWindow* Single = new SWindow();
	Single->OnResize(Rect);
	SetRoot(Single);
	bFourSplitActive = false;

	// 뷰/클라 재구성 (1쌍)
	CreateViewportsAndClients(1);
}

void UViewportManager::BuildFourSplitLayout()
{
	if (!Root) return;
	const FRect Rect = Root->GetRect();

	// 4-way splitter tree
	SSplitter* RootSplit = new SSplitterV(); RootSplit->Ratio = 0.5f;

	static float SharedY = 0.5f;
	SSplitter* Left = new SSplitterH(); Left->SetSharedRatio(&SharedY);  Left->Ratio = SharedY;
	SSplitter* Right = new SSplitterH(); Right->SetSharedRatio(&SharedY); Right->Ratio = SharedY;

	RootSplit->SetChildren(Left, Right);

	SWindow* Top = new SWindow();
	SWindow* Front = new SWindow();
	SWindow* Side = new SWindow();
	SWindow* Persp = new SWindow();

	Left->SetChildren(Top, Front);
	Right->SetChildren(Side, Persp);

	RootSplit->OnResize(Rect);
	SetRoot(RootSplit);
	bFourSplitActive = true;

	// 뷰/클라 재구성 (4쌍)
	CreateViewportsAndClients(4);

	// 뷰 타입 지정(Top/Front/Right/Persp)
	if (Clients.size() == 4)
	{
		Clients[0]->SetViewType(EViewType::OrthoTop);
		Clients[1]->SetViewType(EViewType::Perspective);
		Clients[2]->SetViewType(EViewType::OrthoFront);
		Clients[3]->SetViewType(EViewType::OrthoRight);
	}
}

void UViewportManager::GetLeafRects(TArray<FRect>& OutRects)
{
	OutRects.clear();
	if (!Root) return;

	// 재귀 수집
	struct Local
	{
		static void Collect(SWindow* Node, TArray<FRect>& Out)
		{
			if (!Node) return;
			if (auto* Split = Cast(Node))
			{
				Collect(Split->SideLT, Out);
				Collect(Split->SideRB, Out);
			}
			else
			{
				Out.emplace_back(Node->GetRect());
			}
		}
	};
	Local::Collect(Root, OutRects);
}

void UViewportManager::CreateViewportsAndClients(int32 InCount)
{
	// 1) 기존 자원 정리 (매니저가 소유하므로 직접 delete)
	for (FViewport* Viewport : Viewports) { delete Viewport; }
	for (FViewportClient* ViewportClient : Clients) { delete ViewportClient; }
	Viewports.clear();
	Clients.clear();

	Viewports.reserve(InCount);
	Clients.reserve(InCount);

	// 2) 새로 생성
	for (int32 i = 0; i < InCount; ++i)
	{
		FViewport* VP = new FViewport();
		FViewportClient* CL = new FViewportClient();

		// 오쏘 공유 카메라(읽기 전용) 주입
		CL->SetSharedOrthoCamera(OrthographicCamera);

		// 뷰 ↔ 클라 연결
		VP->SetViewportClient(CL);

		Viewports.emplace_back(VP);
		Clients.emplace_back(CL);
	}

	// 3) 현재 리프 Rect를 뽑아 뷰포트에 반영
	SyncRectsToViewports();
}

void UViewportManager::SyncRectsToViewports()
{
	TArray<FRect> Leaves;
	GetLeafRects(Leaves);

	// 싱글 모드: 리프가 1개, 4분할: 리프가 4개
	const int32 N = min<int32>((int32)Leaves.size(), (int32)Viewports.size());
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->SetRect(Leaves[i]);
		Clients[i]->OnResize({ Leaves[i].W, Leaves[i].H });
	}
}

void UViewportManager::PumpAllViewportInput()
{
	// 각 뷰포트가 스스로 InputManager에서 읽어
	// 로컬 좌표로 변환 → 각자의 Client로 MouseMove/CapturedMouseMove 전달
	const int32 N = (int32)Viewports.size();
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->PumpMouseFromInputManager();
	}
}

void UViewportManager::TickCameras(float DeltaSeconds)
{
	// 1) 공유 오쏘 카메라: 프레임당 1회만 Update (입력은 클라가 했더라도 실제 상태 변경은 카메라 내부에서 처리)
	if (OrthographicCamera)
	{
		OrthographicCamera->SetCameraType(ECameraType::ECT_Orthographic);
		OrthographicCamera->Update();
	}

	// 2) 각 클라이언트 Tick (퍼스펙티브는 각자 Update, 오쏘는 Tick 내부에서 Update 안 하는 게 이상적)
	const int32 N = (int32)Clients.size();
	for (int32 i = 0; i < N; ++i)
	{
		Clients[i]->Tick(DeltaSeconds);
	}
}

void UViewportManager::Update()
{
	if (!Root) return;

	// Δt
	const double Now = UTimeManager::GetInstance().GetGameTime(); // 있으면
	const float  DeltaTime = (float)max(0.0, Now - LastTime);
	LastTime = Now;

	// 1) 리프 Rect 변화 동기화(스플리터 드래그 등)
	SyncRectsToViewports();

	// 2) 입력 라우팅 (각 뷰포트가 InputManager에서 읽어 로컬 좌표로 변환/전달)
	PumpAllViewportInput();

	// 3) 카메라 업데이트 (공유 오쏘 1회, 퍼스펙티브는 각자)
	TickCameras(DeltaTime);

	// 4) 드로우(3D) — 실제 렌더러 루프에서 Viewport 적용 후 호출해도 됨
	//    여기서는 뷰/클라 페어 순회만 보여줌. (RS 바인딩은 네 렌더러 Update에서 수행 중)
	const int32 N = (int32)Clients.size();
	for (int32 i = 0; i < N; ++i)
	{
		Clients[i]->Draw(Viewports[i]);
	}
}

void UViewportManager::RenderOverlay()
{
	if (!Root)
	{
		return;
	}
	Root->OnPaint();
}

//void UViewportManager::TickInput()
//{
//    if (!Root)
//    {
//        return;
//    }
//
//    auto& InputManager = UInputManager::GetInstance();
//    const FVector& MousePosition = InputManager.GetMousePosition();
//    FPoint P{ LONG(MousePosition.X), LONG(MousePosition.Y) };
//
//    SWindow* Target = Capture ? static_cast<SWindow*>(Capture) : Root->HitTest(P);
//
//	if (!Capture)
//	{
//		if (auto* S = Cast(Target))
//		{
//			if (S->IsHandleHover(P)) {}
//				//UE_LOG("hover splitter");
//		}
//	}
//
//    if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
//    {
//        if (Target && Target->OnMouseDown(P, 0))
//        {
//			UE_LOG("OnMouseDown act");
//            Capture = Target;
//        }
//    }
//
//    // Mouse move while captured (always forward to allow cross-drag detection)
//    if (Capture)
//    {
//        Capture->OnMouseMove(P);
//    }
//
//    if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
//    {
//        if (Capture)
//        {
//            Capture->OnMouseUp(P, 0);
//            Capture = nullptr;
//        }
//    }
//
//    if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
//    {
//        Capture->OnMouseUp(P, /*Button*/0);
//        Capture = nullptr;
//    }
//}



//namespace {
//    void CollectLeavesRecursive(SWindow* Node, TArray<FRect>& Out)
//    {
//        if (!Node) return;
//        if (auto* Split = Cast(Node))
//        {
//            CollectLeavesRecursive(Split->SideLT, Out);
//            CollectLeavesRecursive(Split->SideRB, Out);
//        }
//        else
//        {
//            Out.emplace_back(Node->GetRect());
//        }
//    }
//}

//void UViewportManager::GetLeafRects(TArray<FRect>& OutRects)
//{
//    OutRects.clear();
//    if (!Root) return;
//    CollectLeavesRecursive(Root, OutRects);
//}

//void UViewportManager::BuildFourSplitLayout()
//{
//    if (!Root)
//        return;
//
//    const FRect rect = Root->GetRect();
//    Capture = nullptr;
//
//    // Build 4-way splitter tree
//    SSplitter* RootSplit = new SSplitterV();
//    RootSplit->Ratio = 0.5f;
//
//    static float SharedY = 0.5f;
//
//    SSplitter* Left = new SSplitterH(); Left->Ratio = 0.5f;
//    Left->SetSharedRatio(&SharedY);
//    Left->Ratio = SharedY;
//
//    SSplitter* Right = new SSplitterH(); Right->Ratio = 0.5f;
//    Right->SetSharedRatio(&SharedY);
//    Right->Ratio = SharedY;
//
//    RootSplit->SetChildren(Left, Right);
//
//    SWindow* Top = new SWindow();
//    SWindow* Front = new SWindow();
//    SWindow* Side = new SWindow();
//    SWindow* Persp = new SWindow();
//    Left->SetChildren(Top, Front);
//    Right->SetChildren(Side, Persp);
//
//    RootSplit->OnResize(rect);
//    SetRoot(RootSplit);
//    bFourSplitActive = true;
//}
