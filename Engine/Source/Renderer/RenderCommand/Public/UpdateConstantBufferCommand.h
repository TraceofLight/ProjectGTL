#pragma once
#include "RenderCommand.h"

class URHIDevice;

/**
 * @brief ConstantBuffer 업데이트 Command 클래스
 */
class FRHIUpdateConstantBufferCommand :
    public IRHICommand
{
public:
    FRHIUpdateConstantBufferCommand(URHIDevice* InRHIDevice, const FMatrix& InModelMatrix,
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
    URHIDevice* RHIDevice;
    FMatrix ModelMatrix;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
