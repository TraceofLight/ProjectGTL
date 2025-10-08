#include "pch.h"
#include "Runtime/Renderer/Public/SceneRenderer.h"

#include "Runtime/Renderer/Public/BasePass.h"
#include "Runtime/Renderer/Public/DepthPrePass.h"
#include "Runtime/Renderer/Public/DebugPass.h"
#include "Runtime/Renderer/Public/RenderPass.h"
#include "Runtime/Renderer/Public/SceneView.h"
#include "Runtime/Renderer/Public/SceneViewFamily.h"
#include "Runtime/Renderer/Public/RHICommandList.h"
#include "Runtime/RHI/Public/RHIDevice.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Global/CoreTypes.h"

FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily& InViewFamily)
{
    return new FSceneRenderer(InViewFamily);
}

FSceneRenderer::FSceneRenderer(const FSceneViewFamily& InViewFamily)
    : ViewFamily(&InViewFamily)
    , CommandList(nullptr)
{
    // RenderCommandList 생성
    if (GDynamicRHI)
    {
        CommandList = new FRHICommandList(GDynamicRHI);
    }

    CreateDefaultRenderPasses();
}

FSceneRenderer::~FSceneRenderer()
{
    Cleanup();
}

void FSceneRenderer::Render()
{
    if (!ViewFamily || !ViewFamily->IsValid() || !GDynamicRHI)
    {
        return;
    }

    // ViewFamily에서 Views 추출
    const TArray<FSceneView*>& Views = ViewFamily->GetViews();

    for (FSceneView* SceneView : Views)
    {
        if (SceneView)
        {
            RenderView(SceneView);
        }
    }
}

void FSceneRenderer::RenderView(const FSceneView* InSceneView)
{
    if (!InSceneView || !CommandList)
    {
        return;
    }

    // CommandList 초기화
    CommandList->Clear();

    // 각 렌더 패스 실행
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass && Pass->IsEnabled())
        {
            Pass->Execute(InSceneView, this);
        }
    }

    // 10K+ 커맨드에서는 멀티스레딩 정렬로 성능 극대화
    if (CommandList->GetCommandCount() > 10000)
    {
        CommandList->ExecuteWithMultithreadedSorting();
    }
    else
    {
        CommandList->ExecuteWithMaterialSorting();
    }
}

void FSceneRenderer::Cleanup()
{
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass)
        {
            Pass->Cleanup();
            delete Pass;
        }
    }

    RenderPasses.Empty();

    // CommandList 정리
    if (CommandList)
    {
        delete CommandList;
        CommandList = nullptr;
    }
}

void FSceneRenderer::CreateDefaultRenderPasses()
{
    // 기본 렌더 패스들 생성 및 초기화
    FDepthPrePass* DepthPass = new FDepthPrePass();
    DepthPass->Initialize();
    RenderPasses.Add(DepthPass);

    FBasePass* BasePass = new FBasePass();
    BasePass->Initialize();
    RenderPasses.Add(BasePass);

    FDebugPass* DebugPass = new FDebugPass();
    DebugPass->Initialize();
    RenderPasses.Add(DebugPass);
}

void FSceneRenderer::RenderGizmoPrimitive(const FEditorPrimitive& Primitive, const FRenderState& RenderState, 
                                         const FVector& CameraLocation, float ViewportWidth, float ViewportHeight)
{
    // GDynamicRHI 유효성 검사
    if (!GDynamicRHI || !GDynamicRHI->IsInitialized())
    {
        UE_LOG_ERROR("RenderGizmoPrimitive: GDynamicRHI가 초기화되지 않았습니다");
        return;
    }

    ID3D11DeviceContext* DeviceContext = GDynamicRHI->GetDeviceContext();
    if (!DeviceContext)
    {
        UE_LOG_ERROR("RenderGizmoPrimitive: DeviceContext가 null입니다");
        return;
    }

    // Primitive 유효성 검사
    if (!Primitive.Vertexbuffer || Primitive.NumVertices == 0)
    {
        UE_LOG_ERROR("RenderGizmoPrimitive: 유효하지 않은 Primitive 데이터입니다");
        return;
    }

    // 기즈모 렌더링을 위한 기본 설정
    try
    {
        // 1. 버텍스 버퍼 설정
        UINT stride = sizeof(FVertex);  // 버텍스 크기
        UINT offset = 0;
        DeviceContext->IASetVertexBuffers(0, 1, &Primitive.Vertexbuffer, &stride, &offset);
        
        // 2. 토폴로지 설정
        DeviceContext->IASetPrimitiveTopology(Primitive.Topology);
        
        // 3. 입력 레이아웃 설정 (있는 경우)
        if (Primitive.InputLayout)
        {
            DeviceContext->IASetInputLayout(Primitive.InputLayout);
        }
        
        // 4. 셰이더 설정 (있는 경우)
        if (Primitive.VertexShader)
        {
            DeviceContext->VSSetShader(Primitive.VertexShader, nullptr, 0);
        }
        
        if (Primitive.PixelShader)
        {
            DeviceContext->PSSetShader(Primitive.PixelShader, nullptr, 0);
        }
        
        // 5. 래스터라이저 상태 설정 (ViewMode를 통해 기본 설정 사용)
        // TODO: RenderState에 따른 세밀한 래스터라이저 상태 설정
        GDynamicRHI->RSSetState(EViewMode::Lit);  // 기본 Lit 모드 사용
        
        // 6. 화면 공간 기즈모 스케일 계산 및 변환 행렬 업데이트
        // ViewportWidth, ViewportHeight, CameraLocation을 이용해 스케일 계산
        float Distance = (CameraLocation - Primitive.Location).Length();
        float ScreenSpaceScale = max(Distance * 0.1f, 1.0f);  // 간단한 스케일 계산
        
        // TODO: 실제 MVP 행렬 계산 및 상수 버퍼 업데이트
        // 현재는 기본 렌더링만 수행
        
        // 7. 드로우 콜
        if (Primitive.IndexBuffer && Primitive.NumIndices > 0)
        {
            DeviceContext->IASetIndexBuffer(Primitive.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            DeviceContext->DrawIndexed(Primitive.NumIndices, 0, 0);
        }
        else
        {
            DeviceContext->Draw(Primitive.NumVertices, 0);
        }
        
        UE_LOG("기즈모 Primitive 렌더링 완료: 버텍스 %d개", Primitive.NumVertices);
    }
    catch (...)
    {
        UE_LOG_ERROR("RenderGizmoPrimitive: 렌더링 중 예외 발생");
    }
}
