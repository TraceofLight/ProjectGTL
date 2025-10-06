#include "pch.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"
#include <d3dcompiler.h>

IMPLEMENT_SINGLETON_CLASS(URHIDevice, UObject)

URHIDevice::URHIDevice() = default;
URHIDevice::~URHIDevice() = default;

void URHIDevice::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	if (!InDevice || !InDeviceContext)
	{
		UE_LOG_ERROR("RHIDevice: Initialize 실패 - Device 또는 DeviceContext가 null입니다");
		return;
	}

	Device = InDevice;
	DeviceContext = InDeviceContext;
	bIsInitialized = true;

	UE_LOG_SUCCESS("RHIDevice: 초기화 완료");
}

void URHIDevice::Shutdown()
{
	Device = nullptr;
	DeviceContext = nullptr;
	bIsInitialized = false;

	UE_LOG("RHIDevice: 종료 완료");
}

ID3D11Buffer* URHIDevice::CreateVertexBuffer(const void* InData, uint32 InByteWidth, bool bCpuAccess)
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

void URHIDevice::ReleaseVertexBuffer(ID3D11Buffer* InBuffer)
{
	if (InBuffer)
	{
		InBuffer->Release();
	}
}

bool URHIDevice::UpdateVertexBuffer(ID3D11Buffer* InBuffer, const void* InData, uint32 InByteWidth)
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

ID3D11Buffer* URHIDevice::CreateIndexBuffer(const void* InData, uint32 InByteWidth)
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

void URHIDevice::ReleaseIndexBuffer(ID3D11Buffer* InBuffer)
{
	if (InBuffer)
	{
		InBuffer->Release();
	}
}

bool URHIDevice::CreateVertexShaderAndInputLayout(
	const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InLayoutDesc,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	if (!bIsInitialized || !Device)
	{
		UE_LOG_ERROR("RHIDevice: Shader 생성 실패 - Device가 초기화되지 않았습니다");
		return false;
	}

	ID3DBlob* ShaderBlob = nullptr;
	if (!CompileShaderFromFile(InFilePath, "main", "vs_5_0", &ShaderBlob))
	{
		UE_LOG_ERROR("RHIDevice: VertexShader 컴파일 실패 - %ls", InFilePath.c_str());
		return false;
	}

	// Vertex Shader 생성
	HRESULT hr = Device->CreateVertexShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		OutVertexShader);

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: VertexShader 생성 실패 (HRESULT: 0x%08lX)", hr);
		ShaderBlob->Release();
		return false;
	}

	// Input Layout 생성
	hr = Device->CreateInputLayout(
		InLayoutDesc.GetData(),
		static_cast<UINT>(InLayoutDesc.Num()),
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		OutInputLayout);

	ShaderBlob->Release();

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: InputLayout 생성 실패 (HRESULT: 0x%08lX)", hr);
		(*OutVertexShader)->Release();
		*OutVertexShader = nullptr;
		return false;
	}

	return true;
}

bool URHIDevice::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader)
{
	if (!bIsInitialized || !Device)
	{
		UE_LOG_ERROR("RHIDevice: Shader 생성 실패 - Device가 초기화되지 않았습니다");
		return false;
	}

	ID3DBlob* ShaderBlob = nullptr;
	if (!CompileShaderFromFile(InFilePath, "main", "ps_5_0", &ShaderBlob))
	{
		UE_LOG_ERROR("RHIDevice: PixelShader 컴파일 실패 - %ls", InFilePath.c_str());
		return false;
	}

	HRESULT hr = Device->CreatePixelShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		OutPixelShader);

	ShaderBlob->Release();

	if (FAILED(hr))
	{
		UE_LOG_ERROR("RHIDevice: PixelShader 생성 실패 (HRESULT: 0x%08lX)", hr);
		return false;
	}

	return true;
}

void URHIDevice::ReleaseShader(IUnknown* InShader)
{
	if (InShader)
	{
		InShader->Release();
	}
}

ID3D11ShaderResourceView* URHIDevice::CreateTextureFromFile(const wstring& InFilePath)
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

ID3D11ShaderResourceView* URHIDevice::CreateTextureFromMemory(const void* InData, size_t InDataSize)
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

void URHIDevice::ReleaseTexture(ID3D11ShaderResourceView* InTexture)
{
	if (InTexture)
	{
		InTexture->Release();
	}
}

bool URHIDevice::CompileShaderFromFile(const wstring& InFilePath, const char* InEntryPoint,
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
