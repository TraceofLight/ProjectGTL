#pragma once
#include "Core/Public/Object.h"

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

    // 마우스 입력을 스플리터/윈도우 트리에 라우팅 (드래그/리사이즈 처리)
    void TickInput();

	// 레이아웃
    void BuildSingleLayout();
    void BuildFourSplitLayout();

	// 루트 접근
    void SetRoot(SWindow* InRoot);
    SWindow* GetRoot();

	// 리프 Rect 수집
	void GetLeafRects(TArray<FRect>& OutRects);

	// 공유 오쏘 카메라(읽기 전용 포인터를 클라에 주입)
    UCamera* GetOrthographicCamera() const { return OrthographicCamera; }

    TArray<FViewport*>& GetViewports() { return Viewports; }
    TArray<FViewportClient*>& GetClients() { return Clients; }

private:
	// 내부 유틸
    void CreateViewportsAndClients(int32 InCount); // 싱글(1) 또는 쿼드(4)
    void SyncRectsToViewports();                   // 리프Rect → Viewport.Rect
    void PumpAllViewportInput();                   // 각 뷰포트 → 클라 입력 전달
    void TickCameras(float InDeltaSeconds);          // 카메라 업데이트 일원화(공유 오쏘 1회)
    void UpdateActiveRmbViewportIndex();            // 우클릭 드래그 대상 뷰포트 인덱스 계산

private:
    SWindow* Root = nullptr;
    SWindow* Capture = nullptr;
    bool bFourSplitActive = false;

	TArray<FViewport*>       Viewports;
	TArray<FViewportClient*> Clients;

    UCamera* OrthographicCamera = nullptr;

    // 프레임 타이밍
    double LastTime = 0.0;

    // 현재 우클릭(카메라 제어) 입력이 적용될 뷰포트 인덱스 (-1이면 없음)
    int32 ActiveRmbViewportIdx = -1;
};
