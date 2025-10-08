#include "pch.h"
#include "Editor/Public/Gizmo.h"

#include "Runtime/Actor/Public/Actor.h"
#include "Global/Quaternion.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"
#include "Runtime/Renderer/Public/RendererModule.h"
#include "Runtime/Actor/Public/CameraActor.h"

IMPLEMENT_CLASS(UGizmo, UObject)

UGizmo::UGizmo()
{

}

UGizmo::UGizmo(FRendererModule* InRenderModule)
	: RenderModule(InRenderModule)
{
	FEditorRenderResources* EditorResources = RenderModule->GetEditorResources();

	Primitives.SetNum(3);
	GizmoColor.SetNum(3);

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
	Primitives[0].Vertexbuffer = EditorResources->GetGizmoVertexBuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = EditorResources->GetGizmoVertexCount(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/* *
	* @brief Rotation Setting
	*/
	Primitives[1].Vertexbuffer = EditorResources->GetGizmoVertexBuffer(EPrimitiveType::Ring);
	Primitives[1].NumVertices = EditorResources->GetGizmoVertexCount(EPrimitiveType::Ring);
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/* *
	* @brief Scale Setting
	*/
	Primitives[2].Vertexbuffer = EditorResources->GetGizmoVertexBuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = EditorResources->GetGizmoVertexCount(EPrimitiveType::CubeArrow);
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

void UGizmo::RenderGizmo(AActor* InActor, const FVector& InCameraLocation, const ACameraActor* InCamera, float InViewportWidth,
                         float InViewportHeight, int32 ViewportIndex)
{
	TargetActor = InActor;
	if (!TargetActor || !RenderModule)
	{
		return;
	}

	const int Mode = static_cast<int>(GizmoMode);
	auto& P = Primitives[Mode];
	P.Location = TargetActor->GetActorLocation();

	// 화면 공간 기반 렌더링을 위해 스케일은 기본값으로 설정
	// 스케일 계산은 이제 렌더러에서 처리
	float BaseScale = 1.0f;

	// 피킹을 위해 현재 렌더링 중인 스케일을 계산해서 저장
	// (기존 CalculateScreenSpaceScale 로직은 피킹용으로만 사용)
	float CalculatedScale = CalculateScreenSpaceScale(InCameraLocation, InCamera, InViewportWidth, InViewportHeight);
	CurrentRenderScale = CalculatedScale;

	TranslateCollisionConfig.Scale = CalculatedScale;
	RotateCollisionConfig.Scale = CalculatedScale;

	P.Scale = FVector(BaseScale, BaseScale, BaseScale);

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
	P.Color = ColorFor(EGizmoDirection::Forward, ViewportIndex);
	RenderModule->RenderGizmoPrimitive(P, RenderState, InCameraLocation, InViewportWidth, InViewportHeight);

	// Y축 (Right) - 초록색 (Z축 주위로 90도 회전)
	FQuaternion RotY = LocalRotation * FQuaternion::FromAxisAngle(FVector::UpVector(), 90.0f * (PI / 180.0f));
	P.Rotation = RotY.ToEuler();
	P.Color = ColorFor(EGizmoDirection::Right, ViewportIndex);
	RenderModule->RenderGizmoPrimitive(P, RenderState, InCameraLocation, InViewportWidth, InViewportHeight);

	// Z축 (Up) - 파란색 (Y축 주위로 -90도 회전)
	FQuaternion RotZ = LocalRotation * FQuaternion::FromAxisAngle(FVector::RightVector(), -90.0f * (PI / 180.0f));
	P.Rotation = RotZ.ToEuler();
	P.Color = ColorFor(EGizmoDirection::Up, ViewportIndex);
	RenderModule->RenderGizmoPrimitive(P, RenderState, InCameraLocation, InViewportWidth, InViewportHeight);
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
 * @param ViewportIndex 뷰포트 인덱스
 * @return 색상 벡터
 */
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis, int32 ViewportIndex) const
{
	const int Index = AxisIndex(InAxis);
	const FVector4& BaseColor = GizmoColor[Index];

	// 뷰포트별 기즈모 선택 상태 확인
	EGizmoDirection ViewportGizmoDirection = EGizmoDirection::None;
	const EGizmoDirection* FoundDirectionPtr = ViewportGizmoDirections.Find(ViewportIndex);
	if (FoundDirectionPtr)
	{
		ViewportGizmoDirection = *FoundDirectionPtr;
	}

	const bool bIsHighlight = (InAxis == ViewportGizmoDirection);

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
 * @brief 뷰포트별 기즈모 방향 설정하는 함수
 * @param InViewportIndex 뷰포트 인덱스
 * @param InDirection 설정할 기즈모 방향
 */
void UGizmo::SetGizmoDirectionForViewport(int32 InViewportIndex, EGizmoDirection InDirection)
{
	ViewportGizmoDirections[InViewportIndex] = InDirection;
	// 전역 방향도 업데이트
	if (InViewportIndex == 0)
	{
		GizmoDirection = InDirection;
	}
}

/**
 * @brief 뷰포트별 기즈모 방향 가져오는 함수
 * @param InViewportIndex 뷰포트 인덱스
 * @return 뷰포트의 기즈모 방향
 */
EGizmoDirection UGizmo::GetGizmoDirectionForViewport(int32 InViewportIndex) const
{
	return ViewportGizmoDirections.FindRef(InViewportIndex);
}

/**
 * @brief 화면에서 일정한 픽셀 크기를 유지하는 스케일을 계산하는 함수 (피킹용)
 */
float UGizmo::CalculateScreenSpaceScale(const FVector& InCameraLocation, const ACameraActor* InCamera,
                                        float InViewportWidth, float InViewportHeight, float InDesiredPixelSize) const
{
	if (!InCamera || !InCamera->GetCameraComponent() || !TargetActor || InViewportWidth <= 0.0f || InViewportHeight <= 0.0f)
	{
		return 1.0f;
	}

	// 기즈모 위치와 카메라 사이의 거리
	float DistanceToCamera = (InCameraLocation - TargetActor->GetActorLocation()).Length();
	DistanceToCamera = max(DistanceToCamera, 0.1f);

	// 원근 투영
	if (InCamera->GetCameraComponent()->GetCameraType() == ECameraType::ECT_Perspective)
	{
		// FOV와 거리를 이용한 화면 공간 크기 계산
		float FovYRadians = InCamera->GetCameraComponent()->GetFovY() * (PI / 180.0f);
		float TanHalfFov = tanf(FovYRadians * 0.5f);

		// 화면 공간에서 1픽셀이 월드 공간에서 차지하는 크기
		float WorldUnitsPerPixel = (2.0f * DistanceToCamera * TanHalfFov) / InViewportHeight;

		// 원하는 픽셀 크기를 월드 스케일로 변환
		return WorldUnitsPerPixel * InDesiredPixelSize;
	}
	// 직교 투영
	else
	{
		// 직교 투영에서는 GetOrthographicHeight() 사용
		float OrthographicHeight = InCamera->GetCameraComponent()->GetOrthographicHeight();

		if (OrthographicHeight <= 0.0f)
		{
			return 1.0f;
		}

		float WorldUnitsPerPixel = OrthographicHeight / InViewportHeight;
		return WorldUnitsPerPixel * InDesiredPixelSize;
	}
}
