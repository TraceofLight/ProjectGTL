#include "pch.h"
#include "Runtime/UI/Widget/Public/ToolbarWidget.h"

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/Level/Public/Level.h"
#include "Runtime/Actor/Public/Actor.h"
#include "Runtime/Actor/Public/StaticMeshActor.h"
#include "Runtime/Component/Public/StaticMeshComponent.h"
#include "Runtime/Subsystem/Viewport/Public/ViewportSubsystem.h"

IMPLEMENT_CLASS(UToolbarWidget, UWidget)

UToolbarWidget::UToolbarWidget()
	: UWidget("ToolbarWidget")
{
}

void UToolbarWidget::Initialize()
{
	UE_LOG("ToolbarWidget: 툴바 위젯이 초기화되었습니다");
}

void UToolbarWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UToolbarWidget::RenderWidget()
{
	if (!bIsToolbarVisible)
	{
		ToolbarHeight = 0.0f;
		return;
	}

	// 툴바 스타일 설정 - 위아래 여백 완전 제거
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

	// 메인 뷰포트 영역에 툴바를 직접 배치
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	float MenuBarHeight = GEngine->GetEngineSubsystem<UUISubsystem>()->GetMainMenuBarHeight();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + MenuBarHeight)); // 메뉴바 바로 아래
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 0)); // 너비는 전체, 높이는 자동

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground;

	if (ImGui::Begin("##Toolbar", nullptr, windowFlags))
	{
		if (ImGui::BeginMenuBar())
		{
			// MenuBar의 실제 높이만 저장
			ToolbarHeight = ImGui::GetFrameHeight();

			// 레벨 이름 표시
			RenderLevelName();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// PlaceActor 드롭다운 메뉴
			RenderPlaceActorMenu();

			ImGui::EndMenuBar();
		}

		ImGui::End();
	}
	else
	{
		ToolbarHeight = 0.0f;
	}

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(4);
}

void UToolbarWidget::RenderLevelName()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();

	if (CurrentLevel)
	{
		FString LevelName = CurrentLevel->GetName().ToString();
		ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%s", LevelName.data());
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Level None");
	}
}

void UToolbarWidget::RenderPlaceActorMenu()
{
	if (ImGui::BeginMenu("Place Actor"))
	{
		if (ImGui::MenuItem("Actor"))
		{
			SpawnActorAtViewportCenter();
		}

		// if (ImGui::MenuItem("Static Mesh Actor"))
		// {
		// 	SpawnStaticMeshActorAtViewportCenter();
		// }

		ImGui::EndMenu();
	}
}

void UToolbarWidget::SpawnActorAtViewportCenter()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ToolbarWidget: 현재 레벨이 없습니다");
		return;
	}

	// 뷰포트 서브시스템에서 마지막으로 사용된 뷰포트 인덱스 가져오기
	UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	int32 ViewportIndex = ViewportSS->GetViewportIndexUnderMouse();

	// 유효하지 않으면 기본값으로 설정
	ViewportIndex = max(ViewportIndex, 0);

	// 일단 원점 배치
	// TODO(KHJ): 카메라 화면 중앙
	FVector SpawnLocation(0.0f, 0.0f, 0.0f);

	// Actor 생성 (기본 SceneComponent만 가진 빈 액터)
	AActor* NewActor = CurrentLevel->SpawnActor<AActor>();
	if (NewActor)
	{
		NewActor->SetActorLocation(SpawnLocation);

		UE_LOG("ToolbarWidget: Actor가 생성되었습니다 - 위치: (%.2f, %.2f, %.2f)",
		       SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
	}
	else
	{
		UE_LOG("ToolbarWidget: Actor 생성에 실패했습니다");
	}
}

void UToolbarWidget::SpawnStaticMeshActorAtViewportCenter()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	ULevel* CurrentLevel = WorldSS->GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG("ToolbarWidget: 현재 레벨이 없습니다");
		return;
	}

	// 뷰포트 서브시스템에서 마지막으로 사용된 뷰포트 인덱스 가져오기
	UViewportSubsystem* ViewportSS = GEngine->GetEngineSubsystem<UViewportSubsystem>();
	int32 ViewportIndex = ViewportSS->GetViewportIndexUnderMouse();

	// 유효하지 않으면 기본값으로 설정
	ViewportIndex = max(ViewportIndex, 0);

	// 일단 원점 배치
	FVector SpawnLocation(0.0f, 0.0f, 0.0f);

	// StaticMeshActor 생성
	AStaticMeshActor* NewActor = CurrentLevel->SpawnActor<AStaticMeshActor>();
	if (NewActor)
	{
		NewActor->SetActorLocation(SpawnLocation);

		UE_LOG("ToolbarWidget: StaticMeshActor가 생성되었습니다 - 위치: (%.2f, %.2f, %.2f)",
		       SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
	}
	else
	{
		UE_LOG("ToolbarWidget: StaticMeshActor 생성에 실패했습니다");
	}
}
