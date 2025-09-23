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


static FVector OrthoCamFwd[6] = {
	FVector(0, 0, -1), // Top: 위에서 아래로 본다 (카메라 forward = -Z)
	FVector(0, 0,  1), // Bottom
	FVector(0, -1, 0), // Left
	FVector(0,  1, 0), // Right
	FVector(-1,  0, 0), // Front
	FVector(1, 0, 0), // Back
};

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

	// Start with a single window, initial rect (menu height applied on first Update)
	SWindow* Single = new SWindow();
	Single->OnResize({0, 0, Width, Height});
	SetRoot(Single);

	// 뷰포트 & 클라이언트 생성
	InitializeViewportAndClient();

	// 직교 6방향 카메라 초기화
	InitializeOrthoGraphicCamera();

	// perspective 4개 초기화
	InitializePerspectiveCamera();

	// 직교 카메라 중점 업데이트
	UpdateOrthoGraphicCameraPoint();

	// 바인드
	BindOrthoGraphicCameraToClient();

}

void UViewportManager::SetRoot(SWindow* InRoot)
{
	Root = InRoot;
}

SWindow* UViewportManager::GetRoot()
{
	return Root;
}

void UViewportManager::BuildSingleLayout(int32 PromoteIdx)
{
	if (!Root) return;
	// 0) 싱글로 전환하기 전에 "어떤 뷰포트가 메인으로 갈지" 결정
	//PromoteIdx = (int32)GetViewportIndexUnderMouse(); // 마우스 아래 인덱스

	UE_LOG("%d", PromoteIdx);

	if (PromoteIdx < 0 || PromoteIdx >= (int32)Viewports.size())
	{
		// 우클릭 드래그 중인 뷰가 있으면 그걸 우선, 없으면 0번 유지
		PromoteIdx = (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < (int32)Viewports.size())
			? ActiveRmbViewportIdx
			: 0;
	}

	// 0.5) 선택된 인덱스를 0번으로 올리기(뷰포트/클라이언트 페어 동기 스왑)
	if (PromoteIdx != 0)
	{
		std::swap(Viewports[0], Viewports[PromoteIdx]);
		std::swap(Clients[0], Clients[PromoteIdx]);
	}

	// 1) 윈도우 크기 읽기
	int32 Width = 0, Height = 0;
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}
	const int32 MenuH = (int32)UUIManager::GetInstance().GetMainMenuBarHeight();
	const FRect Rect{ 0, MenuH, Width, max(0, Height - MenuH) };

	// 2) 기존 캡처 해제 (안전)
	Capture = nullptr;

	// 3) 새 루트 구성 (싱글)
	SWindow* Single = new SWindow();
	Single->OnResize(Rect);
	SetRoot(Single);
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



	// 초기 4분할 뷰타입 배치: Top / Front / Right / Perspective
	if (Clients.size() == 4)
	{
		Clients[0]->SetViewType(EViewType::Perspective);
		Clients[0]->SetViewMode(EViewMode::Unlit);

		Clients[1]->SetViewType(EViewType::OrthoRight);
		Clients[1]->SetViewMode(EViewMode::WireFrame);

		Clients[2]->SetViewType(EViewType::OrthoTop);
		Clients[2]->SetViewMode(EViewMode::WireFrame);

		Clients[3]->SetViewType(EViewType::OrthoLeft);
		Clients[3]->SetViewMode(EViewMode::WireFrame);
	}

	ForceRefreshOrthoViewsAfterLayoutChange();
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

	const FVector& MousePosition = Input.GetMousePosition();
	const LONG MousePositionX = (LONG)MousePosition.X;
	const LONG MousePositioinY = (LONG)MousePosition.Y;

	const int32 N = (int32)Viewports.size();
	for (int32 i = 0; i < N; ++i)
	{
		const FRect& Rect = Viewports[i]->GetRect();
		if (MousePositionX >= Rect.X && MousePositionX < Rect.X + Rect.W && MousePositioinY >= Rect.Y && MousePositioinY < Rect.Y + Rect.H)
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
		}
	}
}

void UViewportManager::Update()
{
	if (!Root)
	{
		return;
	}

	int32 w = 0, h = 0;
	if (AppWindow) AppWindow->GetClientSize(w, h);

	const int menuH = (int)UUIManager::GetInstance().GetMainMenuBarHeight();
	if (w > 0 && h > 0)
	{
		Root->OnResize(FRect{0, menuH, w, max(0, h - menuH)});
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
			int32 Index = GetViewportIndexUnderMouse();
			if (Index >= 0 && Index < (int32)Clients.size())
			{
				FViewportClient* Client = Clients[Index];
				if (Client && Client->IsOrtho())
				{
					if (UCamera* OrthoGraphicCamera = Client->GetOrthoCamera())
					{
						const float DollyStep = 5.0f; // 튜닝 가능
						SharedFovY -=  WheelDelta * DollyStep;

						UInputManager::GetInstance().SetMouseWheelDelta(0.0f);
					}
				}
			}
		}
	}

	UpdateOrthoGraphicCameraPoint();

	UpdateOrthoGraphicCameraFov();

	//ApplySharedOrthoCenterToAllCameras();

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

	int32 N = (int32)Viewports.size();
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


	if (ViewportChange == EViewportChange::Single)
	{
		N = 1;
	}
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
					EViewType NewType = IndexToViewType[curIdx];
					Clients[i]->SetViewType(NewType);
					
						
					// Apply camera basis + refresh immediately so toolbar change takes effect without RMB
					if (NewType == EViewType::Perspective)
					{
						// 퍼스펙티브 바인딩
						if (UCamera* Cam = PerspectiveCameras[i])
						{
							Clients[i]->SetPerspectiveCamera(Cam);
							Cam->SetCameraType(ECameraType::ECT_Perspective);
							Cam->Update();
						}
					}
					else
					{
						const int OrthoIdx = curIdx - 1;

						if (OrthoIdx >= 0 && OrthoIdx < (int)OrthoGraphicCameras.size())
						{
							UCamera* Cam = OrthoGraphicCameras[OrthoIdx];
							Clients[i]->SetOrthoCamera(Cam);
							Cam->SetCameraType(ECameraType::ECT_Orthographic);
							Cam->Update();
						}
					}
				}
			}

			{
				// 우측 여백(툴바 끝과 버튼 사이), 버튼 크기(아이콘 버튼이면 보통 24~28px)
				const float kRightPadding = 6.0f;

				// 만약 FUEImgui에 사이즈 질의가 있다면 사용:
				// const ImVec2 iconSize = FUEImgui::GetToolbarIconSize(); // 가정
				// const float  btnW     = iconSize.x;

				// 없다면 고정값(필요하면 프로젝트 값에 맞춰 조정)
				const float  btnW = 24.0f;

				// 현재 윈도우의 컨텐츠 우측 x좌표
				const float contentRight = ImGui::GetWindowContentRegionMax().x;
				// 현재 커서 y는 유지하고, x만 우측으로 점프
				float targetX = contentRight - kRightPadding - btnW;
				// 혹시 앞의 위젯이 너무 오른쪽까지 차지했다면 음수되지 않도록 보정
				targetX = std::max(targetX, ImGui::GetCursorPosX());

				// 같은 줄 유지 후 커서 이동
				ImGui::SameLine();
				ImGui::SetCursorPosX(targetX);
			}

			// 현재 레이아웃 상태 보관
			enum class ELayout : uint8 { Single, Quad };
			static ELayout CurrentLayout = ELayout::Quad;

			if (ViewportChange == EViewportChange::Single)
			{
				if (FUEImgui::ToolbarIconButton("LayoutQuad", EUEViewportIcon::Quad, CurrentLayout == ELayout::Quad))
				{
					CurrentLayout = ELayout::Quad;
					ViewportChange = EViewportChange::Quad;
					BuildFourSplitLayout();
				}
			}
			if (ViewportChange == EViewportChange::Quad)
			{
				if (FUEImgui::ToolbarIconButton("LayoutSingle", EUEViewportIcon::Single, CurrentLayout == ELayout::Single))
				{
					CurrentLayout = ELayout::Single;
					ViewportChange = EViewportChange::Single;
					BuildSingleLayout(i);
				}
			}
		}

		


		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopID();
	}
}


void UViewportManager::InitializeViewportAndClient()
{
	for (int i = 0;i < 4;i++)
	{
		FViewport* Viewport = new FViewport();
		FViewportClient* ViewportClient = new FViewportClient();

		Viewport->SetViewportClient(ViewportClient);

		Clients.push_back(ViewportClient);
		Viewports.push_back(Viewport);

	}
}

void UViewportManager::InitializeOrthoGraphicCamera()
{
	for (int i = 0; i < 6; ++i)
	{
		UCamera* Cam = NewObject<UCamera>();
		Cam->SetDisplayName("Camera" + to_string(UCamera::GetNextGenNumber()));
		OrthoGraphicCameras.push_back(Cam);
	}

	// 상단 
	OrthoGraphicCameras[0]->SetLocation(FVector(0.0f, 0.0f, 100.0f));
	OrthoGraphicCameras[0]->SetRotation(FVector(0.0f, 90.0f, 180.0f));
	OrthoGraphicCameras[0]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[0]->SetFarZ(150.0f);

	// 하단 			 
	OrthoGraphicCameras[1]->SetLocation(FVector(0.0f, 0.0f, -100.0f));
	OrthoGraphicCameras[1]->SetRotation(FVector(0.0f,-90.0f, 0.0f));
	OrthoGraphicCameras[1]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[1]->SetFarZ(150.0f);

	// 왼쪽			 
	OrthoGraphicCameras[2]->SetLocation(FVector(00.0f, 100.0f, 0.0f));
	OrthoGraphicCameras[2]->SetRotation(FVector(0.0f, 0.0f, -90.0f));
	OrthoGraphicCameras[2]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[2]->SetFarZ(150.0f);

	// 오른쪽 			
	OrthoGraphicCameras[3]->SetLocation(FVector(0.0f, -100.0f, 0.0f));
	OrthoGraphicCameras[3]->SetRotation(FVector(0.0f, 0.0f, 90.0f));
	OrthoGraphicCameras[3]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[3]->SetFarZ(150.0f);

	// 정면 			 
	OrthoGraphicCameras[4]->SetLocation(FVector(100.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[4]->SetRotation(FVector(180.0f, 180.0f, 0.0f));
	OrthoGraphicCameras[4]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[4]->SetFarZ(150.0f);

	// 후면 			 
	OrthoGraphicCameras[5]->SetLocation(FVector(-100.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[5]->SetRotation(FVector(0.0f, 0.0f, 0.0f));
	OrthoGraphicCameras[5]->SetCameraType(ECameraType::ECT_Orthographic);
	OrthoGraphicCameras[5]->SetFarZ(150.0f);



	InitialOffsets.push_back(FVector(0.0f, 0.0f, 100.0f));
	InitialOffsets.push_back(FVector(0.0f, 0.0f, -100.0f));
	InitialOffsets.push_back(FVector(0.0f, 100.0f, 0.0f));
	InitialOffsets.push_back(FVector(0.0f, -100.0f, 0.0f));
	InitialOffsets.push_back(FVector(100.0f, 0.0f, 0.0f));
	InitialOffsets.push_back(FVector(-100.0f, 0.0f, 0.0f));
}

void UViewportManager::InitializePerspectiveCamera()
{
	for (int i = 0; i < 4; ++i)
	{		
		UCamera* Cam = NewObject<UCamera>();
		Cam->SetDisplayName("Camera" + to_string(UCamera::GetNextGenNumber()));
		PerspectiveCameras.push_back(Cam);

		Clients[i]->SetPerspectiveCamera(Cam);
	}
}

void UViewportManager::UpdateOrthoGraphicCameraPoint()
{
	// RMB 드래그 중인 오쏘 뷰포트만 처리
	auto& Input = UInputManager::GetInstance();
	if (!Input.IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	const int32 Index = ActiveRmbViewportIdx;
	if (Index < 0 || Index >= (int32)Clients.size()) return;

	FViewportClient* ViewportClient = Clients[Index];
	if (!ViewportClient || !ViewportClient->IsOrtho()) return;

	FViewport* Viewport = Viewports[Index];
	if (!Viewport) return;

	const FRect& Rect = Viewport->GetRect();
	const int32 ToolH = Viewport->GetToolbarHeight(); // 델타에는 영향 X, 높이 > 폭 계산용
	const float Width = (float)Rect.W;
	const float Height = (float)max<LONG>(0, Rect.H - ToolH);
	if (Width <= 0.0f || Height <= 0.0f) return;

	UCamera* OrthoCam = ViewportClient->GetOrthoCamera();
	if (!OrthoCam) return;

	// ViewType → fwd/right/up 기저 구성
	const EViewType ViewType = ViewportClient->GetViewType();

	FVector Fwd(1, 0, 0);
	switch (ViewType)
	{
	case EViewType::OrthoTop:    Fwd = FVector(0, 0, -1); break;
	case EViewType::OrthoBottom: Fwd = FVector(0, 0, 1); break;
	case EViewType::OrthoFront:  Fwd = FVector(-1, 0, 0); break;
	case EViewType::OrthoBack:   Fwd = FVector(1, 0, 0); break;
	case EViewType::OrthoRight:  Fwd = FVector(0, 1, 0); break;
	case EViewType::OrthoLeft:   Fwd = FVector(0, -1, 0); break;
	default: return; // 퍼스펙티브면 여기선 스킵
	}
	Fwd.Normalize();

	
	FVector UpRef(0, 0, 1);
	if (std::fabs(Fwd.Dot(UpRef)) > 0.999f) UpRef = FVector(1, 0, 0);



	FVector Right = Fwd.Cross(UpRef); Right.Normalize();
	FVector Up = Right.Cross(Fwd);  Up.Normalize();

	// 마우스 델타(px) → NDC 델타 → 월드 델타
	const FVector& d = Input.GetMouseDelta();
	if (d.X == 0.0f && d.Y == 0.0f) return;

	//UE_LOG("%f %f", -d.X, d.Y);

	const float Aspect = (Height > 0.f) ? (Width / Height) : 1.0f;
	const float Rad = FVector::GetDegreeToRadian(OrthoCam->GetFovY());
	const float OrthoWidth = 2.0f * std::tanf(Rad * 0.5f);
	const float OrthoHeight = (Aspect > 0.0f) ? (OrthoWidth / Aspect) : OrthoWidth;

	// 화면 기준 드래그 비율(NDC 스케일) 기본 부호
	float sX = -1.0f;
	float sY = 1.0f;

	// Top/Bottom은 화면 축이 180° 뒤집혀 보이므로 둘 다 반전
	if (ViewType == EViewType::OrthoTop || ViewType == EViewType::OrthoBottom)
	{
		sX *= -1.0f;
		sY *= -1.0f;
	}

	// NDC 델타 (마우스 픽셀 → -1..+1)
	const float NdcDX = sX * (d.X / Width) * 2.0f;
	const float NdcDY = sY * (d.Y / Height) * 2.0f;

	const FVector WorldDelta = Right * (NdcDX * (OrthoWidth * 0.5f)) + Up * (NdcDY * (OrthoHeight * 0.5f));

	// 공유 중심 누적 이동
	OrthoGraphicCamerapoint += WorldDelta;

	UE_LOG("%f %f %f", WorldDelta.X, WorldDelta.Y, WorldDelta.Z);

	for (int32 i = 0; i < (int32)OrthoGraphicCameras.size(); ++i)
	{
		if (UCamera* Cam = OrthoGraphicCameras[i])
		{
			Cam->SetLocation(OrthoGraphicCamerapoint + InitialOffsets[i]);
			Cam->Update();
		}
	}

	//OrthoGraphicCameras[0]->SetLocation(FVector(0.0f, 0.0f, 100.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[1]->SetLocation(FVector(0.0f, 0.0f, -100.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[2]->SetLocation(FVector(0.0f, 100.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[3]->SetLocation(FVector(0.0f, -100.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[4]->SetLocation(FVector(100.0f, 0.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[5]->SetLocation(FVector(-100.0f, 0.0f, 0.0f) + OrthoGraphicCamerapoint);

	//UE_LOG("%f %f %f", OrthoGraphicCamerapoint.X, OrthoGraphicCamerapoint.Y, OrthoGraphicCamerapoint.Z);
}

void UViewportManager::UpdateOrthoGraphicCameraFov()
{
	for (int i = 0;i < 6;++i)
	{
		OrthoGraphicCameras[i]->SetFovY(SharedFovY);
	}
}

void UViewportManager::BindOrthoGraphicCameraToClient()
{
	Clients[1]->SetViewType(EViewType::OrthoLeft);
	Clients[1]->SetOrthoCamera(OrthoGraphicCameras[3]);

	Clients[2]->SetViewType(EViewType::OrthoTop);
	Clients[2]->SetOrthoCamera(OrthoGraphicCameras[0]);

	Clients[3]->SetViewType(EViewType::OrthoRight);
	Clients[3]->SetOrthoCamera(OrthoGraphicCameras[2]);
}

void UViewportManager::ForceRefreshOrthoViewsAfterLayoutChange()
{
	// 1) 현재 리프 Rect들을 뷰포트에 즉시 반영 (전환 직후 크기 확정)
	SyncRectsToViewports();

	// 2) 각 클라이언트에 카메라 재바인딩 + Update + OnResize
	const int32 N = (int32)Clients.size();
	for (int32 i = 0; i < N; ++i)
	{
		FViewportClient* C = Clients[i];
		if (!C) continue;

		const EViewType VT = C->GetViewType();
		if (VT == EViewType::Perspective)
		{
			// 퍼스펙티브는 자신 카메라 보장
			if (i < (int32)PerspectiveCameras.size() && PerspectiveCameras[i])
			{
				C->SetPerspectiveCamera(PerspectiveCameras[i]);
				PerspectiveCameras[i]->SetCameraType(ECameraType::ECT_Perspective);
				PerspectiveCameras[i]->Update();
			}
		}
		else
		{
			// 오쏘: ViewType → 6방향 인덱스 매핑
			int OrthoIdx = -1;
			switch (VT)
			{
			case EViewType::OrthoTop:    OrthoIdx = 0; break;
			case EViewType::OrthoBottom: OrthoIdx = 1; break;
			case EViewType::OrthoLeft:   OrthoIdx = 2; break;
			case EViewType::OrthoRight:  OrthoIdx = 3; break;
			case EViewType::OrthoFront:  OrthoIdx = 4; break;
			case EViewType::OrthoBack:   OrthoIdx = 5; break;
			default: break;
			}

			if (OrthoIdx >= 0 && OrthoIdx < (int)OrthoGraphicCameras.size())
			{
				UCamera* Cam = OrthoGraphicCameras[OrthoIdx];
				C->SetOrthoCamera(Cam);
				Cam->SetCameraType(ECameraType::ECT_Orthographic);
				Cam->Update();
			}
		}

		// 크기 반영(툴바 높이 포함) — 깜빡임/툴바 좌표 안정화
		const FRect& R = Viewports[i]->GetRect();
		Viewports[i]->SetToolbarHeight(24);
		C->OnResize({ R.W, R.H });
	}

	// 3) 오쏘 6방향 카메라를 공유 중심점에 맞춰 정렬만 수행(이동 입력 없이)
	auto Reposition = [&](int CamIndex, const FVector& CamFwd)
		{
			if (CamIndex < 0 || CamIndex >= (int)OrthoGraphicCameras.size()) return;
			if (UCamera* Cam = OrthoGraphicCameras[CamIndex])
			{
				const FVector Loc = Cam->GetLocation();
				const FVector ToC = Loc - OrthoGraphicCamerapoint;
				const float   Dist = std::fabs(ToC.Dot(CamFwd));
				Cam->SetLocation(OrthoGraphicCamerapoint - CamFwd * Dist);
				Cam->Update();
			}
		};

	Reposition(0, FVector(0, 0, -1)); // Top
	Reposition(1, FVector(0, 0, 1)); // Bottom
	Reposition(2, FVector(0, -1, 0)); // Left
	Reposition(3, FVector(0, 1, 0)); // Right
	Reposition(4, FVector(1, 0, 0)); // Front
	Reposition(5, FVector(-1, 0, 0)); // Back

	// 4) 드래그 대상 초기화
	ActiveRmbViewportIdx = -1;
}

void UViewportManager::ApplySharedOrthoCenterToAllCameras()
{
	// OrthoGraphicCamerapoint 기준으로 6개 오쏘 카메라 위치 갱신
	for (UCamera* Cam : OrthoGraphicCameras)
	{
		if (!Cam) continue;

		// 카메라 '전방'을 "센터 - 카메라 위치"로 잡으면, Top/Right/... 어떤 회전이라도 일관되게 작동
		FVector fwd = (OrthoGraphicCamerapoint - Cam->GetLocation()).Normalized();

		// 특이 케이스 보호: fwd가 0이면 월드 Z를 피벗으로 대체
		if (fwd.IsNearlyZero())
		{
			fwd = FVector(0, 0, -1); // 아무거나 기본값
		}

		// 현재 센터까지의 전방 거리 유지
		const FVector toC = Cam->GetLocation() - OrthoGraphicCamerapoint;
		const float   dist = std::fabs(toC.Dot(fwd));

		Cam->SetLocation(OrthoGraphicCamerapoint - fwd * dist);
		Cam->Update();
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

