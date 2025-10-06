#pragma once

class SWindow;
class SSplitter;
class FAppWindow;
class ACameraActor;
class FViewport;
class FViewportClient;

/**
 * @brief 뷰포트 관리 서브시스템
 * 기존 UViewportManager의 모든 기능을 서브시스템 패턴으로 이관
 * 뷰포트 레이아웃 관리 (Single/Quad)
 * 스플리터 시스템
 * 뷰포트 애니메이션
 * 카메라 관리 (Orthographic/Perspective)
 * 입력 처리 및 라우팅
 */
UCLASS()
class UViewportSubsystem :
	public UEngineSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS(UViewportSubsystem, UEngineSubsystem)

public:
	UViewportSubsystem();
	~UViewportSubsystem() override;

	// ISubsystem 인터페이스 구현
	void Initialize() override;
	void PostInitialize() override;
	void Deinitialize() override;
	bool IsTickable() const override { return true; }
	void Tick() override;

	/**
	 * @brief 뷰포트 서브시스템 초기화
	 * @param InWindow 애플리케이션 윈도우
	 */
	void InitializeViewportSystem(FAppWindow* InWindow);

	/**
	 * @brief 매 프레임 업데이트
	 */
	void Update();

	/**
	 * @brief 오버레이 렌더링 (ImGui 등)
	 */
	void RenderOverlay();

	/**
	 * @brief 서브시스템 해제
	 */
	void Release();

	/**
	 * @brief 마우스 입력을 스플리터/윈도우 트리에 라우팅 (드래그/리사이즈 처리)
	 */
	void TickInput();

	// 레이아웃 관리
	/**
	 * @brief 단일 뷰포트 레이아웃 구성
	 * @param PromoteIdx 승격할 뷰포트 인덱스 (-1이면 현재 활성 뷰포트)
	 */
	void BuildSingleLayout(int32 PromoteIdx = -1);

	/**
	 * @brief 4분할 뷰포트 레이아웃 구성
	 */
	void BuildFourSplitLayout();

	// 루트 접근
	void SetRoot(SWindow* InRoot) { Root = InRoot; }
	SWindow* GetRoot() const { return Root; }

	/**
	 * @brief OrthoGraphicCamera의 중심점을 외부에서 설정하는 함수
	 */
	void SetOrthoGraphicCenter(const FVector& NewCenter);

	/**
	 * @brief 리프 Rect 수집
	 */
	void GetLeafRects(TArray<FRect>& OutRects) const;

	/**
	 * @brief 현재 마우스가 위치한 뷰포트 인덱스(-1이면 없음)
	 */
	int32 GetViewportIndexUnderMouse() const;

	/**
	 * @brief 주어진 뷰포트 인덱스 기준으로 로컬 NDC 계산(true면 성공)
	 */
	bool ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const;

	TArray<FViewport*>& GetViewports() { return Viewports; }
	TArray<FViewportClient*>& GetClients() { return Clients; }

	// 카메라 배열 접근자
	const TArray<ACameraActor*>& GetOrthographicCameras() const { return OrthoGraphicCameras; }
	const TArray<ACameraActor*>& GetPerspectiveCameras() const { return PerspectiveCameras; }

	// ViewportChange 상태 접근자
	EViewportChange GetViewportChange() const { return ViewportChange; }
	void SetViewportChange(EViewportChange InChange) { ViewportChange = InChange; }

	/**
	 * @brief 스플리터 비율 저장
	 */
	void PersistSplitterRatios();

	// 애니메이션 시스템 공개 인터페이스
	bool IsAnimating() const { return ViewportAnimation.bIsAnimating; }
	void StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex = -1);

private:
	// 내부 유틸
	void SyncRectsToViewports() const; // 리프Rect → Viewport.Rect
	void SyncAnimationRectsToViewports() const; // 애니메이션 중 리프Rect → Viewport.Rect
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

	// 애니메이션 시스템 내부 함수들
	void StartViewportAnimation(bool bSingleToQuad, int32 PromoteIdx = -1);
	void UpdateViewportAnimation();
	float EaseInOutCubic(float t) const;
	void CreateAnimationSplitters();
	void AnimateSplitterTransition(float Progress);
	void RestoreOriginalLayout();
	void FinalizeSingleLayoutFromAnimation();
	void FinalizeFourSplitLayoutFromAnimation();

private:
	SWindow* Root = nullptr;
	SWindow* Capture = nullptr;

	// App main window for querying current client size each frame
	FAppWindow* AppWindow = nullptr;

	TArray<FViewport*> Viewports;
	TArray<FViewportClient*> Clients;

	TArray<ACameraActor*> OrthoGraphicCameras;
	TArray<ACameraActor*> PerspectiveCameras;

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

	// Viewport Animation System
	struct FViewportAnimation
	{
		bool bIsAnimating = false;
		float AnimationTime = 0.0f;
		float AnimationDuration = 0.5f; // 애니메이션 지속 시간 (초)

		bool bSingleToQuad = true; // true: Single→Quad, false: Quad→Single

		// SWindow 트리 백업 및 복원을 위한 데이터
		SWindow* BackupRoot = nullptr;
		SWindow* AnimationRoot = nullptr; // 애니메이션용 임시 루트

		// 스플리터 비율 정보
		float CurrentVerticalRatio = 0.5f; // 현재 수직 스플리터 비율
		float CurrentHorizontalRatio = 0.5f; // 현재 수평 스플리터 비율
		float StartRatio = 0.5f; // 시작 스플리터 비율
		float TargetRatio = 0.5f; // 목표 스플리터 비율
		int32 PromotedViewportIndex = 0;
	};

	FViewportAnimation ViewportAnimation;
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

		ImDrawList* DL = ImGui::GetWindowDrawList();
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
