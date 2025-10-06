#pragma once
#include "RenderCommand.h"

class URHIDevice;

/**
 * @brief Line Batch End Command Class
 */
class FRHIEndLineBatchCommand :
    public IRHICommand
{
public:
    FRHIEndLineBatchCommand(URHIDevice* InRHIDevice, const FMatrix& InModelMatrix,
                            const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : RHIDevice(InRHIDevice), ModelMatrix(InModelMatrix), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::DrawIndexedPrimitives;
    }

private:
    URHIDevice* RHIDevice;
    FMatrix ModelMatrix;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
