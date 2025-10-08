#pragma once
#include "RenderCommand.h"

class URHIDevice;

/**
* @brief HighLight Buffer 업데이트 명령
 */
class FRHIUpdateHighlightBufferCommand : public IRHICommand
{
public:
    FRHIUpdateHighlightBufferCommand(URHIDevice* InRHIDevice, bool bInIsSelected,
                                     const FVector& InColor, uint32 InGizmo = 0)
        : RHIDevice(InRHIDevice), bIsSelected(bInIsSelected), Color(InColor), Gizmo(InGizmo),
          X(0), Y(0), Z(0)
    {
    }

    // 기즈모용 생성자 (원래 함수 시그니처에 맞춤)
    FRHIUpdateHighlightBufferCommand(URHIDevice* InRHIDevice, bool bInIsSelected,
                                     const FVector& InColor, uint32 InX, uint32 InY, uint32 InZ, uint32 InGizmo)
        : RHIDevice(InRHIDevice), bIsSelected(bInIsSelected), Color(InColor), Gizmo(InGizmo),
          X(InX), Y(InY), Z(InZ)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URHIDevice* RHIDevice;
    bool bIsSelected;
    FVector Color;
    uint32 Gizmo;
    uint32 X, Y, Z;
};
