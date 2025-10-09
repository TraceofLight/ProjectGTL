#include "pch.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Component/Mesh/Public/VertexDatas.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Core/Public/VertexTypes.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Runtime/Renderer/Public/EditorGrid.h"
#include "Runtime/Renderer/Public/LineBatcher.h"

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

	// LineBatcher 초기화
	LineBatcher = MakeUnique<FLineBatcher>();
	LineBatcher->Initialize(RHIDevice);

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

	// LineBatcher 종료
	if (LineBatcher)
	{
		LineBatcher->Shutdown();
		LineBatcher.Reset();
	}

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

	ID3D11Buffer* ArrowBuffer = CreateVertexBuffer(VerticesArrow.GetData(),
	                                               VerticesArrow.Num() * sizeof(FVertex), false);
	if (ArrowBuffer)
	{
		GizmoVertexBuffers[EPrimitiveType::Arrow] = ArrowBuffer;
		GizmoVertexCounts[EPrimitiveType::Arrow] = VerticesArrow.Num();
	}

	ID3D11Buffer* RingBuffer = CreateVertexBuffer(VerticesRing.GetData(),
	                                              VerticesRing.Num() * sizeof(FVertex), false);
	if (RingBuffer)
	{
		GizmoVertexBuffers[EPrimitiveType::Ring] = RingBuffer;
		GizmoVertexCounts[EPrimitiveType::Ring] = VerticesRing.Num();
	}

	ID3D11Buffer* CubeArrowBuffer = CreateVertexBuffer(VerticesCubeArrow.GetData(),
	                                                   VerticesCubeArrow.Num() * sizeof(FVertex), false);
	if (CubeArrowBuffer)
	{
		GizmoVertexBuffers[EPrimitiveType::CubeArrow] = CubeArrowBuffer;
		GizmoVertexCounts[EPrimitiveType::CubeArrow] = VerticesCubeArrow.Num();
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
