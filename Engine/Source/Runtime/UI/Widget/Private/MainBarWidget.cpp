#include "pch.h"
#include "Runtime/UI/Widget/Public/MainBarWidget.h"

#include <shobjidl.h>

#include "Runtime/Engine/Public/Engine.h"
#include "Runtime/Subsystem/World/Public/WorldSubsystem.h"
#include "Runtime/UI/Window/Public/UIWindow.h"
#include "Runtime/Subsystem/UI/Public/UISubsystem.h"
#include "Runtime/Level/Public/Level.h"

IMPLEMENT_CLASS(UMainBarWidget, UWidget)

UMainBarWidget::UMainBarWidget()
	: UWidget("MainBarWidget")
{
}

/**
 * @brief MainBarWidget 초기화 함수
 * UIManager 인스턴스를 여기서 가져온다
 */
void UMainBarWidget::Initialize()
{
	auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
	if (!UISS)
	{
		UE_LOG("MainBarWidget: UIManager를 찾을 수 없습니다!");
		return;
	}

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	if (Viewport && Viewport->PlatformHandle)
	{
		MainWindowHandle = static_cast<HWND>(Viewport->PlatformHandle);
	}

	UE_LOG("MainBarWidget: 메인 메뉴바 위젯이 초기화되었습니다");
}

void UMainBarWidget::Update()
{
	// 업데이트 정보 필요할 경우 추가할 것
}

void UMainBarWidget::RenderWidget()
{
	if (!bIsMenuBarVisible)
	{
		MenuBarHeight = 0.0f;
		return;
	}

	// ImGui Context 유효성 검사
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (!ctx)
	{
		UE_LOG_ERROR("MainBarWidget: ImGui Context가 유효하지 않습니다");
		MenuBarHeight = 0.0f;
		return;
	}

	// ImGui Backend 초기화 상태 검사
	ImGuiIO& io = ImGui::GetIO();
	if (!io.BackendRendererUserData || !io.BackendPlatformUserData)
	{
		UE_LOG_ERROR("MainBarWidget: ImGui Backend가 초기화되지 않았습니다");
		MenuBarHeight = 0.0f;
		return;
	}

	// 메뉴바 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

	// 메인 메뉴바 시작
	if (ImGui::BeginMainMenuBar())
	{
		// 뷰포트 조정을 위해 메뉴바 높이 저장
		MenuBarHeight = ImGui::GetWindowSize().y;

		//UE_LOG("%f", MenuBarHeight);

		// 메뉴 Listing
		RenderFileMenu();
		// TODO(KHJ): Deprecated인 거 같은데 아직 안 쓴지 얼마 안된 관계로 주석 처리만 진행
		// RenderViewMenu();
		RenderShowFlagsMenu();
		RenderWindowsMenu();
		RenderHelpMenu();

		// 윈도우 컨트롤 버튼 (우측 정렬)
		RenderWindowControls();

		// 메인 메뉴바 종료
		ImGui::EndMainMenuBar();
	}
	else
	{
		MenuBarHeight = 0.0f;
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

/**
 * @brief File 메뉴를 렌더링합니다
 */
void UMainBarWidget::RenderFileMenu()
{
	if (ImGui::BeginMenu("파일"))
	{
		// 레벨 관련 메뉴
		if (ImGui::MenuItem("새 레벨", "Ctrl+N"))
		{
			CreateNewLevel();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("레벨 열기", "Ctrl+O"))
		{
			LoadLevel();
		}

		if (ImGui::MenuItem("레벨 저장", "Ctrl+S"))
		{
			SaveCurrentLevel();
		}

		ImGui::Separator();

		// 일반 파일 작업
		if (ImGui::MenuItem("일반 파일 열기"))
		{
			UE_LOG("MainBarWidget: 일반 파일 열기 메뉴 선택됨");
			// TODO(KHJ): 일반 파일 열기 로직 구현
		}

		ImGui::Separator();

		if (ImGui::MenuItem("종료", "Alt+F4"))
		{
			UE_LOG("MainBarWidget: 프로그램 종료 메뉴 선택됨");
			// TODO(KHJ): 프로그램 종료 로직 구현
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief Windows 토글 메뉴를 렌더링하는 함수
 * 등록된 MainMenu를 제외한 모든 UIWindow의 토글 옵션을 표시
 */
void UMainBarWidget::RenderWindowsMenu() const
{
	auto* UISS = GEngine->GetEngineSubsystem<UUISubsystem>();
	if (ImGui::BeginMenu("창"))
	{
		if (!UISS)
		{
			ImGui::Text("UIManager를 사용할 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// 모든 등록된 UIWindow에 대해 토글 메뉴 항목 생성
		const auto& AllWindows = UISS->GetAllUIWindows();

		if (AllWindows.IsEmpty())
		{
			ImGui::Text("등록된 창이 없습니다");
		}
		else
		{
			for (auto Window : AllWindows)
			{
				if (!Window)
				{
					continue;
				}

				if (Window->GetWindowTitle() == "MainMenuBar" || Window->GetWindowTitle() == "Console")
				{
					continue;
				}

				if (ImGui::MenuItem(Window->GetWindowTitle().ToString().data(), nullptr, Window->IsVisible()))
				{
					Window->ToggleVisibility();

					// Outliner나 Details 패널인 경우 레이아웃 재정리 요청
					const FString& Title = Window->GetWindowTitle().ToString();
					if (Title == "Outliner" || Title == "Details")
					{
						UISS->OnPanelVisibilityChanged();
					}

					UE_LOG("MainBarWidget: %s 창 토글됨 (현재 상태: %s)",
					       Window->GetWindowTitle().ToString().data(),
					       Window->IsVisible() ? "표시" : "숨김");
				}
			}

			ImGui::Separator();

			// 전체 창 제어 옵션
			if (ImGui::MenuItem("모든 창 표시"))
			{
				UISS->ShowAllWindows();
				UE_LOG("MainBarWidget: 모든 창 표시됨");
			}

			if (ImGui::MenuItem("모든 창 숨김"))
			{
				for (auto Window : UISS->GetAllUIWindows())
				{
					if (!Window)
					{
						continue;
					}

					if (Window->GetWindowTitle() == "MainMenuBar")
					{
						continue;
					}


					Window->SetWindowState(EUIWindowState::Hidden);
				}

				UE_LOG("MainBarWidget: 모든 창 숨겨짐");
			}
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief View 메뉴를 렌더링하는 함수
 * ViewMode 선택 기능 (Lit, Unlit, Wireframe)
 */
void UMainBarWidget::RenderViewMenu()
{
	if (ImGui::BeginMenu("보기"))
	{
		UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
		if (!WorldSS)
		{
			return;
		}

		UEditor* EditorInstance = WorldSS->GetEditor();
		if (!EditorInstance)
		{
			ImGui::Text("에디터를 사용할 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		EViewMode CurrentMode = EditorInstance->GetViewMode();

		// ViewMode 메뉴 아이템
		bool bIsLit = (CurrentMode == EViewMode::Lit);
		bool bIsUnlit = (CurrentMode == EViewMode::Unlit);
		bool bIsWireframe = (CurrentMode == EViewMode::WireFrame);

		if (ImGui::MenuItem("조명 적용(Lit)", nullptr, bIsLit) && !bIsLit)
		{
			EditorInstance->SetViewMode(EViewMode::Lit);
			UE_LOG("MainBarWidget: ViewMode를 Lit으로 변경");
		}

		if (ImGui::MenuItem("조명 비적용(Unlit)", nullptr, bIsUnlit) && !bIsUnlit)
		{
			EditorInstance->SetViewMode(EViewMode::Unlit);
			UE_LOG("MainBarWidget: ViewMode를 Unlit으로 변경");
		}

		if (ImGui::MenuItem("와이어프레임(Wireframe)", nullptr, bIsWireframe) && !bIsWireframe)
		{
			EditorInstance->SetViewMode(EViewMode::WireFrame);
			UE_LOG("MainBarWidget: ViewMode를 Wireframe으로 변경");
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief ShowFlags 메뉴를 렌더링하는 함수
 * Static Mesh, BillBoard 등의 플래그 상태 확인 및 토글
 */
void UMainBarWidget::RenderShowFlagsMenu()
{
	if (ImGui::BeginMenu("표시 옵션"))
	{
		UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
		if (!WorldSS)
		{
			return;
		}

		ULevel* CurrentLevel = WorldSS->GetCurrentLevel();
		if (!CurrentLevel)
		{
			ImGui::Text("현재 레벨을 찾을 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// ShowFlags 가져오기
		uint64 ShowFlags = CurrentLevel->GetShowFlags();

		// Primitives 표시 옵션
		bool bShowPrimitives = (ShowFlags & EEngineShowFlags::SF_Primitives) != 0;
		if (ImGui::MenuItem("프리미티브 표시", nullptr, bShowPrimitives))
		{
			if (bShowPrimitives)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}

		// BillBoard Text 표시 옵션
		bool bShowBillboardText = (ShowFlags & EEngineShowFlags::SF_BillboardText) != 0;
		if (ImGui::MenuItem("빌보드 표시", nullptr, bShowBillboardText))
		{
			if (bShowBillboardText)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}

		// Bounds 표시 옵션
		bool bShowBounds = (ShowFlags & EEngineShowFlags::SF_Bounds) != 0;
		if (ImGui::MenuItem("바운딩박스 표시", nullptr, bShowBounds))
		{
			if (bShowBounds)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief Help 메뉴에 대한 렌더링 함수
 */
void UMainBarWidget::RenderHelpMenu()
{
	if (ImGui::BeginMenu("도움말"))
	{
		if (ImGui::MenuItem("정보", "F1"))
		{
			UE_LOG("MainBarWidget: 정보 메뉴 선택됨");
			// TODO(KHJ): 정보 다이얼로그 표시
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief 레벨 저장 기능
 */
void UMainBarWidget::SaveCurrentLevel()
{
	path FilePath = OpenSaveFileDialog();
	if (!FilePath.empty())
	{
		UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
		if (!WorldSS)
		{
			return;
		}

		try
		{
			bool bSuccess = WorldSS->SaveCurrentLevel(FilePath.string());

			if (bSuccess)
			{
				UE_LOG("MainMenuBar: SceneIO: Level Saved Successfully");
			}
			else
			{
				UE_LOG("MainMenuBar: SceneIO: Failed To Save Level");
			}
		}
		catch (const exception& Exception)
		{
			FString StatusMessage = FString("Save Error: ") + Exception.what();
			UE_LOG("MainMenuBar: SceneIO: Save Error: %s", Exception.what());
		}
	}
}

/**
 * @brief 레벨 로드 기능
 */
void UMainBarWidget::LoadLevel()
{
	path FilePath = OpenLoadFileDialog();

	if (!FilePath.empty())
	{
		try
		{
			// 파일명에서 확장자를 제외하고 레벨 이름 추출
			FString LevelName = FilePath.stem().string();

			UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
			if (!WorldSS)
			{
				return;
			}

			bool bSuccess = WorldSS->LoadLevel(LevelName, FilePath.string());

			if (bSuccess)
			{
				UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 성공했습니다");
			}
			else
			{
				UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 실패했습니다");
			}
		}
		catch (const exception& Exception)
		{
			FString StatusMessage = FString("Load Error: ") + Exception.what();
			UE_LOG("SceneIO: Load Error: %s", Exception.what());
		}
	}
}

/**
 * @brief 새로운 레벨 생성 기능
 */
void UMainBarWidget::CreateNewLevel()
{
	UWorldSubsystem* WorldSS = GEngine->GetEngineSubsystem<UWorldSubsystem>();
	if (!WorldSS)
	{
		return;
	}

	WorldSS->CreateNewLevel();
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * PWSTR: WideStringPointer 클래스
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenSaveFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		IFileSaveDialog* FileSaveDialogPtr = nullptr;

		// 2. FileSaveDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
		                                IID_IFileSaveDialog, reinterpret_cast<void**>(&FileSaveDialogPtr));

		if (SUCCEEDED(ResultHandle))
		{
			// 3. 대화상자 옵션 설정
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.scene)", L"*.scene"},
				{L"All Files (*.*)", L"*.*"}
			};
			FileSaveDialogPtr->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);

			// 기본 필터를 "JSON Files" 로 설정
			FileSaveDialogPtr->SetFileTypeIndex(1);

			// 기본 확장자 설정
			FileSaveDialogPtr->SetDefaultExtension(L"json");

			// 대화상자 제목 설정
			FileSaveDialogPtr->SetTitle(L"Save Level File");

			// Set Flag
			DWORD DoubleWordFlags;
			FileSaveDialogPtr->GetOptions(&DoubleWordFlags);
			FileSaveDialogPtr->SetOptions(DoubleWordFlags | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);

			// Show Modal
			// 현재 활성 창을 부모로 가짐
			UE_LOG("SceneIO: Save Dialog Modal Opening...");
			ResultHandle = FileSaveDialogPtr->Show(GetActiveWindow());

			// 결과 처리
			// 사용자가 '저장' 을 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileSaveDialogPtr->GetResult(&ShellItemResult);
				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR FilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(FilePath);
						CoTaskMemFree(FilePath);
					}

					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소'를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 취소됨");
			}

			// Release FileSaveDialog
			FileSaveDialogPtr->Release();
		}

		// COM 라이브러리 해제
		CoUninitialize();
	}

	return ResultPath;
}

/**
 * @brief Windows API를 활용한 파일 로드 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenLoadFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		IFileOpenDialog* FileOpenDialog = nullptr;

		// FileOpenDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
		                                IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));

		if (SUCCEEDED(ResultHandle))
		{
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.scene)", L"*.scene"},
				{L"All Files (*.*)", L"*.*"}
			};

			FileOpenDialog->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);

			// 기본 필터를 "JSON Files" 로 설정
			FileOpenDialog->SetFileTypeIndex(1);

			// 대화상자 제목 설정
			FileOpenDialog->SetTitle(L"Load Level File");

			// Flag Setting
			DWORD DoubleWordFlags;
			FileOpenDialog->GetOptions(&DoubleWordFlags);
			FileOpenDialog->SetOptions(DoubleWordFlags | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

			// Open Modal
			UE_LOG("SceneIO: Load Dialog Modal Opening...");
			ResultHandle = FileOpenDialog->Show(GetActiveWindow()); // 현재 활성 창을 부모로

			// 결과 처리
			// 사용자가 '열기' 를 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileOpenDialog->GetResult(&ShellItemResult);

				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR ReturnFilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &ReturnFilePath);

					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(ReturnFilePath);
						CoTaskMemFree(ReturnFilePath);
					}

					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소' 를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 취소됨");
			}

			FileOpenDialog->Release();
		}

		// COM 라이브러리 해제
		CoUninitialize();
	}

	return ResultPath;
}

/**
 * @brief 윈도우 컨트롤 버튼 (최소화, 최대화, 닫기) 렌더링
 */
void UMainBarWidget::RenderWindowControls() const
{
	if (!MainWindowHandle)
	{
		return;
	}

	// 우측 정렬을 위해 여백 계산
	const float ButtonWidth = 46.0f;
	const float ButtonCount = 3.0f;
	const float Padding = 30.0f; // 우측 여백
	const float TotalWidth = ButtonWidth * ButtonCount + Padding;

	ImGui::SameLine(ImGui::GetWindowWidth() - TotalWidth);

	// 버튼 기본 스타일 설정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

	// 최소화 버튼
	if (ImGui::Button("-", ImVec2(ButtonWidth, 0)))
	{
		ShowWindow(MainWindowHandle, SW_MINIMIZE);
	}

	// 최대화/복원 버튼
	ImGui::SameLine();
	bool bIsMaximized = IsZoomed(MainWindowHandle);
	if (ImGui::Button("□", ImVec2(ButtonWidth, 0)))
	{
		ShowWindow(MainWindowHandle, bIsMaximized ? SW_RESTORE : SW_MAXIMIZE);
	}

	ImGui::PopStyleColor(3);

	// 닫기 버튼
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
	if (ImGui::Button("X", ImVec2(ButtonWidth, 0)))
	{
		PostMessageW(MainWindowHandle, WM_CLOSE, 0, 0);
	}
	ImGui::PopStyleColor(3);
}
