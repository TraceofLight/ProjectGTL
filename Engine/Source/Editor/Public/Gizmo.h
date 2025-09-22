#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Actor/Public/Actor.h"

class UCamera;
class UObjectPicker;

enum class EGizmoMode : uint8
{
	Translate,
	Rotate,
	Scale
};

enum class EGizmoDirection : uint8
{
	Right,
	Up,
	Forward,
	None
};

struct FGizmoTranslationCollisionConfig
{
	FGizmoTranslationCollisionConfig()
		: Radius(0.04f), Height(0.9f), Scale(2.f)
	{
	}

	float Radius = {};
	float Height = {};
	float Scale = {};
};

struct FGizmoRotateCollisionConfig
{
	FGizmoRotateCollisionConfig()
		: OuterRadius(1.0f), InnerRadius(0.9f), Scale(2.f)
	{
	}

	float OuterRadius = {1.0f}; // 링 큰 반지름
	float InnerRadius = {0.9f}; // 링 굵기 r
	float Scale = {2.0f};
};

UCLASS()
class UGizmo :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UGizmo, UObject)

public:
	void RenderGizmo(AActor* InActor, const FVector& InCameraLocation, const UCamera* InCamera = nullptr,
	                 float InViewportWidth = 0.0f, float InViewportHeight = 0.0f);
	void ChangeGizmoMode();
	void ChangeGizmoMode(EGizmoMode InMode) { GizmoMode = InMode; }

	// Getter
	EGizmoDirection GetGizmoDirection() const { return GizmoDirection; }
	const FVector& GetGizmoLocation() { return Primitives[static_cast<int>(GizmoMode)].Location; }
	const FVector& GetActorRotation() const { return TargetActor->GetActorRotation(); }
	const FVector& GetActorScale() const { return TargetActor->GetActorScale3D(); }
	const FVector& GetDragStartMouseLocation() { return DragStartMouseLocation; }
	const FVector& GetDragStartActorLocation() { return DragStartActorLocation; }
	const FVector& GetDragStartActorRotation() { return DragStartActorRotation; }
	const FVector& GetDragStartActorScale() { return DragStartActorScale; }
	EGizmoMode GetGizmoMode() const { return GizmoMode; }
	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * GetCurrentRenderScale(); }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * GetCurrentRenderScale(); }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * GetCurrentRenderScale(); }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * GetCurrentRenderScale(); }
	AActor* GetSelectedActor() const { return TargetActor; }
	bool IsInRadius(float InRadius) const;

	float GetRotateThickness() const
	{
		return max(0.001f, RotateCollisionConfig.InnerRadius * GetCurrentRenderScale());
	}

	FVector GetGizmoAxis() const
	{
		FVector Axis[3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
		return Axis[AxisIndex(GizmoDirection)];
	}

	// Setter
	void SetLocation(const FVector& InLocation) const;
	void SetGizmoDirection(EGizmoDirection InDirection) { GizmoDirection = InDirection; }
	void SetActorRotation(const FVector& InRotation) const { TargetActor->SetActorRotation(InRotation); }
	void SetActorScale(const FVector& InScale) const { TargetActor->SetActorScale3D(InScale); }

	void SetWorld() { bIsWorld = true; }
	void SetLocal() { bIsWorld = false; }
	bool IsWorldMode() const { return bIsWorld; }

	float CalculateScreenSpaceScale(const FVector& InCameraLocation, const UCamera* InCamera, float InViewportWidth,
	                                float InViewportHeight, float InDesiredPixelSize = 120.0f) const;

	float GetCurrentRenderScale() const { return CurrentRenderScale; } // 현재 렌더링에 사용되는 실제 스케일 값 반환
	void SetCurrentRenderScaleForPicking(float Scale) const { CurrentRenderScale = Scale; }

	// Mouse information
	void EndDrag() { bIsDragging = false; }
	bool IsDragging() const { return bIsDragging; }
	void OnMouseDragStart(const FVector& CollisionPoint);

	static void OnMouseHovering() {}
	static void OnMouseRelease(EGizmoDirection DirectionReleased) {}

	// Special member function
	UGizmo();
	~UGizmo() override;

private:
	static int AxisIndex(EGizmoDirection InDirection)
	{
		switch (InDirection)
		{
		case EGizmoDirection::Forward: return 0;
		case EGizmoDirection::Right: return 1;
		case EGizmoDirection::Up: return 2;
		default: return 2;
		}
	}

	// 렌더 시 하이라이트 색상 계산 (상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;

	TArray<FEditorPrimitive> Primitives;
	AActor* TargetActor = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;
	FVector DragStartActorScale;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	float HoveringFactor = 0.8f;
	const float ScaleFactor = 0.2f;
	const float MinScaleFactor = 7.0f;
	bool bIsDragging = false;
	bool bIsWorld = true; // Gizmo coordinate mode (true; world)

	// 피킹에서 사용할 현재 렌더링 스케일 값
	mutable float CurrentRenderScale = 1.0f;

	FRenderState RenderState;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode GizmoMode = EGizmoMode::Translate;
};
