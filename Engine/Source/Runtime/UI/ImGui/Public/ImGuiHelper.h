#pragma once
#include "Runtime/Core/Public/Object.h"

/**
 * @brief ImGui 초기화/렌더링/해제를 담당하는 Helper 클래스
 * UIManager에서 사용하는 유틸리티 클래스
 */
class UImGuiHelper :
	public UObject
{
public:
	UImGuiHelper();
	~UImGuiHelper() override;

	void Initialize(HWND InWindowHandle);
	void Release();

	// Device lost 후 새 D3D11 디바이스로 백엔드 재바인딩
	void RebindDevice(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);

	void BeginFrame() const;
	void EndFrame() const;

	static LRESULT WndProcHandler(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

private:
	bool bIsInitialized = false;
};
