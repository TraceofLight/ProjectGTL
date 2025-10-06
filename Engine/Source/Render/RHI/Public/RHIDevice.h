#pragma once
#include "Runtime/Core/Public/Object.h"

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

	// Device/Context 접근자
	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
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

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	bool bIsInitialized = false;

	// Shader 컴파일 헬퍼
	bool CompileShaderFromFile(const wstring& InFilePath, const char* InEntryPoint,
	                           const char* InShaderModel, ID3DBlob** OutBlob);
};
