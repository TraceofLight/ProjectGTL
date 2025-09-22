#include "pch.h"

#include "Manager/Viewport/Public/ViewportManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Runtime/Core/Public/AppWindow.h"
#include "Window/Public/Window.h"
#include "Window/Public/Splitter.h"
#include "Window/Public/SplitterV.h"
#include "Window/Public/SplitterH.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
// Main menu height for top margin
#include "Manager/UI/Public/UIManager.h"
//#include "Manager/Time/Public/TimeManager.h"

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
	AppWindow = InWindow;

	OrthographicCamera = new UCamera;
	OrthographicCamera->SetCameraType(ECameraType::ECT_Orthographic);
	OrthographicCamera->SetFarZ(10000.0f);

	// Start with a single window, initial rect (menu height applied on first Update)
	SWindow* Single = new SWindow();
	Single->OnResize({0, 0, Width, Height});
	SetRoot(Single);
	bFourSplitActive = false;

	// 싱글 모드용 Viewport/Client 1쌍
	CreateViewportsAndClients(1);

	//LastTime = UTimeManager::GetInstance().GetGameTime(); // 있다면
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
	int32 w = 0, h = 0;
	if (AppWindow) AppWindow->GetClientSize(w, h);
	const int menuH = (int)UUIManager::GetInstance().GetMainMenuBarHeight();
	const FRect Rect{0, menuH, w, max(0, h - menuH)};

	// 기존 캡처 해제 (안전)
	Capture = nullptr;

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
	int32 w = 0, h = 0;
	if (AppWindow) AppWindow->GetClientSize(w, h);
	const int menuH = (int)UUIManager::GetInstance().GetMainMenuBarHeight();
	const FRect Rect{0, menuH, w, max(0, h - menuH)};

	// 기존 캡처 해제 (안전)
	Capture = nullptr;

	// 4-way splitter tree
	SSplitter* RootSplit = new SSplitterV();
	RootSplit->Ratio = 0.5f;

	static float SharedY = 0.5f;
	SSplitter* Left = new SSplitterH();
	Left->SetSharedRatio(&SharedY);
	Left->Ratio = SharedY;
	SSplitter* Right = new SSplitterH();
	Right->SetSharedRatio(&SharedY);
	Right->Ratio = SharedY;

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

	// 초기 4분할 뷰타입 배치: Top / Front / Right / Perspective
	if (Clients.size() == 4)
	{
		Clients[0]->SetViewType(EViewType::Perspective);
		Clients[1]->SetViewMode(EViewMode::Unlit);

		Clients[1]->SetViewType(EViewType::OrthoRight);
		Clients[1]->SetViewMode(EViewMode::WireFrame);

		Clients[2]->SetViewType(EViewType::OrthoTop);
		Clients[2]->SetViewMode(EViewMode::WireFrame);

		Clients[3]->SetViewType(EViewType::OrthoLeft);
		Clients[3]->SetViewMode(EViewMode::WireFrame);

		// 오쏘 3개는 공유 상태(센터/줌)에 맞춰 즉시 정렬
		for (int idx : {1, 2, 3})
		{
			if (UCamera* O = Clients[idx]->GetOrthoCamera())
			{
				Clients[idx]->ApplyOrthoBasisForViewType(*O);
				O->SetCameraType(ECameraType::ECT_Orthographic);
				if (!bOrthoSharedInit)
				{
					OrthoSharedCenter = O->GetLocation();
					OrthoSharedFovY = O->GetFovY();
					bOrthoSharedInit = true;
				}
				O->SetFovY(OrthoSharedFovY);
				O->SetLocation(OrthoSharedCenter);
				O->UpdateMatrixByOrth();
				O->Update();
			}
		}
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
	// 0) 이전 첫 번째 카메라 상태 백업 (레이아웃 전환 시 1번 뷰 카메라를 유지하기 위함)
	bool bHadPrev = !Clients.empty();
	UCamera PrevPerspCopy;
	UCamera PrevOrthoCopy;
	if (bHadPrev)
	{
		FViewportClient* PrevC = Clients[0];
		if (PrevC)
		{
			if (auto* P = PrevC->GetPerspectiveCamera()) { PrevPerspCopy = *P; }
			if (auto* O = PrevC->GetOrthoCamera()) { PrevOrthoCopy = *O; }
		}
	}

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

		// 각 클라이언트별 카메라 구성 (오쏘/퍼스 각각 전용)
		{
			TObjectPtr<UCamera> OrthoCam = NewObject<UCamera>();
			OrthoCam->SetDisplayName("Camera" + to_string(UCamera::GetNextGenNumber()));
			OrthoCam->SetCameraType(ECameraType::ECT_Orthographic);
			// Initialize with shared defaults; will be synced below
			OrthoCam->SetFovY(OrthoSharedFovY);
			OrthoCam->SetLocation(OrthoSharedCenter);
			CL->SetOrthoCamera(OrthoCam);

			TObjectPtr<UCamera> PerspCam = NewObject<UCamera>();
			PerspCam->SetDisplayName("Camera" + to_string(UCamera::GetNextGenNumber()));
			PerspCam->SetCameraType(ECameraType::ECT_Perspective);
			CL->SetPerspectiveCamera(PerspCam);
		}

		// 뷰 ↔ 클라 연결
		VP->SetViewportClient(CL);

		Viewports.emplace_back(VP);
		Clients.emplace_back(CL);
	}

	// 2.5) 이전 1번 뷰 카메라 상태를 신규 1번에 반영 (요구사항: 단일/4분할 전환해도 1번 카메라 유지)
	if (InCount >= 1 && bHadPrev)
	{
		FViewportClient* NewC = Clients[0];
		if (NewC)
		{
			if (auto* NewP = NewC->GetPerspectiveCamera()) { *NewP = PrevPerspCopy; }
			if (auto* NewO = NewC->GetOrthoCamera()) { *NewO = PrevOrthoCopy; }
		}
	}

	// Initialize shared ortho state from first client if not set
	if (!bOrthoSharedInit && !Clients.empty())
	{
		if (UCamera* O0 = Clients[0]->GetOrthoCamera())
		{
			OrthoSharedCenter = O0->GetLocation();
			OrthoSharedFovY = O0->GetFovY();
			bOrthoSharedInit = true;
		}
	}

	// Apply shared state to all ortho cams
	for (auto* C : Clients)
	{
		if (!C) continue;
		if (UCamera* O = C->GetOrthoCamera())
		{
			C->ApplyOrthoBasisForViewType(*O);
			O->SetCameraType(ECameraType::ECT_Orthographic);
			O->SetFovY(OrthoSharedFovY);
			O->SetLocation(OrthoSharedCenter);
			O->UpdateMatrixByOrth();
		}
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
		Clients[i]->OnResize({Leaves[i].W, Leaves[i].H});
		// Toolbar 높이 설정 (3D 렌더 offset용)
		Viewports[i]->SetToolbarHeight(24);
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

void UViewportManager::UpdateActiveRmbViewportIndex()
{
	ActiveRmbViewportIdx = -1;
	const auto& Input = UInputManager::GetInstance();
	if (!Input.IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	const FVector& mp = Input.GetMousePosition();
	const LONG mx = (LONG)mp.X;
	const LONG my = (LONG)mp.Y;

	const int32 N = (int32)Viewports.size();
	for (int32 i = 0; i < N; ++i)
	{
		const FRect& r = Viewports[i]->GetRect();
		if (mx >= r.X && mx < r.X + r.W && my >= r.Y && my < r.Y + r.H)
		{
			ActiveRmbViewportIdx = i;
			break;
		}
	}
}

void UViewportManager::TickCameras(float DeltaSeconds)
{
	// 우클릭이 눌린 상태라면 해당 뷰포트의 카메라만 입력 반영(Update)
	// 그렇지 않다면(비상호작용) 카메라 이동 입력은 생략하여 타 뷰포트에 영향이 가지 않도록 함
	const int32 N = (int32)Clients.size();
	const bool bRmbDown = UInputManager::GetInstance().IsKeyDown(EKeyInput::MouseRight);
	if (bRmbDown)
	{
		if (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < N)
		{
			Clients[ActiveRmbViewportIdx]->Tick(DeltaSeconds);

			// If current view is orthographic, update shared center/zoom and sync others
			if (Clients[ActiveRmbViewportIdx]->IsOrtho())
			{
				if (UCamera* O = Clients[ActiveRmbViewportIdx]->GetOrthoCamera())
				{
					OrthoSharedCenter = O->GetLocation();
					OrthoSharedFovY = O->GetFovY();
					bOrthoSharedInit = true;
					SyncOrthographicSharedState(ActiveRmbViewportIdx);
				}
			}
		}
	}
	else
	{
		// 우클릭이 아닐 때는 입력 기반 이동이 없어도 무방하므로, 필요한 경우에만 Tick을 호출할 수 있음
		// 여기서는 보수적으로 아무 것도 하지 않음 (Draw에서 행렬 갱신 수행)
	}


	if (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < N)
	{
		Clients[ActiveRmbViewportIdx]->Tick(DeltaSeconds);

		// If current view is orthographic, update shared center/zoom and sync others
		if (Clients[ActiveRmbViewportIdx]->IsOrtho())
		{
			if (UCamera* O = Clients[ActiveRmbViewportIdx]->GetOrthoCamera())
			{
				OrthoSharedCenter = O->GetLocation();
				OrthoSharedFovY = O->GetFovY();
				bOrthoSharedInit = true;
				SyncOrthographicSharedState(ActiveRmbViewportIdx);
			}
		}
	}
}

void UViewportManager::Update()
{
	if (!Root) return;

	// Δt
	//const double Now = UTimeManager::GetInstance().GetGameTime(); // 있으면
	//const float  DeltaTime = (float)max(0.0, Now - LastTime);
	// LastTime = Now;

	// -1) 매 프레임 루트 레이아웃을 메인 메뉴바 높이를 반영하여 갱신
	{
		int32 w = 0, h = 0;
		if (AppWindow) AppWindow->GetClientSize(w, h);
		//UE_LOG("%d   %d", w, h);

		const int menuH = (int)UUIManager::GetInstance().GetMainMenuBarHeight();
		if (w > 0 && h > 0)
		{
			Root->OnResize(FRect{0, menuH, w, max(0, h - menuH)});
		}
	}


	// 0) 스플리터 등 윈도우 트리 입력 처리 (캡처/드래그 우선)
	TickInput();

	// 1) 리프 Rect 변화 동기화(스플리터 드래그 등)
	SyncRectsToViewports();

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
		float WheelDelta = UInputManager::GetInstance().GetMouseWheelDelta();

		if (WheelDelta != 0.0f)
		{
			int32 idx = GetViewportIndexUnderMouse();
			if (idx >= 0 && idx < (int32)Clients.size())
			{
				FViewportClient* C = Clients[idx];
				if (C && C->IsOrtho())
				{
					if (UCamera* O = C->GetOrthoCamera())
					{
						const float DollyStep = 5.0f; // 튜닝 가능
						O->SetLocation(O->GetLocation() + O->GetForward() * (WheelDelta * DollyStep));
						// 공유 상태 갱신 및 동기화
						OrthoSharedCenter = O->GetLocation();
						OrthoSharedFovY = O->GetFovY();
						bOrthoSharedInit = true;
						SyncOrthographicSharedState(idx);
						UInputManager::GetInstance().SetMouseWheelDelta(0.0f);
					}
				}
			}
		}
	}

	// 3) 카메라 업데이트 (공유 오쏘 1회, 퍼스펙티브는 각자)
	TickCameras(DT);

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

	const int32 N = (int32)Viewports.size();
	if (N == 0) return;

	// ViewType ViewMode 콤보 라벨 & 매핑
	static const char* ViewModeLabels[] = {
		"Lit", "Unlit", "WireFrame"
	};
	static const EViewMode IndexToViewMode[] = {
		EViewMode::Lit,
		EViewMode::Unlit,
		EViewMode::WireFrame
	};


	static const char* ViewTypeLabels[] = {
		"Perspective", "OrthoTop", "OrthoBottom", "OrthoLeft", "OrthoRight", "OrthoFront", "OrthoBack"
	};
	static const EViewType IndexToViewType[] = {
		EViewType::Perspective,
		EViewType::OrthoTop,
		EViewType::OrthoBottom,
		EViewType::OrthoLeft,
		EViewType::OrthoRight,
		EViewType::OrthoFront,
		EViewType::OrthoBack
	};

	for (int32 i = 0; i < N; ++i)
	{
		const FRect& r = Viewports[i]->GetRect();
		if (r.W <= 0 || r.H <= 0) continue;

		const int ToolbarH = Viewports[i]->GetToolbarHeight();
		const ImVec2 a{(float)r.X, (float)r.Y};
		const ImVec2 b{(float)(r.X + r.W), (float)(r.Y + ToolbarH)};

		// 1) 1안처럼 오버레이 배경/보더 그리기
		ImDrawList* dl = ImGui::GetForegroundDrawList();
		dl->AddRectFilled(a, b, IM_COL32(30, 30, 30, 100));
		dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, b.y), IM_COL32(70, 70, 70, 120), 1.0f);

		// 2) 같은 영역에 "배경 없는" 작은 윈도우를 띄워 콤보/버튼 UI 배치 (2안의 핵심)
		ImGui::PushID(i);

		ImGui::SetNextWindowPos(ImVec2(a.x, a.y));
		ImGui::SetNextWindowSize(ImVec2(b.x - a.x, b.y - a.y));

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground | // ★ 배경 없음(우리가 drawlist로 그렸으므로)
			ImGuiWindowFlags_NoFocusOnAppearing;

		// 얇은 툴바 느낌을 위한 패딩/스페이싱 축소
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

		// 윈도우 이름은 뷰포트마다 고유해야 함
		char winName[64];
		snprintf(winName, sizeof(winName), "ViewportToolbar##%d", i);

		if (ImGui::Begin(winName, nullptr, flags))
		{
			EViewMode CurrentMode = Clients[i]->GetViewMode();
			int32 CurrentModeIndex = 0;
			for (int k = 0; k < IM_ARRAYSIZE(IndexToViewMode); ++k)
			{
				if (IndexToViewMode[k] == CurrentMode)
				{
					CurrentModeIndex = k;
					break;
				}
			}
			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::Combo("##ViewMode", &CurrentModeIndex, ViewModeLabels, IM_ARRAYSIZE(ViewModeLabels)))
			{
				if (CurrentModeIndex >= 0 && CurrentModeIndex < IM_ARRAYSIZE(IndexToViewMode))
				{
					Clients[i]->SetViewMode(IndexToViewMode[CurrentModeIndex]);
				}
			}

			// 같은 줄로 이동 + 약간의 간격
			ImGui::SameLine(0.0f, 10.0f);
			ImGui::TextDisabled("|");
			ImGui::SameLine(0.0f, 10.0f);

			// ViewType 콤보 (2안처럼 정상 동작)
			// 현재 타입을 인덱스로 변환
			EViewType curType = Clients[i]->GetViewType();
			int curIdx = 0;
			for (int k = 0; k < IM_ARRAYSIZE(IndexToViewType); ++k)
			{
				if (IndexToViewType[k] == curType)
				{
					curIdx = k;
					break;
				}
			}

			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::Combo("##ViewType", &curIdx, ViewTypeLabels, IM_ARRAYSIZE(ViewTypeLabels)))
			{
				if (curIdx >= 0 && curIdx < IM_ARRAYSIZE(IndexToViewType))
				{
					UE_LOG("%d", curIdx);
					EViewType NewType = IndexToViewType[curIdx];
					Clients[i]->SetViewType(NewType);

					// Apply camera basis + refresh immediately so toolbar change takes effect without RMB
					if (NewType == EViewType::Perspective)
					{
						if (UCamera* P = Clients[i]->GetPerspectiveCamera())
						{
							P->SetCameraType(ECameraType::ECT_Perspective);
							P->Update();
						}
					}
					else
					{
						if (UCamera* O = Clients[i]->GetOrthoCamera())
						{
							Clients[i]->ApplyOrthoBasisForViewType(*O);

							// Sync with shared ortho state (center + zoom)
							//UE_LOG("88888888");
							if (!bOrthoSharedInit)
							{
								//UE_LOG("9999999");
								OrthoSharedCenter = O->GetLocation();
								OrthoSharedFovY = O->GetFovY();
								bOrthoSharedInit = true;
							}
							O->SetFovY(OrthoSharedFovY);
							O->SetLocation(OrthoSharedCenter);
							O->UpdateMatrixByOrth();
							// Also push to other orthographic viewports
							SyncOrthographicSharedState(i);
							O->Update();
						}
					}
				}
			}
		}
		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopID();
	}
}

void UViewportManager::SyncOrthographicSharedState(int32 SourceIdx)
{
	const int32 N = (int32)Clients.size();
	for (int32 i = 0; i < N; ++i)
	{
		if (i == SourceIdx) continue;
		FViewportClient* C = Clients[i];
		if (!C) continue;
		if (!C->IsOrtho()) continue;
		if (UCamera* O = C->GetOrthoCamera())
		{
			C->ApplyOrthoBasisForViewType(*O);
			O->SetCameraType(ECameraType::ECT_Orthographic);
			O->SetFovY(OrthoSharedFovY);
			O->SetLocation(OrthoSharedCenter);
			O->UpdateMatrixByOrth();
		}
	}
}

void UViewportManager::TickInput()
{
	if (!Root)
	{
		return;
	}

	auto& InputManager = UInputManager::GetInstance();
	const FVector& MousePosition = InputManager.GetMousePosition();
	FPoint P{LONG(MousePosition.X), LONG(MousePosition.Y)};

	SWindow* Target = Capture ? static_cast<SWindow*>(Capture) : Root->HitTest(P);

	if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
	{
		if (Target && Target->OnMouseDown(P, 0))
		{
			Capture = Target;
		}
	}

	// Mouse move while captured (always forward to allow cross-drag detection)
	const FVector& d = InputManager.GetMouseDelta();
	if ((d.X != 0.0f || d.Y != 0.0f) && Capture)
	{
		Capture->OnMouseMove(P);
	}

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Capture)
		{
			Capture->OnMouseUp(P, 0);
			Capture = nullptr;
		}
	}

	if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
	{
		Capture->OnMouseUp(P, /*Button*/0);
		Capture = nullptr;
	}
}

/**
 * @brief 마우스가 컨트롤 중인 Viewport의 인덱스를 반환하는 함수
 * @return 기본적으로는 index를 반환, 아무 것도 만지지 않았다면 -1
 */
int8 UViewportManager::GetViewportIndexUnderMouse() const
{
	const auto& InputManager = UInputManager::GetInstance();

	const FVector& MousePosition = InputManager.GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositionY = static_cast<LONG>(MousePosition.Y);

	for (int8 i = 0; i < static_cast<int8>(Viewports.size()); ++i)
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

bool UViewportManager::ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const
{
	if (Index < 0 || Index >= (int32)Viewports.size()) return false;
	const FRect& Rect = Viewports[Index]->GetRect();
	if (Rect.W <= 0 || Rect.H <= 0) return false;

	// 툴바 높이를 고려한 실제 렌더링 영역 기준으로 NDC 계산
	const int32 ToolbarHeight = Viewports[Index]->GetToolbarHeight();
	const float RenderAreaY = static_cast<float>(Rect.Y + ToolbarHeight);
	const float RenderAreaHeight = static_cast<float>(Rect.H - ToolbarHeight);

	if (RenderAreaHeight <= 0)
	{
		return false;
	}

	const FVector& MousePosition = UInputManager::GetInstance().GetMousePosition();
	const float LocalX = (MousePosition.X - (float)Rect.X);
	const float LocalY = (MousePosition.Y - RenderAreaY);
	const float Width = (float)Rect.W;
	const float Height = RenderAreaHeight;

	OutNdcX = (LocalX / Width) * 2.0f - 1.0f;
	OutNdcY = 1.0f - (LocalY / Height) * 2.0f;
	return true;
}


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
