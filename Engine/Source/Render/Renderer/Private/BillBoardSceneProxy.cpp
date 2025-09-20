#include "pch.h"
#include "Render/Renderer/Public/BillBoardSceneProxy.h"
#include "Component/Public/BillBoardComponent.h"
#include "Actor/Public/Actor.h"

FBillBoardSceneProxy::FBillBoardSceneProxy(const UBillBoardComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	if (!InComponent)
	{
		return;
	}

	// BillBoard의 RT 매트릭스 저장
	RTMatrix = InComponent->GetRTMatrix();

	// 표시할 텍스트 생성 (Actor의 UUID)
	if (InComponent->GetOwner())
	{
		AActor* OwnerActor = Cast<AActor>(InComponent->GetOwner());
		if (OwnerActor)
		{
			DisplayText = "UID: " + std::to_string(OwnerActor->GetUUID());
		}
	}
}

bool FBillBoardSceneProxy::IsValidForRendering() const
{
	// BillBoard는 FontRenderer를 사용하므로 특별한 처리 필요
	// 텍스트가 있으면 렌더링 가능
	return !DisplayText.empty();
}
