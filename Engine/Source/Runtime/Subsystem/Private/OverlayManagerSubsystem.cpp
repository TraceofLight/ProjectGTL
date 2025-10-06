#include "pch.h"
#include "Runtime/Subsystem/Public/OverlayManagerSubsystem.h"
#include "Runtime/Core/Public/EngineStatics.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Global/Memory.h"
#include <psapi.h>

// memory update
static constexpr float MEMORY_UPDATE_INTERVAL = 1.0f;

// overlay display position
static constexpr float OVERLAY_MARGIN_X = 20.0f;
static constexpr float OVERLAY_MARGIN_Y = 60.0f;
static constexpr float OVERLAY_LINE_HEIGHT = 25.0f;

TArray<FString> TypeNames = {"None", "FPS", "Memory", "All"};

IMPLEMENT_CLASS(UOverlayManagerSubsystem, UEngineSubsystem)

/**
 * @brief OverlayManagerSubsystem 생성자
 */
UOverlayManagerSubsystem::UOverlayManagerSubsystem()
{
	UE_LOG("OverlayManagerSubsystem: Overlay 서브시스템 생성");
}

/**
 * @brief 오버레이 매니저 서브시스템 초기화
 */
void UOverlayManagerSubsystem::Initialize()
{
	UE_LOG("OverlayManagerSubsystem: 초기화 시작");
	Super::Initialize();

	// 지연 초기화: Renderer가 준비될 때까지 Direct2D 초기화를 연기 (subsystem에 존재하기 때문)
	// TODO(KHJ): Slate 기반 시스템으로 이동시킬 것
	UE_LOG("OverlayManagerSubsystem: 지연 초기화 방식 사용: 최초 렌더링에서 Direct2D 초기화 예정");
	UE_LOG_SUCCESS("OverlayManagerSubsystem: 기본 초기화 완료");
}

/**
 * @brief 오버레이 매니저 서브시스템 정리
 */
void UOverlayManagerSubsystem::Deinitialize()
{
	ReleaseDirectWrite();
	ReleaseDirect2D();

	Super::Deinitialize();
	UE_LOG_SUCCESS("OverlayManagerSubsystem: 정리 완료");
}

/**
 * @brief 매 프레임 업데이트 함수
 */
void UOverlayManagerSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFPS();
	UpdateMemoryInfo();
}

/**
 * @brief 오버레이 타입 설정
 */
void UOverlayManagerSubsystem::SetOverlayType(EOverlayType InType)
{
	CurrentOverlayType = InType;

	UE_LOG("OverlayManagerSubsystem: 오버레이 타입 변경: %s", TypeNames[static_cast<int>(InType)].data());
}

/**
 * @brief stat 명령어를 처리하는 함수
 * 따로 예외에 대한 처리는 진행하지 않도록 구현... 시간 부족
 */
void UOverlayManagerSubsystem::ProcessStatCommand(const FString& InCommand)
{
	FString CommandLower = InCommand;
	std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower);

	if (CommandLower == "stat fps")
	{
		if (CurrentOverlayType == EOverlayType::Memory)
		{
			SetOverlayType(EOverlayType::All);
		}
		else
		{
			SetOverlayType(EOverlayType::FPS);
		}
	}
	else if (CommandLower == "stat memory")
	{
		if (CurrentOverlayType == EOverlayType::FPS)
		{
			SetOverlayType(EOverlayType::All);
		}
		else
		{
			SetOverlayType(EOverlayType::Memory);
		}
	}
	else if (CommandLower == "stat all")
	{
		SetOverlayType(EOverlayType::All);
	}
	else if (CommandLower == "stat none")
	{
		SetOverlayType(EOverlayType::None);
	}
}

/**
 * @brief 오버레이 렌더링 처리 진행하는 함수
 * 내부적으로 DrawText 호출을 진행
 */
void UOverlayManagerSubsystem::RenderOverlay()
{
	if (CurrentOverlayType == EOverlayType::None)
	{
		return;
	}

	// 지연 초기화: 처음 렌더링 시점에 Direct2D 초기화 수행
	if (!D2DRenderTarget)
	{
		static bool bInitializationAttempted = false;
		if (!bInitializationAttempted)
		{
			UE_LOG("OverlayManagerSubsystem: 첫 렌더링: 지연 Direct2D 초기화 시도");
			bInitializationAttempted = true;

			// Direct2D 초기화 시도
			if (!InitializeDirect2D())
			{
				UE_LOG_ERROR("OverlayManagerSubsystem: 지연 Direct2D 초기화 실패");
				return;
			}

			// DirectWrite 초기화 시도
			if (!InitializeDirectWrite())
			{
				UE_LOG_ERROR("OverlayManagerSubsystem: 지연 DirectWrite 초기화 실패");
				return;
			}

			UE_LOG_SUCCESS(
				"OverlayManagerSubsystem: 지연 초기화 성공 (D2DRenderTarget: 0x%p)", static_cast<void*>(D2DRenderTarget));
		}
		else
		{
			// 이미 초기화를 시도했지만 실패한 경우
			return;
		}
	}

	// 여전히 D2DRenderTarget이 null이면 초기화 실패
	if (!D2DRenderTarget)
	{
		return;
	}

	D2DRenderTarget->BeginDraw();

	switch (CurrentOverlayType)
	{
	case EOverlayType::FPS:
		RenderFPSOverlay();
		break;

	case EOverlayType::Memory:
		RenderMemoryOverlay();
		break;

	case EOverlayType::All:
		RenderFPSOverlay();
		RenderMemoryOverlay();
		break;

	default:
		break;
	}

	HRESULT ResultHandle = D2DRenderTarget->EndDraw();
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: Direct2D 렌더링 실패: 0x%08lX", ResultHandle);
		if (ResultHandle == D2DERR_RECREATE_TARGET)
		{
			UE_LOG("OverlayManagerSubsystem: 렌더 타겟 재생성 필요");
		}
	}
	else
	{
		// if (renderCount <= 5)
		// {
		// 	UE_LOG("OverlayManagerSubsystem: 렌더링 성공");
		// }
	}
}

/**
 * @brief Direct2D 초기화 진행 함수
 */
bool UOverlayManagerSubsystem::InitializeDirect2D()
{
	UE_LOG("OverlayManagerSubsystem: Direct2D 초기화 시작 (DXGI Surface 상호운용)");

	// Renderer에서 DeviceResources 가져오기
	auto& Renderer = URenderer::GetInstance();

	auto* DeviceResources = Renderer.GetDeviceResources();
	if (!DeviceResources)
	{
		UE_LOG("OverlayManagerSubsystem: DeviceResources를 찾을 수 없습니다");
		return false;
	}

	// Direct2D Factory 생성
	HRESULT ResultHandle = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: D2D1Factory 생성 실패: 0x%08X", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: D2D1Factory 생성 성공");

	// Swapchain 가져오기
	IDXGISwapChain* SwapChain = Renderer.GetSwapChain();
	if (!SwapChain)
	{
		UE_LOG("OverlayManagerSubsystem: SwapChain을 찾을 수 없습니다");
		return false;
	}

	// DXGI Surface 가져오기
	IDXGISurface* DxgiBackBuffer = nullptr;
	ResultHandle = SwapChain->GetBuffer(0, IID_PPV_ARGS(&DxgiBackBuffer));
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DXGI 백버퍼 가져오기 실패: 0x%08X", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DXGI 백버퍼 얻기 성공");

	// Direct2D RenderTarget 속성 설정
	D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		96.0f, // dpiX
		96.0f // dpiY
	);

	// DXGI Surface와 연결된 Direct2D RenderTarget 생성
	UE_LOG("OverlayManagerSubsystem: DXGI Surface와 Direct2D RenderTarget 연결 시도 중...");
	ResultHandle = D2DFactory->CreateDxgiSurfaceRenderTarget(
		DxgiBackBuffer,
		&RenderTargetProperties,
		&D2DRenderTarget
	);

	// DXGI Surface 해제 (참조 카운트 감소)
	DxgiBackBuffer->Release();

	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DXGI Surface RenderTarget 생성 실패: 0x%08lX", ResultHandle);
		switch (ResultHandle)
		{
		case E_INVALIDARG:
			UE_LOG("OverlayManagerSubsystem: 잘못된 인수 (E_INVALIDARG)");
			break;
		case E_OUTOFMEMORY:
			UE_LOG("OverlayManagerSubsystem: 메모리 부족 (E_OUTOFMEMORY)");
			break;
		case D2DERR_UNSUPPORTED_PIXEL_FORMAT:
			UE_LOG("OverlayManagerSubsystem: 지원되지 않는 픽셀 포맷");
			break;
		default:
			UE_LOG("OverlayManagerSubsystem: 알 수 없는 오류: 0x%08lX", ResultHandle);
			break;
		}
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DXGI Surface RenderTarget 생성 성공");

	// 텍스트 브러시 생성
	UE_LOG("OverlayManagerSubsystem: TextBrush 생성 시도 중...");
	ResultHandle = D2DRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		&TextBrush
	);

	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: TextBrush 생성 실패: 0x%08lX", ResultHandle);
		return false;
	}

	UE_LOG("OverlayManagerSubsystem: TextBrush 생성 성공 (Brush: 0x%p)", static_cast<void*>(TextBrush));
	UE_LOG_SUCCESS("OverlayManagerSubsystem: Direct2D 초기화 완료");

	return true;
}

/**
 * @brief DirectWrite 초기화 진행 함수
 */
bool UOverlayManagerSubsystem::InitializeDirectWrite()
{
	UE_LOG("OverlayManagerSubsystem: DirectWrite 초기화 시작");

	// DirectWrite Factory 생성
	HRESULT ResultHandle = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&DWriteFactory)
	);
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: DWriteFactory 생성 실패: 0x%08X", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: DWriteFactory 생성 성공");

	// 텍스트 포맷 생성
	UE_LOG("OverlayManagerSubsystem: TextFormat 생성 시도 중...");
	ResultHandle = DWriteFactory->CreateTextFormat(
		L"Arial", // font family
		nullptr, // font collection
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		18.0f, // font size
		L"ko-kr", // locale
		&TextFormat
	);
	if (FAILED(ResultHandle))
	{
		UE_LOG("OverlayManagerSubsystem: TextFormat 생성 실패: 0x%08lX", ResultHandle);
		return false;
	}
	UE_LOG("OverlayManagerSubsystem: TextFormat 생성 성공");

	return true;
}

/**
 * @brief Direct2D 리소스 해제하는 함수
 */
void UOverlayManagerSubsystem::ReleaseDirect2D()
{
	if (TextBrush)
	{
		TextBrush->Release();
		TextBrush = nullptr;
	}

	if (D2DRenderTarget)
	{
		D2DRenderTarget->Release();
		D2DRenderTarget = nullptr;
	}

	if (D2DFactory)
	{
		D2DFactory->Release();
		D2DFactory = nullptr;
	}
}

/**
 * @brief DirectWrite 리소스 해제하는 함수
 */
void UOverlayManagerSubsystem::ReleaseDirectWrite()
{
	if (TextFormat)
	{
		TextFormat->Release();
		TextFormat = nullptr;
	}

	if (DWriteFactory)
	{
		DWriteFactory->Release();
		DWriteFactory = nullptr;
	}
}

/**
 * @brief FPS 업데이트 함수
 */
void UOverlayManagerSubsystem::UpdateFPS()
{
	// FPSWidget과 동일한 방식으로 FPS 계산
	if (GDeltaTime > 0.0f)
	{
		// FPS 샘플 배열에 추가
		FrameSpeedSamples[FrameSpeedSampleIndex] = 1.0f / GDeltaTime;
		FrameSpeedSampleIndex = (FrameSpeedSampleIndex + 1) % FPS_SAMPLE_COUNT;
	}

	// 평균 FPS 계산
	float FPSSum = 0.0f;
	int32 ValidSampleCount = 0;

	for (int32 i = 0; i < FPS_SAMPLE_COUNT; ++i)
	{
		if (FrameSpeedSamples[i] > 0.0f)
		{
			FPSSum += FrameSpeedSamples[i];
			++ValidSampleCount;
		}
	}

	if (ValidSampleCount > 0)
	{
		CurrentFPS = FPSSum / ValidSampleCount;
	}
}

/**
 * @brief 메모리 정보 업데이트를 진행하는 함수
 */
void UOverlayManagerSubsystem::UpdateMemoryInfo()
{
	MemoryUpdateTimer += GDeltaTime;

	// 프로세스 메모리 정보
	if (MemoryUpdateTimer >= MEMORY_UPDATE_INTERVAL)
	{
		PROCESS_MEMORY_COUNTERS_EX ProcessMemoryCounters;
		if (GetProcessMemoryInfo(GetCurrentProcess(),
		                         reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&ProcessMemoryCounters),
		                         sizeof(ProcessMemoryCounters)))
		{
			CurrentMemoryUsage = ProcessMemoryCounters.WorkingSetSize;
			PeakMemoryUsage = max(CurrentMemoryUsage, PeakMemoryUsage);
		}

		// 동적 할당 정보 업데이트
		CurrentAllocationCount = TotalAllocationCount;
		CurrentAllocationBytes = TotalAllocationBytes;

		MemoryUpdateTimer = 0.0f;
	}
}

/**
 * @brief FPS 오버레이 렌더링 함수
 */
void UOverlayManagerSubsystem::RenderFPSOverlay() const
{
	static int fpsRenderCount = 0;
	if (fpsRenderCount < 3)
	{
		UE_LOG("OverlayManagerSubsystem: FPS 오버레이 렌더링 (TextBrush: 0x%p, TextFormat: 0x%p)", TextBrush, TextFormat);
		fpsRenderCount++;
	}

	if (!TextBrush || !TextFormat)
	{
		if (fpsRenderCount <= 3)
		{
			UE_LOG("OverlayManagerSubsystem: TextBrush 또는 TextFormat이 null입니다");
		}
		return;
	}

	wchar_t FPSInfoText[64];
	(void)swprintf_s(FPSInfoText, L"FPS: %.1f", CurrentFPS);

	uint32 RenderColor = GetFPSColor(CurrentFPS);
	DrawText(FPSInfoText, OVERLAY_MARGIN_X, OVERLAY_MARGIN_Y, RenderColor);
}

/**
 * @brief 메모리 오버레이 렌더링 함수
 */
void UOverlayManagerSubsystem::RenderMemoryOverlay() const
{
	if (!TextBrush || !TextFormat)
		return;

	float yOffset = (CurrentOverlayType == EOverlayType::All)
		                ? OVERLAY_MARGIN_Y + OVERLAY_LINE_HEIGHT
		                : OVERLAY_MARGIN_Y;

	// 프로세스 메모리 정보
	FString TotalCurrentMemory = GetMemorySizeString(CurrentMemoryUsage);
	FString TotalPeakMemory = GetMemorySizeString(PeakMemoryUsage);

	wchar_t ProcessMemoryInfoText[128];
	(void)swprintf_s(ProcessMemoryInfoText, L"Process: %s / Peak: %s",
	                 wstring(TotalCurrentMemory.begin(), TotalCurrentMemory.end()).data(),
	                 wstring(TotalPeakMemory.begin(), TotalPeakMemory.end()).data());

	DrawText(ProcessMemoryInfoText, OVERLAY_MARGIN_X, yOffset, 0xFF00FF00); // 초록색

	// 동적 할당 정보
	float AllocationKB = static_cast<float>(CurrentAllocationBytes / KILO);

	wchar_t HeapAllocText[128];
	(void)swprintf_s(HeapAllocText,
	                 L"동적할당 정보: %u objs, %.1f KB", CurrentAllocationCount, AllocationKB);

	// 청록색
	DrawText(HeapAllocText, OVERLAY_MARGIN_X, yOffset + OVERLAY_LINE_HEIGHT, 0xFF00FFFF);
}

/**
 * @brief 텍스트 렌더링 함수
 * 내부적으로 D2D Render의 동명 함수를 호출
 */
void UOverlayManagerSubsystem::DrawText(const wchar_t* InText, float InX, float InY, uint32 InColor) const
{
	if (!D2DRenderTarget || !TextBrush || !TextFormat)
		return;

	// 색상 설정
	D2D1_COLOR_F color;
	color.a = ((InColor >> 24) & 0xFF) / 255.0f;
	color.r = ((InColor >> 16) & 0xFF) / 255.0f;
	color.g = ((InColor >> 8) & 0xFF) / 255.0f;
	color.b = (InColor & 0xFF) / 255.0f;

	TextBrush->SetColor(color);

	// 텍스트 렌더링 영역 설정
	D2D1_RECT_F layoutRect = D2D1::RectF(InX, InY, InX + 400.0f, InY + 50.0f);

	D2DRenderTarget->DrawText(
		InText,
		static_cast<UINT32>(wcslen(InText)),
		TextFormat,
		&layoutRect,
		TextBrush
	);
}

/**
 * @brief 메모리 크기를 문자열로 변환하는 함수
 */
FString UOverlayManagerSubsystem::GetMemorySizeString(size_t InBytes)
{
	if (InBytes >= GIGA)
	{
		return to_string(InBytes / MEGA) + " MB"; // GB는 너무 큰 단위이므로 MB로 표시
	}
	else if (InBytes >= MEGA)
	{
		return to_string(InBytes / MEGA) + " MB";
	}
	else if (InBytes >= KILO)
	{
		return to_string(InBytes / KILO) + " KB";
	}
	else
	{
		return to_string(InBytes) + " B";
	}
}

/**
 * @brief FPS 수치에 따른 적절한 색상 반환을 담당하는 함수
 */
uint32 UOverlayManagerSubsystem::GetFPSColor(float InFPS)
{
	if (InFPS >= 60.0f)
	{
		return 0xFF00FF00; // 녹색 (우수)
	}
	else if (InFPS >= 30.0f)
	{
		return 0xFFFFFF00; // 노란색 (보통)
	}
	else
	{
		return 0xFFFF0000; // 빨간색 (주의)
	}
}
