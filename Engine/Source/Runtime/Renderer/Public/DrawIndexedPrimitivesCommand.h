#pragma once
#include "RenderCommand.h"

#include "SetRenderTargetCommand.h"

class UPrimitiveComponent;
class UMaterial;

/**
 * @brief 메시 렌더링 Command를 정의한 함수
 * Material 정보를 포함하여 정렬 가능
 */
class FRHIDrawIndexedPrimitivesCommand :
    public IRHICommand
{
public:
    FRHIDrawIndexedPrimitivesCommand(FRHIDevice* InRHIDevice, UPrimitiveComponent* InComponent,
                                     const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : RHIDevice(InRHIDevice), Component(InComponent), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix), MaterialPtr(nullptr),
          OverrideColor(FVector(1.0f, 1.0f, 1.0f)), bUseOverrideColor(false), bIsHovering(false)
    {
        InitializeSortingKey(InComponent, InViewMatrix);
    }

    // 기즈모용 색상 오버라이드 생성자
    FRHIDrawIndexedPrimitivesCommand(FRHIDevice* InRHIDevice, UPrimitiveComponent* InComponent,
                                     const FMatrix& InViewMatrix, const FMatrix& InProjMatrix,
                                     const FVector& InOverrideColor)
        : RHIDevice(InRHIDevice), Component(InComponent), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix), MaterialPtr(nullptr),
          OverrideColor(InOverrideColor), bUseOverrideColor(true), bIsHovering(false)
    {
        InitializeSortingKey(InComponent, InViewMatrix);
    }

    // 기즈모 호버링용 생성자
    FRHIDrawIndexedPrimitivesCommand(FRHIDevice* InRHIDevice, UPrimitiveComponent* InComponent,
                                     const FMatrix& InViewMatrix, const FMatrix& InProjMatrix,
                                     const FVector& InOverrideColor, bool bInIsHovering)
        : RHIDevice(InRHIDevice), Component(InComponent), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix), MaterialPtr(nullptr),
          OverrideColor(InOverrideColor), bUseOverrideColor(true), bIsHovering(bInIsHovering)
    {
        InitializeSortingKey(InComponent, InViewMatrix);
    }

    void Execute() override;

    void SetupShaderForComponent(UPrimitiveComponent* InComponent);
    void RenderComponent(UPrimitiveComponent* InComponent);
    void RenderStaticMeshComponent(class UStaticMeshComponent* StaticMeshComp);
    void RenderGizmoComponent(class UStaticMeshComponent* StaticMeshComp);

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::DrawIndexedPrimitives;
    }

    // Sorting Key 기반 정렬 지원 (초고속 Radix Sort 용)
    uint64 GetSortKey() const { return GetSortingKey(); }
    class UMaterial* GetMaterial() const { return MaterialPtr; }
    UPrimitiveComponent* GetComponent() const { return Component; }

    // 후위 호환성을 위한 MaterialID 접근자 (내부에서 Sorting Key 사용)
    uint16 GetMaterialID() const { return IRHICommand::GetMaterialID(); }

    // Sorting Key 기반 비교 연산자 (초고속!)
    bool operator<(const FRHIDrawIndexedPrimitivesCommand& Other) const
    {
        return GetSortingKey() < Other.GetSortingKey();
    }

private:
    FRHIDevice* RHIDevice;
    UPrimitiveComponent* Component;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;

    // Material 정보 (실제 렌더링용)
    class UMaterial* MaterialPtr;  // 실제 Material 포인터

    // 기즈모 색상 오버라이드 (기즈모 전용)
    FVector OverrideColor;         // RGB 색상 (기즈모용)
    bool bUseOverrideColor;        // 색상 오버라이드 사용 여부
    bool bIsHovering;              // 호버링 상태 (노란색 하이라이트용)

    // Sorting Key 초기화 헬퍼 메소드
    void InitializeSortingKey(UPrimitiveComponent* InComponent, const FMatrix& InViewMatrix);
};
