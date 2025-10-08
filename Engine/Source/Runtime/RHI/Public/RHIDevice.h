#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Runtime/Core/Public/Containers/TMap.h"
#include "Source/Global/Enum.h"

struct FObjMaterialInfo;
struct FVertex;

/**
 * @brief RHI(Rendering Hardware Interface) Device 클래스
 *
 * AssetSubsystem과 Renderer 간의 중개자 역할
 * - AssetSubsystem의 Renderer 직접 의존성 제거
 * - 그래픽 리소스(버퍼, 셰이더, 텍스처) 생성/해제 책임만 담당
 * - DeviceResources에 대한 접근을 캡슐화
 */
UCLASS()
class URHIDevice :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(URHIDevice, UObject)

public:
	void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void Shutdown();

	// Device/Context/SwapChain 접근자
	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; }
	void SetSwapChain(IDXGISwapChain* InSwapChain) { SwapChain = InSwapChain; }
	bool IsInitialized() const { return bIsInitialized; }

	// Vertex Buffer 관리
	ID3D11Buffer* CreateVertexBuffer(const void* InData, uint32 InByteWidth, bool bCpuAccess = false);
	void ReleaseVertexBuffer(ID3D11Buffer* InBuffer);
	bool UpdateVertexBuffer(ID3D11Buffer* InBuffer, const void* InData, uint32 InByteWidth);

	// Index Buffer 관리
	ID3D11Buffer* CreateIndexBuffer(const void* InData, uint32 InByteWidth);
	void ReleaseIndexBuffer(ID3D11Buffer* InBuffer);

	// Shader 관리
	bool CreateVertexShaderAndInputLayout(
		const wstring& InFilePath,
		const TArray<D3D11_INPUT_ELEMENT_DESC>& InLayoutDesc,
		ID3D11VertexShader** OutVertexShader,
		ID3D11InputLayout** OutInputLayout);
	bool CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader);
	void ReleaseShader(IUnknown* InShader);

	// Texture 관리 (DirectXTK 래퍼)
	ID3D11ShaderResourceView* CreateTextureFromFile(const wstring& InFilePath);
	ID3D11ShaderResourceView* CreateTextureFromMemory(const void* InData, size_t InDataSize);
	void ReleaseTexture(ID3D11ShaderResourceView* InTexture);

	// 상태 설정 함수들
	void RSSetState(EViewMode ViewMode);
	void OmSetDepthStencilState(EComparisonFunc CompareFunction);
	void OmSetBlendState(bool bEnableBlending);

	// 상수 버퍼 업데이트
	void UpdateConstantBuffers(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix);
	void UpdateHighLightConstantBuffers(const uint32 InPicked, const FVector& InColor, const uint32 X, const uint32 Y, const uint32 Z, const uint32 Gizmo);
	void UpdateColorConstantBuffers(const FVector4& InColor);
	void UpdatePixelConstantBuffers(const FObjMaterialInfo& InMaterialInfo, bool bHasMaterial, bool bHasTexture);

	// 추가 블렌드 상태 함수
	void OMSetBlendState(bool bIsBlendMode);

	// Depth/Color Write 설정
	void OMSetDepthWriteEnabled(bool bEnabled);
	void OMSetColorWriteEnabled(bool bEnabled);

	// 샘플러 상태 설정
	void PSSetDefaultSampler(UINT StartSlot);

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	bool bIsInitialized = false;

	// 상태 객체들 캐시
	TMap<EViewMode, ID3D11RasterizerState*> RasterizerStates;
	TMap<EComparisonFunc, ID3D11DepthStencilState*> DepthStencilStates;
	ID3D11BlendState* BlendStateEnabled = nullptr;
	ID3D11BlendState* BlendStateDisabled = nullptr;

	// 상수 버퍼들
	ID3D11Buffer* ConstantBuffer = nullptr;
	ID3D11Buffer* HighLightCB = nullptr;
	ID3D11Buffer* ColorCB = nullptr;
	ID3D11Buffer* PixelConstCB = nullptr;

	// 샘플러 상태
	ID3D11SamplerState* DefaultSamplerState = nullptr;

	// Shader 컴파일 헬퍼
	bool CompileShaderFromFile(const wstring& InFilePath, const char* InEntryPoint,
	                           const char* InShaderModel, ID3DBlob** OutBlob);

	// 상태 객체 생성 헬퍼들
	void CreateRasterizerStates();
	void CreateDepthStencilStates();
	void CreateBlendStates();
	void CreateConstantBuffer();
	void CreateSamplerState();
};

extern URHIDevice* GDynamicRHI;
