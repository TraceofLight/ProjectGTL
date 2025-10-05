#include "pch.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "Utility/Public/JsonSerializer.h"

UFontRenderer::UFontRenderer() = default;

UFontRenderer::~UFontRenderer()
{
	Release();
}

/**
 * @brief 폰트 렌더러 초기화
 * 셰이더 컴파일, 텍스처 로드, 버퍼 생성 등을 수행
 * @return
 */
bool UFontRenderer::Initialize()
{
	// 렌더러에서 Device와 DeviceContext 가져오기
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("FontRenderer: Device 또는 DeviceContext가 null입니다");
		return false;
	}

	// 셰이더 컴파일 및 생성
	if (!CreateShaders())
	{
		UE_LOG_ERROR("FontRenderer: 셰이더 생성 실패");
		return false;
	}

	// 폰트 아틀라스 텍스처 로드
	if (!LoadFontTexture())
	{
		UE_LOG_ERROR("FontRenderer: 텍스처 로드 실패");
		return false;
	}

	// 샘플러 스테이트 생성
	if (!CreateSamplerState())
	{
		UE_LOG_ERROR("FontRenderer: 샘플러 생성 실패");
		return false;
	}

	// 상수 버퍼 생성
	if (!CreateConstantBuffer())
	{
		UE_LOG_ERROR("FontRenderer: 상수 버퍼 생성 실패");
		return false;
	}

	// ConstantBufferData를 로드된 아틀라스 크기로 업데이트
	ConstantBufferData.AtlasSize = FVector2(static_cast<float>(AtlasWidth), static_cast<float>(AtlasHeight));
	UE_LOG("FontRenderer: 상수 버퍼에 아틀라스 크기 설정: %.0fx%.0f",
	       ConstantBufferData.AtlasSize.X, ConstantBufferData.AtlasSize.Y);

	// CreateVertexBufferForText 사용하는 곳이 없어서 주석 처리
	// if (!CreateVertexBufferForText("HI", -1.0f, 0.0f, 0.05f, 0.05f))
	// {
	// 	UE_LOG_ERROR("FontRenderer: 정점 버퍼 생성 실패");
	// 	return false;
	// }

	UE_LOG_SUCCESS("FontRenderer: 초기화 완료");
	return true;
}

/// @brief 리소스 해제
void UFontRenderer::Release()
{
	// 버퍼 해제
	if (FontVertexBuffer)
	{
		FontVertexBuffer->Release();
		FontVertexBuffer = nullptr;
	}

	if (FontConstantBuffer)
	{
		FontConstantBuffer->Release();
		FontConstantBuffer = nullptr;
	}

	// 텍스처 및 샘플러 해제
	// if (FontAtlasTexture)
	// {
	//     FontAtlasTexture->Release();
	//     FontAtlasTexture = nullptr;
	// }

	if (FontSampler)
	{
		FontSampler->Release();
		FontSampler = nullptr;
	}

	// 셰이더 해제
	if (FontVertexShader)
	{
		FontVertexShader->Release();
		FontVertexShader = nullptr;
	}

	if (FontPixelShader)
	{
		FontPixelShader->Release();
		FontPixelShader = nullptr;
	}

	if (FontInputLayout)
	{
		FontInputLayout->Release();
		FontInputLayout = nullptr;
	}
}

/**
 * @brief 임의의 텍스트 렌더링
 * @param InText 문자열
 * @param InWorldMatrix
 * @param InViewProjectionCostants
 * @param InCenterY
 * @param InStartZ
 * @param InCharWidth
 * @param InCharHeight
 */
void UFontRenderer::RenderText(FString InText, const FMatrix& InWorldMatrix,
                               const FViewProjConstants& InViewProjectionCostants,
                               float InCenterY, float InStartZ, float InCharWidth, float InCharHeight) const
{
	// TODO(KHJ): 여기서 전체 파이프라인을 다 처리하는데 이렇게 하면 안되니 나중에 합칠 것
	if (InText.empty())
	{
		UE_LOG_WARNING("FontRenderer: 빈 텍스트 시도");
		return;
	}

	// Renderer에서 Device와 DeviceContext 가져오기
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext || !FontVertexShader || !FontPixelShader ||
		!FontInputLayout || !FontAtlasTexture)
	{
		UE_LOG_ERROR("FontRenderer: 렌더링에 필요한 리소스가 null - Device:%p, Context:%p, VS:%p, PS:%p, Layout:%p, Tex:%p",
		             static_cast<void*>(Device), static_cast<void*>(DeviceContext),
		             static_cast<void*>(FontVertexShader), static_cast<void*>(FontPixelShader),
		             static_cast<void*>(FontInputLayout), static_cast<void*>(FontAtlasTexture)
		);
		return;
	}

	// 텍스트를 위한 새 정점 버퍼 생성
	ID3D11Buffer* tempVertexBuffer = nullptr;
	uint32 TempVertexCount = 0;

	// 텍스트 길이 및 정점 개수 계산
	size_t textLength = InText.length();
	TempVertexCount = static_cast<uint32>(textLength * 6); // 각 문자당 6개 정점

	// 정점 데이터 생성
	TArray<FFontVertex> Vertices;
	Vertices.reserve(TempVertexCount);

	// 텍스트 전체 너비 계산 (중앙 정렬을 위해)
	float TotalWidth = 0.0f;
	for (size_t i = 0; i < textLength; ++i)
	{
		uint32 Unicode = UTF8ToUnicode(InText, static_cast<uint32>(i));
		CharacterMetric Metric = GetCharacterMetric(Unicode);
		TotalWidth += static_cast<float>(Metric.advance_x) * InCharWidth;
	}

	// 시작 X 좌표 (중앙 정렬)
	float StartX = InCenterY - TotalWidth / 2.0f;
	float CurrentX = StartX;

	// UE_LOG("FontRenderer Debug: 텍스트: '%s', 전체 너비: %.2f, 시작X: %.2f", InText.data(), TotalWidth, StartX);

	// 각 문자에 대해 쿼드 생성
	for (size_t i = 0; i < textLength; ++i)
	{
		uint32 Unicode = UTF8ToUnicode(InText, static_cast<uint32>(i));

		// JSON에서 로드한 문자 메트릭 사용
		CharacterMetric Metric = GetCharacterMetric(Unicode);

		// 첫 번째 문자에 대해서만 메트릭 디버그 출력
		// if (i == 0) {
		// 	UE_LOG("문자 '%c' 메트릭 - x:%d, y:%d, w:%d, h:%d, left:%d, top:%d, advance:%d",
		// 	       (char)Unicode, Metric.x, Metric.y, Metric.width, Metric.height,
		// 	       Metric.left, Metric.top, Metric.advance_x);
		// }

		// 아틀라스에서의 UV 좌표 계산
		float u1 = static_cast<float>(Metric.x) / AtlasWidth;
		float v1 = static_cast<float>(Metric.y) / AtlasHeight;
		float u2 = static_cast<float>(Metric.x + Metric.width) / AtlasWidth;
		float v2 = static_cast<float>(Metric.y + Metric.height) / AtlasHeight;

		FVector2 uv_topLeft(u1, v1);
		FVector2 uv_topRight(u2, v1);
		FVector2 uv_bottomLeft(u1, v2);
		FVector2 uv_bottomRight(u2, v2);

		// 메트릭에서 바로 사이즈 가져오기
		float CharWidth = static_cast<float>(Metric.width) * InCharWidth;
		float CharHeight = static_cast<float>(Metric.height) * InCharHeight;

		// 메트릭 오프셋 적용
		float OffsetX = static_cast<float>(Metric.left) * InCharWidth;

		// 기준선 기반 수직 위치 계산 (가장 중요한 수정)
		// InStartZ를 기준선(baseline)으로 사용
		// Metric.top은 기준선에서 글리프 상단까지의 거리
		float TopY = InStartZ + (static_cast<float>(Metric.top) * InCharHeight);
		float BottomY = TopY - CharHeight;

		// 쿼드의 4개 모서리 좌표 계산 (Billboard용 좌표계)
		float LeftX = CurrentX + OffsetX;
		float RightX = LeftX + CharWidth;

		FVector TopLeft(0.0f, LeftX, TopY);
		FVector TopRight(0.0f, RightX, TopY);
		FVector BottomLeft(0.0f, LeftX, BottomY);
		FVector BottomRight(0.0f, RightX, BottomY);

		// 다음 문자 위치로 이동
		CurrentX += static_cast<float>(Metric.advance_x) * InCharWidth;

		// 첫 번째 삼각형 (왼쪽 위, 오른쪽 위, 왼쪽 아래)
		FFontVertex Vertex1 = {TopLeft, uv_topLeft, Unicode};
		FFontVertex Vertex2 = {TopRight, uv_topRight, Unicode};
		FFontVertex Vertex3 = {BottomLeft, uv_bottomLeft, Unicode};
		Vertices.push_back(Vertex1);
		Vertices.push_back(Vertex2);
		Vertices.push_back(Vertex3);

		// 두 번째 삼각형 (오른쪽 위, 오른쪽 아래, 왼쪽 아래)
		FFontVertex Vertex4 = {TopRight, uv_topRight, Unicode};
		FFontVertex Vertex5 = {BottomRight, uv_bottomRight, Unicode};
		FFontVertex Vertex6 = {BottomLeft, uv_bottomLeft, Unicode};
		Vertices.push_back(Vertex4);
		Vertices.push_back(Vertex5);
		Vertices.push_back(Vertex6);
	}

	// 임시 정점 버퍼 생성
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.ByteWidth = sizeof(FFontVertex) * TempVertexCount;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = Vertices.data();

	HRESULT ResultHandle = Device->CreateBuffer(&BufferDesc, &InitData, &tempVertexBuffer);
	if (FAILED(ResultHandle))
	{
		UE_LOG_ERROR("FontRenderer: 임시 정점 버퍼 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
		return;
	}

	// 상수 버퍼 업데이트 (월드 행렬과 뷰-프로젝션 행렬)
	struct WorldMatrixBuffer
	{
		FMatrix World;
	};

	WorldMatrixBuffer WorldBuffer;
	WorldBuffer.World = InWorldMatrix;

	// ViewProjection 상수 버퍼 생성
	ID3D11Buffer* ViewProjConstantBuffer = nullptr;
	D3D11_BUFFER_DESC ViewProjBufferDesc = {};
	ViewProjBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	ViewProjBufferDesc.ByteWidth = sizeof(InViewProjectionCostants);
	ViewProjBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ViewProjBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (SUCCEEDED(Device->CreateBuffer(&ViewProjBufferDesc, nullptr, &ViewProjConstantBuffer)))
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		if (SUCCEEDED(DeviceContext->Map(ViewProjConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		{
			memcpy(MappedResource.pData, &InViewProjectionCostants, sizeof(InViewProjectionCostants));
			DeviceContext->Unmap(ViewProjConstantBuffer, 0);
		}
	}

	// 월드 매트릭스 상수 버퍼 업데이트
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	if (SUCCEEDED(DeviceContext->Map(FontConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
	{
		memcpy(MappedResource.pData, &WorldBuffer, sizeof(WorldBuffer));
		DeviceContext->Unmap(FontConstantBuffer, 0);
	}

	// 폰트 데이터 상수 버퍼 생성
	ID3D11Buffer* FontDataBuffer = nullptr;
	D3D11_BUFFER_DESC fontBufferDesc = {};
	fontBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	fontBufferDesc.ByteWidth = sizeof(FFontConstantBuffer);
	fontBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fontBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (SUCCEEDED(Device->CreateBuffer(&fontBufferDesc, nullptr, &FontDataBuffer)))
	{
		if (SUCCEEDED(DeviceContext->Map(FontDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		{
			(void)memcpy(MappedResource.pData, &ConstantBufferData, sizeof(ConstantBufferData));
			DeviceContext->Unmap(FontDataBuffer, 0);
		}
	}

	// 알파 블렌딩 활성화
	ID3D11BlendState* PrevBlendState = nullptr;
	FLOAT PrevBlendFactor[4];
	UINT PrevSampleMask;
	DeviceContext->OMGetBlendState(&PrevBlendState, PrevBlendFactor, &PrevSampleMask);

	ID3D11BlendState* AlphaBlendState = nullptr;
	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (SUCCEEDED(Device->CreateBlendState(&BlendDesc, &AlphaBlendState)))
	{
		FLOAT BlendFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		DeviceContext->OMSetBlendState(AlphaBlendState, BlendFactor, 0xFFFFFFFF);
	}

	// 렌더링 파이프라인 설정
	DeviceContext->IASetInputLayout(FontInputLayout);

	UINT Stride = sizeof(FFontVertex);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &tempVertexBuffer, &Stride, &Offset);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D11_FILL_SOLID; // ← 와이어프레임 대신 Solid
	RasterDesc.CullMode = D3D11_CULL_BACK; // 보통은 Back-face culling
	RasterDesc.FrontCounterClockwise = FALSE;
	RasterDesc.DepthClipEnable = TRUE;
	RasterDesc.ScissorEnable = TRUE;

	ID3D11RasterizerState* solidState = nullptr;
	Device->CreateRasterizerState(&RasterDesc, &solidState);
	DeviceContext->RSSetState(solidState);

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE; // 깊이 검사 끄기
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* depthDisabledState = nullptr;
	Device->CreateDepthStencilState(&dsDesc, &depthDisabledState);
	DeviceContext->OMSetDepthStencilState(depthDisabledState, 1);

	// 셰이더 설정
	DeviceContext->VSSetShader(FontVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(FontPixelShader, nullptr, 0);

	// 상수 버퍼 바인딩
	DeviceContext->VSSetConstantBuffers(0, 1, &FontConstantBuffer);
	DeviceContext->VSSetConstantBuffers(1, 1, &ViewProjConstantBuffer);
	DeviceContext->VSSetConstantBuffers(2, 1, &FontDataBuffer);

	// 텍스처 및 샘플러 바인딩
	DeviceContext->PSSetShaderResources(0, 1, &FontAtlasTexture);
	DeviceContext->PSSetSamplers(0, 1, &FontSampler);

	// 드로우 콜
	DeviceContext->Draw(TempVertexCount, 0);

	// 리소스 정리
	DeviceContext->OMSetBlendState(PrevBlendState, PrevBlendFactor, PrevSampleMask);
	if (PrevBlendState) PrevBlendState->Release();
	if (AlphaBlendState) AlphaBlendState->Release();
	if (tempVertexBuffer) tempVertexBuffer->Release();
	if (ViewProjConstantBuffer) ViewProjConstantBuffer->Release();
	if (FontDataBuffer) FontDataBuffer->Release();
	if (solidState) solidState->Release();
	if (depthDisabledState) depthDisabledState->Release();
}

/// @brief 셰이더 생성 및 컴파일
bool UFontRenderer::CreateShaders()
{
	URenderer& Renderer = URenderer::GetInstance();

	// 입력 레이아웃 정의 (위치 + UV + 문자 인덱스)
	TArray<D3D11_INPUT_ELEMENT_DESC> layoutDesc = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// 버텍스 셰이더 및 입력 레이아웃 생성
	UE_LOG("FontRenderer: 셰이더 컴파일 시작...");
	Renderer.CreateVertexShaderAndInputLayout(
		L"Asset/Shader/ShaderFont.hlsl",
		layoutDesc,
		&FontVertexShader,
		&FontInputLayout
	);
	UE_LOG("FontRenderer: 셰이더 컴파일 완료");

	if (!FontVertexShader || !FontInputLayout)
	{
		UE_LOG_ERROR("FontRenderer: 버텍스 셰이더 생성 실패");
		return false;
	}

	// 픽셀 셰이더 생성
	Renderer.CreatePixelShader(L"Asset/Shader/ShaderFont.hlsl", &FontPixelShader);
	if (!FontPixelShader)
	{
		UE_LOG_ERROR("FontRenderer: 픽셀 셰이더 생성 실패");
		return false;
	}

	UE_LOG_SUCCESS("FontRenderer: 셰이더 생성 완료");
	return true;
}

/**
 * @brief 폰트 텍스처 및 폰트 메트릭 로드 함수
 * @return 성공 로드 여부
 */
bool UFontRenderer::LoadFontTexture()
{
	URHIDevice& RHIDevice = URHIDevice::GetInstance();

	path FontFilePath = "Asset/Texture/billboard_font_atlas.dds";
	path MetricFilePath = "Asset/Texture/billboard_font_atlas_info.json";

	// DDS 폰트 아틀라스 로드
	UE_LOG("FontRenderer: DDS 폰트 텍스처 로드 시도: %ls", FontFilePath.c_str());
	FontAtlasTexture = RHIDevice.CreateTextureFromFile(FontFilePath.wstring());

	if (!FontAtlasTexture)
	{
		UE_LOG_ERROR("FontRenderer: DDS 폰트 텍스처 로드 실패");
		return false;
	}
	UE_LOG("FontRenderer: DDS 폰트 텍스처 로드 성공 (Pointer: %p)", static_cast<void*>(FontAtlasTexture));

	// JSON 폰트 메트릭 로드
	UE_LOG("FontRenderer: JSON 폰트 메트릭 로드 시도: %ls", MetricFilePath.c_str());
	if (!FJsonSerializer::LoadFontMetrics(FontMetrics, AtlasWidth, AtlasHeight, MetricFilePath.string()))
	{
		UE_LOG_ERROR("FontRenderer: JSON 폰트 메트릭 로드 실패");
		return false;
	}

	if (AtlasHeight <= 0.0f)
	{
		UE_LOG_ERROR("아틀라스 높이가 0 또는 음수: %.0f", AtlasHeight);
		return false;
	}

	UE_LOG("FontRenderer: JSON 폰트 메트릭 로드 성공 (총 %llu개 문자, 아틀라스 크기: %.0fx%.0f)", FontMetrics.size(), AtlasWidth,
	       AtlasHeight);
	UE_LOG_SUCCESS("FontRenderer: 폰트 텍스처 로드 완료");

	return true;
}

/// @brief 샘플러 스테이트 생성
bool UFontRenderer::CreateSamplerState()
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();

	D3D11_SAMPLER_DESC samplerDesc = {};
	// 폰트 텍스처는 선형 필터링을 사용해야 부드럽게 보임
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // UV가 범위를 벗어나면 클램프
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = Device->CreateSamplerState(&samplerDesc, &FontSampler);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("FontRenderer: 샘플러 스테이트 생성 실패 (HRESULT: 0x%08lX)", hr);
		return false;
	}

	UE_LOG_SUCCESS("FontRenderer: 샘플러 스테이트 생성 완료");
	return true;
}

/// @brief 상수 버퍼 생성
bool UFontRenderer::CreateConstantBuffer()
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(FMatrix); // 월드 매트릭스용
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &FontConstantBuffer);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("FontRenderer: 상수 버퍼 생성 실패 (HRESULT: 0x%08lX)", hr);
		return false;
	}

	UE_LOG_SUCCESS("FontRenderer: 상수 버퍼 생성 완료");
	return true;
}

/**
 * @brief 문자의 메트릭 정보 가져오는 함수 (Unicode 지원)
 * JsonSerializer를 활용하는 방식
 */
CharacterMetric UFontRenderer::GetCharacterMetric(uint32 InUnicode) const
{
	auto Iter = FontMetrics.find(InUnicode);
	if (Iter != FontMetrics.end())
	{
		return Iter->second;
	}

	// 기본 문자 반환 (공백)
	auto SpaceIter = FontMetrics.find(static_cast<uint32>(' '));
	if (SpaceIter != FontMetrics.end())
	{
		return SpaceIter->second;
	}

	// JsonSerializer의 기본값 사용
	return FJsonSerializer::GetDefaultCharacterMetric();
}
