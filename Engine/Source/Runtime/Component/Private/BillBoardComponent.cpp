#include "pch.h"
#include "Runtime/Component/Public/BillBoardComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Editor/Public/Editor.h"
#include "Runtime/Actor/Public/Actor.h"

IMPLEMENT_CLASS(UBillBoardComponent, UPrimitiveComponent)

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UBillBoardComponent::UBillBoardComponent()
{
	Type = EPrimitiveType::BillBoard;
	UBillBoardComponent::GetClass()->IncrementGenNumber();
}

void UBillBoardComponent::UpdateRotationMatrix(const ACameraActor* InCamera)
{
	if (!InCamera)
	{
		return;
	}

    const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	const FVector& CameraForward = InCamera->GetCameraComponent()->GetForward();
	const FVector& CameraRight = InCamera->GetCameraComponent()->GetRight();
	const FVector& CameraUp = InCamera->GetCameraComponent()->GetUp();

	// 빌보드가 뷰 평면에 평행하도록 카메라의 방향 벡터를 그대로 사용
	// Forward 벡터가 빌보드의 normal이 되고, Right와 Up은 빌보드의 로컬 축이 됨
	RTMatrix = FMatrix(CameraForward, CameraRight, CameraUp);

	// 오프셋을 적용한 위치로 이동
	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, 5.0f);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}


// 공통 렌더링 인터페이스 구현
bool UBillBoardComponent::HasRenderData() const
{
	// BillBoard는 항상 텍스트를 렌더링할 데이터가 있음
	return GetOwner() != nullptr;
}

bool UBillBoardComponent::UseIndexedRendering() const
{
	// BillBoard는 FontRenderer를 사용하므로 일반적인 인덱스 렌더링을 사용하지 않음
	return false;
}

EShaderType UBillBoardComponent::GetShaderType() const
{
	// BillBoard는 기본 셰이더 사용 (FontRenderer가 별도 처리)
	return EShaderType::Default;
}
