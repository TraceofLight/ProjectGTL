#include "pch.h"
#include "Render/Renderer/Public/EditorRenderResources.h"
#include "Render/RHI/Public/RHIDevice.h"
#include "Runtime/Component/Mesh/Public/VertexDatas.h"

void FEditorRenderResources::InitRHI(URHIDevice* InRHIDevice)
{
	if (bIsInitialized || !InRHIDevice || !InRHIDevice->IsInitialized())
	{
		UE_LOG_ERROR("EditorRenderResources: InitRHI 실패 - 이미 초기화되었거나 RHIDevice가 유효하지 않습니다");
		return;
	}

	UE_LOG("EditorRenderResources: GPU 리소스 초기화 시작...");

	InitializeGizmoBuffers(InRHIDevice);
	InitializeEditorShaders(InRHIDevice);

	bIsInitialized = true;
	UE_LOG("EditorRenderResources: GPU 리소스 초기화 완료");
}

void FEditorRenderResources::ReleaseRHI(URHIDevice* InRHIDevice)
{
	if (!bIsInitialized)
	{
		return;
	}

	UE_LOG("EditorRenderResources: GPU 리소스 해제 시작...");

	ReleaseGizmoBuffers(InRHIDevice);
	ReleaseEditorShaders(InRHIDevice);

	bIsInitialized = false;
	UE_LOG("EditorRenderResources: GPU 리소스 해제 완료");
}

ID3D11Buffer* FEditorRenderResources::GetGizmoVertexBuffer(EPrimitiveType Type) const
{
	auto It = GizmoVertexBuffers.find(Type);
	return (It != GizmoVertexBuffers.end()) ? It->second : nullptr;
}

uint32 FEditorRenderResources::GetGizmoVertexCount(EPrimitiveType Type) const
{
	auto It = GizmoVertexCounts.find(Type);
	return (It != GizmoVertexCounts.end()) ? It->second : 0;
}

ID3D11VertexShader* FEditorRenderResources::GetEditorVertexShader(EShaderType Type) const
{
	auto It = EditorVertexShaders.find(Type);
	return (It != EditorVertexShaders.end()) ? It->second : nullptr;
}

ID3D11PixelShader* FEditorRenderResources::GetEditorPixelShader(EShaderType Type) const
{
	auto It = EditorPixelShaders.find(Type);
	return (It != EditorPixelShaders.end()) ? It->second : nullptr;
}

ID3D11InputLayout* FEditorRenderResources::GetEditorInputLayout(EShaderType Type) const
{
	auto It = EditorInputLayouts.find(Type);
	return (It != EditorInputLayouts.end()) ? It->second : nullptr;
}

void FEditorRenderResources::InitializeGizmoBuffers(URHIDevice* InRHIDevice)
{
	// Arrow 기즈모 버퍼 생성
	ID3D11Buffer* ArrowBuffer = InRHIDevice->CreateVertexBuffer(
		VerticesArrow.data(),
		static_cast<uint32>(VerticesArrow.size() * sizeof(FVertex)));
	if (ArrowBuffer)
	{
		GizmoVertexBuffers.emplace(EPrimitiveType::Arrow, ArrowBuffer);
		GizmoVertexCounts.emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.size()));
		UE_LOG("EditorRenderResources: Arrow 버퍼 생성 완료");
	}

	// CubeArrow 기즈모 버퍼 생성
	ID3D11Buffer* CubeArrowBuffer = InRHIDevice->CreateVertexBuffer(
		VerticesCubeArrow.data(),
		static_cast<uint32>(VerticesCubeArrow.size() * sizeof(FVertex)));
	if (CubeArrowBuffer)
	{
		GizmoVertexBuffers.emplace(EPrimitiveType::CubeArrow, CubeArrowBuffer);
		GizmoVertexCounts.emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.size()));
		UE_LOG("EditorRenderResources: CubeArrow 버퍼 생성 완료");
	}

	// Ring 기즈모 버퍼 생성
	ID3D11Buffer* RingBuffer = InRHIDevice->CreateVertexBuffer(
		VerticesRing.data(),
		static_cast<uint32>(VerticesRing.size() * sizeof(FVertex)));
	if (RingBuffer)
	{
		GizmoVertexBuffers.emplace(EPrimitiveType::Ring, RingBuffer);
		GizmoVertexCounts.emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.size()));
		UE_LOG("EditorRenderResources: Ring 버퍼 생성 완료");
	}

	UE_LOG("EditorRenderResources: 기즈모 버퍼 초기화 완료");
}

void FEditorRenderResources::InitializeEditorShaders(URHIDevice* InRHIDevice)
{
	// BatchLine Shader (기즈모용)
	{
		TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		ID3D11VertexShader* VS = nullptr;
		ID3D11InputLayout* Layout = nullptr;

		if (InRHIDevice->CreateVertexShaderAndInputLayout(
			L"Asset/Shader/BatchLineVS.hlsl", LayoutDesc, &VS, &Layout))
		{
			EditorVertexShaders.emplace(EShaderType::BatchLine, VS);
			EditorInputLayouts.emplace(EShaderType::BatchLine, Layout);
			UE_LOG("EditorRenderResources: BatchLine VS 생성 완료");
		}

		ID3D11PixelShader* PS = nullptr;
		if (InRHIDevice->CreatePixelShader(L"Asset/Shader/BatchLinePS.hlsl", &PS))
		{
			EditorPixelShaders.emplace(EShaderType::BatchLine, PS);
			UE_LOG("EditorRenderResources: BatchLine PS 생성 완료");
		}
	}

	// StaticMesh Shader
	{
		TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		ID3D11VertexShader* VS = nullptr;
		ID3D11InputLayout* Layout = nullptr;

		if (InRHIDevice->CreateVertexShaderAndInputLayout(
			L"Asset/Shader/StaticMeshShader.hlsl", LayoutDesc, &VS, &Layout))
		{
			EditorVertexShaders.emplace(EShaderType::StaticMesh, VS);
			EditorInputLayouts.emplace(EShaderType::StaticMesh, Layout);
			UE_LOG("EditorRenderResources: StaticMesh VS 생성 완료");
		}

		ID3D11PixelShader* PS = nullptr;
		if (InRHIDevice->CreatePixelShader(L"Asset/Shader/StaticMeshShader.hlsl", &PS))
		{
			EditorPixelShaders.emplace(EShaderType::StaticMesh, PS);
			UE_LOG("EditorRenderResources: StaticMesh PS 생성 완료");
		}
	}

	UE_LOG("EditorRenderResources: 에디터 셰이더 초기화 완료");
}

void FEditorRenderResources::ReleaseGizmoBuffers(URHIDevice* InRHIDevice)
{
	for (auto& Pair : GizmoVertexBuffers)
	{
		InRHIDevice->ReleaseVertexBuffer(Pair.second);
	}
	GizmoVertexBuffers.clear();
	GizmoVertexCounts.clear();
}

void FEditorRenderResources::ReleaseEditorShaders(URHIDevice* InRHIDevice)
{
	for (auto& Pair : EditorVertexShaders)
	{
		InRHIDevice->ReleaseShader(Pair.second);
	}
	EditorVertexShaders.clear();

	for (auto& Pair : EditorPixelShaders)
	{
		InRHIDevice->ReleaseShader(Pair.second);
	}
	EditorPixelShaders.clear();

	for (auto& Pair : EditorInputLayouts)
	{
		InRHIDevice->ReleaseShader(Pair.second);
	}
	EditorInputLayouts.clear();
}
