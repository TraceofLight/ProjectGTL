#include "pch.h"
#include "Component/Public/BillBoardComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"
#include "Render/Renderer/Public/BillBoardSceneProxy.h"
/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UBillBoardComponent::UBillBoardComponent()
{
	Type = EPrimitiveType::BillBoard;
}

UBillBoardComponent::~UBillBoardComponent()
{
}

void UBillBoardComponent::UpdateRotationMatrix()
{
	const FVector& CameraLocation = ULevelManager::GetInstance().GetEditor()->GetCameraLocation();
    const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	FVector ToCamera = CameraLocation;
	ToCamera.Normalize();

	const FVector4 worldUp4 = FVector4(0, 0, 1, 1);
	const FVector worldUp = { worldUp4.X, worldUp4.Y, worldUp4.Z };
	FVector Right = worldUp.Cross(ToCamera);
	Right.Normalize();
	FVector Up = ToCamera.Cross(Right);
	Up.Normalize();

	RTMatrix = FMatrix(FVector4(0, 1, 0, 1), worldUp4, FVector4(1,0,0,1));
	RTMatrix = FMatrix(ToCamera, Right, Up);

	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, 5.0f);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}

FBillBoardSceneProxy* UBillBoardComponent::CreateSceneProxy()
{
	// 매 프레임마다 카메라를 향하도록 RT 매트릭스 업데이트
	UpdateRotationMatrix();

	// Scene Proxy 생성 및 반환
	return new FBillBoardSceneProxy(this);
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
