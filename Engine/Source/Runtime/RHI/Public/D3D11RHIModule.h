#pragma once
#include "Runtime/Core/Public/ModuleManager.h"
#include "Runtime/Renderer/Public/EditorRenderResources.h"

/**
 * @brief DirectX 11 RHI 모듈
 * RHI 시스템 초기화 담당
 * GDynamicRHI 전역 변수를 이 모듈에서 초기화함
 */
class FD3D11RHIModule : public IModuleInterface
{
public:
	// IModuleInterface 구현
	void StartupModule() override;
	void ShutdownModule() override;
	bool IsGameModule() const override { return false; }

	// EditorRenderResources 접근자
	FEditorRenderResources* GetEditorResources() const { return EditorResources.Get(); }

	// 정적 접근자 (기존 FRendererModule::Get() 대체)
	static FD3D11RHIModule& GetInstance();


private:
	bool CreateD3D11DeviceAndSwapChain();
	static HWND GetWindowHandle();

	TUniquePtr<FEditorRenderResources> EditorResources;
};
