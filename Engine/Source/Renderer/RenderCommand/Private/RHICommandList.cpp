#include "pch.h"
#include "Renderer/RenderCommand/Public/RHICommandList.h"

#include "Render/RHI/Public/RHIDevice.h"
#include "Renderer/RenderCommand/Public/DrawIndexedPrimitivesCommand.h"
#include "Renderer/RenderCommand/Public/SetBlendStateCommand.h"
#include "Renderer/RenderCommand/Public/SetDepthStencilStateCommand.h"
#include "Renderer/RenderCommand/Public/UpdateConstantBufferCommand.h"
#include "Renderer/RenderCommand/Public/PresentCommands.h"

class FRHISetRenderTargetCommand;

FRHICommandList::FRHICommandList(URHIDevice* InRHIDevice)
    : RHIDevice(InRHIDevice)
      , ExecutedCommandCount(0)
      , TotalDrawCalls(0)
{
}

FRHICommandList::~FRHICommandList()
{
    Clear();
}

void FRHICommandList::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    // 중복 실행 방지
    if (bIsExecuting)
    {
        return;
    }

    // 빈 큐에 대한 안전 체크
    if (PendingCommands.empty())
    {
        return;
    }

    bIsExecuting = true;
    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;

    // 큐가 비어있지 않은 동안 계속 처리
    while (!PendingCommands.empty())
    {
        IRHICommand* Command = PendingCommands.front();
        PendingCommands.pop();

        if (Command)
        {
            InternalExecuteCommand(Command);
            delete Command;
        }
    }

    bIsExecuting = false;
}

// Radix Sort 기능 구현
void FRHICommandList::RadixSortDrawCommands(FRHIDrawIndexedPrimitivesCommand** Commands,
                                            size_t Count) const
{
    if (Count <= 1)
    {
        return;
    }

    // 64-bit Sorting Key 기반 초고속 Radix Sort (Unreal Engine 스타일)
    static constexpr size_t RADIX_BITS = 8;
    static constexpr size_t RADIX_SIZE = 1 << RADIX_BITS;
    static constexpr size_t PASSES = sizeof(uint64) * 8 / RADIX_BITS; // 64-bit = 8 passes

    // 임시 버퍼 (대용량 대응)
    thread_local FRHIDrawIndexedPrimitivesCommand* TempBuffer[65536];
    FRHIDrawIndexedPrimitivesCommand** Source = Commands;
    FRHIDrawIndexedPrimitivesCommand** Dest = TempBuffer;

    // 8번의 pass로 64-bit key를 완전 정렬
    for (size_t pass = 0; pass < PASSES; ++pass)
    {
        size_t CountArray[RADIX_SIZE] = {0};
        size_t shift = pass * RADIX_BITS;

        // Counting phase - 64-bit Sorting Key 사용
        for (size_t i = 0; i < Count; ++i)
        {
            uint64 sortingKey = Source[i]->GetSortKey();
            size_t digit = (sortingKey >> shift) & (RADIX_SIZE - 1);
            CountArray[digit]++;
        }

        // Prefix sum
        size_t total = 0;
        for (size_t i = 0; i < RADIX_SIZE; ++i)
        {
            size_t temp = CountArray[i];
            CountArray[i] = total;
            total += temp;
        }

        // Distribution phase - 안정적인 정렬 (Stable Sort)
        for (size_t i = 0; i < Count; ++i)
        {
            uint64 sortingKey = Source[i]->GetSortKey();
            size_t digit = (sortingKey >> shift) & (RADIX_SIZE - 1);
            Dest[CountArray[digit]++] = Source[i];
        }

        // 버퍼 스왑 (다음 pass에서 사용)
        std::swap(Source, Dest);
    }

    // 최종 결과가 TempBuffer에 있으면 원본에 복사
    if (Source != Commands)
    {
        memcpy(Commands, Source, Count * sizeof(FRHIDrawIndexedPrimitivesCommand*));
    }
}

void FRHICommandList::ExecuteWithMaterialSorting()
{
    if (!RHIDevice)
    {
        return;
    }

    // 중복 실행 방지
    if (bIsExecuting)
    {
        return;
    }

    // 빈 큐에 대한 안전 체크
    if (PendingCommands.empty())
    {
        return;
    }

    bIsExecuting = true;
    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;

    // Command들을 vector로 이동 (정렬을 위해)
    TArray<IRHICommand*> AllCommands;
    TArray<FRHIDrawIndexedPrimitivesCommand*> DrawCommands;
    TArray<IRHICommand*> OtherCommands;

    while (!PendingCommands.empty())
    {
        IRHICommand* Command = PendingCommands.front();
        PendingCommands.pop();

        if (Command)
        {
            AllCommands.Add(Command);

            // DrawIndexedPrimitives Command만 별도로 분리
            if (Command->GetCommandType() == ERHICommandType::DrawIndexedPrimitives)
            {
                DrawCommands.Add(static_cast<FRHIDrawIndexedPrimitivesCommand*>(Command));
            }
            else
            {
                OtherCommands.Add(Command);
            }
        }
    }

    // Draw Command들을 64-bit Sorting Key로 Radix Sort
    if (!DrawCommands.IsEmpty())
    {
        RadixSortDrawCommands(DrawCommands.GetData(), DrawCommands.Num());
    }

    // 3. 정렬된 순서로 실행 (Context Switching 최소화)
    // 먼저 기타 Command들 실행
    for (IRHICommand* Command : OtherCommands)
    {
        if (Command)
        {
            InternalExecuteCommand(Command);
            delete Command;
        }
    }

    // 4. Material별로 정렬된 Draw Command 실행
    UMaterial* CurrentMaterial = reinterpret_cast<UMaterial*>(-1);
    // 처음 설정도 카운트하기 위해 유효하지 않은 값으로 초기화
    for (FRHIDrawIndexedPrimitivesCommand* DrawCmd : DrawCommands)
    {
        if (DrawCmd)
        {
            // Material이 변경된 경우 (처음 설정 포함)
            UMaterial* NewMaterial = DrawCmd->GetMaterial();
            if (NewMaterial != CurrentMaterial)
            {
                CurrentMaterial = NewMaterial;
            }

            InternalExecuteCommand(DrawCmd);
            delete DrawCmd;
        }
    }

    bIsExecuting = false;
}

void FRHICommandList::Clear()
{
    while (!PendingCommands.empty())
    {
        IRHICommand* Command = PendingCommands.front();
        PendingCommands.pop();
        delete Command;
    }

    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;
}

void FRHICommandList::InternalExecuteCommand(IRHICommand* Command)
{
    if (!Command)
        return;

    Command->Execute();
    ExecutedCommandCount++;

    // Draw Call 통계 업데이트
    ERHICommandType CommandType = Command->GetCommandType();
    if (CommandType == ERHICommandType::DrawIndexedPrimitives)
    {
        TotalDrawCalls++;
    }
}

// 래퍼 메서드들
void FRHICommandList::SetRenderTarget(const FSceneView* View)
{
    EnqueueCommand<FRHISetRenderTargetCommand>(View);
}

void FRHICommandList::SetDepthStencilState(EComparisonFunc Func)
{
    EnqueueCommand<FRHISetDepthStencilStateCommand>(Func);
}

void FRHICommandList::SetBlendState(bool bEnableBlending)
{
    EnqueueCommand<FRHISetBlendStateCommand>(bEnableBlending);
}

void FRHICommandList::UpdateConstantBuffers(const FMatrix& Model, const FMatrix& View,
                                            const FMatrix& Projection)
{
    EnqueueCommand<FRHIUpdateConstantBufferCommand>(Model, View, Projection);
}

void FRHICommandList::DrawIndexedPrimitive(UPrimitiveComponent* Component,
                                           const FMatrix& ViewMatrix, const FMatrix& ProjMatrix)
{
    EnqueueCommand<FRHIDrawIndexedPrimitivesCommand>(Component, ViewMatrix, ProjMatrix);
}

void FRHICommandList::DrawIndexedPrimitiveWithColor(UPrimitiveComponent* Component,
                                                    const FMatrix& ViewMatrix,
                                                    const FMatrix& ProjMatrix, const FVector& Color)
{
    EnqueueCommand<FRHIDrawIndexedPrimitivesCommand>(Component, ViewMatrix, ProjMatrix, Color);
}

void FRHICommandList::DrawIndexedPrimitiveWithColorAndHovering(
    UPrimitiveComponent* Component, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix,
    const FVector& Color, bool bIsHovering)
{
    EnqueueCommand<FRHIDrawIndexedPrimitivesCommand>(Component, ViewMatrix, ProjMatrix, Color,
                                                     bIsHovering);
}

void FRHICommandList::SetViewport(float X, float Y, float Width, float Height, float MinDepth,
                                  float MaxDepth)
{
    // 직접 RHI 호출 (단순한 작업이므로 Command로 감쌀 필요 없음)
    if (RHIDevice)
    {
        D3D11_VIEWPORT Viewport = {};
        Viewport.TopLeftX = X;
        Viewport.TopLeftY = Y;
        Viewport.Width = Width;
        Viewport.Height = Height;
        Viewport.MinDepth = MinDepth;
        Viewport.MaxDepth = MaxDepth;

        RHIDevice->GetDeviceContext()->RSSetViewports(1, &Viewport);
    }
}

void FRHICommandList::ClearRenderTarget(float R, float G, float B, float A)
{
    // 직접 RHI 호출
    if (RHIDevice)
    {
        // TODO: 현재 렌더 타겟을 클리어하는 로직 구현
        // RHIDevice에 ClearRenderTarget 메소드가 있다면 사용
    }
}

void FRHICommandList::InitializeWorkerThreads()
{
    // 워커 스레드 초기화는 필요시만 수행
    // 현재는 std::async로 동적 생성
}

void FRHICommandList::ShutdownWorkerThreads()
{
    // 비동기 작업 완료 대기
    if (SortingTask.valid())
    {
        SortingTask.wait();
    }
}

void FRHICommandList::ExecuteWithMultithreadedSorting()
{
    if (!RHIDevice || bIsExecuting) return;

    // 빈 큐에 대한 안전 체크
    if (PendingCommands.empty()) return;

    bIsExecuting = true;
    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;

    // 50K+ Commands를 위한 멀티스레드 처리!

    // 1. Command 스냅샷 생성 (안전한 복사)
    std::vector<IRHICommand*> CommandSnapshot;
    CommandSnapshot.reserve(PendingCommands.size());

    while (!PendingCommands.empty())
    {
        CommandSnapshot.push_back(PendingCommands.front());
        PendingCommands.pop();
    }

    // 2. 백그라운드에서 정렬 시작
    StartBackgroundSorting();

    // 3. 비동기 정렬 시작
    if (!bIsSorting.load())
    {
        bIsSorting.store(true);
        bSortingComplete.store(false);

        SortingTask = std::async(std::launch::async, [this, CommandSnapshot]() mutable {
            BackgroundSortingTask(std::move(CommandSnapshot));
        });
    }

    // 4. 정렬 완료 대기 (대기 시간 최소화)
    if (SortingTask.valid())
    {
        SortingTask.wait(); // 최소 대기
    }

    // 5. 정렬된 Command 실행
    ExecuteSortedCommandsMultithreaded();

    bIsExecuting = false;
}

void FRHICommandList::StartBackgroundSorting()
{
    // 비동기 정렬 시작 준비
    bSortingComplete.store(false);
}

bool FRHICommandList::IsSortingComplete() const
{
    return bSortingComplete.load();
}

void FRHICommandList::BackgroundSortingTask(std::vector<IRHICommand*> CommandSnapshot) const
{
    try
    {
        // Command 분류 (대용량 처리 최적화)
        SortedDrawCommands.clear();
        SortedOtherCommands.clear();

        size_t commandCount = CommandSnapshot.size();
        SortedDrawCommands.reserve(commandCount);
        SortedOtherCommands.reserve(commandCount / 8); // DrawCommand가 대부분

        // 고속 분류 (쪐시 친화적)
        for (IRHICommand* Command : CommandSnapshot)
        {
            if (!Command) continue;

            if (Command->GetCommandType() == ERHICommandType::DrawIndexedPrimitives)
            {
                SortedDrawCommands.push_back(static_cast<FRHIDrawIndexedPrimitivesCommand*>(Command));
            }
            else
            {
                SortedOtherCommands.push_back(Command);
            }
        }

        // 초고속 Radix Sort (50K+ Commands 지원!)
        if (!SortedDrawCommands.empty())
        {
            RadixSortDrawCommands(SortedDrawCommands.data(), SortedDrawCommands.size());
        }

        bSortingComplete.store(true);
    }
    catch (const std::exception&)
    {
        // 예외 처리
        bSortingComplete.store(false);
    }

    bIsSorting.store(false);

    // 정렬 완료 알림
    SortingCV.notify_one();
}

void FRHICommandList::ExecuteSortedCommandsMultithreaded()
{
    // 기타 Command 실행 (빠른 순회)
    for (IRHICommand* Command : SortedOtherCommands)
    {
        if (Command)
        {
            InternalExecuteCommand(Command);
            delete Command;
        }
    }

    // Draw Command 실행 (Material 배치 최적화!)
    if (!SortedDrawCommands.empty())
    {
        UMaterial* CurrentMaterial = nullptr;

        for (FRHIDrawIndexedPrimitivesCommand* DrawCmd : SortedDrawCommands)
        {
            if (!DrawCmd) continue;

            // Material 변경 추적
            UMaterial* NewMaterial = DrawCmd->GetMaterial();
            if (NewMaterial != CurrentMaterial)
            {
                CurrentMaterial = NewMaterial;
            }

            InternalExecuteCommand(DrawCmd);
            delete DrawCmd;
        }
    }

    // 버퍼 정리
    SortedDrawCommands.clear();
    SortedOtherCommands.clear();
}

// Present 및 BackBuffer 접근 메서드들

void FRHICommandList::Present()
{
    EnqueueCommand<FRHIPresentCommand>();
}

IDXGISurface* FRHICommandList::GetBackBufferSurface()
{
    IDXGISurface* Surface = nullptr;

    // 즉시 실행 (BackBuffer에 대한 접근이므로)
    FRHIGetBackBufferSurfaceCommand Command(RHIDevice, &Surface);
    Command.Execute();

    return Surface;
}

ID3D11RenderTargetView* FRHICommandList::GetBackBufferRTV()
{
    ID3D11RenderTargetView* RTV = nullptr;

    // 즉시 실행 (BackBuffer에 대한 접근이므로)
    FRHIGetBackBufferRTVCommand Command(RHIDevice, &RTV);
    Command.Execute();

    return RTV;
}
