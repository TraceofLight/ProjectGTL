#include "pch.h"
#include "Runtime/RHI/Public/D3D11RHIModule.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Core/Public/AppWindow.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

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

	// 스왑체인 설정
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Width = 1920;
	SwapChainDesc.BufferDesc.Height = 1080;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = GetWindowHandle();
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Windowed = TRUE;

	// DirectX 디바이스, 컨텍스트, 스왑체인 생성
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
#ifdef _DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
#else
		0,
#endif
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
