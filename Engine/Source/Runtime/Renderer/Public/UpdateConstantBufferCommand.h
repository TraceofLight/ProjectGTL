#pragma once
#include "RenderCommand.h"

class FRHIDevice;

/**
 * @brief ConstantBuffer 업데이트 Command 클래스
 */
class FRHIUpdateConstantBufferCommand :
    public IRHICommand
{
public:
    FRHIUpdateConstantBufferCommand(FRHIDevice* InRHIDevice, const FMatrix& InModelMatrix,
                                    const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : RHIDevice(InRHIDevice), ModelMatrix(InModelMatrix), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    FRHIDevice* RHIDevice;
    FMatrix ModelMatrix;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
