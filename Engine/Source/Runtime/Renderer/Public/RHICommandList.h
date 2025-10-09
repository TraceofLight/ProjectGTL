#pragma once
#include <future>

#include "RenderCommand.h"

class UPrimitiveComponent;
class FRHIDevice;

/**
 * @brief RenderCommand들을 수집하고 일괄 실행하는 클래스
 * 여러 패스에서 생성된 RenderCommand들을 모아서 효율적으로 GPU에 제출
 */
class FRHICommandList
{
public:
	FRHICommandList(FRHIDevice* InRHIDevice);
	~FRHICommandList();

	// RenderCommand 추가
	template <typename TCommand, typename... TArgs>
	void EnqueueCommand(TArgs&&... Args)
	{
		TCommand* Command = new TCommand(RHIDevice, std::forward<TArgs>(Args)...);
		PendingCommands.push(Command);
	}

	// 모든 Command 실행
	void Execute();

	// Material별 정렬 후 실행
	void ExecuteWithMaterialSorting();

	// 멀티스레드 실행
	void ExecuteWithMultithreadedSorting();

	// 비동기 정렬 시작
	void StartBackgroundSorting();
	bool IsSortingComplete() const;

	// 워커 스레드 초기화 정리
	void InitializeWorkerThreads();
	void ShutdownWorkerThreads();

	// Command 큐 비우기
	void Clear();

	// 통계
	int32 GetCommandCount() const { return static_cast<int32>(PendingCommands.size()); }
	bool IsEmpty() const { return PendingCommands.empty(); }

	// RHI Device 접근자
	FRHIDevice* GetRHIDevice() const { return RHIDevice; }

	// 자주 사용되는 Command 래퍼들
	void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f);
	void SetRenderTarget(const class FSceneView* View);
	void ClearRenderTarget(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f);
	void SetDepthStencilState(EComparisonFunc Func);
	void SetBlendState(bool bEnableBlending);
	void UpdateConstantBuffers(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection);
	void DrawIndexedPrimitive(UPrimitiveComponent* Component, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix);
	void DrawIndexedPrimitiveWithColor(UPrimitiveComponent* Component, const FMatrix& ViewMatrix,
	                                   const FMatrix& ProjMatrix, const FVector& Color);
	void DrawIndexedPrimitiveWithColorAndHovering(UPrimitiveComponent* Component, const FMatrix& ViewMatrix,
	                                              const FMatrix& ProjMatrix, const FVector& Color, bool bIsHovering);
	void DrawLineBatch(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, uint32 IndexCount,
	                   const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix,
	                   const FRect* ScissorRect = nullptr);

	// Present 및 BackBuffer 접근
	void Present();
	IDXGISurface* GetBackBufferSurface();
	ID3D11RenderTargetView* GetBackBufferRTV();

private:
	FRHIDevice* RHIDevice;
	TQueue<IRHICommand*> PendingCommands;

	// 통계 추적
	int32 ExecutedCommandCount = 0;
	int32 TotalDrawCalls = 0;

	// 실행 상태 추적 (중복 실행 방지)
	bool bIsExecuting = false;

	// 멀티스레드 지원
	mutable mutex CommandMutex;
	mutable std::condition_variable SortingCV;
	mutable atomic<bool> bIsSorting{false};
	mutable atomic<bool> bSortingComplete{false};

	// 정렬된 Command 버퍼
	mutable TArray<class FRHIDrawIndexedPrimitivesCommand*> SortedDrawCommands;
	mutable TArray<IRHICommand*> SortedOtherCommands;

	// 비동기 작업 스레드
	mutable std::future<void> SortingTask;
	static constexpr size_t MAX_WORKER_THREADS = 2;

	void InternalExecuteCommand(IRHICommand* Command);

	// Sorting Key 기반 정렬
	void RadixSortDrawCommands(FRHIDrawIndexedPrimitivesCommand** Commands, size_t Count) const;

	// 멀티스레드 헬퍼
	void BackgroundSortingTask(TArray<IRHICommand*> CommandSnapshot) const;
	void ExecuteSortedCommandsMultithreaded();
};
