#pragma once
#include "Runtime/Core/Public/Object.h"
#include "Source/Global/Enum.h"
#include <d3d11.h>

/**
 * @brief 언리얼 엔진 스타일의 셰이더 클래스
 * DirectX 11 셰이더들을 캡슐화하고 관리
 */
UCLASS()
class UShader : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UShader, UObject)

public:
	UShader();
	virtual ~UShader();

	// 셰이더 초기화
	bool Initialize(const FString& InFilePath, EVertexLayoutType InLayoutType);
	void Release();

	// 셰이더 리소스 접근
	ID3D11VertexShader* GetVertexShader() const { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return PixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return InputLayout; }

	// 셰이더 정보
	const FString& GetFilePath() const { return FilePath; }
	EVertexLayoutType GetVertexLayoutType() const { return LayoutType; }
	bool IsValid() const { return bIsValid; }

	// 셰이더 바인딩
	void Bind();
	void Unbind();

protected:
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	FString FilePath;
	EVertexLayoutType LayoutType = EVertexLayoutType::PositionColor;
	bool bIsValid = false;

private:
	// 셰이더 컴파일 헬퍼
	bool CompileVertexShader(const FString& InFilePath);
	bool CompilePixelShader(const FString& InFilePath);
	bool CreateInputLayout(EVertexLayoutType InLayoutType, ID3DBlob* VertexShaderBlob);
};