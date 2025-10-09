#include "pch.h"
#include "Runtime/Renderer/Public/LineBatcher.h"
#include "Runtime/Renderer/Public/RHICommandList.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Runtime/Core/Public/VertexTypes.h"

FLineBatcher::FLineBatcher() = default;

FLineBatcher::~FLineBatcher()
{
	Shutdown();
}

void FLineBatcher::Initialize(FRHIDevice* InRHIDevice)
{
	if (bIsInitialized)
	{
		return;
	}

	RHIDevice = InRHIDevice;
	if (!RHIDevice)
	{
		UE_LOG_ERROR("FLineBatcher::Initialize - RHIDevice is null");
		return;
	}

	bIsInitialized = true;
	UE_LOG("FLineBatcher initialized");
}

void FLineBatcher::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	BatchedLineStartPoints.Empty();
	BatchedLineEndPoints.Empty();
	BatchedLineColors.Empty();
	
	RHIDevice = nullptr;
	bIsInitialized = false;
	bIsBatchActive = false;
}

void FLineBatcher::BeginBatch()
{
	bIsBatchActive = true;

	// 배치 데이터 클리어
	BatchedLineStartPoints.Empty();
	BatchedLineEndPoints.Empty();
	BatchedLineColors.Empty();
}

void FLineBatcher::AddLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	if (!bIsBatchActive)
	{
		UE_LOG_WARNING("FLineBatcher::AddLine - Batch is not active. Call BeginBatch first.");
		return;
	}

	BatchedLineStartPoints.Add(Start);
	BatchedLineEndPoints.Add(End);
	BatchedLineColors.Add(Color);
}

void FLineBatcher::AddLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints, const TArray<FVector4>& Colors)
{
	if (!bIsBatchActive)
	{
		UE_LOG_WARNING("FLineBatcher::AddLines - Batch is not active. Call BeginBatch first.");
		return;
	}

	// 배열 크기 검증
	if (StartPoints.Num() != EndPoints.Num() || StartPoints.Num() != Colors.Num())
	{
		UE_LOG_ERROR("FLineBatcher::AddLines - Array size mismatch");
		return;
	}

	// 배치에 추가
	BatchedLineStartPoints.Append(StartPoints);
	BatchedLineEndPoints.Append(EndPoints);
	BatchedLineColors.Append(Colors);
}

void FLineBatcher::EndBatch(FRHICommandList* CommandList, const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix, const FRect* ScissorRect)
{
	if (!bIsBatchActive)
	{
		return;
	}

	if (!CommandList || !RHIDevice || BatchedLineStartPoints.IsEmpty())
	{
		bIsBatchActive = false;
		return;
	}

	// FVertexSimple로 변환 (Position + Color)
	TArray<FVertexSimple> Vertices;
	TArray<uint32> Indices;

	Vertices.Reserve(BatchedLineStartPoints.Num() * 2);
	Indices.Reserve(BatchedLineStartPoints.Num() * 2);

	for (int32 i = 0; i < BatchedLineStartPoints.Num(); ++i)
	{
		uint32 StartIndex = static_cast<uint32>(Vertices.Num());

		Vertices.Add(FVertexSimple(BatchedLineStartPoints[i], BatchedLineColors[i]));
		Vertices.Add(FVertexSimple(BatchedLineEndPoints[i], BatchedLineColors[i]));

		Indices.Add(StartIndex);
		Indices.Add(StartIndex + 1);
	}

	// 동적 버퍼 생성
	ID3D11Buffer* VertexBuffer = RHIDevice->CreateVertexBuffer(Vertices.GetData(), Vertices.Num() * sizeof(FVertexSimple), true);
	ID3D11Buffer* IndexBuffer = RHIDevice->CreateIndexBuffer(Indices.GetData(), Indices.Num() * sizeof(uint32));

	if (VertexBuffer && IndexBuffer)
	{
		// CommandList에 렌더 명령 추가
		CommandList->DrawLineBatch(VertexBuffer, IndexBuffer, Indices.Num(), ModelMatrix, ViewMatrix, ProjectionMatrix, ScissorRect);

		// 버퍼는 CommandList 실행 후 정리되어야 하므로 여기서는 정리하지 않음
		// CommandList가 버퍼 수명을 관리하도록 함
	}
	else
	{
		UE_LOG_ERROR("FLineBatcher::EndBatch - Failed to create vertex/index buffers");
		
		// 버퍼 생성 실패 시 정리
		if (VertexBuffer)
		{
			RHIDevice->ReleaseVertexBuffer(VertexBuffer);
		}
		if (IndexBuffer)
		{
			RHIDevice->ReleaseIndexBuffer(IndexBuffer);
		}
	}

	bIsBatchActive = false;
}
