#include "pch.h"
#include "Editor/Public/Editor.h"

#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Manager/Viewport/Public/ViewportManager.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Runtime/Level/Public/Level.h"
#include "Window/Public/ViewportClient.h"
#include "Window/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_CLASS(UEditor, UObject)

UEditor::UEditor() = default;

UEditor::~UEditor() = default;

void UEditor::Initialize()
{
	// 렌더링 리소스가 필요한 모든 멤버를 여기서 초기화
	Gizmo = MakeUnique<UGizmo>();
	Axis = MakeUnique<UAxis>();
	Grid = MakeUnique<UBatchLines>();
}

/**
 * @brief Editor 메인 갱신 함수
 */
void UEditor::Update()
{
	if (!Gizmo)
	{
		return;
	}

	// Viewport 내 마우스 상호 작용 처리
	HandleViewportInteraction();

	// 그리드 정점 데이터 업데이트
	Grid->UpdateUGridVertices(Grid->GetCellSize());

	// Actor 바운딩 박스 업데이트
	UpdateSelectionBounds();

	// 외각선 Render에 사용할 버퍼 최종 업데이트
	Grid->UpdateVertexBuffer();
}

/**
 * @brief Editor의 렌더링 함수
 */
void UEditor::RenderEditor()
{
	if (!Gizmo)
	{
		return;
	}

	Grid->Render();
	Axis->Render();

	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;
	AActor* SelectedActor = CurrentLevel ? CurrentLevel->GetSelectedActor() : nullptr;

	// 렌더러에서 현재 렌더링 중인 뷰포트 인덱스를 가져와서 기즈모 크기 계산
	auto& ViewportManager = UViewportManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	// 렌더러에서 현재 렌더링 중인 뷰포트 인덱스 가져오기
	int32 CurrentViewportIndex = Renderer.GetViewportIdx();

	if (CurrentViewportIndex >= 0 && CurrentViewportIndex < static_cast<int32>(Viewports.size()) &&
		CurrentViewportIndex < static_cast<int32>(Clients.size()) &&
		Viewports[CurrentViewportIndex] && Clients[CurrentViewportIndex])
	{
		// 현재 렌더링 중인 뷰포트의 카메라와 크기 정보 가져오기
		FViewportClient* CurrentClient = Clients[CurrentViewportIndex];
		FViewport* CurrentViewport = Viewports[CurrentViewportIndex];

		ACameraActor* ViewportCamera = CurrentClient->IsOrtho()
			                               ? CurrentClient->GetOrthoCamera()
			                               : CurrentClient->GetPerspectiveCamera();

		if (ViewportCamera && ViewportCamera->GetCameraComponent())
		{
			const FRect& ViewportRect = CurrentViewport->GetRect();
			float ViewportWidth = static_cast<float>(ViewportRect.W);
			float ViewportHeight = static_cast<float>(ViewportRect.H - CurrentViewport->GetToolbarHeight());

			// 뷰포트별로 카메라 정보를 이용해 기즈모 크기 계산
			Gizmo->RenderGizmo(SelectedActor, ViewportCamera->GetCameraComponent()->GetRelativeLocation(),
			                   ViewportCamera, ViewportWidth,
			                   ViewportHeight, CurrentViewportIndex);
		}
		else
		{
			// 카메라 정보가 없으면 기본 방식 사용
			if (Camera && Camera->GetCameraComponent())
			{
				Gizmo->RenderGizmo(SelectedActor, Camera->GetCameraComponent()->GetRelativeLocation());
			}
		}
	}
	else
	{
		// 뷰포트 인덱스가 유효하지 않으면 기본 방식 사용
		if (ACameraActor* ActiveCam = GetActivePickingCamera())
		{
			if (ActiveCam->GetCameraComponent())
			{
				Gizmo->RenderGizmo(SelectedActor, ActiveCam->GetCameraComponent()->GetRelativeLocation(), ActiveCam);
			}
		}
		else if (Camera && Camera->GetCameraComponent())
		{
			Gizmo->RenderGizmo(SelectedActor, Camera->GetCameraComponent()->GetRelativeLocation(), Camera.Get());
		}
	}
}

/**
 * @brief Viewport와 상호작용하는 부분들에 대한 에디터 처리를 진행하는 전반적인 흐름을 담당하는 함수
 * Prepare: 기본적인 객체를 준비
 * Handle Shortcut: 마우스의 위치에 따른 분기가 없는 단축키를 우선 처리
 * Ray Creation: 마우스 커서 위치 기반의 Ray 생성
 * Gizmo Interaction: 기즈모의 다양한 상태에 대응하는 내용을 처리
 */
void UEditor::HandleViewportInteraction()
{
	if (!Gizmo)
	{
		return;
	}

	// Prepare
	ACameraActor* PickingCamera = GetActivePickingCamera();
	if (!PickingCamera)
	{
		return;
	}

	// Handle Shortcut
	HandleGlobalShortcuts();

	// Ray Creation
	FRay WorldRay = CreateWorldRayFromMouse(PickingCamera);
	ObjectPicker.SetCamera(PickingCamera);

	// Gizmo Interaction
	if (Gizmo->IsDragging())
	{
		// 이미 드래그 중이라면, 기즈모의 트랜스폼만 계속 업데이트
		UpdateGizmoDrag(WorldRay, *PickingCamera);
	}
	else
	{
		// 드래그 중이 아니라면, 새로운 상호작용을 처리 (Hover, Pick, DragStart)
		HandleNewInteraction(WorldRay);
	}
}

/**
 * @brief 선택된 액터의 바운딩 박스를 계산하고 그리기 위한 정점 데이터를 업데이트하는 함수
 */
void UEditor::UpdateSelectionBounds()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;
	AActor* SelectedActor = CurrentLevel ? CurrentLevel->GetSelectedActor() : nullptr;

	if (SelectedActor && CurrentLevel)
	{
		const auto& Components = SelectedActor->GetOwnedComponents();

		for (const auto& Component : Components)
		{
			if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				FVector WorldMin, WorldMax;
				PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);

				// 프리미티브와 바운딩박스 플래그가 모두 켜져있을 때만 바운딩박스 표시
				uint64 ShowFlags = CurrentLevel->GetShowFlags();
				if ((ShowFlags & EEngineShowFlags::SF_Primitives) && (ShowFlags & EEngineShowFlags::SF_Bounds))
				{
					Grid->UpdateBoundingBoxVertices(FAABB(WorldMin, WorldMax));
				}
				else
				{
					Grid->UpdateBoundingBoxVertices({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
				}
			}
		}
	}
	else
	{
		// 선택된 Actor가 없으면 바운딩박스 비활성화
		Grid->DisableRenderBoundingBox();
	}
}

/**
 * @brief 단축키에 따라 state가 변경되는 내용들을 처리하는 함수
 */
void UEditor::HandleGlobalShortcuts()
{
	if (!Gizmo)
	{
		return;
	}

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	// 로컬 & 월드 변환
	if (InputSubsystem->IsKeyDown(EKeyInput::Ctrl) && InputSubsystem->IsKeyPressed(EKeyInput::Backquote))
	{
		Gizmo->IsWorldMode() ? Gizmo->SetLocal() : Gizmo->SetWorld();
	}

	// 기즈모 전체 순회로 모드 변경
	if (InputSubsystem->IsKeyPressed(EKeyInput::Space))
	{
		Gizmo->ChangeGizmoMode();
	}

	// 단축키로 기즈모 모드 변경
	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseRight) && InputSubsystem->IsKeyPressed(EKeyInput::W))
	{
		Gizmo->ChangeGizmoMode(EGizmoMode::Translate);
	}
	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseRight) && InputSubsystem->IsKeyPressed(EKeyInput::E))
	{
		Gizmo->ChangeGizmoMode(EGizmoMode::Rotate);
	}
	if (!InputSubsystem->IsKeyDown(EKeyInput::MouseRight) && InputSubsystem->IsKeyPressed(EKeyInput::R))
	{
		Gizmo->ChangeGizmoMode(EGizmoMode::Scale);
	}

	// 마우스 버튼을 놓으면 무조건 드래그 종료
	if (InputSubsystem->IsKeyReleased(EKeyInput::MouseLeft))
	{
		Gizmo->EndDrag();
	}
}

/**
 * @brief 마우스의 현 지점으로부터 WorldRay를 생성하는 함수
 * @param InPickingCamera 현재 사용 중인 에디터의 Picking Camera
 * @return 생성된 Ray 인스턴스
 */
FRay UEditor::CreateWorldRayFromMouse(const ACameraActor* InPickingCamera)
{
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		assert(!"InputSubsystem does not exist");
	}

	auto& ViewportManager = UViewportManager::GetInstance();

	float NdcX = 0.0f, NdcY = 0.0f;
	int32 ViewportIndex = max(0, static_cast<int32>(ViewportManager.GetViewportIndexUnderMouse()));
	bool bHaveLocalNDC = ViewportManager.ComputeLocalNDCForViewport(ViewportIndex, NdcX, NdcY);

	// 뷰포트 로컬 좌표가 있으면 사용하고, 없으면 전체 화면 기준의 좌표를 사용
	if (InPickingCamera && InPickingCamera->GetCameraComponent())
	{
		return InPickingCamera->GetCameraComponent()->ConvertToWorldRay(
			bHaveLocalNDC ? NdcX : InputSubsystem->GetMouseNDCPosition().X,
			bHaveLocalNDC ? NdcY : InputSubsystem->GetMouseNDCPosition().Y
		);
	}

	return {};
}

/**
 * @brief 기즈모 드래그 상태를 업데이트하는 함수
 * @param InWorldRay 마우스를 기준으로 생성된 충돌용 Ray
 * @param InPickingCamera 에디터의 카메라
 */
void UEditor::UpdateGizmoDrag(const FRay& InWorldRay, ACameraActor& InPickingCamera)
{
	if (!Gizmo || !Gizmo->GetSelectedActor())
	{
		return;
	}

	switch (Gizmo->GetGizmoMode())
	{
	case EGizmoMode::Translate:
		Gizmo->SetLocation(GetGizmoDragLocation(InWorldRay, InPickingCamera));
		break;
	case EGizmoMode::Rotate:
		Gizmo->SetActorRotation(GetGizmoDragRotation(InWorldRay));
		break;
	case EGizmoMode::Scale:
		Gizmo->SetActorScale(GetGizmoDragScale(InWorldRay, InPickingCamera));
		break;
	}
}

/**
 * @brief 새로운 상호작용의 시작을 처리하는 함수
 * @param InWorldRay 마우스를 기준으로 생성된 충돌용 Ray
 */
void UEditor::HandleNewInteraction(const FRay& InWorldRay)
{
	if (!Gizmo)
	{
		return;
	}

	// ImGui UI가 마우스 입력을 사용 중이면 에디터 내 상호작용을 무시 및 초기화 진행
	if (ImGui::GetIO().WantCaptureMouse)
	{
		// 모든 뷰포트의 기즈모 상태 초기화
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();
		for (int32 i = 0; i < static_cast<int32>(Viewports.size()); ++i)
		{
			Gizmo->SetGizmoDirectionForViewport(i, EGizmoDirection::None);
		}
		return;
	}

	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	ULevel* CurrentLevel = WorldSS ? WorldSS->GetCurrentLevel() : nullptr;
	if (!CurrentLevel)
	{
		return;
	}

	FVector CollisionPoint;

	// 기즈모의 상태를 먼저 업데이트
	// 현재 뷰포트 정보를 가져와서 올바른 피킹 범위 계산
	auto& ViewportManager = UViewportManager::GetInstance();
	int32 ViewportIndex = max(0, static_cast<int32>(ViewportManager.GetViewportIndexUnderMouse()));
	float ViewportWidth = 0.0f, ViewportHeight = 0.0f;

	if (ViewportIndex >= 0 && ViewportIndex < static_cast<int32>(ViewportManager.GetViewports().size()))
	{
		const auto& Viewports = ViewportManager.GetViewports();
		if (Viewports[ViewportIndex])
		{
			const FRect& ViewportRect = Viewports[ViewportIndex]->GetRect();
			ViewportWidth = static_cast<float>(ViewportRect.W);
			ViewportHeight = static_cast<float>(ViewportRect.H - Viewports[ViewportIndex]->GetToolbarHeight());
		}
	}

	ObjectPicker.PickGizmo(InWorldRay, *Gizmo, CollisionPoint, ViewportWidth, ViewportHeight, ViewportIndex);

	// 업데이트된 기즈모의 상태를 확인하여 상호작용 여부를 판단
	if (Gizmo->GetGizmoDirectionForViewport(ViewportIndex) != EGizmoDirection::None)
	{
		if (InputSubsystem->IsKeyPressed(EKeyInput::MouseLeft))
		{
			Gizmo->OnMouseDragStart(CollisionPoint);
		}
		else
		{
			UGizmo::OnMouseHovering();
		}
	}
	// 기즈모와 상호작용하지 않았다면, 새로운 Actor picking 시도
	else
	{
		if (InputSubsystem->IsKeyPressed(EKeyInput::MouseLeft))
		{
			TObjectPtr<AActor> PickedActor = nullptr;
			if (CurrentLevel->GetShowFlags() & EEngineShowFlags::SF_Primitives)
			{
				TArray<UPrimitiveComponent*> Candidate = FindCandidateActors(CurrentLevel);

				float ActorDistance = -1.0f;

				UPrimitiveComponent* PrimitiveCollided =
					ObjectPicker.PickPrimitive(InWorldRay, Candidate, &ActorDistance);

				if (PrimitiveCollided)
				{
					PickedActor = PrimitiveCollided->GetOwner();
				}
			}
			CurrentLevel->SetSelectedActor(PickedActor);
		}
	}
}

/**
 * @brief 충돌할만한 Actor들을 탐색하는 과정
 * @param InLevel 현재 Level
 * @return 충돌이 예측되는 Primitive 모음
 */
TArray<UPrimitiveComponent*> UEditor::FindCandidateActors(const ULevel* InLevel)
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

	// TODO(KHJ): Actor 단위로 바꿔줘야 할 듯?
	return Candidate;
}

/**
 * @brief 마우스 입력을 기반으로 기즈모를 드래그할 때의 새로운 월드 위치를 계산하는 함수
 * 현재는 각 카메라 타입에 맞는 함수로 반환하도록 처리
 * @param InWorldRay 마우스 커서의 위치에서 월드 공간으로 쏘는 Ray
 * @param InCamera 현재 에디터의 뷰포트 카메라
 * @return 계산된 기즈모의 새로운 월드 공간 위치 벡터
 */
FVector UEditor::GetGizmoDragLocation(const FRay& InWorldRay, ACameraActor& InCamera)
{
	if (!Gizmo || !InCamera.GetCameraComponent())
	{
		return {};
	}

	// Orthographic
	if (InCamera.GetCameraComponent()->GetCameraType() == ECameraType::ECT_Orthographic)
	{
		return GetGizmoDragLocationForOrthographic(InCamera);
	}
	// Perspective
	else
	{
		return GetGizmoDragLocationForPerspective(InWorldRay, InCamera);
	}
}

FVector UEditor::GetGizmoDragLocationForPerspective(const FRay& InWorldRay, ACameraActor& InCamera)
{
	FVector MouseWorld;
	FVector PlaneOrigin{Gizmo->GetGizmoLocation()};

	// 현재 마우스가 있는 뷰포트의 기즈모 방향 사용
	auto& ViewportManager = UViewportManager::GetInstance();
	int32 ViewportIndex = ViewportManager.GetViewportIndexUnderMouse();
	ViewportIndex = max(ViewportIndex, 0);
	EGizmoDirection CurrentDirection = Gizmo->GetGizmoDirectionForViewport(ViewportIndex);

	// 방향에 따른 축 벡터 계산
	FVector GizmoAxis;
	switch (CurrentDirection)
	{
	// X축
	case EGizmoDirection::Forward:
		GizmoAxis = FVector(1, 0, 0);
		break;
	// Y축
	case EGizmoDirection::Right:
		GizmoAxis = FVector(0, 1, 0);
		break;
	// Z축
	case EGizmoDirection::Up:
		GizmoAxis = FVector(0, 0, 1);
		break;
	// 선택된 축이 없으면 현재 위치 반환
	default:
		return Gizmo->GetGizmoLocation();
	}

	if (!Gizmo->IsWorldMode())
	{
		FVector4 GizmoAxis4{GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f};
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo->GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (InCamera.GetCameraComponent() && ObjectPicker.IsRayCollideWithPlane(InWorldRay, PlaneOrigin,
	                                                                        InCamera.GetCameraComponent()->
	                                                                        CalculatePlaneNormal(GizmoAxis).Cross(
		                                                                        GizmoAxis), MouseWorld))
	{
		FVector MouseDistance = MouseWorld - Gizmo->GetDragStartMouseLocation();
		return Gizmo->GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	else
	{
		// 드래그 실패 시 현재 위치 반환
		return Gizmo->GetGizmoLocation();
	}
}

/**
 * @brief Orthographic 카메라에서 기즈모 드래그 위치 계산하는 함수
 * @param InCamera 현재 카메라
 * @return 계산된 새로운 위치
 */
FVector UEditor::GetGizmoDragLocationForOrthographic(const ACameraActor& InCamera)
{
	if (!Gizmo)
	{
		return {};
	}

	// 현재 뷰포트 정보 가져오기
	auto& ViewportManager = UViewportManager::GetInstance();
	int32 ViewportIndex = ViewportManager.GetViewportIndexUnderMouse();
	if (ViewportIndex < 0)
	{
		return Gizmo->GetGizmoLocation();
	}
	const auto& Clients = ViewportManager.GetClients();
	if (ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return Gizmo->GetGizmoLocation();
	}
	FViewportClient* ViewportClient = Clients[ViewportIndex];
	if (!ViewportClient)
	{
		return Gizmo->GetGizmoLocation();
	}
	const EViewType ViewType = ViewportClient->GetViewType();
	if (ViewType == EViewType::Perspective)
	{
		return Gizmo->GetGizmoLocation();
	}

	// 뷰포트 크기 정보 가져오기
	const auto& Viewports = ViewportManager.GetViewports();
	if (ViewportIndex >= static_cast<int32>(Viewports.size()))
	{
		return Gizmo->GetGizmoLocation();
	}

	FViewport* Viewport = Viewports[ViewportIndex];
	if (!Viewport)
	{
		return Gizmo->GetGizmoLocation();
	}

	const FRect& Rect = Viewport->GetRect();
	const int32 ToolH = Viewport->GetToolbarHeight();
	const float Width = static_cast<float>(Rect.W);
	const float Height = static_cast<float>(max<LONG>(0, Rect.H - ToolH));
	if (Width <= 0.0f || Height <= 0.0f)
	{
		return Gizmo->GetGizmoLocation();
	}

	// 뷰포트 타입에 따른 기저 벡터 계산
	FVector Fwd, Right, Up;
	CalculateBasisVectorsForViewType(ViewType, Fwd, Right, Up);

	// 마우스 델타 가져오기
	UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
	if (!InputSubsystem)
	{
		return Gizmo->GetGizmoLocation();
	}

	const FVector& MouseDelta = InputSubsystem->GetMouseDelta();
	if (MouseDelta.X == 0.0f && MouseDelta.Y == 0.0f)
	{
		return Gizmo->GetGizmoLocation();
	}

	// NDC 델타 계산 (ViewportManager와 동일한 방식)
	const float Aspect = (Height > 0.f) ? (Width / Height) : 1.0f;
	const float Rad = InCamera.GetCameraComponent()
		                  ? FVector::GetDegreeToRadian(InCamera.GetCameraComponent()->GetFovY())
		                  : 0.0f;
	const float OrthoWidth = 2.0f * std::tanf(Rad * 0.5f);
	const float OrthoHeight = (Aspect > 0.0f) ? (OrthoWidth / Aspect) : OrthoWidth;

	// 화면 기준 드래그 비율 기본 부호
	float sX = -1.0f;
	float sY = 1.0f;

	// 카메라에 따라 추가 보정
	if (ViewType != EViewType::OrthoTop && ViewType != EViewType::OrthoBottom)
	{
		sX *= -1.0f;
		sY *= -1.0f;
	}

	// NDC 델타 계산
	const float NdcDX = sX * (MouseDelta.X / Width) * 2.0f;
	const float NdcDY = sY * (MouseDelta.Y / Height) * 2.0f;

	// 월드 델타 계산
	const FVector WorldDelta = Right * (NdcDX * (OrthoWidth * 0.5f)) + Up * (NdcDY * (OrthoHeight * 0.5f));

	// 뷰포트별 기즈모 방향에 따른 축 계산
	EGizmoDirection CurrentDirection = Gizmo->GetGizmoDirectionForViewport(ViewportIndex);
	if (CurrentDirection == EGizmoDirection::None) return Gizmo->GetGizmoLocation();

	// 방향에 따른 축 벡터 계산
	FVector GizmoAxis;
	switch (CurrentDirection)
	{
	// X축
	case EGizmoDirection::Forward:
		GizmoAxis = FVector(1, 0, 0);
		break;
	// Y축
	case EGizmoDirection::Right:
		GizmoAxis = FVector(0, 1, 0);
		break;
	// Z축
	case EGizmoDirection::Up:
		GizmoAxis = FVector(0, 0, 1);
		break;
	default:
		return Gizmo->GetGizmoLocation();
	}

	if (!Gizmo->IsWorldMode())
	{
		FVector4 GizmoAxis4{GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f};
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo->GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	// 축 방향 성분만 추출
	float AxisMovement = WorldDelta.Dot(GizmoAxis);
	return Gizmo->GetGizmoLocation() + GizmoAxis * AxisMovement;
}


/**
 * @brief 뷰포트 타입에 따른 기저 벡터 계산 헬퍼 함수 (ViewportManager 로직 참고)
 * @param InViewType 뷰포트 타입
 * @param OutForward Forward 벡터
 * @param OutRight Right 벡터
 * @param OutUp Up 벡터
 */
void UEditor::CalculateBasisVectorsForViewType(EViewType InViewType, FVector& OutForward, FVector& OutRight,
                                               FVector& OutUp)
{
	// ViewportManager::UpdateOrthoGraphicCameraPoint의 로직 참고
	switch (InViewType)
	{
	case EViewType::OrthoTop: OutForward = FVector(0, 0, -1);
		break;
	case EViewType::OrthoBottom: OutForward = FVector(0, 0, 1);
		break;
	case EViewType::OrthoFront: OutForward = FVector(-1, 0, 0);
		break;
	case EViewType::OrthoBack: OutForward = FVector(1, 0, 0);
		break;
	case EViewType::OrthoRight: OutForward = FVector(0, 1, 0);
		break;
	case EViewType::OrthoLeft: OutForward = FVector(0, -1, 0);
		break;
	default: OutForward = FVector(1, 0, 0);
		break; // Perspective 등 기본값
	}
	OutForward.Normalize();

	// Up 레퍼런스 벡터 계산 (ViewportManager 로직과 동일)
	FVector UpRef(0, 0, 1);
	if (std::fabs(OutForward.Dot(UpRef)) > 0.999f) UpRef = FVector(1, 0, 0);

	// 기저 벡터 계산
	OutRight = OutForward.Cross(UpRef);
	OutRight.Normalize();
	OutUp = OutRight.Cross(OutForward);
	OutUp.Normalize();
}

/**
 * @brief 마우스 드래그 입력을 기반으로 기즈모를 회전시키고 새로운 회전 값을 계산하는 함수
 * @param InWorldRay 마우스 커서의 위치에서 월드 공간으로 쏘는 Ray
 * @return 계산된 객체의 새로운 회전값
 */
FVector UEditor::GetGizmoDragRotation(const FRay& InWorldRay)
{
	if (!Gizmo)
	{
		return {};
	}

	FVector MouseWorld;
	FVector PlaneOrigin{Gizmo->GetGizmoLocation()};

	// 현재 마우스가 있는 뷰포트의 기즈모 방향 사용
	auto& ViewportManager = UViewportManager::GetInstance();
	int32 ViewportIndex = ViewportManager.GetViewportIndexUnderMouse();
	ViewportIndex = max(ViewportIndex, 0);
	EGizmoDirection CurrentDirection = Gizmo->GetGizmoDirectionForViewport(ViewportIndex);

	// 방향에 따른 축 벡터 계산
	FVector GizmoAxis;
	switch (CurrentDirection)
	{
	// X축
	case EGizmoDirection::Forward:
		GizmoAxis = FVector(1, 0, 0);
		break;
	// Y축
	case EGizmoDirection::Right:
		GizmoAxis = FVector(0, 1, 0);
		break;
	// Z축
	case EGizmoDirection::Up:
		GizmoAxis = FVector(0, 0, 1);
		break;
	default: return Gizmo->GetActorRotation(); // 선택된 축이 없으면 현재 회전값 반환
	}

	// Local 모드 변환 전 원본 축 저장
	FVector OriginalAxis = GizmoAxis;

	if (!Gizmo->IsWorldMode())
	{
		FVector4 GizmoAxis4{GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f};
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo->GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker.IsRayCollideWithPlane(InWorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo->GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float DotResult = (PlaneOriginToMouseStart).Dot(PlaneOriginToMouse);
		float Angle = acosf(std::max(-1.0f, std::min(1.0f, DotResult))); //플레인 중심부터 마우스까지 벡터 이용해서 회전각도 구하기
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0) // 회전축 구하기
		{
			Angle = -Angle;
		}
		//return Gizmo->GetDragStartActorRotation() + GizmoAxis * FVector::GetRadianToDegree(Angle);
		FQuaternion StartRotQuat = FQuaternion::FromEuler(Gizmo->GetDragStartActorRotation());

		// World 모드에서는 원본 축, Local 모드에서는 변환된 축을 사용
		FVector RotationAxis = Gizmo->IsWorldMode() ? OriginalAxis : GizmoAxis;
		FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(RotationAxis, Angle);
		if (Gizmo->IsWorldMode())
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
	{
		// Ray가 평면과 충돌하지 않으면, 회전하지 않도록 현재 액터의 회전 값을 그대로 반환
		return Gizmo->GetActorRotation();
	}
}

/**
 * @brief 마우스 드래그 입력을 기반으로 기즈모를 사용하여 객체의 크기를 조절하고 새로운 스케일 값을 계산하는 함수
 * @param InWorldRay 마우스 커서의 위치에서 월드 공간으로 쏘는 Ray
 * @param InCamera 현재 에디터의 뷰포트 카메라
 * @return 계산된 객체의 새로운 스케일 벡터
 */
FVector UEditor::GetGizmoDragScale(const FRay& InWorldRay, ACameraActor& InCamera)
{
	if (!Gizmo)
	{
		return {};
	}

	FVector MouseWorld;
	FVector PlaneOrigin = Gizmo->GetGizmoLocation();

	// 현재 마우스가 있는 뷰포트의 기즈모 방향 사용
	auto& ViewportManager = UViewportManager::GetInstance();
	int32 ViewportIndex = ViewportManager.GetViewportIndexUnderMouse();
	ViewportIndex = max(ViewportIndex, 0);
	EGizmoDirection CurrentDirection = Gizmo->GetGizmoDirectionForViewport(ViewportIndex);

	// 방향에 따른 축 벡터 계산
	FVector CardinalAxis;
	switch (CurrentDirection)
	{
	// X축
	case EGizmoDirection::Forward:
		CardinalAxis = FVector(1, 0, 0);
		break;
	// Y축
	case EGizmoDirection::Right:
		CardinalAxis = FVector(0, 1, 0);
		break;
	// Z축
	case EGizmoDirection::Up:
		CardinalAxis = FVector(0, 0, 1);
		break;
	// 선택된 축이 없으면 현재 스케일값 반환
	default:
		return Gizmo->GetActorScale();
	}

	FVector4 GizmoAxis4{CardinalAxis.X, CardinalAxis.Y, CardinalAxis.Z, 0.0f};
	FVector RadRotation = FVector::GetDegreeToRadian(Gizmo->GetActorRotation());
	FVector GizmoAxis;
	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);

	FVector PlaneNormal = InCamera.GetCameraComponent()
		                      ? InCamera.GetCameraComponent()->CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis)
		                      : FVector();
	if (ObjectPicker.IsRayCollideWithPlane(InWorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo->GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis); // CardinalAxis
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis); // CardinalAxis?
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = Gizmo->GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			if (Gizmo->GetSelectedActor()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(CardinalAxis);
				return FVector(UniformValue, UniformValue, UniformValue) + FVector(1, 1, 1) * (ScaleFactor - 1) *
					UniformValue;
			}
			else
			{
				return DragStartScale + CardinalAxis * (ScaleFactor - 1) * DragStartScale.Dot(CardinalAxis);
			}
		}
		else
		{
			// 드래그가 실패하면 현재 스케일 값을 반환
			return Gizmo->GetActorScale();
		}
	}
	else
	{
		return Gizmo->GetActorScale();
	}
}

/**
 * @brief 현재 사용하는 Viewport의 카메라를 가져오는 함수
 * @return 타입에 따라 Perspective / Orthogonal Viewport 카메라
 */
ACameraActor* UEditor::GetActivePickingCamera()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Clients = ViewportManager.GetClients();

	int32 Index = ViewportManager.GetViewportIndexUnderMouse();
	Index = max(Index, 0);

	if (Index >= static_cast<int32>(Clients.size()))
	{
		return nullptr;
	}

	FViewportClient* ViewportClient = Clients[Index];
	if (!ViewportClient)
	{
		return nullptr;
	}
	if (ViewportClient->GetViewType() == EViewType::Perspective)
	{
		return ViewportClient->GetPerspectiveCamera();
	}
	else
	{
		return ViewportClient->GetOrthoCamera();
	}
}
