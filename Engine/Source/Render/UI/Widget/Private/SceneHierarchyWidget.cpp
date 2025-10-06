#include "pch.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

#include "Window/Public/ViewportClient.h"
#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/Subsystem/Input/Public/InputSubsystem.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Runtime/Component/Public/PrimitiveComponent.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

USceneHierarchyWidget::USceneHierarchyWidget()
	: UWidget("Scene Hierarchy Widget")
{
}

USceneHierarchyWidget::~USceneHierarchyWidget() = default;

void USceneHierarchyWidget::Initialize()
{
	UE_LOG("SceneHierarchyWidget: Initialized");
}

void USceneHierarchyWidget::Update()
{
	// 카메라 애니메이션 업데이트 (카메라별로 개별 진행)
	auto* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();

	for (FViewportClient* Client : ViewportSS->GetClients())
	{
		// Perspective 카메라 업데이트
		TObjectPtr<ACameraActor> PerspectiveCamera = TObjectPtr(Client->GetPerspectiveCamera());
		if (PerspectiveCamera && CameraAnimatingStates[PerspectiveCamera->GetName()])
		{
			UpdateCameraAnimation(PerspectiveCamera);
		}

		// Orthographic 카메라 업데이트
		TObjectPtr<ACameraActor> OrthoCamera = TObjectPtr(Client->GetOrthoCamera());
		if (OrthoCamera && CameraAnimatingStates[OrthoCamera->GetName()])
		{
			UpdateCameraAnimation(OrthoCamera);
		}
	}
}

void USceneHierarchyWidget::RenderWidget()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	// 헤더 정보
	ImGui::Text("Level: %s", CurrentLevel->GetName().ToString().c_str());
	ImGui::Separator();

	// 검색창 렌더링
	RenderSearchBar();

	const TArray<TObjectPtr<AActor>>& LevelActors = CurrentLevel->GetLevelActors();

	if (LevelActors.IsEmpty())
	{
		ImGui::TextUnformatted("No Actors in Level");
		return;
	}

	// 필터링 업데이트
	if (bNeedsFilterUpdate)
	{
		UE_LOG("SceneHierarchy: 필터 업데이트 실행 중...");
		UpdateFilteredActors(LevelActors);
		bNeedsFilterUpdate = false;
	}

	// Actor 개수 표시
	if (SearchFilter.empty())
	{
		ImGui::Text("Total Actors: %d", LevelActors.Num());
	}
	else
	{
		ImGui::Text("%d / %d actors", static_cast<int32>(FilteredIndices.Num()), LevelActors.Num());
	}
	ImGui::Spacing();

	// Actor 리스트를 스크롤 가능한 영역으로 표시
	if (ImGui::BeginChild("ActorList", ImVec2(0, 0), true))
	{
		if (SearchFilter.empty())
		{
			// 검색어가 없으면 모든 Actor 표시
			for (int32 i = 0; i < static_cast<int32>(LevelActors.Num()); ++i)
			{
				if (LevelActors[i])
				{
					RenderActorInfo(LevelActors[i], i);
				}
			}
		}
		else
		{
			// 필터링된 Actor들만 표시
			for (int32 FilteredIndex : FilteredIndices)
			{
				if (FilteredIndex < LevelActors.Num() && LevelActors[FilteredIndex])
				{
					RenderActorInfo(LevelActors[FilteredIndex], FilteredIndex);
				}
			}

			// 검색 결과가 없으면 메시지 표시
			if (FilteredIndices.IsEmpty())
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "검색 결과가 없습니다.");
			}
		}
	}
	ImGui::EndChild();
}

/**
 * @brief Actor 정보를 렌더링하는 헬퍼 함수
 * @param InActor 렌더링할 Actor
 * @param InIndex Actor의 인덱스
 */
void USceneHierarchyWidget::RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex)
{
	// TODO(KHJ): 컴포넌트 정보 탐색을 위한 트리 노드를 작업 후 남겨두었음, 필요하다면 사용할 것

	if (!InActor)
	{
		return;
	}

	ImGui::PushID(InIndex);

	// 현재 선택된 Actor인지 확인
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	UEditor* Editor = WorldSS->GetEditor();
	if (!Editor)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();
	bool bIsSelected = CurrentLevel && Editor->GetGizmo()->GetSelectedActor() == InActor;

	// 선택된 Actor는 하이라이트
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 노란색
	}

	FName ActorName = InActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	// Actor의 PrimitiveComponent들의 Visibility 체크
	bool bHasPrimitive = false;
	bool bAllVisible = true;
	TObjectPtr<UPrimitiveComponent> FirstPrimitive = nullptr;

	// Actor의 모든 Component 중에서 PrimitiveComponent 찾기
	for (auto& Component : InActor->GetOwnedComponents())
	{
		if (TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			bHasPrimitive = true;

			if (!FirstPrimitive)
			{
				FirstPrimitive = PrimitiveComponent;
			}

			if (!PrimitiveComponent->IsVisible())
			{
				bAllVisible = false;
			}
		}
	}

	// PrimitiveComponent가 있는 경우에만 Visibility 버튼 표시
	if (bHasPrimitive)
	{
		if (ImGui::SmallButton(bAllVisible ? "[O]" : "[X]"))
		{
			// 모든 PrimitiveComponent의 Visibility 토글
			bool bNewVisibility = !bAllVisible;
			for (auto& Component : InActor->GetOwnedComponents())
			{
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
				{
					PrimComp->SetVisibility(bNewVisibility);
				}
			}
			UE_LOG_INFO("SceneHierarchy: %s의 가시성이 %s로 변경되었습니다",
			            ActorName.ToString().data(),
			            bNewVisibility ? "Visible" : "Hidden");
		}
	}
	else
	{
		// PrimitiveComponent가 없는 경우 비활성화된 버튼 표시
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		ImGui::SmallButton("[-]");
		ImGui::PopStyleVar();
	}

	// 이름 클릭 감지 (오른쪽)
	ImGui::SameLine();

	// 이름 변경 모드인지 확인
	if (RenamingActor == InActor)
	{
		// 이름 변경 입력창
		ImGui::PushItemWidth(-1.0f);
		bool bEnterPressed = ImGui::InputText("##Rename", RenameBuffer, sizeof(RenameBuffer),
		                                      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopItemWidth();

		// Enter 키로 확인
		if (bEnterPressed)
		{
			FinishRenaming(true);
		}
		// ESC 키로 취소
		else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			FinishRenaming(false);
		}
		// 다른 곳 클릭으로 취소
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())
		{
			FinishRenaming(false);
		}

		// 포커스 설정 (첫 렌더링에서만)
		if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive())
		{
			ImGui::SetKeyboardFocusHere(-1);
		}
	}
	else
	{
		// 일반 선택 모드
		bool bClicked = ImGui::Selectable(ActorDisplayName.data(), bIsSelected, ImGuiSelectableFlags_SpanAllColumns);

		if (bClicked)
		{
			double CurrentTime = ImGui::GetTime();

			// 이미 선택된 Actor를 2초 이내 다시 클릭한 경우
			if (bIsSelected && LastClickedActor == InActor &&
				(CurrentTime - LastClickTime) > RENAME_CLICK_DELAY &&
				(CurrentTime - LastClickTime) < 2.0f)
			{
				// 이름 변경 모드 시작
				StartRenaming(InActor);
			}
			else
			{
				// 일반 선택
				SelectActor(InActor, false);
			}

			LastClickTime = CurrentTime;
			LastClickedActor = InActor;
		}

		UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();

		// 더블 클릭 감지: 카메라 이동 수행 (hover 필요)
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			SelectActor(InActor, true);

			// 더블클릭 시 이름변경 모드 비활성화
			FinishRenaming(false);
		}

		// F키 입력 감지: 선택된 Actor에 대해서는 hover 조건 없이 포커싱 가능
		if (InputSubsystem && bIsSelected && InputSubsystem->IsKeyDown(EKeyInput::F))
		{
			SelectActor(InActor, true);

			// F키 포커싱 시 이름변경 모드 비활성화
			FinishRenaming(false);
		}
	}

	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}

	ImGui::PopID();
}

/**
 * @brief Actor를 선택하는 함수
 * @param InActor 선택할 Actor
 * @param bInFocusCamera 카메라 포커싱 여부 (더블 클릭 시 true)
 */
void USceneHierarchyWidget::SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera)
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	TObjectPtr<ULevel> CurrentLevel = WorldSS->GetCurrentLevel();
	if (CurrentLevel)
	{
		UEditor* Editor = WorldSS->GetEditor();
		if (!Editor)
		{
			return;
		}

		Editor->GetGizmo()->SetSelectedActor(InActor);

		// 카메라 포커싱은 더블 클릭에서만 수행
		// 우클릭 중에는 포커싱하지 않음 (카메라 조작 모드)
		UInputSubsystem* InputSubsystem = GEngine->GetEngineSubsystem<UInputSubsystem>();
		if (InActor && bInFocusCamera && InputSubsystem && !InputSubsystem->IsKeyDown(EKeyInput::MouseRight))
		{
			auto* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();

			// Start focus on all cameras
			for (const auto Client : ViewportSS->GetClients())
			{
				// Perspective 카메라 포커싱
				if (Client->GetViewType() == EViewType::Perspective)
				{
					TObjectPtr<ACameraActor> PerspectiveCamera = TObjectPtr(Client->GetPerspectiveCamera());
					FocusOnActor(PerspectiveCamera, InActor);
				}
				// Orthographic 카메라 포커싱
				else
				{
					TObjectPtr<ACameraActor> OrthoCamera = TObjectPtr(Client->GetOrthoCamera());
					FocusOnActor(OrthoCamera, InActor);
				}
			}
		}
		else
		{
			UE_LOG("SceneHierarchy: %s를 선택했습니다", InActor->GetName().ToString().data());
		}
	}
}

/**
 * @brief 단일 카메라를 특정 Actor에 포커스하는 함수
 * @param InCamera 포커싱 처리할 카메라
 * @param InActor 포커스할 Actor
 */
void USceneHierarchyWidget::FocusOnActor(TObjectPtr<ACameraActor> InCamera, TObjectPtr<AActor> InActor)
{
	if (!InCamera || !InActor)
	{
		return;
	}

	FName CameraName = InCamera->GetName();

	// 이미 해당 카메라가 애니메이션 중이면 무시
	if (CameraAnimatingStates[CameraName])
	{
		return;
	}

	// 현재 카메라의 위치와 회전을 저장
	CameraStartLocations[CameraName] = InCamera->GetActorLocation();
	CameraCurrentRotations[CameraName] = InCamera->GetActorRotation();

	FVector ActorLocation = InActor->GetActorLocation();
	FVector TargetLocation;

	if (InCamera->GetCameraComponent()->GetCameraType() == ECameraType::ECT_Orthographic)
	{
		// Orthographic 카메라의 경우, 오브젝트를 화면 중심에 위치시키기 위해
		// 카메라의 현재 뷰 방향을 유지하면서 위치만 조정
		FVector CurrentCameraLocation = InCamera->GetActorLocation();
		FVector CameraForward = InCamera->GetCameraComponent()->GetForward();

		// Actor 위치에서 카메라 위치로의 벡터
		FVector ToCamera = CurrentCameraLocation - ActorLocation;

		// Forward 방향의 거리는 유지 (뷰 축과의 거리 유지)
		float ForwardDistance = ToCamera.Dot(CameraForward);

		// Actor 위치에서 Forward 방향으로 현재 거리만큼 떨어진 지점을 타겟으로 설정
		// 이렇게 하면 오브젝트가 화면 중앙에 위치하게 됨
		TargetLocation = ActorLocation + (CameraForward * ForwardDistance);

		UE_LOG_SUCCESS("SceneHierarchy: Ortho 카메라 포커싱 완료");
	}
	else
	{
		// 카메라의 정확한 Forward 벡터를 사용하여 화면 중앙 배치 보정
		// Camera 클래스에서 이미 계산된 정확한 Forward 벡터 사용
		FVector CameraForward = InCamera->GetCameraComponent()->GetForward();
		TargetLocation = ActorLocation - (CameraForward * FOCUS_DISTANCE);
	}

	// Actor를 정확히 화면 중앙에 놓기 위해 Forward 방향의 반대로 거리를 둔 위치에 카메라 배치
	// 이렇게 하면 카메라 회전 유지 상태에서 Actor가 정확히 화면 중심에 위치함
	CameraTargetLocations[CameraName] = TargetLocation;

	// 카메라별 애니메이션 시작
	CameraAnimatingStates[CameraName] = true;
	CameraAnimationTimes[CameraName] = 0.0f;
	CameraAnimationTargets[CameraName] = InActor;
}

/**
 * @brief 단일 카메라 애니메이션을 업데이트하는 함수
 * 선형 보간을 활용한 부드러운 움직임을 구현함
 */
void USceneHierarchyWidget::UpdateCameraAnimation(TObjectPtr<ACameraActor> InCamera)
{
	if (!InCamera)
	{
		return;
	}

	FName CameraName = InCamera->GetName();

	// 해당 카메라가 애니메이션 중인지 확인
	if (!CameraAnimatingStates[CameraName])
	{
		return;
	}

	CameraAnimationTimes[CameraName] += DT;

	// 애니메이션 진행 비율 계산
	float Progress = CameraAnimationTimes[CameraName] / CAMERA_ANIMATION_DURATION;

	if (Progress >= 1.0f)
	{
		// 애니메이션 완료
		Progress = 1.0f;
		CameraAnimatingStates[CameraName] = false;
	}

	// Easing 함수를 사용하여 부드러운 움직임 확보 (easeInOutQuart)
	float SmoothProgress;
	if (Progress < 0.5f)
	{
		// 움직인 전반에서는 아주 천천히 가속
		SmoothProgress = 8.0f * Progress * Progress * Progress * Progress;
	}
	else
	{
		// 움직인 후반에서는 아주 천천히 감속
		float ProgressFromEnd = Progress - 1.0f;
		SmoothProgress = 1.0f - 8.0f * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd;
	}

	FVector CameraStartLocation = CameraStartLocations[CameraName];
	FVector CameraTargetLocation = CameraTargetLocations[CameraName];

	// Linear interpolation으로 위치 보간
	FVector CurrentLocation = CameraStartLocation + (CameraTargetLocation - CameraStartLocation) * SmoothProgress;

	// 카메라 위치 설정
	// 의도가 카메라의 위치만 옮겨서 화면 중앙에 오브젝트를 두는 것이었기 때문에 Rotation은 처리하지 않음
	InCamera->SetActorLocation(CurrentLocation);

	if (!CameraAnimatingStates[CameraName])
	{
		UE_LOG_SUCCESS("SceneHierarchy: %s: 포커싱 애니메이션 완료", CameraName.ToString().data());

		// 애니메이션이 완료된 Ortho 카메라에 대해서만 중심점 업데이트
		if (InCamera->GetCameraComponent()->GetCameraType() == ECameraType::ECT_Orthographic)
		{
			TObjectPtr<AActor> TargetActor = CameraAnimationTargets[CameraName];
			if (TargetActor)
			{
				GEngine->GetEngineSubsystem<UViewportSubsystem>()->SetOrthoGraphicCenter(TargetActor->GetActorLocation());
				UE_LOG_SUCCESS("SceneHierarchy: Ortho 중심점을 %s 위치로 업데이트", TargetActor->GetName().ToString().c_str());
			}
		}
	}
}

/**
 * @brief 검색창을 렌더링하는 함수
 */
void USceneHierarchyWidget::RenderSearchBar()
{
	// 검색 지우기 버튼
	if (ImGui::SmallButton("X"))
	{
		memset(SearchBuffer, 0, sizeof(SearchBuffer));
		SearchFilter.clear();
		bNeedsFilterUpdate = true;
	}

	// 검색창
	ImGui::SameLine();
	ImGui::PushItemWidth(-1.0f); // 나머지 너비 모두 사용
	bool bTextChanged = ImGui::InputTextWithHint("##Search", "검색...", SearchBuffer, sizeof(SearchBuffer));
	ImGui::PopItemWidth();

	// 검색어가 변경되면 필터 업데이트 플래그 설정
	if (bTextChanged)
	{
		FString NewSearchFilter = FString(SearchBuffer);
		if (NewSearchFilter != SearchFilter)
		{
			UE_LOG("SceneHierarchy: 검색어 변경: '%s' -> '%s'", SearchFilter.data(), NewSearchFilter.data());
			SearchFilter = NewSearchFilter;
			bNeedsFilterUpdate = true;
		}
	}
}

/**
 * @brief 필터링된 Actor 인덱스 리스트를 업데이트하는 함수
 * @param InLevelActors 레벨의 모든 Actor 리스트
 */
void USceneHierarchyWidget::UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors)
{
	FilteredIndices.Empty();

	if (SearchFilter.empty())
	{
		return; // 검색어가 없으면 모든 Actor 표시
	}

	// 검색 성능 최적화: 대소문자 변환을 한 번만 수행
	FString SearchLower = SearchFilter;
	std::transform(SearchLower.begin(), SearchLower.end(), SearchLower.begin(), ::tolower);

	// UE_LOG("SceneHierarchy: 검색어 = '%s', 변환된 검색어 = '%s'", SearchFilter.data(), SearchLower.data());
	// UE_LOG("SceneHierarchy: Level에 %zu개의 Actor가 있습니다", InLevelActors.Num());

	for (int32 i = 0; i < InLevelActors.Num(); ++i)
	{
		if (InLevelActors[i])
		{
			FString ActorName = InLevelActors[i]->GetName().ToString();
			bool bMatches = IsActorMatchingSearch(ActorName, SearchLower);
			// UE_LOG("SceneHierarchy: Actor[%d] = '%s', 매치 = %s", i, ActorName.c_str(), bMatches ? "Yes" : "No");

			if (bMatches)
			{
				FilteredIndices.Add(i);
			}
		}
	}

	UE_LOG("SceneHierarchy: 필터링 결과: %d개 찾음", FilteredIndices.Num());
}

/**
 * @brief Actor 이름이 검색어와 일치하는지 확인
 * @param InActorName Actor 이름
 * @param InSearchTerm 검색어 (대소문자를 무시)
 * @return 일치하면 true
 */
bool USceneHierarchyWidget::IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm)
{
	if (InSearchTerm.empty())
	{
		return true;
	}

	FString ActorNameLower = InActorName;
	std::transform(ActorNameLower.begin(), ActorNameLower.end(), ActorNameLower.begin(), ::tolower);

	bool bResult = ActorNameLower.find(InSearchTerm) != std::string::npos;

	return bResult;
}

/**
 * @brief 이름 변경 모드를 시작하는 함수
 * @param InActor 이름을 변경할 Actor
 */
void USceneHierarchyWidget::StartRenaming(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	RenamingActor = InActor;
	FString CurrentName = InActor->GetName().ToString();

	// 현재 이름을 버퍼에 복사
	strncpy_s(RenameBuffer, CurrentName.data(), sizeof(RenameBuffer) - 1);
	RenameBuffer[sizeof(RenameBuffer) - 1] = '\0';

	UE_LOG("SceneHierarchy: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

/**
 * @brief 이름 변경을 완료하는 함수
 * @param bInConfirm true면 적용, false면 취소
 */
void USceneHierarchyWidget::FinishRenaming(bool bInConfirm)
{
	if (!RenamingActor)
	{
		return;
	}

	if (bInConfirm)
	{
		FString NewName = FString(RenameBuffer);
		// 빈 이름 방지 및 이름 변경 여부 확인
		if (!NewName.empty() && NewName != RenamingActor->GetName().ToString())
		{
			// Detail 패널과 동일한 방식 사용
			RenamingActor->SetDisplayName(NewName);
			UE_LOG_SUCCESS("SceneHierarchy: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.c_str());

			// 검색 필터를 업데이트해야 할 수도 있음
			bNeedsFilterUpdate = true;
		}
		else if (NewName.empty())
		{
			UE_LOG_WARNING("SceneHierarchy: 빈 이름으로 인해 이름 변경 취소됨");
		}
	}
	else
	{
		UE_LOG_WARNING("SceneHierarchy: 이름 변경 취소");
	}

	// 상태 초기화
	RenamingActor = nullptr;
	RenameBuffer[0] = '\0';
}

/**
 * @brief 특정 카메라의 포커싱 애니메이션을 중단하는 함수
 * @param InCamera 애니메이션을 중단할 카메라
 */
void USceneHierarchyWidget::StopCameraAnimation(TObjectPtr<ACameraActor> InCamera)
{
	if (!InCamera)
	{
		return;
	}

	FName CameraName = InCamera->GetName();
	if (CameraAnimatingStates[CameraName])
	{
		CameraAnimatingStates[CameraName] = false;
		CameraAnimationTimes[CameraName] = 0.0f;
		UE_LOG("SceneHierarchy: 카메라 '%s' 애니메이션 중단", CameraName.ToString().c_str());
	}
}

/**
 * @brief 특정 카메라가 애니메이션 중인지 확인하는 함수
 * @param InCamera 확인할 카메라
 * @return 애니메이션 중이면 true
 */
bool USceneHierarchyWidget::IsCameraAnimating(TObjectPtr<ACameraActor> InCamera) const
{
	if (!InCamera)
	{
		return false;
	}

	FName CameraName = InCamera->GetName();
	auto It = CameraAnimatingStates.find(CameraName);
	return It != CameraAnimatingStates.end() && It->second;
}

/**
 * @brief 현재 어떤 카메라든 애니메이션 중인지 확인하는 함수
 * @return 하나라도 애니메이션 중이면 true
 */
bool USceneHierarchyWidget::IsAnyCameraAnimating() const
{
	for (const auto& Pair : CameraAnimatingStates)
	{
		if (Pair.second)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief 모든 카메라의 포커싱 애니메이션을 중단하는 함수
 */
void USceneHierarchyWidget::StopAllCameraAnimations()
{
	// 이미 비어있으면 아무것도 하지 않음
	if (CameraAnimatingStates.empty())
	{
		return;
	}

	bool bStoppedAny = false;
	for (auto& Pair : CameraAnimatingStates)
	{
		if (Pair.second)
		{
			Pair.second = false;
			bStoppedAny = true;
		}
	}

	if (bStoppedAny)
	{
		UE_LOG("SceneHierarchy: 모든 카메라 애니메이션 중단됨");
	}
}
