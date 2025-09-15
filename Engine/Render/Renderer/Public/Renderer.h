#pragma once
#include "DeviceResources.h"
#include "Core/Public/Object.h"
#include "Mesh/Public/SceneComponent.h"
#include "Editor/Public/EditorPrimitive.h"

class UPipeline;
class UDeviceResources;
class UPrimitiveComponent;
class AActor;
class AGizmo;
class UEditor;

/**
 * @brief Rendering Pipeline 전반을 처리하는 클래스
 *
 * Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑 체인(Swap Chain)을 관리하기 위한 포인터들
 * @param Device GPU와 통신하기 위한 Direct3D 장치
 * @param DeviceContext GPU 명령 실행을 담당하는 컨텍스트
 * @param SwapChain 프레임 버퍼를 교체하는 데 사용되는 스왑 체인
 *
 * // 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
 * @param FrameBuffer 화면 출력용 텍스처
 * @param FrameBufferRTV 텍스처를 렌더 타겟으로 사용하는 뷰
 * @param RasterizerState 래스터라이저 상태(컬링, 채우기 모드 등 정의)
 * @param ConstantBuffer 쉐이더에 데이터를 전달하기 위한 상수 버퍼
 *
 * @param ClearColor 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
 * @param ViewportInfo 렌더링 영역을 정의하는 뷰포트 정보
 *
 * @param DefaultVertexShader
 * @param DefaultPixelShader
 * @param DefaultInputLayout
 * @param Stride
 *
 * @param vertexBufferSphere
 * @param numVerticesSphere
 */
UCLASS()
class URenderer :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(URenderer, UObject)

public:
	void Init(HWND InWindowHandle);
	void Release();

	void CreateRasterizerState();
	void CreateDepthStencilState();
	void ReleaseRasterizerState();

	void ReleaseResource();

	void CreateDefaultShader();
	void ReleaseDefaultShader();
	void Update(UEditor* Editor);
	//void Update();
	void RenderBegin();
	void RenderLevel();
	void RenderLocalOBB(const UPrimitiveComponent* InPrimitive);
	void RenderWorldAABB(const UPrimitiveComponent* InPrimitive);
	void RenderEnd() const;
	void RenderPrimitive(FEditorPrimitive& InPrimitive, struct FRenderState& InRenderState);
	void RenderPrimitiveIndexed(FEditorPrimitive& InPrimitive, struct FRenderState& InRenderState, bool bUseBaseConstantBuffer, uint32 stride, uint32 indexBufferStride);

	void OnResize(uint32 Inwidth = 0, uint32 InHeight = 0);
	bool GetIsResizing() { return bIsResizing; }
	void SetIsResizing(bool isResizing) { bIsResizing = isResizing; }

	//Testing Func
	ID3D11Buffer* CreateVertexBuffer(FVertex* InVertices, uint32 InByteWidth) const;
	ID3D11Buffer* CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const;
	ID3D11Buffer* CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const;
	static void ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer);
	void CreateVertexShaderAndInputLayout(const wstring& filePath, const TArray<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDescs, ID3D11VertexShader** outVertexShader, ID3D11InputLayout** outInputLayout);
	void CreatePixelShader(const wstring& filePath, ID3D11PixelShader** pixelShader);

	void CreateConstantBuffer();
	void ReleaseConstantBuffer();
	void UpdateConstant(const UPrimitiveComponent* Primitive);
	void UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const;
	void UpdateConstant(const FViewProjConstants& InViewProjConstants) const;
	void UpdateConstant(const FMatrix& InMatrix) const;
	void UpdateConstant(const FVector4& Color) const;
	//void UpdateAndSetBatchLineConstant(const BatchLineContants& batchLineConstant) const;

	//template<typename T>
	//void UpdateConstantGeneralType(const T* constData, ) const
	//{
	//	if (constData)
	//	{
	//		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

	//		GetDeviceContext()->Map(, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
	//		// update constant buffer every frame
	//		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
	//		{
	//			*constants = FMatrix::GetModelMatrix(Primitive->GetRelativeLocation(), FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()), Primitive->GetRelativeScale3D());
	//		}
	//		GetDeviceContext()->Unmap(constData, 0);
	//	}
	//}

	bool UpdateVertexBuffer(ID3D11Buffer* vertexBuffer, const std::vector<FVector>& vertices);

	ID3D11Device* GetDevice() const { return DeviceResources->GetDevice(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceResources->GetDeviceContext(); }
	IDXGISwapChain* GetSwapChain() const { return DeviceResources->GetSwapChain(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return DeviceResources->GetRenderTargetView(); }
	UDeviceResources* GetDeviceResources() const { return DeviceResources; }

private:
	UPipeline* Pipeline = nullptr;
	UDeviceResources* DeviceResources = nullptr;
	TArray<UPrimitiveComponent*> PrimitiveComponents;

private:
	ID3D11DepthStencilState* DefaultDepthStencilState = nullptr;
	ID3D11DepthStencilState* DisabledDepthStencilState = nullptr;
	ID3D11Buffer* ConstantBufferModels = nullptr;
	ID3D11Buffer* ConstantBufferViewProj = nullptr;
	ID3D11Buffer* ConstantBufferColor = nullptr;
	ID3D11Buffer* ConstantBufferBatchLine = nullptr;

	FLOAT ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};

	ID3D11VertexShader* DefaultVertexShader = nullptr;
	ID3D11PixelShader* DefaultPixelShader = nullptr;
	ID3D11InputLayout* DefaultInputLayout = nullptr;
	uint32 Stride = 0;

private:
	struct FRasterKey
	{
		D3D11_FILL_MODE FillMode = {};
		D3D11_CULL_MODE CullMode = {};

		bool operator==(const FRasterKey& InKey) const
		{
			return FillMode == InKey.FillMode && CullMode == InKey.CullMode;
		}
	};

	struct FRasterKeyHasher
	{
		size_t operator()(const FRasterKey& InKey) const noexcept
		{
			auto Mix = [](size_t& H, size_t V)
			{
				H ^= V + 0x9e3779b97f4a7c15ULL + (H << 6) + (H << 2);
			};

			size_t H = 0;
			Mix(H, (size_t)InKey.FillMode);
			Mix(H, (size_t)InKey.CullMode);

			return H;
		}
	};

	TMap<FRasterKey, ID3D11RasterizerState*, FRasterKeyHasher> RasterCache;

	ID3D11RasterizerState* GetRasterizerState(const FRenderState& InRenderState);

	bool bIsResizing = false;

	///////////////////////////////////////////
	// 카메라 VP Matrix 값 전달 받는 용도
	// (차후 리팩터링이 필요합니다)
	FViewProjConstants ViewProjConstants;
	///////////////////////////////////////////
};
