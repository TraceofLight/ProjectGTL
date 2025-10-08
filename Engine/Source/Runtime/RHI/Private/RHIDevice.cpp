#include "pch.h"
#include "Runtime/RHI/Public/RHIDevice.h"

#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"

#include "Asset/Public/StaticMeshData.h"
#include "Runtime/Subsystem/Asset/Public/AssetSubsystem.h"

// 전역 RHI 인스턴스
FRHIDevice* GDynamicRHI = nullptr;

// 참고 파일에서 가져온 구조체들
struct HighLightBufferType
{
	uint32 Picked;
	FVector Color;
	uint32 X;
	uint32 Y;
	uint32 Z;
	uint32 Gizmo;
};

struct ColorBufferType
{
	FVector4 Color;
};

struct FMaterialInPs
{
	FVector DiffuseColor; // Kd
	float OpticalDensity; // Ni

	FVector AmbientColor; // Ka
	float Transparency; // Tr Or d

	FVector SpecularColor; // Ks
	float SpecularExponent; // Ns

	FVector EmissiveColor; // Ke
	uint32 IlluminationModel; // illum

	FVector TransmissionFilter; // Tf
	float dummy; // 4 bytes padding
};

struct FPixelConstBufferType
{
	FMaterialInPs Material;
	bool bHasMaterial; // 1 bytes
	bool Dummy[3]; // 3 bytes padding
	bool bHasTexture; // 1 bytes
	bool Dummy2[11]; // 11 bytes padding
};

static_assert(sizeof(FPixelConstBufferType) % 16 == 0, "PixelConstData size mismatch!");

FRHIDevice::FRHIDevice() = default;
FRHIDevice::~FRHIDevice() = default;

void FRHIDevice::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	if (!InDevice || !InDeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: Initialize 실패 - Device 또는 DeviceContext가 null입니다");
		return;
	}

	Device = InDevice;
	DeviceContext = InDeviceContext;

	// 상태 객체들 생성
	CreateRasterizerStates();
	CreateDepthStencilStates();
	CreateBlendStates();
	CreateConstantBuffer();
	CreateSamplerState();

	bIsInitialized = true;

	UE_LOG_SUCCESS("RHIDevice: 초기화 완료");
}

void FRHIDevice::Shutdown()
{
	// 상태 객체들 정리
	for (auto& Pair : RasterizerStates)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	RasterizerStates.Empty();

	for (auto& Pair : DepthStencilStates)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	DepthStencilStates.Empty();

	if (BlendStateEnabled)
	{
		BlendStateEnabled->Release();
		BlendStateEnabled = nullptr;
	}

	if (BlendStateDisabled)
	{
		BlendStateDisabled->Release();
		BlendStateDisabled = nullptr;
	}

	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}

	if (HighLightCB)
	{
		HighLightCB->Release();
		HighLightCB = nullptr;
	}

	if (ColorCB)
	{
		ColorCB->Release();
		ColorCB = nullptr;
	}

	if (PixelConstCB)
	{
		PixelConstCB->Release();
		PixelConstCB = nullptr;
	}

	if (DefaultSamplerState)
	{
		DefaultSamplerState->Release();
		DefaultSamplerState = nullptr;
	}

	SwapChain = nullptr;
	Device = nullptr;
	DeviceContext = nullptr;
	bIsInitialized = false;

	UE_LOG("RHIDevice: 종료 완료");
}

ID3D11Buffer* FRHIDevice::CreateVertexBuffer(const void* InData, uint32 InByteWidth, bool bCpuAccess)
{
	if (!bIsInitialized || !Device)
	{
		UE_LOG_ERROR("RHIDevice: VertexBuffer 생성 실패 - Device가 초기화되지 않았습니다");
		return nullptr;
	}

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = InByteWidth;
	BufferDesc.Usage = bCpuAccess ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = bCpuAccess ? D3D11_CPU_ACCESS_WRITE : 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = InData;

	ID3D11Buffer* Buffer = nullptr;
	HRESULT hr = Device->CreateBuffer(&BufferDesc, InData ? &InitData : nullptr, &Buffer);

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: VertexBuffer 생성 실패 (HRESULT: 0x%08lX)", hr);
		return nullptr;
	}

	return Buffer;
}

void FRHIDevice::ReleaseVertexBuffer(ID3D11Buffer* InBuffer)
{
	if (InBuffer)
	{
		InBuffer->Release();
	}
}

bool FRHIDevice::UpdateVertexBuffer(ID3D11Buffer* InBuffer, const void* InData, uint32 InByteWidth)
{
	if (!bIsInitialized || !DeviceContext || !InBuffer || !InData)
	{
		UE_LOG_ERROR("RHIDevice: VertexBuffer 업데이트 실패 - 잘못된 파라미터");
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(InBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: VertexBuffer Map 실패 (HRESULT: 0x%08lX)", hr);
		return false;
	}

	memcpy(MappedResource.pData, InData, InByteWidth);
	DeviceContext->Unmap(InBuffer, 0);

	return true;
}

ID3D11Buffer* FRHIDevice::CreateIndexBuffer(const void* InData, uint32 InByteWidth)
{
	if (!bIsInitialized || !Device)
	{
		UE_LOG_ERROR("RHIDevice: IndexBuffer 생성 실패 - Device가 초기화되지 않았습니다");
		return nullptr;
	}

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = InByteWidth;
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = InData;

	ID3D11Buffer* Buffer = nullptr;
	HRESULT hr = Device->CreateBuffer(&BufferDesc, &InitData, &Buffer);

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: IndexBuffer 생성 실패 (HRESULT: 0x%08lX)", hr);
		return nullptr;
	}

	return Buffer;
}

void FRHIDevice::ReleaseIndexBuffer(ID3D11Buffer* InBuffer)
{
	if (InBuffer)
	{
		InBuffer->Release();
	}
}

bool FRHIDevice::CreateVertexShaderAndInputLayout(
	const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InLayoutDesc,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	path FullPath = FPaths::GetShaderPath() / InFilePath;

	ID3DBlob* VertexShaderBlob = nullptr;
	if (!CompileShaderFromFile(FullPath.wstring(), "VSMain", "vs_5_0", &VertexShaderBlob))
	{
		return false;
	}

	HRESULT hr = Device->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);
	if (FAILED(hr))
	{
		VertexShaderBlob->Release();
		return false;
	}

	hr = Device->CreateInputLayout(InLayoutDesc.GetData(), InLayoutDesc.Num(), VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), OutInputLayout);
	VertexShaderBlob->Release();

	return SUCCEEDED(hr);
}

bool FRHIDevice::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader)
{
	std::filesystem::path FullPath = FPaths::GetShaderPath() / InFilePath;

	ID3DBlob* PixelShaderBlob = nullptr;
	if (!CompileShaderFromFile(FullPath.wstring(), "PSMain", "ps_5_0", &PixelShaderBlob))
	{
		return false;
	}

	HRESULT hr = Device->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);
	PixelShaderBlob->Release();

	return SUCCEEDED(hr);
}

void FRHIDevice::ReleaseShader(IUnknown* InShader)
{
	if (InShader)
	{
		InShader->Release();
	}
}

ID3D11ShaderResourceView* FRHIDevice::CreateTextureFromFile(const wstring& InFilePath)
{
	if (!bIsInitialized || !Device || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: Texture 생성 실패 - Device가 초기화되지 않았습니다");
		return nullptr;
	}

	// 파일 확장자 확인
	size_t ExtPos = InFilePath.find_last_of(L'.');
	if (ExtPos == wstring::npos)
	{
		UE_LOG_ERROR("RHIDevice: 확장자가 없는 파일 - %ls", InFilePath.c_str());
		return nullptr;
	}

	wstring Extension = InFilePath.substr(ExtPos);
	transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT hr;

	try
	{
		if (Extension == L".dds")
		{
			hr = DirectX::CreateDDSTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr,
				&TextureSRV);

			if (SUCCEEDED(hr))
			{
				UE_LOG_SUCCESS("RHIDevice: DDS 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("RHIDevice: DDS 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)",
					InFilePath.c_str(), hr);
			}
		}
		else
		{
			hr = DirectX::CreateWICTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr,
				&TextureSRV);

			if (SUCCEEDED(hr))
			{
				UE_LOG_SUCCESS("RHIDevice: WIC 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("RHIDevice: WIC 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)",
					InFilePath.c_str(), hr);
			}
		}
	}
	catch (const exception& e)
	{
		UE_LOG_ERROR("RHIDevice: 텍스처 로드 중 예외 발생 - %ls: %s", InFilePath.c_str(), e.what());
		return nullptr;
	}

	return SUCCEEDED(hr) ? TextureSRV : nullptr;
}

ID3D11ShaderResourceView* FRHIDevice::CreateTextureFromMemory(const void* InData, size_t InDataSize)
{
	if (!bIsInitialized || !Device || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: Texture 생성 실패 - Device가 초기화되지 않았습니다");
		return nullptr;
	}

	if (!InData || InDataSize == 0)
	{
		UE_LOG_ERROR("RHIDevice: 잘못된 메모리 데이터");
		return nullptr;
	}

	// DDS 매직 넘버 확인
	const uint32 DDS_MAGIC = 0x20534444; // "DDS " in little-endian
	bool bIsDDS = (InDataSize >= 4 && *static_cast<const uint32*>(InData) == DDS_MAGIC);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT hr;

	try
	{
		if (bIsDDS)
		{
			hr = DirectX::CreateDDSTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr,
				&TextureSRV);

			if (SUCCEEDED(hr))
			{
				UE_LOG_SUCCESS("RHIDevice: DDS 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("RHIDevice: DDS 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", hr);
			}
		}
		else
		{
			hr = DirectX::CreateWICTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr,
				&TextureSRV);

			if (SUCCEEDED(hr))
			{
				UE_LOG_SUCCESS("RHIDevice: WIC 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("RHIDevice: WIC 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", hr);
			}
		}
	}
	catch (const exception& e)
	{
		UE_LOG_ERROR("RHIDevice: 메모리 텍스처 생성 중 예외 발생: %s", e.what());
		return nullptr;
	}

	return SUCCEEDED(hr) ? TextureSRV : nullptr;
}

void FRHIDevice::ReleaseTexture(ID3D11ShaderResourceView* InTexture)
{
	if (InTexture)
	{
		InTexture->Release();
	}
}

bool FRHIDevice::CompileShaderFromFile(const wstring& InFilePath, const char* InEntryPoint,
	const char* InShaderModel, ID3DBlob** OutBlob)
{
	DWORD ShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	ShaderFlags |= D3DCOMPILE_DEBUG;
	ShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* ErrorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(
		InFilePath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		InEntryPoint,
		InShaderModel,
		ShaderFlags,
		0,
		OutBlob,
		&ErrorBlob);

	if (FAILED(hr))
	{
		if (ErrorBlob)
		{
			UE_LOG_ERROR("RHIDevice: 셰이더 컴파일 오류 - %s", (char*)ErrorBlob->GetBufferPointer());
			ErrorBlob->Release();
		}
		return false;
	}

	if (ErrorBlob)
	{
		ErrorBlob->Release();
	}

	return true;
}

// 상태 객체 생성 헬퍼들
void FRHIDevice::CreateRasterizerStates()
{
	if (!Device) return;

	// Lit (Solid fill)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;

		ID3D11RasterizerState* state = nullptr;
		HRESULT hr = Device->CreateRasterizerState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			RasterizerStates.Add(EViewMode::Lit, state);
		}
	}

	// Unlit (Solid fill - 동일하게 처리)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;

		ID3D11RasterizerState* state = nullptr;
		HRESULT hr = Device->CreateRasterizerState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			RasterizerStates.Add(EViewMode::Unlit, state);
		}
	}

	// WireFrame
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;

		ID3D11RasterizerState* state = nullptr;
		HRESULT hr = Device->CreateRasterizerState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			RasterizerStates.Add(EViewMode::WireFrame, state);
		}
	}
}

void FRHIDevice::CreateDepthStencilStates()
{
	if (!Device) return;

	// 기본 템플릿
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.StencilEnable = FALSE;

	// Never: 깊이 테스트 비활성화
	{
		desc.DepthEnable = FALSE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_NEVER;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::Never, state);
		}
	}

	// Less: 더 가까우면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::Less, state);
		}
	}

	// Equal: 같으면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_EQUAL;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::Equal, state);
		}
	}

	// LessEqual: 더 가까거나 같으면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::LessEqual, state);
		}
	}

	// Greater: 더 멀면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_GREATER;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::Greater, state);
		}
	}

	// NotEqual: 다르면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::NotEqual, state);
		}
	}

	// GreaterEqual: 더 멀거나 같으면 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::GreaterEqual, state);
		}
	}

	// Always: 항상 통과
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 쓰기는 비활성화
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;

		ID3D11DepthStencilState* state = nullptr;
		HRESULT hr = Device->CreateDepthStencilState(&desc, &state);
		if (SUCCEEDED(hr))
		{
			DepthStencilStates.Add(EComparisonFunc::Always, state);
		}
	}
}

void FRHIDevice::CreateBlendStates()
{
	if (!Device) return;

	// 블렌드 비활성화 상태
	{
		D3D11_BLEND_DESC desc = {};
		auto& rt = desc.RenderTarget[0];
		rt.BlendEnable = FALSE;
		rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = Device->CreateBlendState(&desc, &BlendStateDisabled);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: BlendStateDisabled 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}

	// 블렌드 활성화 상태 (Alpha Blending)
	{
		D3D11_BLEND_DESC desc = {};
		auto& rt = desc.RenderTarget[0];
		rt.BlendEnable = TRUE;
		rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D11_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D11_BLEND_ONE;
		rt.DestBlendAlpha = D3D11_BLEND_ZERO;
		rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = Device->CreateBlendState(&desc, &BlendStateEnabled);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: BlendStateEnabled 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}
}

void FRHIDevice::CreateConstantBuffer()
{
	if (!Device) return;

	// 기본 상수 버퍼 (Model, View, Projection 매트릭스)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(FMatrix) * 3;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&desc, nullptr, &ConstantBuffer);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: ConstantBuffer 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}

	// HighLight 상수 버퍼
	{
		D3D11_BUFFER_DESC hlDesc = {};
		hlDesc.Usage = D3D11_USAGE_DYNAMIC;
		hlDesc.ByteWidth = sizeof(HighLightBufferType);
		hlDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		hlDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&hlDesc, nullptr, &HighLightCB);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: HighLightCB 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}

	// Color 상수 버퍼
	{
		D3D11_BUFFER_DESC colorDesc = {};
		colorDesc.Usage = D3D11_USAGE_DYNAMIC;
		colorDesc.ByteWidth = sizeof(ColorBufferType);
		colorDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		colorDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&colorDesc, nullptr, &ColorCB);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: ColorCB 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}

	// Pixel Constant 버퍼
	{
		D3D11_BUFFER_DESC pixelConstDesc = {};
		pixelConstDesc.Usage = D3D11_USAGE_DYNAMIC;
		pixelConstDesc.ByteWidth = sizeof(FPixelConstBufferType);
		pixelConstDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		pixelConstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&pixelConstDesc, nullptr, &PixelConstCB);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("RHIDevice: PixelConstCB 생성 실패 (HRESULT: 0x%08lX)", hr);
		}
	}
}

// 상수 버퍼 업데이트
void FRHIDevice::UpdateConstantBuffers(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
	if (!bIsInitialized || !DeviceContext || !ConstantBuffer)
	{
		UE_LOG_ERROR("RHIDevice: UpdateConstantBuffers 실패 - 어나 이상이나 초기화되지 않았습니다");
		return;
	}

	// 매트릭스를 Transpose하여 상수 버퍼 데이터 준비
	struct ConstantBufferData
	{
		FMatrix World;
		FMatrix View;
		FMatrix Projection;
	} cbData;

	cbData.World = ModelMatrix.Transpose();
	cbData.View = ViewMatrix.Transpose();
	cbData.Projection = ProjectionMatrix.Transpose();

	// 상수 버퍼 업데이트
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(MappedResource.pData, &cbData, sizeof(cbData));
		DeviceContext->Unmap(ConstantBuffer, 0);

		// 상수 버퍼를 젠자 쉐이더와 픽셀 쉐이더에 바인드
		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: 상수 버퍼 업데이트 실패 (HRESULT: 0x%08lX)", hr);
	}
}

// 추가 상수 버퍼 업데이트 함수들
void FRHIDevice::UpdateHighLightConstantBuffers(const uint32 InPicked, const FVector& InColor, const uint32 X, const uint32 Y, const uint32 Z, const uint32 Gizmo)
{
	if (!bIsInitialized || !DeviceContext || !HighLightCB)
	{
		UE_LOG_ERROR("RHIDevice: UpdateHighLightConstantBuffers 실패 - 초기화되지 않았습니다");
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = DeviceContext->Map(HighLightCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr))
	{
		auto* dataPtr = reinterpret_cast<HighLightBufferType*>(mapped.pData);
		dataPtr->Picked = InPicked;
		dataPtr->Color = InColor;
		dataPtr->X = X;
		dataPtr->Y = Y;
		dataPtr->Z = Z;
		dataPtr->Gizmo = Gizmo;

		DeviceContext->Unmap(HighLightCB, 0);
		DeviceContext->VSSetConstantBuffers(2, 1, &HighLightCB); // b2 슬롯
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: HighLightCB Map 실패 (HRESULT: 0x%08lX)", hr);
	}
}

void FRHIDevice::UpdateColorConstantBuffers(const FVector4& InColor)
{
	if (!bIsInitialized || !DeviceContext || !ColorCB)
	{
		UE_LOG_ERROR("RHIDevice: UpdateColorConstantBuffers 실패 - 초기화되지 않았습니다");
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = DeviceContext->Map(ColorCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr))
	{
		auto* dataPtr = reinterpret_cast<ColorBufferType*>(mapped.pData);
		dataPtr->Color = InColor;

		DeviceContext->Unmap(ColorCB, 0);
		DeviceContext->PSSetConstantBuffers(3, 1, &ColorCB); // b3 슬롯
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: ColorCB Map 실패 (HRESULT: 0x%08lX)", hr);
	}
}

void FRHIDevice::UpdatePixelConstantBuffers(const FObjMaterialInfo& InMaterialInfo, bool bHasMaterial, bool bHasTexture)
{
	if (!bIsInitialized || !DeviceContext || !PixelConstCB)
	{
		UE_LOG_ERROR("RHIDevice: UpdatePixelConstantBuffers 실패 - 초기화되지 않았습니다");
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = DeviceContext->Map(PixelConstCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr))
	{
		FPixelConstBufferType* dataPtr = reinterpret_cast<FPixelConstBufferType*>(mapped.pData);

		dataPtr->bHasMaterial = bHasMaterial;
		dataPtr->bHasTexture = bHasTexture;
		dataPtr->Material.DiffuseColor = InMaterialInfo.DiffuseColor;
		dataPtr->Material.AmbientColor = InMaterialInfo.AmbientColor;
		// 다른 매테리얼 속성들도 필요에 따라 추가 가능

		DeviceContext->Unmap(PixelConstCB, 0);
		DeviceContext->PSSetConstantBuffers(4, 1, &PixelConstCB); // b4 슬롯
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: PixelConstCB Map 실패 (HRESULT: 0x%08lX)", hr);
	}
}

void FRHIDevice::OMSetBlendState(bool bIsBlendMode)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: OMSetBlendState 실패 - DeviceContext가 초기화되지 않았습니다");
		return;
	}

	if (bIsBlendMode && BlendStateEnabled)
	{
		float blendFactor[4] = {0, 0, 0, 0};
		DeviceContext->OMSetBlendState(BlendStateEnabled, blendFactor, 0xffffffff);
	}
	else if (!bIsBlendMode && BlendStateDisabled)
	{
		DeviceContext->OMSetBlendState(BlendStateDisabled, nullptr, 0xffffffff);
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: BlendState가 생성되지 않았습니다");
	}
}

// 상태 설정 함수들 구현
void FRHIDevice::RSSetState(EViewMode ViewMode)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: RSSetState 실패 - DeviceContext가 초기화되지 않았습니다");
		return;
	}

	// 캐시된 래스터라이저 상태 가져오기
	auto* FoundState = RasterizerStates.Find(ViewMode);
	if (FoundState && *FoundState)
	{
		DeviceContext->RSSetState(*FoundState);
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: RasterizerState를 찾을 수 없습니다 - ViewMode: %d", (int)ViewMode);
	}
}

void FRHIDevice::OmSetDepthStencilState(EComparisonFunc CompareFunction)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: OmSetDepthStencilState 실패 - DeviceContext가 초기화되지 않았습니다");
		return;
	}

	// 캐시된 되스스텐실 상태 가져오기
	auto* FoundState = DepthStencilStates.Find(CompareFunction);
	if (FoundState && *FoundState)
	{
		DeviceContext->OMSetDepthStencilState(*FoundState, 1);
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: DepthStencilState를 찾을 수 없습니다 - CompareFunction: %d", (int)CompareFunction);
	}
}

void FRHIDevice::OmSetBlendState(bool bEnableBlending)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: OmSetBlendState 실패 - DeviceContext가 초기화되지 않았습니다");
		return;
	}

	// 캐시된 블렌드 상태 선택
	ID3D11BlendState* BlendState = bEnableBlending ? BlendStateEnabled : BlendStateDisabled;
	if (BlendState)
	{
		DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: BlendState가 생성되지 않았습니다");
	}
}

void FRHIDevice::CreateSamplerState()
{
	if (!Device) return;

	D3D11_SAMPLER_DESC SampleDesc = {};
	SampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampleDesc.MinLOD = 0;
	SampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = Device->CreateSamplerState(&SampleDesc, &DefaultSamplerState);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: DefaultSamplerState 생성 실패 (HRESULT: 0x%08lX)", hr);
	}
}

void FRHIDevice::PSSetDefaultSampler(UINT StartSlot)
{
	if (!bIsInitialized || !DeviceContext || !DefaultSamplerState)
	{
		UE_LOG_ERROR("RHIDevice: PSSetDefaultSampler 실패 - 초기화되지 않았습니다");
		return;
	}

	DeviceContext->PSSetSamplers(StartSlot, 1, &DefaultSamplerState);
}

void FRHIDevice::SetShader(TObjectPtr<UShader> InShader)
{
	if (InShader)
	{
		DeviceContext->VSSetShader(InShader->GetVertexShader(), nullptr, 0);
		DeviceContext->PSSetShader(InShader->GetPixelShader(), nullptr, 0);
		DeviceContext->IASetInputLayout(InShader->GetInputLayout());
	}
	else
	{
		// Unbind shaders
		DeviceContext->VSSetShader(nullptr, nullptr, 0);
		DeviceContext->PSSetShader(nullptr, nullptr, 0);
		DeviceContext->IASetInputLayout(nullptr);
	}
}

void FRHIDevice::OMSetDepthWriteEnabled(bool bEnabled)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: OMSetDepthWriteEnabled 실패 - 초기화되지 않았습니다");
		return;
	}

	// 기본 Depth Stencil 상태 생성
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = bEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;

	ID3D11DepthStencilState* TempState = nullptr;
	HRESULT hr = Device->CreateDepthStencilState(&desc, &TempState);
	if (SUCCEEDED(hr))
	{
		DeviceContext->OMSetDepthStencilState(TempState, 1);
		TempState->Release(); // 임시 상태이므로 즉시 해제
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: DepthStencilState 생성 실패 (HRESULT: 0x%08lX)", hr);
	}
}

void FRHIDevice::OMSetColorWriteEnabled(bool bEnabled)
{
	if (!bIsInitialized || !DeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: OMSetColorWriteEnabled 실패 - 초기화되지 않았습니다");
		return;
	}

	// 기본 Blend 상태 생성
	D3D11_BLEND_DESC desc = {};
	auto& rt = desc.RenderTarget[0];
	rt.BlendEnable = FALSE;
	rt.RenderTargetWriteMask = bEnabled ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;

	ID3D11BlendState* TempState = nullptr;
	HRESULT hr = Device->CreateBlendState(&desc, &TempState);
	if (SUCCEEDED(hr))
	{
		DeviceContext->OMSetBlendState(TempState, nullptr, 0xFFFFFFFF);
		TempState->Release(); // 임시 상태이므로 즉시 해제
	}
	else
	{
		UE_LOG_ERROR("RHIDevice: BlendState 생성 실패 (HRESULT: 0x%08lX)", hr);
	}
}

// 렌더 타겟 관리 메서드들
bool FRHIDevice::BeginFrame()
{
	if (!bIsInitialized || !DeviceContext || !SwapChain)
	{
		UE_LOG_ERROR("RHIDevice: BeginFrame 실패 - 초기화되지 않았습니다");
		return false;
	}

	// 백버퍼에서 생성
	ID3D11Texture2D* BackBuffer = nullptr;
	HRESULT ResultHandle = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer));
	if (FAILED(ResultHandle))
	{
		UE_LOG_ERROR("RHIDevice: SwapChain BackBuffer 가져오기 실패 (HRESULT: 0x%08lX)", ResultHandle);
		return false;
	}

	// 렌더 타겟 뷰 생성
	ResultHandle = Device->CreateRenderTargetView(BackBuffer, nullptr, &MainRenderTargetView);
	BackBuffer->Release();
	if (FAILED(ResultHandle))
	{
		UE_LOG_ERROR("RHIDevice: RenderTargetView 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
		return false;
	}

	// Depth Stencil Buffer 생성 (필요하다면)
	// 일단은 렌더 타겟만 설정

	return true;
}

void FRHIDevice::EndFrame()
{
	if (!bIsInitialized || !SwapChain)
	{
		return;
	}

	// Present
	SwapChain->Present(1, 0);

	// 렌더 타겟 뷰 해제
	if (MainRenderTargetView)
	{
		MainRenderTargetView->Release();
		MainRenderTargetView = nullptr;
	}

	if (MainDepthStencilView)
	{
		MainDepthStencilView->Release();
		MainDepthStencilView = nullptr;
	}
}

void FRHIDevice::ClearRenderTarget(const float ClearColor[4])
{
	if (!bIsInitialized || !DeviceContext || !MainRenderTargetView)
	{
		UE_LOG_ERROR("RHIDevice: ClearRenderTarget 실패 - RenderTargetView가 없습니다");
		return;
	}

	DeviceContext->ClearRenderTargetView(MainRenderTargetView, ClearColor);

	// Depth Stencil도 클리어 (있다면)
	if (MainDepthStencilView)
	{
		DeviceContext->ClearDepthStencilView(MainDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void FRHIDevice::SetMainRenderTarget()
{
	if (!bIsInitialized || !DeviceContext || !MainRenderTargetView)
	{
		UE_LOG_ERROR("RHIDevice: SetMainRenderTarget 실패 - RenderTargetView가 없습니다");
		return;
	}

	DeviceContext->OMSetRenderTargets(1, &MainRenderTargetView, MainDepthStencilView);
}

void FRHIDevice::ClearDepthStencilView(float Depth, uint8 Stencil)
{
	if (MainDepthStencilView)
	{
		DeviceContext->ClearDepthStencilView(MainDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, Depth, Stencil);
	}
}
