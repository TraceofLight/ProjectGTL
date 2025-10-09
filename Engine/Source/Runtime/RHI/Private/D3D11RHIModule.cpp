#include "pch.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Core/Public/AppWindow.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"

void FD3D11RHIModule::StartupModule()
{
	UE_LOG("FD3D11RHIModule: StartupModule 시작");

	// DirectX 11 디바이스와 스왑체인 생성
	if (!CreateD3D11DeviceAndSwapChain())
	{
		UE_LOG_ERROR("FD3D11RHIModule: DirectX 11 디바이스 생성 실패");
		return;
	}

	UE_LOG_SUCCESS("FD3D11RHIModule: StartupModule 완료");
}

void FD3D11RHIModule::ShutdownModule()
{
	UE_LOG("FD3D11RHIModule: ShutdownModule 시작");

	// EditorRenderResources 정리
	if (EditorResources)
	{
		EditorResources->Shutdown();
		EditorResources.Reset();
	}

	// GDynamicRHI 정리
	if (GDynamicRHI)
	{
		GDynamicRHI->Shutdown();
		delete GDynamicRHI;
		GDynamicRHI = nullptr;
	}

	UE_LOG("FD3D11RHIModule: ShutdownModule 완료");
}

/**
 * @brief DirectX 11 디바이스와 컨텍스트, 스왑체인을 생성하는 함수
 * @return 성공 시 true, 실패 시 false
 */
bool FD3D11RHIModule::CreateD3D11DeviceAndSwapChain()
{
	UE_LOG("FD3D11RHIModule: DirectX 11 디바이스 생성 시작");

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	// DirectX 기능 수준 정의
	D3D_FEATURE_LEVEL FeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	// 스왑체인 설정 (더블 버퍼링 + FLIP 모델)
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = 0;  // 창 크기에 맞게 자동 설정
	SwapChainDesc.BufferDesc.Height = 0;  // 창 크기에 맞게 자동 설정
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // CGEngine과 동일
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;  // 기본값 사용
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;  // 기본값 사용
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = GetWindowHandle();
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferCount = 2;  // 더블 버퍼링
	SwapChainDesc.Windowed = TRUE;

	// DirectX 디바이스, 컨텍스트, 스왑체인 생성
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  // FLIP 모델을 위해 필요
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		FeatureLevels,
		ARRAYSIZE(FeatureLevels),
		D3D11_SDK_VERSION,
		&SwapChainDesc,
		&SwapChain,
		&Device,
		nullptr,
		&DeviceContext
	);

	if (FAILED(hr))
	{
		UE_LOG_ERROR("FD3D11RHIModule: D3D11CreateDeviceAndSwapChain 실패 (HRESULT: 0x%08lX)", hr);
		return false;
	}

	// FRHIDevice 인스턴스 생성 및 초기화
	FRHIDevice* RHIDevice = new FRHIDevice;
	RHIDevice->Initialize(Device, DeviceContext);
	RHIDevice->SetSwapChain(SwapChain);

	// 전역 RHI 인스턴스 설정
	GDynamicRHI = RHIDevice;

	// BackBuffer RenderTargetView 생성
	if (!RHIDevice->CreateBackBufferRTV())
	{
		UE_LOG_ERROR("FD3D11RHIModule: BackBuffer RTV 생성 실패");
		return false;
	}

	// EditorRenderResources 초기화
	EditorResources = MakeUnique<FEditorRenderResources>();
	EditorResources->Initialize(RHIDevice);

	UE_LOG_SUCCESS("FD3D11RHIModule: DirectX 11 디바이스 생성 완료");
	return true;
}

/**
 * @brief 적절한 윈도우 핸들을 가져오는 함수
 * @return 윈도우 핸들
 */
HWND FD3D11RHIModule::GetWindowHandle()
{
	// GEngine을 통해 AppWindow 핸들 가져오기
	if (GEngine && GEngine->GetAppWindow())
	{
		return GEngine->GetAppWindow()->GetWindowHandle();
	}

	// 대체 수단
	return GetActiveWindow();
}

FD3D11RHIModule& FD3D11RHIModule::GetInstance()
{
	return FModuleManager::GetInstance().LoadModuleChecked<FD3D11RHIModule>("D3D11RHI");
}

bool FD3D11RHIModule::RecreateAfterDeviceLost()
{
	UE_LOG_ERROR("FD3D11RHIModule: DEVICE LOST detected. Recreating D3D11 device and swap chain...");

	// 1) Editor 리소스 정리
	if (EditorResources)
	{
		EditorResources->Shutdown();
		EditorResources.Reset();
	}

	// 2) 기존 RHI 정리
	if (GDynamicRHI)
	{
		GDynamicRHI->Shutdown();
		delete GDynamicRHI;
		GDynamicRHI = nullptr;
	}

	// 3) 새 디바이스/스왑체인 생성
	if (!CreateD3D11DeviceAndSwapChain())
	{
		UE_LOG_ERROR("FD3D11RHIModule: RecreateAfterDeviceLost failed to create device/swap chain");
		return false;
	}

	// 4) ImGui DX11 백엔드만 재바인딩 (UIWindow 상태 보존)
	if (GEngine)
	{
		if (auto* UISubsystem = GEngine->GetEngineSubsystem<UUISubsystem>())
		{
			UISubsystem->OnGraphicsDeviceRecreated();
		}
	}

	UE_LOG_SUCCESS("FD3D11RHIModule: Device recreation complete");
	return true;
}
