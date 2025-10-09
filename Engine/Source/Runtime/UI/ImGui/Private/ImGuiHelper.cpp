#include "pch.h"
#include "Runtime/UI/ImGui/Public/ImGuiHelper.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
#include "Runtime/RHI/Public/RHIDevice.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

UImGuiHelper::UImGuiHelper() = default;

UImGuiHelper::~UImGuiHelper() = default;

/**
 * @brief ImGui 초기화 함수
 */
void UImGuiHelper::Initialize(HWND InWindowHandle)
{
	if (bIsInitialized)
	{
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(InWindowHandle);

	ImGuiIO& IO = ImGui::GetIO();

	// imgui.ini 파일 생성 비활성화
	IO.IniFilename = nullptr;

	path FontFilePath = FPaths::GetContentPath() / "Font" / "Pretendard-Regular.otf";
	IO.Fonts->AddFontFromFileTTF(
		reinterpret_cast<char*>(FontFilePath.u8string().data()), 16.0f, nullptr, IO.Fonts->GetGlyphRangesKorean());

	if (GDynamicRHI->IsInitialized())
	{
		ImGui_ImplDX11_Init(GDynamicRHI->GetDevice(), GDynamicRHI->GetDeviceContext());
	}
	else
	{
		UE_LOG_ERROR("ImGuiHelper: RHIDevice가 초기화되지 않았습니다");
		bIsInitialized = false;
		return;
	}

	bIsInitialized = true;
}

/**
 * @brief ImGui 자원 해제 함수
 */
void UImGuiHelper::Release()
{
	if (!bIsInitialized)
	{
		return;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	bIsInitialized = false;
}

void UImGuiHelper::RebindDevice(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	// ImGui 컨텍스트와 Win32 플랫폼 백엔드는 유지하고, DX11 백엔드만 교체
	if (!bIsInitialized)
	{
		return; // 아직 Initialize 전이면 아무 것도 하지 않음
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplDX11_Init(InDevice, InDeviceContext);
}

/**
 * @brief ImGui 새 프레임 시작
 */
void UImGuiHelper::BeginFrame() const
{
	if (!bIsInitialized)
	{
		return;
	}

	// Get New Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

/**
 * @brief ImGui 렌더링 종료 및 출력
 */
void UImGuiHelper::EndFrame() const
{
	if (!bIsInitialized)
	{
		return;
	}

	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief WndProc Handler 래핑 함수
 * @return ImGui 자체 함수 반환
 */
LRESULT UImGuiHelper::WndProcHandler(HWND hWnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
