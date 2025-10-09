#include "pch.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Core/Public/VertexTypes.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Runtime/Renderer/Public/EditorGrid.h"

FEditorRenderResources::FEditorRenderResources() = default;

FEditorRenderResources::~FEditorRenderResources()
{
	Shutdown();
}

void FEditorRenderResources::Initialize(FRHIDevice* InRHIDevice)
{
	if (bIsInitialized)
	{
		return;
	}

	RHIDevice = InRHIDevice;
	if (!RHIDevice)
	{
		UE_LOG_ERROR("FEditorRenderResources: Initialize 실패 - RHIDevice가 null입니다");
		return;
	}

	LoadEditorShaders();
	InitializeGrid();
	InitializeGizmoResources();
	bIsInitialized = true;

	UE_LOG("FEditorRenderResources: 초기화 완료");
}

void FEditorRenderResources::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	ReleaseEditorShaders();
	ReleaseGrid();
	ReleaseGizmoResources();
	RHIDevice = nullptr;
	bIsInitialized = false;

	UE_LOG("FEditorRenderResources: 종료 완료");
}

ID3D11VertexShader* FEditorRenderResources::GetEditorVertexShader(EShaderType ShaderType) const
{
	switch (ShaderType)
	{
	case EShaderType::BatchLine:
		return EditorShaders.BatchLineVS;
	case EShaderType::StaticMesh:
		return EditorShaders.StaticMeshVS;
	default:
		return EditorShaders.BatchLineVS; // 기본값
	}
}

ID3D11PixelShader* FEditorRenderResources::GetEditorPixelShader(EShaderType ShaderType) const
{
	switch (ShaderType)
	{
	case EShaderType::BatchLine:
		return EditorShaders.BatchLinePS;
	case EShaderType::StaticMesh:
		return EditorShaders.StaticMeshPS;
	default:
		return EditorShaders.BatchLinePS; // 기본값
	}
}

ID3D11InputLayout* FEditorRenderResources::GetEditorInputLayout(EShaderType ShaderType) const
{
	switch (ShaderType)
	{
	case EShaderType::BatchLine:
		return EditorShaders.BatchLineIL;
	case EShaderType::StaticMesh:
		return EditorShaders.StaticMeshIL;
	default:
		return EditorShaders.BatchLineIL; // 기본값
	}
}

ID3D11Buffer* FEditorRenderResources::CreateVertexBuffer(const void* Data, uint32 SizeInBytes, bool bCpuAccess)
{
	if (!RHIDevice)
	{
		return nullptr;
	}

	return RHIDevice->CreateVertexBuffer(Data, SizeInBytes, bCpuAccess);
}

ID3D11Buffer* FEditorRenderResources::CreateIndexBuffer(const void* Data, uint32 SizeInBytes)
{
	if (!RHIDevice)
	{
		return nullptr;
	}

	return RHIDevice->CreateIndexBuffer(Data, SizeInBytes);
}

void FEditorRenderResources::ReleaseVertexBuffer(ID3D11Buffer* Buffer)
{
	if (RHIDevice)
	{
		RHIDevice->ReleaseVertexBuffer(Buffer);
	}
}

void FEditorRenderResources::ReleaseIndexBuffer(ID3D11Buffer* Buffer)
{
	if (RHIDevice)
	{
		RHIDevice->ReleaseIndexBuffer(Buffer);
	}
}

void FEditorRenderResources::RenderPrimitiveIndexed(const FEditorPrimitive& Primitive, const FRenderState& RenderState,
                                                    bool bIsBlendMode, uint32 VertexStride, uint32 IndexStride)
{
	if (!RHIDevice || !RHIDevice->GetDeviceContext())
	{
		return;
	}

	ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();

	// 셰이더 설정
	DeviceContext->VSSetShader(Primitive.VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(Primitive.PixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(Primitive.InputLayout);

	// 버퍼 바인딩
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &Primitive.Vertexbuffer, &VertexStride, &offset);
	DeviceContext->IASetIndexBuffer(Primitive.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DeviceContext->IASetPrimitiveTopology(Primitive.Topology);

	// 블렌드 상태 설정
	RHIDevice->OMSetBlendState(bIsBlendMode);

	// 렌더링
	DeviceContext->DrawIndexed(Primitive.NumIndices, 0, 0);
}

void FEditorRenderResources::UpdateVertexBuffer(ID3D11Buffer* Buffer, const TArray<FVector>& Vertices)
{
	if (!RHIDevice || !Buffer)
	{
		return;
	}

	RHIDevice->UpdateVertexBuffer(Buffer, Vertices.GetData(), static_cast<uint32>(Vertices.Num() * sizeof(FVector)));
}

void FEditorRenderResources::LoadEditorShaders()
{
	if (!RHIDevice)
	{
		UE_LOG_ERROR("LoadEditorShaders - RHIDevice is null!");
		return;
	}

	UE_LOG("LoadEditorShaders - Starting shader loading...");

	// BatchLine 셸이더 로드 (간단한 위치+색상 셸이더 - FVertexSimple 사용)
	TArray<D3D11_INPUT_ELEMENT_DESC> BatchLineLayout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	UE_LOG("LoadEditorShaders - Creating BatchLine shaders...");

	RHIDevice->CreateVertexShaderAndInputLayout(
		L"BatchLineVS.hlsl",
		BatchLineLayout,
		&EditorShaders.BatchLineVS,
		&EditorShaders.BatchLineIL
	);

	if (EditorShaders.BatchLineVS && EditorShaders.BatchLineIL)
	{
		UE_LOG("LoadEditorShaders - BatchLine VS and IL created successfully");
	}
	else
	{
		UE_LOG_ERROR("LoadEditorShaders - Failed to create BatchLine VS/IL! VS:%p, IL:%p",
		             EditorShaders.BatchLineVS, EditorShaders.BatchLineIL);
		assert(!"Failed to load BatchLineVS.hlsl shader! Check if shader file exists.");
	}

	RHIDevice->CreatePixelShader(L"BatchLinePS.hlsl", &EditorShaders.BatchLinePS);

	if (EditorShaders.BatchLinePS)
	{
		UE_LOG("LoadEditorShaders - BatchLine PS created successfully");
	}
	else
	{
		UE_LOG_ERROR("LoadEditorShaders - Failed to create BatchLine PS!");
		assert(!"Failed to load BatchLinePS.hlsl shader! Check if shader file exists.");
	}

	// StaticMesh 셰이더 로드 (기본 메시 셰이더)
	TArray<D3D11_INPUT_ELEMENT_DESC> StaticMeshLayout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	RHIDevice->CreateVertexShaderAndInputLayout(
		L"StaticMeshShader.hlsl",
		StaticMeshLayout,
		&EditorShaders.StaticMeshVS,
		&EditorShaders.StaticMeshIL
	);

	RHIDevice->CreatePixelShader(L"StaticMeshShader.hlsl", &EditorShaders.StaticMeshPS);
}

void FEditorRenderResources::ReleaseEditorShaders()
{
	if (RHIDevice)
	{
		RHIDevice->ReleaseShader(EditorShaders.BatchLineVS);
		RHIDevice->ReleaseShader(EditorShaders.BatchLinePS);
		RHIDevice->ReleaseShader(EditorShaders.BatchLineIL);

		RHIDevice->ReleaseShader(EditorShaders.StaticMeshVS);
		RHIDevice->ReleaseShader(EditorShaders.StaticMeshPS);
		RHIDevice->ReleaseShader(EditorShaders.StaticMeshIL);
	}

	EditorShaders = {};
}

void FEditorRenderResources::InitializeGrid()
{
	UE_LOG("EditorRenderResources::InitializeGrid - Creating EditorGrid...");

	EditorGrid = MakeUnique<FEditorGrid>();

	if (EditorGrid)
	{
		uint32 NumVerts = EditorGrid->GetNumVertices();
		UE_LOG("EditorRenderResources::InitializeGrid - EditorGrid created successfully. NumVertices: %u", NumVerts);
	}
	else
	{
		UE_LOG_ERROR("EditorRenderResources::InitializeGrid - Failed to create EditorGrid!");
	}

	// Grid 버퍼 는 RenderGrid()에서 스레드 안전하게 생성됨
	// 초기화 시점에 버퍼를 미리 생성할 필요는 없음
}

void FEditorRenderResources::ReleaseGrid()
{
	if (GridVertexBuffer)
	{
		ReleaseVertexBuffer(GridVertexBuffer);
		GridVertexBuffer = nullptr;
	}

	if (GridIndexBuffer)
	{
		ReleaseIndexBuffer(GridIndexBuffer);
		GridIndexBuffer = nullptr;
	}

	EditorGrid.Reset();
}

void FEditorRenderResources::RenderGrid(const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, float CellSize)
{
	// 이 함수는 더 이상 사용되지 않습니다.
	// 그리드 렌더링은 이제 DebugPass에서 BeginLineBatch/EndLineBatch를 통해 처리됩니다.
	// EditorGrid의 CellSize만 업데이트합니다.

	if (!EditorGrid)
	{
		return;
	}

	// CellSize 변경 시 Grid 업데이트
	if (CellSize != EditorGrid->GetCellSize())
	{
		EditorGrid->UpdateVerticesBy(CellSize);
	}
}

void FEditorRenderResources::BeginLineBatch()
{
	bLineBatchActive = true;

	// 배치 데이터 클리어
	BatchedLineStartPoints.Empty();
	BatchedLineEndPoints.Empty();
	BatchedLineColors.Empty();
}

void FEditorRenderResources::AddLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	if (!bLineBatchActive)
	{
		return;
	}

	BatchedLineStartPoints.Add(Start);
	BatchedLineEndPoints.Add(End);
	BatchedLineColors.Add(Color);
}

void FEditorRenderResources::AddLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints,
                                      const TArray<FVector4>& Colors)
{
	if (!bLineBatchActive)
	{
		return;
	}

	// 배열 크기 검증
	if (StartPoints.Num() != EndPoints.Num() || StartPoints.Num() != Colors.Num())
	{
		UE_LOG_ERROR("FEditorRenderResources: AddLines 배열 크기 불일치");
		return;
	}

	// 배치에 추가
	BatchedLineStartPoints.Append(StartPoints);
	BatchedLineEndPoints.Append(EndPoints);
	BatchedLineColors.Append(Colors);
}

void FEditorRenderResources::EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
                                          const FMatrix& ProjectionMatrix, const FRect* ScissorRect)
{
	if (!bLineBatchActive || !RHIDevice || BatchedLineStartPoints.IsEmpty())
	{
		bLineBatchActive = false;
		return;
	}

	// 동적으로 라인 데이터를 FVertexSimple 버퍼로 변환
	TArray<FVertexSimple> vertices;
	TArray<uint32> Indices;

	vertices.Reserve(BatchedLineStartPoints.Num() * 2);
	Indices.Reserve(BatchedLineStartPoints.Num() * 2);

	for (int32 i = 0; i < BatchedLineStartPoints.Num(); ++i)
	{
		uint32 startIndex = static_cast<uint32>(vertices.Num());

		// FVertexSimple로 변환 (Position + Color)
		vertices.Add(FVertexSimple(BatchedLineStartPoints[i], BatchedLineColors[i]));
		vertices.Add(FVertexSimple(BatchedLineEndPoints[i], BatchedLineColors[i]));

		Indices.Add(startIndex);
		Indices.Add(startIndex + 1);
	}

	// 동적 버퍼 생성 및 렌더링
	ID3D11Buffer* tempVertexBuffer = CreateVertexBuffer(vertices.GetData(), vertices.Num() * sizeof(FVertexSimple),
	                                                    true);
	ID3D11Buffer* tempIndexBuffer = CreateIndexBuffer(Indices.GetData(), Indices.Num() * sizeof(uint32));

	if (tempVertexBuffer && tempIndexBuffer)
	{
		// 상수 버퍼 업데이트
		RHIDevice->UpdateConstantBuffers(ModelMatrix, ViewMatrix, ProjectionMatrix);

		ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();

		// Depth Test 비활성화 (라인은 항상 보여야 함)
		D3D11_DEPTH_STENCIL_DESC depthDesc = {};
		depthDesc.DepthEnable = FALSE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		depthDesc.StencilEnable = FALSE;

		ID3D11DepthStencilState* depthState = nullptr;
		RHIDevice->GetDevice()->CreateDepthStencilState(&depthDesc, &depthState);
		if (depthState)
		{
			DeviceContext->OMSetDepthStencilState(depthState, 0);
		}

		// Blend State 설정 (색상 쓰기 활성화)
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		ID3D11BlendState* blendState = nullptr;
		RHIDevice->GetDevice()->CreateBlendState(&blendDesc, &blendState);
		if (blendState)
		{
			DeviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
		}

		// Rasterizer State 설정
		D3D11_RASTERIZER_DESC ResterizerDescription = {};
		ResterizerDescription.FillMode = D3D11_FILL_SOLID;
		ResterizerDescription.CullMode = D3D11_CULL_NONE; // Culling 꺼기
		ResterizerDescription.FrontCounterClockwise = FALSE;
		ResterizerDescription.DepthClipEnable = TRUE;
		ResterizerDescription.ScissorEnable = TRUE;  // Scissor 테스트 활성화

		ID3D11RasterizerState* rastState = nullptr;
		RHIDevice->GetDevice()->CreateRasterizerState(&ResterizerDescription, &rastState);
		if (rastState)
		{
			DeviceContext->RSSetState(rastState);
		}

		// Scissor Rect 설정 (전달받은 경우)
		if (ScissorRect)
		{
			D3D11_RECT D3DScissorRect;
			D3DScissorRect.left = static_cast<LONG>(ScissorRect->X);
			D3DScissorRect.top = static_cast<LONG>(ScissorRect->Y);
			D3DScissorRect.right = static_cast<LONG>(ScissorRect->X + ScissorRect->W);
			D3DScissorRect.bottom = static_cast<LONG>(ScissorRect->Y + ScissorRect->H);
			DeviceContext->RSSetScissorRects(1, &D3DScissorRect);
		}

		// 쉐이더 설정
		DeviceContext->VSSetShader(GetEditorVertexShader(EShaderType::BatchLine), nullptr, 0);
		DeviceContext->PSSetShader(GetEditorPixelShader(EShaderType::BatchLine), nullptr, 0);
		DeviceContext->IASetInputLayout(GetEditorInputLayout(EShaderType::BatchLine));

		// 버퍼 바인딩
		UINT Stride = sizeof(FVertexSimple);
		UINT Offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &tempVertexBuffer, &Stride, &Offset);
		DeviceContext->IASetIndexBuffer(tempIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		// 렌더링
		DeviceContext->DrawIndexed(Indices.Num(), 0, 0);

		// State 해제
		if (depthState)
		{
			depthState->Release();
		}
		if (blendState)
		{
			blendState->Release();
		}
		if (rastState)
		{
			rastState->Release();
		}

		// 임시 버퍼 정리
		ReleaseVertexBuffer(tempVertexBuffer);
		ReleaseIndexBuffer(tempIndexBuffer);
	}

	bLineBatchActive = false;
}

// Gizmo 관련 메서드 구현
ID3D11Buffer* FEditorRenderResources::GetGizmoVertexBuffer(EPrimitiveType PrimitiveType) const
{
	ID3D11Buffer* const* Found = GizmoVertexBuffers.Find(PrimitiveType);
	return Found ? *Found : nullptr;
}

uint32 FEditorRenderResources::GetGizmoVertexCount(EPrimitiveType PrimitiveType) const
{
	const uint32* Found = GizmoVertexCounts.Find(PrimitiveType);
	return Found ? *Found : 0;
}

void FEditorRenderResources::InitializeGizmoResources()
{
	if (!RHIDevice)
	{
		return;
	}

	// Arrow 생성 (Translation Gizmo)
	{
		// 간단한 Arrow 형태의 정점 데이터 (삼각형 목록)
		TArray<FVector> ArrowVertices = {
			// 화살표 몸체 (Cylinder)
			FVector(0.0f, 0.0f, 0.0f), FVector(0.05f, 0.0f, 0.0f), FVector(0.0f, 0.05f, 0.0f),
			FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.05f, 0.0f), FVector(-0.05f, 0.0f, 0.0f),
			FVector(0.0f, 0.0f, 0.0f), FVector(-0.05f, 0.0f, 0.0f), FVector(0.0f, -0.05f, 0.0f),
			FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, -0.05f, 0.0f), FVector(0.05f, 0.0f, 0.0f),

			// 화살표 몸체 연장
			FVector(0.8f, 0.0f, 0.0f), FVector(0.85f, 0.0f, 0.0f), FVector(0.8f, 0.05f, 0.0f),
			FVector(0.8f, 0.0f, 0.0f), FVector(0.8f, 0.05f, 0.0f), FVector(0.75f, 0.0f, 0.0f),

			// 화살표 머리 (Cone)
			FVector(0.8f, 0.0f, 0.0f), FVector(1.0f, 0.0f, 0.0f), FVector(0.9f, 0.1f, 0.0f),
			FVector(0.8f, 0.0f, 0.0f), FVector(0.9f, 0.1f, 0.0f), FVector(0.9f, -0.1f, 0.0f),
			FVector(0.8f, 0.0f, 0.0f), FVector(0.9f, -0.1f, 0.0f), FVector(1.0f, 0.0f, 0.0f)
		};

		ID3D11Buffer* ArrowBuffer = CreateVertexBuffer(ArrowVertices.GetData(),
		                                               ArrowVertices.Num() * sizeof(FVector), false);
		if (ArrowBuffer)
		{
			GizmoVertexBuffers[EPrimitiveType::Arrow] = ArrowBuffer;
			GizmoVertexCounts[EPrimitiveType::Arrow] = ArrowVertices.Num();
		}
	}

	// Ring 생성 (Rotation Gizmo)
	{
		TArray<FVector> RingVertices;
		const int32 RingSegments = 32;
		const float Radius = 1.0f;
		const float Thickness = 0.05f;

		// Ring 생성 (삼각형 리스트)
		for (int32 i = 0; i < RingSegments; ++i)
		{
			float Angle1 = (static_cast<float>(i) / RingSegments) * 2.0f * PI;
			float Angle2 = (static_cast<float>((i + 1) % RingSegments) / RingSegments) * 2.0f * PI;

			FVector Inner1(cos(Angle1) * (Radius - Thickness), sin(Angle1) * (Radius - Thickness), 0.0f);
			FVector Outer1(cos(Angle1) * (Radius + Thickness), sin(Angle1) * (Radius + Thickness), 0.0f);
			FVector Inner2(cos(Angle2) * (Radius - Thickness), sin(Angle2) * (Radius - Thickness), 0.0f);
			FVector Outer2(cos(Angle2) * (Radius + Thickness), sin(Angle2) * (Radius + Thickness), 0.0f);

			// 기본 리인 세그먼트만 생성
			RingVertices.Add(Inner1);
			RingVertices.Add(Inner2);
			RingVertices.Add(Outer1);

			RingVertices.Add(Inner2);
			RingVertices.Add(Outer2);
			RingVertices.Add(Outer1);
		}

		ID3D11Buffer* RingBuffer = CreateVertexBuffer(RingVertices.GetData(),
		                                              RingVertices.Num() * sizeof(FVector), false);
		if (RingBuffer)
		{
			GizmoVertexBuffers[EPrimitiveType::Ring] = RingBuffer;
			GizmoVertexCounts[EPrimitiveType::Ring] = RingVertices.Num();
		}
	}

	// CubeArrow 생성 (Scale Gizmo)
	{
		// 간단한 사각형 + Arrow 형태
		TArray<FVector> CubeArrowVertices = {
			// Cube 부분
			FVector(0.0f, -0.05f, -0.05f), FVector(0.8f, -0.05f, -0.05f), FVector(0.8f, 0.05f, -0.05f),
			FVector(0.0f, -0.05f, -0.05f), FVector(0.8f, 0.05f, -0.05f), FVector(0.0f, 0.05f, -0.05f),

			FVector(0.0f, -0.05f, 0.05f), FVector(0.8f, 0.05f, 0.05f), FVector(0.8f, -0.05f, 0.05f),
			FVector(0.0f, -0.05f, 0.05f), FVector(0.0f, 0.05f, 0.05f), FVector(0.8f, 0.05f, 0.05f),

			// Arrow 머리
			FVector(0.8f, -0.1f, 0.0f), FVector(1.0f, 0.0f, 0.0f), FVector(0.8f, 0.1f, 0.0f)
		};

		ID3D11Buffer* CubeArrowBuffer = CreateVertexBuffer(CubeArrowVertices.GetData(),
		                                                   CubeArrowVertices.Num() * sizeof(FVector), false);
		if (CubeArrowBuffer)
		{
			GizmoVertexBuffers[EPrimitiveType::CubeArrow] = CubeArrowBuffer;
			GizmoVertexCounts[EPrimitiveType::CubeArrow] = CubeArrowVertices.Num();
		}
	}

	UE_LOG("기즈모 리소스 초기화 완료");
}

void FEditorRenderResources::ReleaseGizmoResources()
{
	for (auto& Pair : GizmoVertexBuffers)
	{
		if (Pair.second)
		{
			ReleaseVertexBuffer(Pair.second);
		}
	}

	GizmoVertexBuffers.Empty();
	GizmoVertexCounts.Empty();
}
