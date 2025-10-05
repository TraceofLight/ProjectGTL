#pragma once
#include <d3d11.h>

class URHIDevice;
struct FVertex;
enum class EPrimitiveType : uint8;
enum class EShaderType : uint8;

/**
 * @brief 에디터 렌더링에 필요한 GPU 리소스 관리 클래스
 *
 * 기즈모, 그리드 등 에디터 전용 시각화 요소의 GPU 리소스를 담당
 * AssetSubsystem에서 분리된 렌더링 리소스 관리 책임을 Renderer로 이관
 */
class FEditorRenderResources
{
public:
	FEditorRenderResources() = default;
	~FEditorRenderResources() = default;

	/**
	 * @brief GPU 리소스 초기화 (RHI 리소스 생성)
	 * Renderer 초기화 시점에 호출
	 */
	void InitRHI(URHIDevice* InRHIDevice);

	/**
	 * @brief GPU 리소스 해제
	 * Renderer 종료 시점에 호출
	 */
	void ReleaseRHI(URHIDevice* InRHIDevice);

	// Vertex Buffer 접근자
	ID3D11Buffer* GetGizmoVertexBuffer(EPrimitiveType Type) const;
	uint32 GetGizmoVertexCount(EPrimitiveType Type) const;

	// Shader 접근자
	ID3D11VertexShader* GetEditorVertexShader(EShaderType Type) const;
	ID3D11PixelShader* GetEditorPixelShader(EShaderType Type) const;
	ID3D11InputLayout* GetEditorInputLayout(EShaderType Type) const;

private:
	// Gizmo용 Vertex Buffers
	TMap<EPrimitiveType, ID3D11Buffer*> GizmoVertexBuffers;
	TMap<EPrimitiveType, uint32> GizmoVertexCounts;

	// Editor Shaders
	TMap<EShaderType, ID3D11VertexShader*> EditorVertexShaders;
	TMap<EShaderType, ID3D11PixelShader*> EditorPixelShaders;
	TMap<EShaderType, ID3D11InputLayout*> EditorInputLayouts;

	bool bIsInitialized = false;

	// 초기화 헬퍼 함수들
	void InitializeGizmoBuffers(URHIDevice* InRHIDevice);
	void InitializeEditorShaders(URHIDevice* InRHIDevice);

	// 해제 헬퍼 함수들
	void ReleaseGizmoBuffers(URHIDevice* InRHIDevice);
	void ReleaseEditorShaders(URHIDevice* InRHIDevice);
};
