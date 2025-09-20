#include "pch.h"
#include "Render/Renderer/Public/BillBoardSceneProxy.h"
#include "Component/Public/BillBoardComponent.h"
#include "Actor/Public/Actor.h"

FBillBoardSceneProxy::FBillBoardSceneProxy(const UBillBoardComponent* InComponent)
    : FPrimitiveSceneProxy(InComponent)
{
    if (InComponent)
    {
        // RT 매트릭스 복사
        RTMatrix = InComponent->GetRTMatrix();

        // Actor의 UUID를 표시 텍스트로 설정
        if (InComponent->GetOwner())
        {
            DisplayText = "UUID:" + to_string(InComponent->GetOwner()->GetUUID());
        }
        else
        {
            DisplayText = "Unknown";
        }
    }
}
