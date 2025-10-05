#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Editor/Public/Editor.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"
#include "Material/Public/Material.h"
#include "Runtime/Subsystem/Public/OverlayManagerSubsystem.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Manager/Viewport/Public/ViewportManager.h"
#include "Window/Public/Viewport.h"
#include "Window/Public/ViewportClient.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "Render/Renderer/Public/EditorRenderResources.h"

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());

	// RHIDevice 초기화 (렌더링 리소스 생성 전담)
	RHIDevice = &URHIDevice::GetInstance();
	RHIDevice->Initialize(GetDevice(), GetDeviceContext());

	// 에디터 렌더링 리소스 초기화 (기즈모, 그리드 등)
	EditorResources = new FEditorRenderResources();
	EditorResources->InitRHI(RHIDevice);

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateSamplerState();
	CreateDefaultShader();
	CreateConstantBuffer();

	// FontRenderer 초기화
	// TODO: 삭제해야하고, 삭제해도 잘 돌아가도록 수정해줘야 합니다~
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
	ReleaseSamplerState();
	ReleaseDepthStencilState();
	ReleaseRasterizerState();

	// FontRenderer 해제
	SafeDelete(FontRenderer);

	// 에디터 렌더링 리소스 해제
	if (EditorResources)
	{
		EditorResources->ReleaseRHI(RHIDevice);
		SafeDelete(EditorResources);
	}

	// RHIDevice 종료
	if (RHIDevice)
	{
		RHIDevice->Shutdown();
		RHIDevice = nullptr;
	}

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

void URenderer::CreateSamplerState()
{
	// NOTE: Mipmap이 없는 텍스처 사용을 전제로 만들어짐.
	// 추후 Mipmap 적용된 텍스처 사용하게 되면 수정 요망.
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	GetDevice()->CreateSamplerState(&SamplerDesc, &DefaultSamplerState);
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
 * @brief Release Default Sampler State
 */
void URenderer::ReleaseSamplerState()
{
	if(DefaultSamplerState)
	{
		DefaultSamplerState->Release();
		DefaultSamplerState = nullptr;
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

    TArray<FRect> ViewRects;
    UViewportManager::GetInstance().GetLeafRects(ViewRects);
	if (ViewRects.empty())
	{
		RenderLevel();
		if (UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>())
		{
			WorldSS->GetEditor()->RenderEditor();
		}
	}
	//리프창이 있을때
	else
	{
		ID3D11DeviceContext* DevictContext = GetDeviceContext();
		const D3D11_VIEWPORT& FullVP = GetDeviceResources()->GetViewportInfo();
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();

		// 초기 뷰포트 값 0초기화
		ViewportIdx = 0;
        for (int32 i = 0;i < ViewRects.size();++i)
        {
            D3D11_VIEWPORT Viewport{};
            int32 ToolH = 0;
            {

                if (i < (int32)Viewports.size() && Viewports[i])
                {
                    ToolH = Viewports[i]->GetToolbarHeight();
                }
            }
            Viewport.TopLeftX = (FLOAT)ViewRects[i].X; Viewport.TopLeftY = (FLOAT)(ViewRects[i].Y + ToolH);
            Viewport.Width = (FLOAT)max(0L, ViewRects[i].W); Viewport.Height = (FLOAT)max(0L, ViewRects[i].H - ToolH);

			// Skip degenerate rects
			if (Viewport.Width <= 0.0f || Viewport.Height <= 0.0f) continue;

			Viewport.MinDepth = 0.0f; Viewport.MaxDepth = 1.0f;
			DevictContext->RSSetViewports(1, &Viewport);
            D3D11_RECT Screen{};
            Screen.left = ViewRects[i].X; Screen.top = ViewRects[i].Y + ToolH; Screen.right = ViewRects[i].X + ViewRects[i].W; Screen.bottom = ViewRects[i].Y + ViewRects[i].H;
            DevictContext->RSSetScissorRects(1, &Screen);

            {
                auto& ViewportManager = UViewportManager::GetInstance();
                const auto& Viewports = ViewportManager.GetViewports();
                if (ViewportIdx < Viewports.size() && Viewports[ViewportIdx])
                {
                    FViewport* ViewportObject = Viewports[ViewportIdx];
                    FViewportClient* ViewportObjectClient = ViewportObject->GetViewportClient();
					if (ViewportObjectClient->GetViewType() == EViewType::Perspective) {
						if (auto* Camera = ViewportObjectClient->GetPerspectiveCamera()) {
							Camera->GetCameraComponent()->SetAspect(Viewport.Width / max(1.f, Viewport.Height));
							// 입력/움직임 갱신은 그대로 두고, 투영만 즉시 갱신하고 싶으면:
							Camera->GetCameraComponent()->UpdateMatrixByPers(); // 또는 필요 시 Cam->Update() 호출
							UpdateConstant(Camera->GetCameraComponent()->GetFViewProjConstants());
						}
					}
					else {
						if (auto* Camera = ViewportObjectClient->GetOrthoCamera()) {
							Camera->GetCameraComponent()->SetAspect(Viewport.Width / max(1.f, Viewport.Height));
							Camera->GetCameraComponent()->UpdateMatrixByOrth(); // 또는 Cam->Update()
							UpdateConstant(Camera->GetCameraComponent()->GetFViewProjConstants());
						}
					}
                }
            }

            // World + editor overlays for this viewport tile
            RenderLevel();
            if (UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>())
            {
                WorldSS->GetEditor()->RenderEditor();
            }

            ViewportIdx++;
        }

		DevictContext->RSSetViewports(1, &FullVP);
		D3D11_RECT ScreenFull{};
		ScreenFull.left = (LONG)FullVP.TopLeftX; ScreenFull.top = (LONG)FullVP.TopLeftY;
		ScreenFull.right = (LONG)(FullVP.TopLeftX + FullVP.Width);
		ScreenFull.bottom = (LONG)(FullVP.TopLeftY + FullVP.Height);
		DevictContext->RSSetScissorRects(1, &ScreenFull);
	}
	// ImGui 자체 Render 처리가 진행되어야 하므로 따로 처리
    UUIManager::GetInstance().Render();

    // 오버레이 렌더링
    if (auto* OverlayManager = GEngine->GetEngineSubsystem<UOverlayManagerSubsystem>())
    {
    	OverlayManager->RenderOverlay();
    }

	RenderEnd();
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin() const
{
	ID3D11RenderTargetView* RenderTargetView = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);

	ID3D11DepthStencilView* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	float MainMenuH = UUIManager::GetInstance().GetMainMenuBarHeight();

	DeviceResources->UpdateViewport(MainMenuH);
	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* RTV[] = { RenderTargetView }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, RTV, DeviceResources->GetDepthStencilView());
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();
	if (!CurrentLevel)
	{
		return;
	}

	const auto& LevelActors = CurrentLevel->GetLevelActors();

	// Actor들을 순회하면서 각 Actor의 PrimitiveComponent들을 렌더링
	for (const auto& Actor : LevelActors)
	{
		if (Actor)
		{
			// Actor의 모든 PrimitiveComponent들을 가져옴
			TArray<UPrimitiveComponent*> PrimitiveComponents = Actor->GetPrimitiveComponents();
			for (auto* PrimitiveComponent : PrimitiveComponents)
			{
				if (PrimitiveComponent && PrimitiveComponent->IsVisible() && PrimitiveComponent->HasRenderData())
				{
					RenderPrimitiveComponent(PrimitiveComponent);
				}
			}
		}
	}
}

/**
 * @brief PrimitiveComponent를 렌더링하는 통합 함수
 * @param InPrimitiveComponent 렌더링할 프리미티브 컴포넌트
 */
void URenderer::RenderPrimitiveComponent(UPrimitiveComponent* InPrimitiveComponent)
{
	if (!InPrimitiveComponent || !InPrimitiveComponent->HasRenderData())
	{
		return;
	}

	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;

	// BillBoard 컴포넌트의 경우 특별 처리
	if (InPrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard)
	{
		if (CurrentLevel && (CurrentLevel->GetShowFlags() & EEngineShowFlags::SF_BillboardText))
		{
			UBillBoardComponent* BillBoardComponent = Cast<UBillBoardComponent>(InPrimitiveComponent);
			if (BillBoardComponent)
			{
				auto& ViewportManager = UViewportManager::GetInstance();
				const auto& VPs = ViewportManager.GetViewports();
				FViewport* VP = (ViewportIdx < VPs.size()) ? VPs[ViewportIdx] : nullptr;
				FViewportClient* VC = VP ? VP->GetViewportClient() : nullptr;

				if (!VC) return;

				ACameraActor* CurrentCamera;
				FViewProjConstants ViewProj;
				if (VC->GetViewType() == EViewType::Perspective)
				{
					CurrentCamera = VC->GetPerspectiveCamera();
					ViewProj = VC->GetPerspectiveViewProjConstData();
				}
				else
				{
					CurrentCamera = VC->GetOrthoCamera();
					ViewProj = VC->GetOrthoGraphicViewProjConstData();
				}

				// 현재 뷰포트 카메라 기준으로 billboard 회전행렬 갱신
				BillBoardComponent->UpdateRotationMatrix(CurrentCamera);

				// 현재 뷰포트의 View/Proj로 텍스트 그리기
				RenderBillBoardDirect(BillBoardComponent, ViewProj);
			}
		}
		return;
	}

	// StaticMesh 컴포넌트는 머터리얼 처리를 위해 전용 경로 사용
	if (InPrimitiveComponent->GetPrimitiveType() == EPrimitiveType::StaticMeshComp)
	{
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(InPrimitiveComponent);
		if (StaticMeshComponent)
		{
			RenderStaticMeshComponent(StaticMeshComponent);
			return;
		}
	}

	// 일반 프리미티브의 경우 기존 렌더링 방식 사용
	// 공통 파이프라인 설정
	SetupRenderPipeline(InPrimitiveComponent);

	// 렌더링 방식 자동 선택
	if (InPrimitiveComponent->UseIndexedRendering())
	{
		RenderIndexed(InPrimitiveComponent);
	}
	else
	{
		RenderDirect(InPrimitiveComponent);
	}
}

/**
 * @brief 렌더링 파이프라인 설정
 * @param InPrimitiveComponent 설정할 프리미티브 컴포넌트
 */

// 이 부분에서 뷰포트의 걸로 그려야함
void URenderer::SetupRenderPipeline(UPrimitiveComponent* InPrimitiveComponent)
{
	FRenderState RenderState = InPrimitiveComponent->GetRenderState();

	// 에디터 뷰 모드에 따른 렌더 상태 조정
	//const EViewModeIndex ViewMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();

	FViewport* Viewport = UViewportManager::GetInstance().GetViewports()[ViewportIdx];
	FViewportClient* ViewportClient = Viewport->GetViewportClient();
	const EViewMode ViewMode = Viewport->GetViewportClient()->GetViewMode();


	if (ViewMode == EViewMode::WireFrame)
	{
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}

	// 셰이더 선택 (StaticMesh면 전용 셰이더, 아니면 기본 셰이더)
	ID3D11VertexShader* VertexShader = DefaultVertexShader;
	ID3D11PixelShader* PixelShader = DefaultPixelShader;
	ID3D11InputLayout* InputLayout = DefaultInputLayout;

	if (InPrimitiveComponent->GetPrimitiveType() == EPrimitiveType::StaticMeshComp)
	{
		ID3D11VertexShader* StaticMeshVS = EditorResources->GetEditorVertexShader(EShaderType::StaticMesh);
		ID3D11PixelShader* StaticMeshPS = EditorResources->GetEditorPixelShader(EShaderType::StaticMesh);
		ID3D11InputLayout* StaticMeshLayout = EditorResources->GetEditorInputLayout(EShaderType::StaticMesh);

		if (StaticMeshVS) VertexShader = StaticMeshVS;
		if (StaticMeshPS) PixelShader = StaticMeshPS;
		if (StaticMeshLayout) InputLayout = StaticMeshLayout;
	}

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

	// 파이프라인 정보 설정
	FPipelineInfo PipelineInfo = {
		InputLayout,
		VertexShader,
		RasterizerState,
		DefaultDepthStencilState,
		PixelShader,
		nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	// 상수 버퍼 업데이트
	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(
		InPrimitiveComponent->GetRelativeLocation(),
		InPrimitiveComponent->GetRelativeRotation(),
		InPrimitiveComponent->GetRelativeScale3D());

	//FViewProjConstants GetPerspectiveViewProjConstData() const { return PerspectiveCamera->GetFViewProjConstants(); }
	//FViewProjConstants GetOrthoGraphicViewProjConstData() const { return OrthoGraphicCameraShared->GetFViewProjConstants(); }

	if (EViewType::Perspective == ViewportClient->GetViewType())
	{
		const FViewProjConstants& ViewProjConstants = ViewportClient->GetPerspectiveViewProjConstData();
		UpdateConstant(ViewProjConstants);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(InPrimitiveComponent->GetColor());
	}
	else
	{
		const FViewProjConstants& ViewProjConstants = ViewportClient->GetOrthoGraphicViewProjConstData();
		UpdateConstant(ViewProjConstants);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(InPrimitiveComponent->GetColor());
	}
}

/**
 * @brief 인덱스 버퍼를 사용한 렌더링
 * @param InPrimitiveComponent 렌더링할 프리미티브 컴포넌트
 */
void URenderer::RenderIndexed(UPrimitiveComponent* InPrimitiveComponent)
{
	ID3D11Buffer* VertexBuffer = InPrimitiveComponent->GetRenderVertexBuffer();
	ID3D11Buffer* IndexBuffer = InPrimitiveComponent->GetRenderIndexBuffer();

	if (!VertexBuffer || !IndexBuffer)
	{
		return;
	}

	Pipeline->SetVertexBuffer(VertexBuffer, InPrimitiveComponent->GetRenderVertexStride());
	Pipeline->SetIndexBuffer(IndexBuffer, sizeof(uint32));
	Pipeline->DrawIndexed(InPrimitiveComponent->GetRenderIndexCount(), 0, 0);
}

/**
 * @brief 직접 정점 배열을 사용한 렌더링
 * @param InPrimitiveComponent 렌더링할 프리미티브 컴포넌트
 */
void URenderer::RenderDirect(UPrimitiveComponent* InPrimitiveComponent)
{
	ID3D11Buffer* VertexBuffer = InPrimitiveComponent->GetRenderVertexBuffer();

	if (!VertexBuffer)
	{
		return;
	}

	Pipeline->SetVertexBuffer(VertexBuffer, InPrimitiveComponent->GetRenderVertexStride());
	Pipeline->Draw(InPrimitiveComponent->GetRenderVertexCount(), 0);
}

/**
 * @brief StaticMeshComponent를 렌더링하는 함수. 머티리얼 등의 처리를 위해 별도 함수 사용.
 * @param InStaticMeshComponent 렌더링할 스태틱 메시 컴포넌트
 */
void URenderer::RenderStaticMeshComponent(UStaticMeshComponent* InStaticMeshComponent)
{
	if (!InStaticMeshComponent || !InStaticMeshComponent->HasRenderData())
	{
		return;
	}

	UStaticMesh* StaticMesh = InStaticMeshComponent->GetStaticMesh();
	if (!StaticMesh || !StaticMesh->IsValidMesh())
	{
		return;
	}

	SetupRenderPipeline(InStaticMeshComponent);

	ID3D11Buffer* VertexBuffer = InStaticMeshComponent->GetRenderVertexBuffer();
	if (!VertexBuffer)
	{
		return;
	}

	Pipeline->SetVertexBuffer(VertexBuffer, InStaticMeshComponent->GetRenderVertexStride());

	const FVector4 FallbackColor = InStaticMeshComponent->GetColor();
	const TArray<UMaterialInterface*>& MaterialSlots = StaticMesh->GetMaterialSlots();

	ID3D11Buffer* IndexBuffer = InStaticMeshComponent->GetRenderIndexBuffer();
	const bool bUseIndexedRendering = InStaticMeshComponent->UseIndexedRendering() && IndexBuffer != nullptr;

	if (bUseIndexedRendering)
	{
		Pipeline->SetIndexBuffer(IndexBuffer, sizeof(uint32));

		const TArray<FStaticMeshSection>& MeshSections = StaticMesh->GetStaticMeshData().Sections;
		bool bDrewSection = false;
		for (const FStaticMeshSection& Section : MeshSections)
		{
			if (Section.IndexCount <= 0)
			{
				continue;
			}

			RenderStaticMeshSection(Section, MaterialSlots, InStaticMeshComponent, FallbackColor);
			bDrewSection = true;
		}

		if (!bDrewSection)
		{
			ApplyMaterial(nullptr, FallbackColor);
			Pipeline->DrawIndexed(InStaticMeshComponent->GetRenderIndexCount(), 0, 0);
		}
	}
	else
	{
		ApplyMaterial(nullptr, FallbackColor);
		Pipeline->Draw(InStaticMeshComponent->GetRenderVertexCount(), 0);
	}
}

/**
 * @brief StaticMesh의 섹션 단위로 렌더링하는 함수
 * @param InSection 렌더링할 스태틱 메시 섹션 정보
 * @param InMaterialSlots 메시가 참조하는 머티리얼 슬롯 배열
 * @param InStaticMeshComp 머티리얼 오버라이드를 참조할 스태틱 메시 컴포넌트
 * @param InFallbackColor 머티리얼이 없을 때 사용할 대체 색상
 */
void URenderer::RenderStaticMeshSection(const FStaticMeshSection& InSection
	, const TArray<UMaterialInterface*>& InMaterialSlots
	, const UStaticMeshComponent* InStaticMeshComp
	, const FVector4& InFallbackColor)
{
	UMaterialInterface* SectionMaterial = nullptr;
	if (InSection.MaterialSlotIndex >= 0 && InSection.MaterialSlotIndex < static_cast<int32>(InMaterialSlots.size()))
	{
		// StaticMeshComponent에 머티리얼 오버라이드가 있으면 우선 적용
		if (InStaticMeshComp)
		{
			UMaterialInterface* OverriddenMaterial = InStaticMeshComp->GetMaterialOverride(InSection.MaterialSlotIndex);
			if (OverriddenMaterial)
			{
				SectionMaterial = OverriddenMaterial;
			}
		}

		if (!SectionMaterial)
		{
			SectionMaterial = InMaterialSlots[InSection.MaterialSlotIndex];
		}
	}

	ApplyMaterial(SectionMaterial, InFallbackColor);
	Pipeline->DrawIndexed(static_cast<uint32>(InSection.IndexCount), static_cast<uint32>(InSection.StartIndex), 0);
}

/**
 * @brief 머티리얼을 적용하는 함수
 * @param InMaterial 적용할 머티리얼 인터페이스
 * @param InFallbackColor 머티리얼이 없을 때 사용할 대체 색상
 */
void URenderer::ApplyMaterial(UMaterialInterface* InMaterial, const FVector4& InFallbackColor)
{
	FVector4 FinalColor = InFallbackColor;
	ID3D11ShaderResourceView* DiffuseSRV = nullptr;
	bool bAppliedMaterial = false;

	auto UseMaterial = [&](UMaterial* Material)
	{
		if (!Material)
		{
			return;
		}

		const FObjMaterialInfo& Info = Material->GetMaterialInfo();
		FinalColor = FVector4(Info.DiffuseColorScalar.X, Info.DiffuseColorScalar.Y, Info.DiffuseColorScalar.Z, Info.TransparencyScalar);
		DiffuseSRV = Material->GetDiffuseTexture();
		bAppliedMaterial = true;
	};

	if (InMaterial)
	{
		if (UMaterial* Material = Cast<UMaterial>(InMaterial))
		{
			UseMaterial(Material);
		}
		else if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(InMaterial))
		{
			UseMaterial(MaterialInstance->Parent);
		}
	}

	if (DefaultSamplerState)
	{
		Pipeline->SetSamplerState(0, false, DefaultSamplerState);
	}

	if (DiffuseSRV)
	{
		Pipeline->SetTexture(0, false, DiffuseSRV);
	}
	else
	{
		ID3D11ShaderResourceView* NullSrv = nullptr;
		GetDeviceContext()->PSSetShaderResources(0, 1, &NullSrv);
	}

	const bool bUseVertexColor = !bAppliedMaterial;
	const bool bUseDiffuseTexture = DiffuseSRV != nullptr;

	UpdateConstant(FinalColor, bUseVertexColor ? 1.0f : 0.0f, bUseDiffuseTexture ? 1.0f : 0.0f);
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
 * @brief 기즈모 전용 렌더링 함수 - 화면 공간에서 일정한 크기 유지
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 * @param InCameraLocation 카메라 위치
 * @param InViewportWidth 뷰포트 폭
 * @param InViewportHeight 뷰포트 높이
 * @param InDesiredPixelSize 희망하는 픽셀 크기
 */
void URenderer::RenderGizmoPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState,
                                    const FVector& InCameraLocation, float InViewportWidth, float InViewportHeight,
                                    float InDesiredPixelSize)
{
	// 기본 유효성 검사
	if (InViewportWidth <= 0.0f || InViewportHeight <= 0.0f)
	{
		// 뷰포트 정보가 없으면 기본 렌더링으로 대체
		RenderPrimitive(InPrimitive, InRenderState);
		return;
	}

	// 기즈모와 카메라 간의 거리 계산
	float DistanceToCamera = (InCameraLocation - InPrimitive.Location).Length();
	DistanceToCamera = max(DistanceToCamera, 0.1f); // 최소 거리 보장

	// ViewProj 행렬에서 FOV 추출 (현재 렌더링 중인 뷰포트의 카메라 사용)
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	if (ViewportIdx >= 0 && ViewportIdx < static_cast<int32>(Viewports.size()) &&
		ViewportIdx < static_cast<int32>(Clients.size()) &&
		Viewports[ViewportIdx] && Clients[ViewportIdx])
	{
		FViewportClient* CurrentClient = Clients[ViewportIdx];
		ACameraActor* ViewportCamera = CurrentClient->IsOrtho()
		                              ? CurrentClient->GetOrthoCamera()
		                              : CurrentClient->GetPerspectiveCamera();

		if (ViewportCamera && ViewportCamera->GetCameraComponent())
		{
			UCameraComponent* CameraComp = ViewportCamera->GetCameraComponent();
			float ScreenSpaceScale = 1.0f;

			// 카메라 타입에 따른 스케일 계산
				if (CameraComp->GetCameraType() == ECameraType::ECT_Perspective)
				{
					// 원근 투영: FOV와 거리를 이용한 화면 공간 크기 계산
					float FovYRadians = CameraComp->GetFovY() * (PI / 180.0f);
					float TanHalfFov = tanf(FovYRadians * 0.5f);

					// 화면 공간에서 1픽셀이 월드 공간에서 차지하는 크기
					float WorldUnitsPerPixel = (2.0f * DistanceToCamera * TanHalfFov) / InViewportHeight;

					// 화면 가장자리 보정 (원근 투영에서 가장자리로 갈수록 커지는 현상 방지)
					FVector CameraToGizmo = InPrimitive.Location - InCameraLocation;
					FVector CameraForward = CameraComp->GetForward();
					CameraToGizmo.Normalize();
					CameraForward.Normalize();

					// 카메라 중심축과의 각도 계산 (코사인 값)
					float CosAngle = CameraForward.Dot(CameraToGizmo);
					CosAngle = max(0.2f, CosAngle); // 최소 값 제한으로 가장자리에서 급격한 증가 방지

					// 가장자리 보정 팩터 (가장자리로 갈수록 작아짐)
					float EdgeCorrectionFactor = CosAngle;

					ScreenSpaceScale = (WorldUnitsPerPixel * InDesiredPixelSize) * EdgeCorrectionFactor;
				}
			else
			{
				// 직교 투영: 거리에 관계없이 일정한 크기
				float OrthographicHeight = CameraComp->GetOrthographicHeight();

				if (OrthographicHeight > 0.0f && InViewportHeight > 0.0f)
				{
					float WorldUnitsPerPixel = OrthographicHeight / InViewportHeight;
					ScreenSpaceScale = WorldUnitsPerPixel * InDesiredPixelSize;

					// 최소/최대 스케일 제한 (기즈모가 너무 작거나 커지는 방지)
					ScreenSpaceScale = max(0.01f, min(100.0f, ScreenSpaceScale));
				}
				else
				{
					ScreenSpaceScale = 1.0f; // 기본값
				}
			}

			// 계산된 스케일로 기존 프리미티브 스케일 조정
			FEditorPrimitive ScaledPrimitive = InPrimitive;
			ScaledPrimitive.Scale = FVector(ScreenSpaceScale, ScreenSpaceScale, ScreenSpaceScale);

			// 일반적인 렌더링으로 처리
			RenderPrimitive(ScaledPrimitive, InRenderState);
			return;
		}
	}

	// 카메라 정보를 찾을 수 없으면 기본 렌더링
	RenderPrimitive(InPrimitive, InRenderState);
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

	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

	Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
	UpdateConstant(InPrimitive.Color);

	// Set buffers and draw indexed
	Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, InStride);
	Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
}

/**
 * @brief BillBoard 직접 렌더링
 * @param InBillBoardComponent BillBoard 컴포넌트
 */
void URenderer::RenderBillBoardDirect(UBillBoardComponent* InBillBoardComponent, const FViewProjConstants& InViewProj)
{
	if (!InBillBoardComponent || !FontRenderer)
	{
		return;
	}

	// BillBoard의 RT 매트릭스와 텍스트 가져오기
	const FMatrix& RTMatrix = InBillBoardComponent->GetRTMatrix();

	// Actor의 UUID를 표시 텍스트로 설정
	FString DisplayText = "Unknown";
	if (auto* Owner = InBillBoardComponent->GetOwner())
		DisplayText = "아이디:" + std::to_string(Owner->GetUUID());

	FontRenderer->RenderText(DisplayText, RTMatrix, InViewProj);
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
	//UE_LOG("%d   %d", InWidth, InHeight);
	//UE_LOG("OnResize Failed");
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
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) * 2 + 0xf & 0xfffffff0; // 16바이트 단위 정렬
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

/**
* @brief 머티리얼 상수 버퍼 업데이트 함수
* @note: Diffuse Texture 사용시 Diffuse Color 대신 DiffuseTexture 색상 사용. Vertex 자체 색상 사용시 머티리얼에 의한 색상은 무시됨.
* @param InColor: 머티리얼의 Diffuse Color
* @param InUseVertexColor: 정점 자체 색상 사용 여부 (1.0f = 사용, 0.0f = 미사용)
* @param InUseDiffuseTexture: 머티리얼의 Diffuse Texture 사용 여부 (1.0f = 사용, 0.0f = 미사용)
*/
void URenderer::UpdateConstant(const FVector4& InColor, float InUseVertexColor, float InUseDiffuseTexture) const
{
	static float TimeAccumulator = 0.0f;
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		FVector4* MaterialConstants = static_cast<FVector4*>(ConstantBufferMSR.pData);
		if (MaterialConstants)
		{
			MaterialConstants[0].X = InColor.X;
			MaterialConstants[0].Y = InColor.Y;
			MaterialConstants[0].Z = InColor.Z;
			MaterialConstants[0].W = InColor.W;

			// UV Scroll 값을 ActorDetailWidget에서 가져오기
			float UseUVScroll = 0.0f;
			if (TObjectPtr<UWidget> Widget = UUIManager::GetInstance().FindWidget(FName("Actor Detail Widget")))
			{
				if (UActorDetailWidget* ActorDetailWidget = Cast<UActorDetailWidget>(Widget))
				{
					if (ActorDetailWidget->GetUseUVScroll())
					{
						UseUVScroll = ActorDetailWidget->GetUseUVScroll() ? 1.0f : 0.0f;
						TimeAccumulator += DT;
					}
					else
					{
						TimeAccumulator = 0.0f; // UV Scroll 사용 안 하면 시간 초기화
					}
				}
			}

			MaterialConstants[1].X = InUseVertexColor;
			MaterialConstants[1].Y = InUseDiffuseTexture;
			MaterialConstants[1].Z = UseUVScroll;
			MaterialConstants[1].W = TimeAccumulator;
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
