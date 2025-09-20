#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Component/Public/BillBoardComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Manager/Viewport/Public/ViewportManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateDefaultShader();
	CreateConstantBuffer();

	// FontRenderer 초기화
	FontRenderer = new UFontRenderer();
	if (!FontRenderer->Initialize())
	{
		UE_LOG("FontRenderer 초기화 실패");
		SafeDelete(FontRenderer);
	}
}

void URenderer::Release()
{
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseDepthStencilState();
	ReleaseRasterizerState();

	// FontRenderer 해제
	SafeDelete(FontRenderer);

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{
	// 현재 따로 생성하지 않음
}

/**
 * @brief Renderer에서 주로 사용할 Depth-Stencil State를 생성하는 함수
 */
void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil 설정 (Depth 판정 O, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};

	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;

	DefaultDescription.StencilEnable = FALSE;
	DefaultDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DefaultDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Disabled Depth Stencil 설정 (Depth 판정 X, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};

	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;

	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
		PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC DefaultLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	GetDevice()->CreateInputLayout(DefaultLayout, ARRAYSIZE(DefaultLayout), VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FVertex);

	// TODO(KHJ): ShaderBlob 파일로 저장하고, 이후 이미 존재하는 경우 컴파일 없이 Blob을 로드할 수 있도록 할 것
	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		if (Cache.second != nullptr)
		{
			Cache.second->Release();
		}
	}
	RasterCache.clear();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	if (DefaultInputLayout)
	{
		DefaultInputLayout->Release();
		DefaultInputLayout = nullptr;
	}

	if (DefaultPixelShader)
	{
		DefaultPixelShader->Release();
		DefaultPixelShader = nullptr;
	}

	if (DefaultVertexShader)
	{
		DefaultVertexShader->Release();
		DefaultVertexShader = nullptr;
	}
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseDepthStencilState()
{
	if (DefaultDepthStencilState)
	{
		DefaultDepthStencilState->Release();
		DefaultDepthStencilState = nullptr;
	}

	if (DisabledDepthStencilState)
	{
		DisabledDepthStencilState->Release();
		DisabledDepthStencilState = nullptr;
	}

	// 렌더 타겟을 초기화
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

void URenderer::Update()
{
	RenderBegin();

	// Multi-viewport rendering via splitter leaf rects
    TArray<FRect> ViewRects;
    UViewportManager::GetInstance().GetLeafRects(ViewRects);
	if (ViewRects.empty())
	{
		// Fallback: single full viewport
		RenderLevel();
		ULevelManager::GetInstance().GetEditor()->RenderEditor();
	}
	else
	{
		ID3D11DeviceContext* ctx = GetDeviceContext();
		const D3D11_VIEWPORT& fullVP = GetDeviceResources()->GetViewportInfo();
		for (const FRect& r : ViewRects)
		{
			D3D11_VIEWPORT vp{};
			vp.TopLeftX = (FLOAT)r.X; vp.TopLeftY = (FLOAT)r.Y;
			vp.Width = (FLOAT)max(0L, r.W); vp.Height = (FLOAT)max(0L, r.H);

			// Skip degenerate rects
			if (vp.Width <= 0.0f || vp.Height <= 0.0f) continue;

			vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
			ctx->RSSetViewports(1, &vp);
			D3D11_RECT sc{};
			sc.left = r.X; sc.top = r.Y; sc.right = r.X + r.W; sc.bottom = r.Y + r.H;
			ctx->RSSetScissorRects(1, &sc);

			RenderLevel();
			ULevelManager::GetInstance().GetEditor()->RenderEditor();
		}
		// Restore full viewport/scissor for UI
		ctx->RSSetViewports(1, &fullVP);
		D3D11_RECT scFull{};
		scFull.left = (LONG)fullVP.TopLeftX; scFull.top = (LONG)fullVP.TopLeftY;
		scFull.right = (LONG)(fullVP.TopLeftX + fullVP.Width);
		scFull.bottom = (LONG)(fullVP.TopLeftY + fullVP.Height);
		ctx->RSSetScissorRects(1, &scFull);
	}

    // 폰트 렌더링
    //RenderFont();

	// ImGui 자체 Render 처리가 진행되어야 하므로 따로 처리
    UUIManager::GetInstance().Render();

	RenderEnd();
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin() const
{
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* rtvs[] = { RenderTargetView }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel()
{
	// Level 없으면 Early Return
	if (!ULevelManager::GetInstance().GetCurrentLevel())
	{
		return;
	}

	TObjectPtr<UBillBoardComponent> BillBoard = nullptr;
	// Render Primitive
	for (auto& PrimitiveComponent : ULevelManager::GetInstance().GetCurrentLevel()->GetLevelPrimitiveComponents())
	{
		// TODO(KHJ) Visible 여기서 Control 하고 있긴 한데 맞는지 Actor 단위 렌더링 할 때도 이렇게 써야할지 고민 필요
		if (!PrimitiveComponent || !PrimitiveComponent->IsVisible())
		{
			continue;
		}

		if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard)
		{
			BillBoard = Cast<UBillBoardComponent>(PrimitiveComponent);
		}
		else
		{
			FRenderState RenderState = PrimitiveComponent->GetRenderState();

			// Get view mode from editor
			const EViewModeIndex ViewMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();
			if (ViewMode == EViewModeIndex::VMI_Wireframe)
			{
				RenderState.CullMode = ECullMode::None;
				RenderState.FillMode = EFillMode::WireFrame;
			}
			ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

			// Update pipeline info
			FPipelineInfo PipelineInfo = {
				DefaultInputLayout,
				DefaultVertexShader,
				LoadedRasterizerState,
				DefaultDepthStencilState,
				DefaultPixelShader,
				nullptr,
			};
			Pipeline->UpdatePipeline(PipelineInfo);

			// Update pipeline buffers
			Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
			UpdateConstant(
				PrimitiveComponent->GetRelativeLocation(),
				PrimitiveComponent->GetRelativeRotation(),
				PrimitiveComponent->GetRelativeScale3D());

			Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
			UpdateConstant(PrimitiveComponent->GetColor());

			Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
			Pipeline->Draw(static_cast<uint32>(PrimitiveComponent->GetVerticesData()->size()), 0);
		}
	}

	if (BillBoard)
	{
		//BillBoard->UpdateRotationMatrix();

		FString UUIDString = "UID: " + std::to_string(BillBoard->GetUUID());

		FMatrix RT = BillBoard->GetRTMatrix();
		//RT = FMatrix::Identity();

		const FViewProjConstants& viewProjConstData = ULevelManager::GetInstance().GetEditor()->GetViewProjConstData();
		//const FMatrix viewProjM = viewProjConstData.View * viewProjConstData.Projection;
		FontRenderer->RenderText(UUIDString.c_str(), RT, viewProjConstData);
	}
}

/**
 * @brief Editor용 Primitive를 렌더링하는 함수 (Gizmo, Axis 등)
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 */
void URenderer::RenderPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState)
{
	// Always visible 옵션에 따라 Depth 테스트 여부 결정
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		RasterizerState,
		DepthStencilState,
		DefaultPixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// Update constant buffers
	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

	Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
	UpdateConstant(InPrimitive.Color);

	// Set vertex buffer and draw
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, Stride);
	Pipeline->Draw(InPrimitive.NumVertices, 0);
}

/**
 * @brief Index Buffer를 사용하는 Editor Primitive 렌더링 함수
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 * @param bInUseBaseConstantBuffer 기본 상수 버퍼 사용 여부
 * @param InStride 정점 스트라이드
 * @param InIndexBufferStride 인덱스 버퍼 스트라이드
 */
void URenderer::RenderPrimitiveIndexed(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState,
	bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride)
{
	// Always visible 옵션에 따라 Depth 테스트 여부 결정
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// 커스텀 셰이더가 있으면 사용, 없으면 기본 셰이더 사용
	ID3D11InputLayout* InputLayout = InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout;
	ID3D11VertexShader* VertexShader = InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader;
	ID3D11PixelShader* PixelShader = InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader;

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		InputLayout,
		VertexShader,
		RasterizerState,
		DepthStencilState,
		PixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// 기본 상수 버퍼 사용하는 경우에만 업데이트
	if (bInUseBaseConstantBuffer)
	{
		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(InPrimitive.Color);
	}

	// Set buffers and draw indexed
	Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, InStride);
	Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

/**
 * @brief FVertex 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVertex* InVertices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 변경되지 않는 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief FVector 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @param bCpuAccess CPU에서 접근 가능한 동적 버퍼 여부
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 기본값: 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	// CPU 접근이 필요한 경우 동적 버퍼로 변경
	if (bCpuAccess)
	{
		VertexBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 자주 수정할 경우
		VertexBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU 쓰기 가능
		VertexBufferDescription.MiscFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices 인덱스 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 인덱스 버퍼
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC IndexBufferDescription = {};
	IndexBufferDescription.ByteWidth = InByteWidth;
	IndexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexBufferInitData = {};
	IndexBufferInitData.pSysMem = InIndices;

	ID3D11Buffer* IndexBuffer = nullptr;
	GetDevice()->CreateBuffer(&IndexBufferDescription, &IndexBufferInitData, &IndexBuffer);
	return IndexBuffer;
}

/**
 * @brief 창 크기 변경 시 렌더 타곟 및 버퍼를 재설정하는 함수
 * @param InWidth 새로운 창 너비
 * @param InHeight 새로운 창 높이
 */
void URenderer::OnResize(uint32 InWidth, uint32 InHeight) const
{
	// 필수 리소스가 유효하지 않으면 Early Return
	if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain())
	{
		return;
	}

	// 기존 버퍼들 해제
	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	// SwapChain 버퍼 크기 재설정
	HRESULT Result = GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(Result))
	{
		UE_LOG("OnResize Failed");
		return;
	}

	// 버퍼 재생성 및 렌더 타겟 설정
	DeviceResources->UpdateViewport();
	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();

	// 새로운 렌더 타겟 바인딩
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthStencilView());
}

/**
 * @brief Vertex Buffer 해제 함수
 * @param InVertexBuffer 해제할 정점 버퍼
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	if (InVertexBuffer)
	{
		InVertexBuffer->Release();
	}
}

/**
 * @brief 커스텀 Vertex Shader와 Input Layout을 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param InInputLayoutDescs Input Layout 스팩 배열
 * @param OutVertexShader 출력될 Vertex Shader 포인터
 * @param OutInputLayout 출력될 Input Layout 포인터
 */
void URenderer::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Vertex Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&VertexShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Vertex Shader 객체 생성
	GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// Input Layout 생성
	GetDevice()->CreateInputLayout(InInputLayoutDescs.data(), static_cast<uint32>(InInputLayoutDescs.size()),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), OutInputLayout);

	// TODO(KHJ): 이 값이 여기에 있는 게 맞나? 검토 필요
	Stride = sizeof(FVertex);

	VertexShaderBlob->Release();
}

/**
 * @brief 커스텀 Pixel Shader를 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param OutPixelShader 출력될 Pixel Shader 포인터
 */
void URenderer::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader) const
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Pixel Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&PixelShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Pixel Shader 객체 생성
	GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(),
		PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);

	PixelShaderBlob->Release();
}

/**
 * @brief 렌더링에 사용될 상수 버퍼들을 생성하는 함수
 */
void URenderer::CreateConstantBuffer()
{
	// 모델 변환 행렬용 상수 버퍼 생성 (Slot 0)
	{
		D3D11_BUFFER_DESC ModelConstantBufferDescription = {};
		ModelConstantBufferDescription.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ModelConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ModelConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ModelConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ModelConstantBufferDescription, nullptr, &ConstantBufferModels);
	}

	// 색상 정보용 상수 버퍼 생성 (Slot 2)
	{
		D3D11_BUFFER_DESC ColorConstantBufferDescription = {};
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ColorConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ColorConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ColorConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ColorConstantBufferDescription, nullptr, &ConstantBufferColor);
	}

	// 카메라 View/Projection 행렬용 상수 버퍼 생성 (Slot 1)
	{
		D3D11_BUFFER_DESC ViewProjConstantBufferDescription = {};
		ViewProjConstantBufferDescription.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ViewProjConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ViewProjConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ViewProjConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ViewProjConstantBufferDescription, nullptr, &ConstantBufferViewProj);
	}
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferViewProj)
	{
		ConstantBufferViewProj->Release();
		ConstantBufferViewProj = nullptr;
	}
}

void URenderer::UpdateConstant(const UPrimitiveComponent* InPrimitive) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* Constants = static_cast<FMatrix*>(constantbufferMSR.pData);
		{
			*Constants = FMatrix::GetModelMatrix(InPrimitive->GetRelativeLocation(),
				FVector::GetDegreeToRadian(InPrimitive->GetRelativeRotation()),
				InPrimitive->GetRelativeScale3D());
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InPosition
 * @param InRotation
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);

		// update constant buffer every frame
		FMatrix* Constants = static_cast<FMatrix*>(constantbufferMSR.pData);
		{
			*Constants = FMatrix::GetModelMatrix(InPosition, FVector::GetDegreeToRadian(InRotation), InScale);
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FViewProjConstants& InViewProjConstants) const
{
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);

	if (ConstantBufferViewProj)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FViewProjConstants* ViewProjectionConstants = static_cast<FViewProjConstants*>(ConstantBufferMSR.pData);
		{
			ViewProjectionConstants->View = InViewProjConstants.View;
			ViewProjectionConstants->Projection = InViewProjConstants.Projection;
		}
		GetDeviceContext()->Unmap(ConstantBufferViewProj, 0);
	}
}

void URenderer::UpdateConstant(const FMatrix& InMatrix) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE MappedSubResource;
		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);
		memcpy(MappedSubResource.pData, &InMatrix, sizeof(FMatrix));
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FVector4& InColor) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FVector4* ColorConstants = static_cast<FVector4*>(ConstantBufferMSR.pData);
		{
			ColorConstants->X = InColor.X;
			ColorConstants->Y = InColor.Y;
			ColorConstants->Z = InColor.Z;
			ColorConstants->W = InColor.W;
		}
		GetDeviceContext()->Unmap(ConstantBufferColor, 0);
	}
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const
{
	if (!GetDeviceContext() || !InVertexBuffer || InVertices.empty())
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT ResultHandle = GetDeviceContext()->Map(
		InVertexBuffer,
		0, // 서브리소스 인덱스 (버퍼는 0)
		D3D11_MAP_WRITE_DISCARD, // 전체 갱신
		0, // 플래그 없음
		&MappedResource
	);

	if (FAILED(ResultHandle))
	{
		return false;
	}

	// GPU 메모리에 새 데이터 복사
	// TODO: 어쩔 때 한번 read access violation 걸림
	memcpy(MappedResource.pData, InVertices.data(), sizeof(FVector) * InVertices.size());

	// GPU 접근 재허용
	GetDeviceContext()->Unmap(InVertexBuffer, 0);

	return true;
}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	D3D11_FILL_MODE FillMode = ToD3D11(InRenderState.FillMode);
	D3D11_CULL_MODE CullMode = ToD3D11(InRenderState.CullMode);

	const FRasterKey Key{ FillMode, CullMode };
	if (auto Iter = RasterCache.find(Key); Iter != RasterCache.end())
	{
		return Iter->second;
	}

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = FillMode;
	RasterizerDesc.CullMode = CullMode;
	RasterizerDesc.DepthClipEnable = TRUE; // ✅ 근/원거리 평면 클리핑 활성화 (핵심)
	RasterizerDesc.ScissorEnable = TRUE;

	HRESULT ResultHandle = GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(ResultHandle))
	{
		return nullptr;
	}

	RasterCache.emplace(Key, RasterizerState);
	return RasterizerState;
}

D3D11_CULL_MODE URenderer::ToD3D11(ECullMode InCull)
{
	switch (InCull)
	{
	case ECullMode::Back:
		return D3D11_CULL_BACK;
	case ECullMode::Front:
		return D3D11_CULL_FRONT;
	case ECullMode::None:
		return D3D11_CULL_NONE;
	default:
		return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE URenderer::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid:
		return D3D11_FILL_SOLID;
	case EFillMode::WireFrame:
		return D3D11_FILL_WIREFRAME;
	default:
		return D3D11_FILL_SOLID;
	}
}

/**
 * @brief 폰트 렌더링 함수 - FontRenderer를 사용하여 텍스트 렌더링
 */
 //void URenderer::RenderFont()
 //{
 //	if (!FontRenderer)
 //	{
 //		return;
 //	}
 //
 //	// 단순한 직교 투영을 사용하여 테스트 (-100~100 좌표계)
 //	FMatrix WorldMatrix = FMatrix::Identity();
 //
 //	// 직교 투영 행렬 생성 (2D 화면에 맞게)
 //	float left = -100.0f, right = 100.0f;
 //	float bottom = -100.0f, top = 100.0f;
 //	float nearPlane = -1.0f, farPlane = 1.0f;
 //
 //	FMatrix OrthoMatrix;
 //	OrthoMatrix.Data[0][0] = 2.0f / (right - left);
 //	OrthoMatrix.Data[1][1] = 2.0f / (top - bottom);
 //	OrthoMatrix.Data[2][2] = -2.0f / (farPlane - nearPlane);
 //	OrthoMatrix.Data[3][0] = -(right + left) / (right - left);
 //	OrthoMatrix.Data[3][1] = -(top + bottom) / (top - bottom);
 //	OrthoMatrix.Data[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
 //	OrthoMatrix.Data[0][1] = OrthoMatrix.Data[0][2] = OrthoMatrix.Data[0][3] = 0.0f;
 //	OrthoMatrix.Data[1][0] = OrthoMatrix.Data[1][2] = OrthoMatrix.Data[1][3] = 0.0f;
 //	OrthoMatrix.Data[2][0] = OrthoMatrix.Data[2][1] = OrthoMatrix.Data[2][3] = 0.0f;
 //	OrthoMatrix.Data[3][3] = 1.0f;
 //
 //	FMatrix ViewProjMatrix = OrthoMatrix; // 단순히 직교 투영만 사용
 //
 //	// FontRenderer를 사용하여 "Hello, World!" 텍스트 렌더링
 //	FontRenderer->RenderHelloWorld(WorldMatrix, ViewProjMatrix);
 //}
