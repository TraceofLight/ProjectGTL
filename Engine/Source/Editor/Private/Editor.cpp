#include "pch.h"
#include "Editor/Public/Editor.h"

#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/Level.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"
#include "Render/UI/Widget/Public/FPSWidget.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"
#include "Global/Quaternion.h"

UEditor::UEditor()
	: ObjectPicker(Camera)
{
	ObjectPicker.SetCamera(Camera);

	// Set Camera to Control Panel
	auto& UIManager = UUIManager::GetInstance();
	auto* CameraControlWidget =
		reinterpret_cast<UCameraControlWidget*>(UIManager.FindWidget("Camera Control Widget"));
	CameraControlWidget->SetCamera(&Camera);

	// Set UBatchLines to FPSWidget Panel
	auto* FPSWidget =
		reinterpret_cast<UFPSWidget*>(UIManager.FindWidget("FPS Widget"));
	FPSWidget->SetBatchLine(&BatchLines);
	// Set Camera to Scene Hierarchy Widget
	auto* SceneHierarchyWidget =
		reinterpret_cast<USceneHierarchyWidget*>(UIManager.FindWidget("Scene Hierarchy Widget"));
	SceneHierarchyWidget->SetCamera(&Camera);
};

void UEditor::Update()
{
	auto& Renderer = URenderer::GetInstance();
	Camera.Update();

	AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor();
	if (SelectedActor)
	{
		for (const auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				FVector WorldMin, WorldMax;
				PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);

				// 프리미티브와 바운딩박스 플래그가 모두 켜져있을 때만 바운딩박스 표시
				uint64 ShowFlags = ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags();

				if ((ShowFlags & EEngineShowFlags::SF_Primitives) && (ShowFlags & EEngineShowFlags::SF_Bounds))
				{
					BatchLines.UpdateBoundingBoxVertices(FAABB(WorldMin, WorldMax));
				}
				else
				{
					BatchLines.UpdateBoundingBoxVertices({ { 0.0f,0.0f,0.0f }, { 0.0f, 0.0f, 0.0f } });
				}
			}
		}
	}
	else
	{
		// 선택된 Actor가 없으면 바운딩박스 비활성화
		BatchLines.DisableRenderBoundingBox();
	}

	BatchLines.UpdateVertexBuffer();

	ProcessMouseInput(ULevelManager::GetInstance().GetCurrentLevel());

	Renderer.UpdateConstant(Camera.GetFViewProjConstants());

	//BatchLines.UpdateConstant({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} });
}

void UEditor::RenderEditor()
{
	//Grid.RenderGrid();
	BatchLines.Render();
	Axis.Render();

	AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor();
	Gizmo.RenderGizmo(SelectedActor, Camera.GetLocation());
}

void UEditor::ProcessMouseInput(ULevel* InLevel)
{
	FRay WorldRay;
	const UInputManager& InputManager = UInputManager::GetInstance();
	FVector MousePositionNdc = InputManager.GetMouseNDCPosition();

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	TObjectPtr<AActor> ActorPicked = InLevel->GetSelectedActor();
	FVector CollisionPoint;
	float ActorDistance = -1;

	if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		Gizmo.IsWorldMode() ? Gizmo.SetLocal() : Gizmo.SetWorld();
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		Gizmo.EndDrag();
	}
	WorldRay = Camera.ConvertToWorldRay(MousePositionNdc.X, MousePositionNdc.Y);

	if (Gizmo.IsDragging() && Gizmo.GetSelectedActor())
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
			{
				FVector GizmoDragLocation = GetGizmoDragLocation(WorldRay);
				Gizmo.SetLocation(GizmoDragLocation);
				break;
			}
		case EGizmoMode::Rotate:
			{
				FVector GizmoDragRotation = GetGizmoDragRotation(WorldRay);
				Gizmo.SetActorRotation(GizmoDragRotation);
				break;
			}
		case EGizmoMode::Scale:
			{
				FVector GizmoDragScale = GetGizmoDragScale(WorldRay);
				Gizmo.SetActorScale(GizmoDragScale);
			}
		}
	}
	else
	{
		if (InLevel->GetSelectedActor()) //기즈모가 출력되고있음. 레이캐스팅을 계속 해야함.
		{
			ObjectPicker.PickGizmo(WorldRay, Gizmo, CollisionPoint);
		}
		else
		{
			Gizmo.SetGizmoDirection(EGizmoDirection::None);
		}
		if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			TArray<UPrimitiveComponent*> Candidate = FindCandidatePrimitives(InLevel);

			UPrimitiveComponent* PrimitiveCollided = nullptr;
			// 만약 Primitive show flag가 꺼져있으면, 오브젝트 피킹이 안되게 함.(단, 이미 피킹이 되있는 경우, 기즈모를 통해 오브젝트 조작가능)
			if (ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags() & EEngineShowFlags::SF_Primitives)
			{
				PrimitiveCollided = ObjectPicker.PickPrimitive(WorldRay, Candidate, &ActorDistance);
			}

			if (PrimitiveCollided)
				ActorPicked = PrimitiveCollided->GetOwner();
			else
				ActorPicked = nullptr;
		}

		if (Gizmo.GetGizmoDirection() == EGizmoDirection::None) //기즈모에 호버링되거나 클릭되지 않았을 때. Actor 업데이트해줌.
		{
			InLevel->SetSelectedActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				Gizmo.OnMouseRelease(PreviousGizmoDirection);
			}
		}
		//기즈모가 선택되었을 때. Actor가 선택되지 않으면 기즈모도 선택되지 않으므로 이미 Actor가 선택된 상황.
		//SelectedActor를 update하지 않고 마우스 인풋에 따라 hovering or drag
		else
		{
			PreviousGizmoDirection = Gizmo.GetGizmoDirection();
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft)) //드래그
			{
				Gizmo.OnMouseDragStart(CollisionPoint);
			}
			else
			{
				Gizmo.OnMouseHovering();
			}
		}
	}
}

TArray<UPrimitiveComponent*> UEditor::FindCandidatePrimitives(ULevel* InLevel)
{
	TArray<UPrimitiveComponent*> Candidate;

	for (AActor* Actor : InLevel->GetLevelActors())
	{
		for (auto& ActorComponent : Actor->GetOwnedComponents())
		{
			TObjectPtr<UPrimitiveComponent> Primitive = Cast<UPrimitiveComponent>(ActorComponent);
			if (Primitive)
			{
				Candidate.push_back(Primitive);
			}
		}
	}

	return Candidate;
}


FVector UEditor::GetGizmoDragLocation(FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{Gizmo.GetGizmoLocation()};
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin,
	                                       Camera.CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis), MouseWorld))
	{
		FVector MouseDistance = MouseWorld - Gizmo.GetDragStartMouseLocation();
		return Gizmo.GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	else
		return Gizmo.GetGizmoLocation();
}

FVector UEditor::GetGizmoDragRotation(FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{Gizmo.GetGizmoLocation()};
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float DotResult = (PlaneOriginToMouseStart).Dot(PlaneOriginToMouse);
		float Angle = acosf(std::max(-1.0f, std::min(1.0f, DotResult))); //플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0) // 회전축 구하기
		{
			Angle = -Angle;
		}
		//return Gizmo.GetDragStartActorRotation() + GizmoAxis * FVector::GetRadianToDegree(Angle);
		FQuaternion StartRotQuat = FQuaternion::FromEuler(Gizmo.GetDragStartActorRotation());
		FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(Gizmo.GetGizmoAxis(), Angle);
		if (Gizmo.IsWorldMode())
		{
			FQuaternion NewRotQuat = DeltaRotQuat * StartRotQuat;
			return NewRotQuat.ToEuler();
		}
		else
		{
			FQuaternion NewRotQuat = StartRotQuat * DeltaRotQuat;
			return NewRotQuat.ToEuler();
		}
	}
	else
		return Gizmo.GetActorRotation();
}

FVector UEditor::GetGizmoDragScale(FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin = Gizmo.GetGizmoLocation();
	FVector CardinalAxis = Gizmo.GetGizmoAxis();

	FVector4 GizmoAxis4{ CardinalAxis.X, CardinalAxis.Y, CardinalAxis.Z, 0.0f };
	FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
	FVector GizmoAxis = Gizmo.GetGizmoAxis();
	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);

	FVector PlaneNormal = Camera.CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis);
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);		// CardinalAxis
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);		// CardinalAxis?
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = Gizmo.GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			if (Gizmo.GetSelectedActor()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(CardinalAxis);
				return FVector(UniformValue, UniformValue, UniformValue) + FVector(1, 1, 1) * (ScaleFactor - 1) *
					UniformValue;
			}
			else
				return DragStartScale + CardinalAxis * (ScaleFactor - 1) * DragStartScale.Dot(CardinalAxis);
		}
		else
			return Gizmo.GetActorScale();
	}
	else
		return Gizmo.GetActorScale();
}
