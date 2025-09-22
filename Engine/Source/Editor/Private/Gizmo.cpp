#include "pch.h"
#include "Editor/Public/Gizmo.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Global/Quaternion.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_CLASS(UGizmo, UObject)

UGizmo::UGizmo()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Primitives.resize(3);
	GizmoColor.resize(3);

	/* *
	* @brief 0: Forward(x), 1: Right(y), 2: Up(z)
	*/
	GizmoColor[0] = FVector4(1, 0, 0, 1);
	GizmoColor[1] = FVector4(0, 1, 0, 1);
	GizmoColor[2] = FVector4(0, 0, 1, 1);

	/* *
	* @brief Translation Setting
	*/
	const float ScaleT = TranslateCollisionConfig.Scale;
	Primitives[0].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/* *
	* @brief Rotation Setting
	*/
	Primitives[1].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Ring);
	Primitives[1].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Ring);
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/* *
	* @brief Scale Setting
	*/
	Primitives[2].Vertexbuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::CubeArrow);
	Primitives[2].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[2].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[2].bShouldAlwaysVisible = true;

	/* *
	* @brief Render State
	*/
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UGizmo::~UGizmo() = default;

void UGizmo::RenderGizmo(AActor* InActor, const FVector& InCameraLocation, const UCamera* InCamera, float InViewportWidth,
                         float InViewportHeight)
{
	TargetActor = InActor;
	if (!TargetActor)
	{
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	const int Mode = static_cast<int>(GizmoMode);
	auto& P = Primitives[Mode];
	P.Location = TargetActor->GetActorLocation();

	// 화면에서 일정한 픽셀 크기를 유지하기 위한 스케일 계산
	float Scale = CalculateScreenSpaceScale(InCameraLocation, InCamera, InViewportWidth, InViewportHeight);

	// 피킹에서 사용할 현재 스케일 값 저장
	CurrentRenderScale = Scale;

	TranslateCollisionConfig.Scale = Scale;
	RotateCollisionConfig.Scale = Scale;

	P.Scale = FVector(Scale, Scale, Scale);

	// 드래그 중에는 나머지 축 유지되는 모드 (회전 후 새로운 로컬 기즈모 보여줌)
	FQuaternion LocalRotation;
	if (GizmoMode == EGizmoMode::Rotate && !bIsWorld && bIsDragging)
	{
		LocalRotation = FQuaternion::FromEuler(DragStartActorRotation);
	}
	else if (GizmoMode == EGizmoMode::Scale)
	{
		LocalRotation = FQuaternion::FromEuler(TargetActor->GetActorRotation());
	}
	else
	{
		LocalRotation = bIsWorld ? FQuaternion::Identity() : FQuaternion::FromEuler(TargetActor->GetActorRotation());
	}

	// X축 (Forward) - 빨간색
	FQuaternion RotationX = LocalRotation * FQuaternion::Identity();
	P.Rotation = RotationX.ToEuler();
	P.Color = ColorFor(EGizmoDirection::Forward);
	Renderer.RenderPrimitive(P, RenderState);

	// Y축 (Right) - 초록색 (Z축 주위로 90도 회전)
	FQuaternion RotY = LocalRotation * FQuaternion::FromAxisAngle(FVector::UpVector(), 90.0f * (PI / 180.0f));
	P.Rotation = RotY.ToEuler();
	P.Color = ColorFor(EGizmoDirection::Right);
	Renderer.RenderPrimitive(P, RenderState);

	// Z축 (Up) - 파란색 (Y축 주위로 -90도 회전)
	FQuaternion RotZ = LocalRotation * FQuaternion::FromAxisAngle(FVector::RightVector(), -90.0f * (PI / 180.0f));
	P.Rotation = RotZ.ToEuler();
	P.Color = ColorFor(EGizmoDirection::Up);
	Renderer.RenderPrimitive(P, RenderState);
}

void UGizmo::ChangeGizmoMode()
{
	switch (GizmoMode)
	{
	case EGizmoMode::Translate:
		GizmoMode = EGizmoMode::Rotate;
		break;
	case EGizmoMode::Rotate:
		GizmoMode = EGizmoMode::Scale;
		break;
	case EGizmoMode::Scale:
		GizmoMode = EGizmoMode::Translate;
	}
}

void UGizmo::SetLocation(const FVector& InLocation) const
{
	TargetActor->SetActorLocation(InLocation);
}

bool UGizmo::IsInRadius(float InRadius) const
{
	if (InRadius >= RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale &&
		InRadius <= RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale)
	{
		return true;
	}

	return false;
}

void UGizmo::OnMouseDragStart(const FVector& CollisionPoint)
{
	bIsDragging = true;
	DragStartMouseLocation = CollisionPoint;
	DragStartActorLocation = Primitives[static_cast<int>(GizmoMode)].Location;
	DragStartActorRotation = TargetActor->GetActorRotation();
	DragStartActorScale = TargetActor->GetActorScale3D();
}


/**
 * @brief 상태 오염 방지를 위해 하이라이트 색상은 렌더 시점에만 계산하기 위한 함수
 * @param InAxis 축 방향
 * @return 색상 벡터
 */
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis) const
{
	const int Index = AxisIndex(InAxis);
	const FVector4& BaseColor = GizmoColor[Index];
	const bool bIsHighlight = (InAxis == GizmoDirection);

	// 드래깅 중이든 아니든 하이라이트된 축은 노란색으로 표시
	if (bIsHighlight)
	{
		// 드래깅 중일 때는 좀 더 진한 노란색, 호버링일 때는 밝은 노란색
		return bIsDragging ? FVector4(1.0f, 0.8f, 0.0f, 1.0f) : FVector4(1.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		// 하이라이트되지 않은 축은 기본 색상
		return BaseColor;
	}
}

/**
 * @brief 언리얼 엔진처럼 화면에서 일정한 픽셀 크기를 유지하는 스케일을 계산하는 함수
 */
float UGizmo::CalculateScreenSpaceScale(const FVector& InCameraLocation, const UCamera* InCamera,
                                        float InViewportWidth, float InViewportHeight, float InDesiredPixelSize) const
{
	if (!InCamera || !TargetActor || InViewportWidth <= 0.0f || InViewportHeight <= 0.0f)
	{
		return 1.0f;
	}

	// 기즈모 위치와 카메라 사이의 거리
	float DistanceToCamera = (InCameraLocation - TargetActor->GetActorLocation()).Length();
	DistanceToCamera = max(DistanceToCamera, 0.1f);

	// 원근 투영
	if (InCamera->GetCameraType() == ECameraType::ECT_Perspective)
	{
		// FOV와 거리를 이용한 화면 공간 크기 계산
		float FovYRadians = InCamera->GetFovY() * (PI / 180.0f);
		float TanHalfFov = tanf(FovYRadians * 0.5f);

		// 원근 투영에서 화면 끝으로 갈수록 커지는 현상 보정
		// 기즈모 위치에서 카메라 중심까지의 벡터와 카메라 방향 벡터의 도트곱 계산
		FVector CameraToGizmo = TargetActor->GetActorLocation() - InCameraLocation;
		FVector CameraForward = InCamera->GetForward();
		CameraToGizmo.Normalize();
		CameraForward.Normalize();

		// 카메라 중심축으로부터의 각도 계산 (코사인 값)
		float CosAngle = CameraForward.Dot(CameraToGizmo);
		CosAngle = max(0.1f, CosAngle); // 너무 가장자리에서의 기하급수적 증가 방지

		// 시야각 보정을 위한 스케일 팩터
		float ViewAngleCorrection = 1.0f / CosAngle;

		// 화면 공간에서 1픽셀이 월드 공간에서 차지하는 크기
		float WorldUnitsPerPixel = (2.0f * DistanceToCamera * TanHalfFov) / InViewportHeight;

		// 원하는 픽셀 크기를 월드 스케일로 변환 (시야각 보정 적용)
		return (WorldUnitsPerPixel * InDesiredPixelSize) / ViewAngleCorrection;
	}
	// 직교 투영
	else
	{
		// FOV 값이 화면 높이를 의미함 (더 작은 크기로 조정)
		float OrthographicHeight = InCamera->GetFovY();
		float WorldUnitsPerPixel = OrthographicHeight / InViewportHeight;

		// 직교 투영에서는 좌표계 차이로 인해 더 작은 크기 사용 (휴리스틱하게 보정함)
		return WorldUnitsPerPixel * (InDesiredPixelSize * 0.05f);
	}
}
