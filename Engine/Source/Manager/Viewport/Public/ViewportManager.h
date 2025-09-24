#pragma once
#include "Runtime/Core/Public/Object.h"

class SWindow;
class SSplitter;
class FAppWindow;
class UCamera;
class FViewport;
class FViewportClient;

UCLASS()
class UViewportManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UViewportManager, UObject)

public:
	void Initialize(FAppWindow* InWindow);
	void Update();
	void RenderOverlay();
	void Release();

	// 마우스 입력을 스플리터/윈도우 트리에 라우팅 (드래그/리사이즈 처리)
	void TickInput();

	// 레이아웃
	void BuildSingleLayout(int32 PromoteIdx = -1);
	void BuildFourSplitLayout();

	// 루트 접근
	void SetRoot(SWindow* InRoot) { Root = InRoot; }
	SWindow* GetRoot() const { return Root; }

	// 리프 Rect 수집
	void GetLeafRects(TArray<FRect>& OutRects) const;

	// 현재 마우스가 위치한 뷰포트 인덱스(-1이면 없음)
	int32 GetViewportIndexUnderMouse() const;

	// 주어진 뷰포트 인덱스 기준으로 로컬 NDC 계산(true면 성공)
	bool ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const;

	TArray<FViewport*>& GetViewports() { return Viewports; }
	TArray<FViewportClient*>& GetClients() { return Clients; }

	// ViewportChange 상태 접근자
	EViewportChange GetViewportChange() const { return ViewportChange; }
	void SetViewportChange(EViewportChange InChange) { ViewportChange = InChange; }

	// 스플리터 비율 저장
	void PersistSplitterRatios();

private:
	// 내부 유틸
	void SyncRectsToViewports() const; // 리프Rect → Viewport.Rect
	void PumpAllViewportInput() const; // 각 뷰포트 → 클라 입력 전달
	void TickCameras() const; // 카메라 업데이트 일원화(공유 오쇼 1회)
	void UpdateActiveRmbViewportIndex(); // 우클릭 드래그 대상 뷰포트 인덱스 계산

	void InitializeViewportAndClient();
	void InitializeOrthoGraphicCamera();
	void InitializePerspectiveCamera();

	void UpdateOrthoGraphicCameraPoint();

	void UpdateOrthoGraphicCameraFov() const;

	void BindOrthoGraphicCameraToClient() const;

	void ForceRefreshOrthoViewsAfterLayoutChange();

	void ApplySharedOrthoCenterToAllCameras() const;

private:
	SWindow* Root = nullptr;
	SWindow* Capture = nullptr;

	// App main window for querying current client size each frame
	FAppWindow* AppWindow = nullptr;

	TArray<FViewport*> Viewports;
	TArray<FViewportClient*> Clients;

	TArray<UCamera*> OrthoGraphicCameras;
	TArray<UCamera*> PerspectiveCameras;

	TArray<FVector> InitialOffsets;

	FVector OrthoGraphicCamerapoint{0.0f, 0.0f, 0.0f};

	// 현재 우클릭(카메라 제어) 입력이 적용될 뷰포트 인덱스 (-1이면 없음)
	int32 ActiveRmbViewportIdx = -1;

	EViewportChange ViewportChange = EViewportChange::Single;

	float SharedFovY = 150.0f;
	float SharedY = 0.5f;

	float IniSaveSharedV = 0.5f;
	float IniSaveSharedH = 0.5f;

	float SplitterValueV = 0.5f;
	float SplitterValueH = 0.5f;

	int32 LastPromotedIdx = -1;
};

// UE 스타일 아이콘 타입
enum class EUEViewportIcon : uint8
{
	Single,
	Quad
};

struct FUEImgui
{
	// 팔레트 (언리얼 톤 느낌)
	static ImU32 ColBG() { return IM_COL32(45, 45, 45, 255); }
	static ImU32 ColBGHover() { return IM_COL32(58, 58, 58, 255); }
	static ImU32 ColBGActive() { return IM_COL32(74, 74, 74, 255); }
	static ImU32 ColBorder() { return IM_COL32(96, 96, 96, 255); }
	static ImU32 ColBorderHot() { return IM_COL32(110, 110, 110, 255); }
	static ImU32 ColIcon() { return IM_COL32(205, 205, 205, 255); }
	static ImU32 ColIconActive() { return IM_COL32(46, 163, 255, 255); } // 액티브 포커스 블루

	// 언리얼풍 툴바 아이콘 버튼
	static bool ToolbarIconButton(const char* InId, EUEViewportIcon InIcon, bool bInActive,
	                              const ImVec2& InSize = ImVec2(32.f, 22.f), float InRounding = 6.f)
	{
		ImGui::PushID(InId);
		ImGui::InvisibleButton("##btn", InSize);
		bool bPressed = ImGui::IsItemClicked();
		bool bHovered = ImGui::IsItemHovered();

		ImDrawList* DL = ImGui::GetForegroundDrawList();
		ImVec2 Min = ImGui::GetItemRectMin();
		ImVec2 Max = ImGui::GetItemRectMax();

		// 배경
		ImU32 bg = bInActive ? ColBGActive() : (bHovered ? ColBGHover() : ColBG());
		DL->AddRectFilled(Min, Max, bg, InRounding);

		// 외곽 테두리
		ImU32 bd = bHovered ? ColBorderHot() : ColBorder();
		DL->AddRect(Min, Max, bd, InRounding, 0, 1.0f);

		// 콘텐츠 패딩
		const float Pad = 5.f;
		ImVec2 CMin = ImVec2(Min.x + Pad, Min.y + Pad);
		ImVec2 CMax = ImVec2(Max.x - Pad, Max.y - Pad);

		// 아이콘 색
		ImU32 ic = bInActive ? ColIconActive() : ColIcon();

		// 아이콘 그리기
		switch (InIcon)
		{
		case EUEViewportIcon::Single:
			{
				// 안쪽 얇은 사각 (언리얼 뷰포트 단일 레이아웃 아이콘 느낌)
				const float r = 3.f;
				DL->AddRect(CMin, CMax, ic, r, 0, 1.5f);
			}
			break;
		case EUEViewportIcon::Quad:
			{
				// 2x2 그리드
				ImVec2 C = ImVec2((CMin.x + CMax.x) * 0.5f, (CMin.y + CMax.y) * 0.5f);
				DL->AddLine(ImVec2(CMin.x, C.y), ImVec2(CMax.x, C.y), ic, 1.2f);
				DL->AddLine(ImVec2(C.x, CMin.y), ImVec2(C.x, CMax.y), ic, 1.2f);
				// 바깥 테두리도 살짝
				DL->AddRect(CMin, CMax, ic, 3.f, 0, 1.2f);
			}
			break;
		default: break;
		}
		ImGui::PopID();
		return bPressed;
	}
};
