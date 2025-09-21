#include "pch.h"
#include "Manager/Asset/Public/AssetManager.h"

#include "Render/Renderer/Public/Renderer.h"
#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "Runtime/Component/Mesh/Public/VertexDatas.h"
#include "Asset/Public/ObjImporter.h"
#include "Asset/Public/StaticMesh.h"
#include "Factory/Public/NewObject.h"
#include "Core/Public/ObjectIterator.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UAssetManager)

UAssetManager::UAssetManager() = default;

UAssetManager::~UAssetManager() = default;

void UAssetManager::Initialize()
{
	URenderer& Renderer = URenderer::GetInstance();

	VertexDatas.emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexDatas.emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexDatas.emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexDatas.emplace(EPrimitiveType::Line, &VerticesLine);

	Vertexbuffers.emplace(EPrimitiveType::Arrow, Renderer.CreateVertexBuffer(
		VerticesArrow.data(), static_cast<int>(VerticesArrow.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::CubeArrow, Renderer.CreateVertexBuffer(
		VerticesCubeArrow.data(), static_cast<int>(VerticesCubeArrow.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Ring, Renderer.CreateVertexBuffer(
		VerticesRing.data(), static_cast<int>(VerticesRing.size() * sizeof(FVertex))));

	NumVertices.emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.size()));
	NumVertices.emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.size()));
	NumVertices.emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.size()));

	// Initialize Shaders
	ID3D11VertexShader* vertexShader;
	ID3D11InputLayout* inputLayout;
	TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	URenderer::GetInstance().CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineVS.hlsl", layoutDesc,
	                                                          &vertexShader, &inputLayout);
	VertexShaders.emplace(EShaderType::BatchLine, vertexShader);
	InputLayouts.emplace(EShaderType::BatchLine, inputLayout);

	ID3D11PixelShader* PixelShader;
	URenderer::GetInstance().CreatePixelShader(L"Asset/Shader/BatchLinePS.hlsl", &PixelShader);
	PixelShaders.emplace(EShaderType::BatchLine, PixelShader);

	// StaticMesh 셰이더 로드
	LoadStaticMeshShaders();

	// 모든 프리미티브 StaticMesh 로드
	InitializeBasicPrimitives();
}

void UAssetManager::Release()
{
	// TMap.Value()
	for (auto& Pair : Vertexbuffers)
	{
		URenderer::GetInstance().ReleaseVertexBuffer(Pair.second);
	}

	// TMap.Empty()
	Vertexbuffers.clear();

	// Texture Resource 해제
	ReleaseAllTextures();

	// TODO: 주석 풀면 엔진 끌때 터짐 메모리 누수 다 잡고 주석 풀 것
	// StaticMesh 애셋 해제 - TObjectIterator를 사용하여 모든 StaticMesh 삭제
	/*for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		delete StaticMesh;
	}*/
}

TArray<FVertex>* UAssetManager::GetVertexData(EPrimitiveType InType)
{
	return VertexDatas[InType];
}

ID3D11Buffer* UAssetManager::GetVertexbuffer(EPrimitiveType InType)
{
	return Vertexbuffers[InType];
}

uint32 UAssetManager::GetNumVertices(EPrimitiveType InType)
{
	return NumVertices[InType];
}

ID3D11VertexShader* UAssetManager::GetVertexShader(EShaderType Type)
{
	return VertexShaders[Type];
}

ID3D11PixelShader* UAssetManager::GetPixelShader(EShaderType Type)
{
	return PixelShaders[Type];
}

ID3D11InputLayout* UAssetManager::GetInputLayout(EShaderType Type)
{
	return InputLayouts[Type];
}

//const FAABB& UAssetManager::GetAABB(EPrimitiveType InType)
//{
//	return AABBs[InType];
//}

/**
 * @brief 파일에서 텍스처를 로드하고 캐시에 저장하는 함수
 * 중복 로딩을 방지하기 위해 이미 로드된 텍스처는 캐시에서 반환
 * @param InFilePath 로드할 텍스처 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::LoadTexture(const FString& InFilePath, const FName& InName)
{
	// 이미 로드된 텍스처가 있는지 확인
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	// 새로운 텍스처 로드
	ID3D11ShaderResourceView* TextureSRV = CreateTextureFromFile(InFilePath);
	if (TextureSRV)
	{
		TextureCache[InFilePath] = TextureSRV;
	}

	return TextureSRV;
}

/**
 * @brief 캐시된 텍스처를 가져오는 함수
 * 이미 로드된 텍스처만 반환하고 새로 로드하지는 않음
 * @param InFilePath 가져올 텍스처 파일의 경로
 * @return 캐시에 있으면 ID3D11ShaderResourceView 포인터, 없으면 nullptr
 */
ComPtr<ID3D11ShaderResourceView> UAssetManager::GetTexture(const FString& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	return nullptr;
}

/**
 * @brief 특정 텍스처를 캐시에서 해제하는 함수
 * DirectX 리소스를 해제하고 캐시에서 제거
 * @param InFilePath 해제할 텍스처 파일의 경로
 */
void UAssetManager::ReleaseTexture(const FString& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		if (Iter->second)
		{
			Iter->second->Release();
		}

		TextureCache.erase(Iter);
	}
}

/**
 * @brief 특정 텍스처가 캐시에 있는지 확인하는 함수
 * @param InFilePath 확인할 텍스처 파일의 경로
 * @return 캐시에 있으면 true, 없으면 false
 */
bool UAssetManager::HasTexture(const FString& InFilePath) const
{
	return TextureCache.find(InFilePath) != TextureCache.end();
}

/**
 * @brief 모든 텍스처 리소스를 해제하는 함수
 * 캐시된 모든 텍스처의 DirectX 리소스를 해제하고 캐시를 비움
 */
void UAssetManager::ReleaseAllTextures()
{
	for (auto& Pair : TextureCache)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	TextureCache.clear();
}

/**
 * @brief 파일에서 DirectX 텍스처를 생성하는 내부 함수
 * DirectXTK의 WICTextureLoader를 사용
 * @param InFilePath 로드할 이미지 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromFile(const path& InFilePath)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: Texture 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	// 파일 확장자에 따라 적절한 로더 선택
	FString FileExtension = InFilePath.extension().string();
	transform(FileExtension.begin(), FileExtension.end(), FileExtension.begin(), ::tolower);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		if (FileExtension == ".dds")
		{
			// DDS 파일은 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr,
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)",
				             InFilePath.c_str(), ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromFile(
				Device,
				DeviceContext,
				InFilePath.c_str(),
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 텍스처 로드 성공 - %ls", InFilePath.c_str());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 텍스처 로드 실패 - %ls (HRESULT: 0x%08lX)"
				             , InFilePath.c_str(), ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 텍스처 로드 중 예외 발생 - %ls: %s", InFilePath.c_str(), Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief 메모리 데이터에서 DirectX 텍스처를 생성하는 함수
 * DirectXTK의 WICTextureLoader와 DDSTextureLoader를 사용하여 메모리 데이터에서 텍스처 생성
 * @param InData 이미지 데이터의 포인터
 * @param InDataSize 데이터의 크기 (Byte)
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 * @note DDS 포맷 감지를 위해 매직 넘버를 확인하고 적절한 로더 선택
 * @note 네트워크에서 다운로드한 이미지나 리소스 팩에서 추출한 데이터 처리에 유용
 */
ID3D11ShaderResourceView* UAssetManager::CreateTextureFromMemory(const void* InData, size_t InDataSize)
{
	if (!InData || InDataSize == 0)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - 잘못된 데이터");
		return nullptr;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		// DDS 매직 넘버 확인 (DDS 파일은 "DDS " 로 시작)
		const uint32 DDS_MAGIC = 0x20534444; // "DDS " in little-endian
		bool bIsDDS = (InDataSize >= 4 && *static_cast<const uint32*>(InData) == DDS_MAGIC);

		if (bIsDDS)
		{
			// DDS 데이터는 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 중 예외 발생: %s", Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

// StaticMesh 관련 함수들
UStaticMesh* UAssetManager::LoadStaticMesh(const FString& InFilePath)
{
	// 이미 로드된 StaticMesh인지 확인
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return StaticMesh;
		}
	}

	// 새 StaticMesh 로드
	UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>();
	if (!NewStaticMesh)
	{
		return nullptr;
	}

	// OBJ 파일에서 StaticMesh 데이터 로드
	FStaticMesh StaticMeshData;
	bool bImportSuccess = FObjImporter::ImportStaticMesh(InFilePath, StaticMeshData);

	if (bImportSuccess)
	{
		NewStaticMesh->SetStaticMeshData(StaticMeshData);

		UE_LOG_SUCCESS("StaticMesh 로드 성공: %s", InFilePath.c_str());
		return NewStaticMesh;
	}
	else
	{
		delete NewStaticMesh;
		UE_LOG_ERROR("StaticMesh 로드 실패: %s", InFilePath.c_str());
		return nullptr;
	}
}

UStaticMesh* UAssetManager::GetStaticMesh(const FString& InFilePath)
{
	// TObjectIterator를 사용하여 로드된 StaticMesh 검색
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return StaticMesh;
		}
	}
	return nullptr;
}

void UAssetManager::ReleaseStaticMesh(const FString& InFilePath)
{
	// TObjectIterator를 사용하여 해당 StaticMesh 찾아서 삭제
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			delete StaticMesh;
			break;
		}
	}
}

bool UAssetManager::HasStaticMesh(const FString& InFilePath) const
{
	// TObjectIterator를 사용하여 해당 StaticMesh가 존재하는지 확인
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPathFileName() == InFilePath)
		{
			return true;
		}
	}
	return false;
}

void UAssetManager::LoadStaticMeshShaders()
{
	// StaticMesh용 Input Layout 설정
	TArray<D3D11_INPUT_ELEMENT_DESC> StaticMeshLayoutDesc = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Renderer를 통해 StaticMesh 셰이더 로드
	auto& Renderer = URenderer::GetInstance();

	// Vertex Shader와 Input Layout 생성
	ID3D11VertexShader* StaticMeshVS = nullptr;
	ID3D11InputLayout* StaticMeshLayout = nullptr;

	Renderer.CreateVertexShaderAndInputLayout(
		L"Asset/Shader/StaticMeshShader.hlsl",
		StaticMeshLayoutDesc,
		&StaticMeshVS,
		&StaticMeshLayout
	);

	// Pixel Shader 생성
	ID3D11PixelShader* StaticMeshPS = nullptr;
	Renderer.CreatePixelShader(L"Asset/Shader/StaticMeshShader.hlsl", &StaticMeshPS);

	// 셰이더들을 AssetManager에 저장
	VertexShaders[EShaderType::StaticMesh] = StaticMeshVS;
	PixelShaders[EShaderType::StaticMesh] = StaticMeshPS;
	InputLayouts[EShaderType::StaticMesh] = StaticMeshLayout;

	if (StaticMeshVS && StaticMeshPS && StaticMeshLayout)
	{
		UE_LOG_SUCCESS("StaticMesh 셰이더 로딩 성공");
	}
	else
	{
		UE_LOG_ERROR("StaticMesh 셰이더 로딩 실패 - VS: %p, PS: %p, Layout: %p",
			StaticMeshVS, StaticMeshPS, StaticMeshLayout);
	}
}

/**
 * @brief 모든 프리미티브 타입의 StaticMesh를 미리 로드하는 함수
 */
void UAssetManager::InitializeBasicPrimitives()
{
	// 각 프리미티브 타입별 OBJ 파일 경로 매핑
	TMap<EPrimitiveType, FString> PrimitiveFilePaths = {
		{EPrimitiveType::Sphere, "Data/Sphere.obj"},
		{EPrimitiveType::Cube, "Data/Cube.obj"},
		{EPrimitiveType::Triangle, "Data/Triangle.obj"},
		{EPrimitiveType::Square, "Data/Square.obj"},
		{EPrimitiveType::Torus, "Data/Torus.obj"},
		{EPrimitiveType::Cylinder, "Data/Cylinder.obj"},
		{EPrimitiveType::Cone, "Data/Cone.obj"},
	};

	// 각 프리미티브 타입별로 StaticMesh 로드
	for (const auto& Pair : PrimitiveFilePaths)
	{
		EPrimitiveType PrimitiveType = Pair.first;
		const FString& FilePath = Pair.second;

		UStaticMesh* LoadedMesh = LoadStaticMesh(FilePath);
		if (LoadedMesh)
		{
			PrimitiveStaticMeshes[PrimitiveType] = LoadedMesh;
			UE_LOG_SUCCESS("프리미티브 StaticMesh 로드 성공: %s", EnumToString(PrimitiveType));
		}
		else
		{
			UE_LOG_ERROR("프리미티브 StaticMesh 로드 실패: %s (경로: %s)",
				EnumToString(PrimitiveType), FilePath.c_str());
		}
	}
}

/**
 * @brief 특정 프리미티브 타입의 StaticMesh를 가져오는 함수
 * @param InPrimitiveType 가져올 프리미티브 타입
 * @return 해당 타입의 StaticMesh 포인터 (없으면 nullptr)
 */
UStaticMesh* UAssetManager::GetPrimitiveStaticMesh(EPrimitiveType InPrimitiveType)
{
	auto It = PrimitiveStaticMeshes.find(InPrimitiveType);
	if (It != PrimitiveStaticMeshes.end())
	{
		return It->second;
	}
	return nullptr;
}

