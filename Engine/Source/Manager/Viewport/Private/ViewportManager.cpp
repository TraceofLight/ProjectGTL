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
#include "Manager/UI/Public/UIManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UViewportManager)

UViewportManager::UViewportManager() = default;
UViewportManager::~UViewportManager() = default;

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


void UViewportManager::Initialize(FAppWindow* InWindow)
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
	Window->OnResize({ 0, 0, Width, Height });

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

	IniSaveSharedV = UConfigManager::GetInstance().GetSplitV();
	IniSaveSharedH = UConfigManager::GetInstance().GetSplitH();

	SplitterValueV = IniSaveSharedV;
	SplitterValueH = IniSaveSharedH;
}

void UViewportManager::BuildSingleLayout(int32 PromoteIdx)
{
	if (!Root)
	{
		return;
	}

	Capture = nullptr;
	DestroyTree(Root);

	// UE_LOG_DEBUG("ViewportManager: PromoteIdx: %d", PromoteIdx);

	if (PromoteIdx < 0 || PromoteIdx >= static_cast<int32>(Viewports.size()))
	{
		// 우클릭 드래그 중인 뷰가 있으면 그걸 우선, 없으면 0번 유지
		PromoteIdx = (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < static_cast<int32>(Viewports.size()))
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

	// 1) 윈도우 크기 읽기
	int32 Width = 0, Height = 0;
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}
	const int32 MenuH = static_cast<int32>(UUIManager::GetInstance().GetMainMenuBarHeight());
	const FRect Rect{0, MenuH, Width, max(0, Height - MenuH)};

	// 2) 기존 캡처 해제 (안전)
	Capture = nullptr;

	// 3) 새 루트 구성 (싱글)
	SWindow* Single = new SWindow();
	Single->OnResize(Rect);
	SetRoot(Single);
}


// 싱글->쿼드로 전환 시, 스플리터V를 할당합니다. 스플리터V는 트리구조로 스플리터H를 2개 자식으로 가지고 최종적으로 4개의 Swindow를 가집니다.
void UViewportManager::BuildFourSplitLayout()
{
	if (!Root)
	{
		return;
	}

	if (LastPromotedIdx > 0 && LastPromotedIdx < (int32)Viewports.size())
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

	const int MenuHeight = static_cast<int>(UUIManager::GetInstance().GetMainMenuBarHeight());

	const FRect Rect{0, MenuHeight, Width, max(0, Height - MenuHeight)};

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

void UViewportManager::GetLeafRects(TArray<FRect>& OutRects) const
{
	OutRects.clear();
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
				Out.emplace_back(Node->GetRect());
			}
		}
	};
	Local::Collect(Root, OutRects);
}


void UViewportManager::SyncRectsToViewports() const
{
	TArray<FRect> Leaves;
	GetLeafRects(Leaves);

	// 싱글 모드: 리프가 1개, 4분할: 리프가 4개
	const int32 N = min<int32>(static_cast<int32>(Leaves.size()), static_cast<int32>(Viewports.size()));
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->SetRect(Leaves[i]);
		Viewports[i]->SetToolbarHeight(24);

		const int32 renderH = max<LONG>(0, Leaves[i].H - Viewports[i]->GetToolbarHeight());
		Clients[i]->OnResize({ Leaves[i].W, renderH });
	}
}

void UViewportManager::PumpAllViewportInput() const
{
	// 각 뷰포트가 스스로 InputManager에서 읽어
	// 로컬 좌표로 변환 → 각자의 Client로 MouseMove/CapturedMouseMove 전달
	const int32 N = static_cast<int32>(Viewports.size());
	for (int32 i = 0; i < N; ++i)
	{
		Viewports[i]->PumpMouseFromInputManager();
	}
}

// 오른쪽 마우스를 누르고 있는 뷰포트 인덱스를 셋 해줍니다.
void UViewportManager::UpdateActiveRmbViewportIndex()
{
	ActiveRmbViewportIdx = -1;
	const auto& Input = UInputManager::GetInstance();
	if (!Input.IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	const FVector& MousePosition = Input.GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositioinY = static_cast<LONG>(MousePosition.Y);

	const int32 N = static_cast<int32>(Viewports.size());
	for (int32 i = 0; i < N; ++i)
	{
		const FRect& Rect = Viewports[i]->GetRect();
		if (MousePositionX >= Rect.X && MousePositionX < Rect.X + Rect.W && MousePositioinY >= Rect.Y && MousePositioinY < Rect.Y + Rect.H)
		{
			ActiveRmbViewportIdx = i;
			//UE_LOG("%d", i);
			break;
		}
	}
}

void UViewportManager::TickCameras() const
{
	// 우클릭이 눌린 상태라면 해당 뷰포트의 카메라만 입력 반영(Update)
	// 그렇지 않다면(비상호작용) 카메라 이동 입력은 생략하여 타 뷰포트에 영향이 가지 않도록 함
	const int32 N = static_cast<int32>(Clients.size());
	const bool bRmbDown = UInputManager::GetInstance().IsKeyDown(EKeyInput::MouseRight);
	if (bRmbDown)
	{
		if (ActiveRmbViewportIdx >= 0 && ActiveRmbViewportIdx < N)
		{
			Clients[ActiveRmbViewportIdx]->Tick();
		}
	}
}

void UViewportManager::Update()
{
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

	const int MenuHeight = static_cast<int>(UUIManager::GetInstance().GetMainMenuBarHeight());
	if (Width > 0 && Height > 0)
	{
		Root->OnResize(FRect{0, MenuHeight, Width, max(0, Height - MenuHeight)});
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
			if (Index >= 0 && Index < static_cast<int32>(Clients.size()))
			{
				FViewportClient* Client = Clients[Index];
				if (Client && Client->IsOrtho())
				{
					if (Client->GetOrthoCamera())
					{
						constexpr float DollyStep = 5.0f; // 튜닝 가능
						SharedFovY -= WheelDelta * DollyStep;

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
	TickCameras();

	// 4) 드로우(3D) — 실제 렌더러 루프에서 Viewport 적용 후 호출해도 됨
	//    여기서는 뷰/클라 페어 순회만 보여줌. (RS 바인딩은 네 렌더러 Update에서 수행 중)
	const int32 N = static_cast<int32>(Clients.size());
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

	int32 N = static_cast<int32>(Viewports.size());
	if (N == 0) return;

	// ViewType ViewMode 콤보 라벨 & 매핑
	static const char* ViewModeLabels[] = {
		"Lit", "Unlit", "WireFrame"
	};
	static constexpr EViewMode IndexToViewMode[] = {
		EViewMode::Lit,
		EViewMode::Unlit,
		EViewMode::WireFrame
	};


	static const char* ViewTypeLabels[] = {
		"Perspective", "OrthoTop", "OrthoBottom", "OrthoLeft", "OrthoRight", "OrthoFront", "OrthoBack"
	};
	static constexpr EViewType IndexToViewType[] = {
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
		const ImVec2 a{static_cast<float>(r.X), static_cast<float>(r.Y)};
		const ImVec2 b{static_cast<float>(r.X + r.W), static_cast<float>(r.Y + ToolbarH)};

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
		(void)snprintf(winName, sizeof(winName), "ViewportToolbar##%d", i);

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
							Clients[i]->SetViewType(EViewType::Perspective);
							Cam->SetCameraType(ECameraType::ECT_Perspective);
							Cam->Update();
						}
					}
					else
					{
						const int OrthoIdx = curIdx - 1;

						if (OrthoIdx >= 0 && OrthoIdx < static_cast<int>(OrthoGraphicCameras.size()))
						{
							UCamera* Cam = OrthoGraphicCameras[OrthoIdx];
							Clients[i]->SetOrthoCamera(Cam);
							Clients[i]->SetViewType(static_cast<EViewType>(curIdx));
							Cam->SetCameraType(ECameraType::ECT_Orthographic);
							Cam->Update();
						}
					}
				}
			}

			{
				constexpr float kRightPadding = 6.0f;
				constexpr float btnW = 24.0f;

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
				if (FUEImgui::ToolbarIconButton("LayoutSingle", EUEViewportIcon::Single,
				                                CurrentLayout == ELayout::Single))
				{
					CurrentLayout = ELayout::Single;
					ViewportChange = EViewportChange::Single;

					PersistSplitterRatios();
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
	for (int i = 0; i < 4; i++)
	{
		FViewport* Viewport = new FViewport();
		FViewportClient* ViewportClient = new FViewportClient();

		Viewport->SetViewportClient(ViewportClient);

		Clients.push_back(ViewportClient);
		Viewports.push_back(Viewport);
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
	OrthoGraphicCameras[1]->SetRotation(FVector(0.0f, -90.0f, 0.0f));
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

	InitialOffsets.emplace_back(0.0f, 0.0f, 100.0f);
	InitialOffsets.emplace_back(0.0f, 0.0f, -100.0f);
	InitialOffsets.emplace_back(0.0f, 100.0f, 0.0f);
	InitialOffsets.emplace_back(0.0f, -100.0f, 0.0f);
	InitialOffsets.emplace_back(100.0f, 0.0f, 0.0f);
	InitialOffsets.emplace_back(-100.0f, 0.0f, 0.0f);
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
	// 인풋매니저를 가져옵니다.
	UInputManager& Input = UInputManager::GetInstance();

	// 오른쪽 클릭이 아니라면 리턴합니다
	if (!Input.IsKeyDown(EKeyInput::MouseRight))
	{
		return;
	}

	// 오른쪽 클릭을 했을 때, 인덱스를 가져옵니다.
	const int32 Index = ActiveRmbViewportIdx;
	// UE_LOG_DEBUG("ViewportManager: Get Index: %d", Index);

	// 인덱스가 범위 밖이라면 리턴합니다
	if (Index < 0 || Index >= static_cast<int32>(Clients.size()))
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

	UCamera* OrthoCam = ViewportClient->GetOrthoCamera();
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
		UpRef = { -1, 0, 0 };
	}

	FVector Right = OrthoCameraFoward.Cross(UpRef);
	Right.Normalize();
	FVector Up = Right.Cross(OrthoCameraFoward);

	// 마우스 델타(px) → NDC 델타 → 월드 델타
	const FVector& d = Input.GetMouseDelta();
	if (d.X == 0.0f && d.Y == 0.0f) return;

	//UE_LOG("ViewportManager: %f %f", -d.X, d.Y);

	const float Aspect = (Height > 0.f) ? (Width / Height) : 1.0f;
	const float Rad = FVector::GetDegreeToRadian(OrthoCam->GetFovY());
	const float OrthoWidth = 2.0f * std::tanf(Rad * 0.5f);
	const float OrthoHeight = (Aspect > 0.0f) ? (OrthoWidth / Aspect) : OrthoWidth;

	// 화면 기준 드래그 비율(NDC 스케일) 기본 부호
	float sX = -1.0f;
	float sY = 1.0f;

	// Top/Bottom은 화면 축이 180° 뒤집혀 보이므로 둘 다 반전
	//if (ViewType == EViewType::OrthoTop || ViewType == EViewType::OrthoBottom)
	//{
	//	sX *= -1.0f;
	//	sY *= -1.0f;
	//}

	// NDC 델타 (마우스 픽셀 → -1..+1)
	const float NdcDX = sX * (d.X / Width) * 2.0f;
	const float NdcDY = sY * (d.Y / Height) * 2.0f;

	const FVector WorldDelta = Right * (NdcDX * (OrthoWidth * 0.5f)) + Up * (NdcDY * (OrthoHeight * 0.5f));

	// 공유 중심 누적 이동
	OrthoGraphicCamerapoint += WorldDelta;

	// UE_LOG("ViewportManager: %f %f %f", WorldDelta.X, WorldDelta.Y, WorldDelta.Z);

	for (int32 i = 0; i < static_cast<int32>(OrthoGraphicCameras.size()); ++i)
	{
		if (UCamera* Cam = OrthoGraphicCameras[i])
		{
			Cam->SetLocation(OrthoGraphicCamerapoint + InitialOffsets[i]);
			//Cam->Update();
		}
	}

	//OrthoGraphicCameras[0]->SetLocation(FVector(0.0f, 0.0f, 100.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[1]->SetLocation(FVector(0.0f, 0.0f, -100.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[2]->SetLocation(FVector(0.0f, 100.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[3]->SetLocation(FVector(0.0f, -100.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[4]->SetLocation(FVector(100.0f, 0.0f, 0.0f) + OrthoGraphicCamerapoint);
	//OrthoGraphicCameras[5]->SetLocation(FVector(-100.0f, 0.0f, 0.0f) + OrthoGraphicCamerapoint);

	//UE_LOG("ViewportManager: %f %f %f", OrthoGraphicCamerapoint.X, OrthoGraphicCamerapoint.Y, OrthoGraphicCamerapoint.Z);
}

void UViewportManager::UpdateOrthoGraphicCameraFov() const
{
	for (int i = 0; i < 6; ++i)
	{
		OrthoGraphicCameras[i]->SetFovY(SharedFovY);
	}
}

void UViewportManager::BindOrthoGraphicCameraToClient() const
{
	Clients[1]->SetViewType(EViewType::OrthoLeft);
	Clients[1]->SetOrthoCamera(OrthoGraphicCameras[2]);

	Clients[2]->SetViewType(EViewType::OrthoTop);
	Clients[2]->SetOrthoCamera(OrthoGraphicCameras[0]);

	Clients[3]->SetViewType(EViewType::OrthoRight);
	Clients[3]->SetOrthoCamera(OrthoGraphicCameras[3]);
}

void UViewportManager::ForceRefreshOrthoViewsAfterLayoutChange()
{
	// 1) 현재 리프 Rect들을 뷰포트에 즉시 반영 (전환 직후 크기 확정)
	SyncRectsToViewports();

	// 2) 각 클라이언트에 카메라 재바인딩 + Update + OnResize
	const int32 N = static_cast<int32>(Clients.size());
	for (int32 i = 0; i < N; ++i)
	{
		FViewportClient* C = Clients[i];
		if (!C) continue;

		const EViewType VT = C->GetViewType();
		if (VT == EViewType::Perspective)
		{
			// 퍼스펙티브는 자신 카메라 보장
			if (i < static_cast<int32>(PerspectiveCameras.size()) && PerspectiveCameras[i])
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

			if (OrthoIdx >= 0 && OrthoIdx < static_cast<int>(OrthoGraphicCameras.size()))
			{
				UCamera* Cam = OrthoGraphicCameras[OrthoIdx];
				C->SetOrthoCamera(Cam);
				Cam->Update();
			}
		}

		// 크기 반영(툴바 높이 포함) — 깜빡임/툴바 좌표 안정화
		const FRect& R = Viewports[i]->GetRect();
		Viewports[i]->SetToolbarHeight(24);
		C->OnResize({R.W, R.H});
	}

	// 3) 오쏘 6방향 카메라를 공유 중심점에 맞춰 정렬만 수행(이동 입력 없이)
	auto Reposition = [&](int CamIndex, const FVector& CamFwd)
	{
		if (CamIndex < 0 || CamIndex >= static_cast<int>(OrthoGraphicCameras.size())) return;
		if (UCamera* Cam = OrthoGraphicCameras[CamIndex])
		{
			const FVector Loc = Cam->GetLocation();
			const FVector ToC = Loc - OrthoGraphicCamerapoint;
			const float Dist = std::fabs(ToC.Dot(CamFwd));
			Cam->SetLocation(OrthoGraphicCamerapoint - CamFwd * Dist);
			Cam->Update();
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

void UViewportManager::ApplySharedOrthoCenterToAllCameras() const
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
		const float dist = std::fabs(toC.Dot(fwd));

		Cam->SetLocation(OrthoGraphicCamerapoint - fwd * dist);
		Cam->Update();
	}
}

void UViewportManager::PersistSplitterRatios()
{
	if (!Root)
	{
		return;
	}

	// 세로
	IniSaveSharedV = static_cast<SSplitter*>(Root)->Ratio;

	auto& CFG = UConfigManager::GetInstance();
	CFG.SetSplitV(IniSaveSharedV);
	CFG.SetSplitH(IniSaveSharedH);
	CFG.SaveEditorSetting();
}

void UViewportManager::TickInput()
{
	if (!Root)
	{
		return;
	}

	auto& InputManager = UInputManager::GetInstance();
	const FVector& MousePosition = InputManager.GetMousePosition();
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


	if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
	{
		if (Target && Target->OnMouseDown(Point, 0))
		{
			Capture = Target;
		}
	}

	// Mouse move while captured (always forward to allow cross-drag detection)
	const FVector& d = InputManager.GetMouseDelta();
	if ((d.X != 0.0f || d.Y != 0.0f) && Capture)
	{
		Capture->OnMouseMove(Point);
	}

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Capture)
		{
			Capture->OnMouseUp(Point, 0);
			Capture = nullptr;
		}
	}

	if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
	{
		Capture->OnMouseUp(Point, 0);
		Capture = nullptr;
	}
}

/**
 * @brief 마우스가 컨트롤 중인 Viewport의 인덱스를 반환하는 함수
 * @return 기본적으로는 index를 반환, 아무 것도 만지지 않았다면 -1
 */
int32 UViewportManager::GetViewportIndexUnderMouse() const
{
	const auto& InputManager = UInputManager::GetInstance();

	const FVector& MousePosition = InputManager.GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositionY = static_cast<LONG>(MousePosition.Y);

	for (int32 i = 0; i < static_cast<int8>(Viewports.size()); ++i)
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
	if (Index < 0 || Index >= static_cast<int32>(Viewports.size())) return false;
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
	const float LocalX = (MousePosition.X - static_cast<float>(Rect.X));
	const float LocalY = (MousePosition.Y - RenderAreaY);
	const float Width = static_cast<float>(Rect.W);
	const float Height = RenderAreaHeight;

	OutNdcX = (LocalX / Width) * 2.0f - 1.0f;
	OutNdcY = 1.0f - (LocalY / Height) * 2.0f;
	return true;
}
