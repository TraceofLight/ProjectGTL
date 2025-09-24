#pragma once
#include "Utility/Public/JsonSerializer.h"

/// @brief 폰트 아틀라스를 사용한 텍스트 렌더링 클래스
/// DejaVu Sans Mono.png 512x512 아틀라스에서 16x16 픽셀 글자를 렌더링
class UFontRenderer
{
public:
    // 폰트 정점 구조체 - 위치, UV, 문자 인덱스 포함
    struct FFontVertex
    {
        FVector Position;        // 3D 월드 좌표
        FVector2 TexCoord;      // 쿼드 내 UV 좌표 (0~1)
        uint32 CharIndex;       // ASCII 문자 코드
    };

    // 폰트 데이터 상수 버퍼 구조체
    struct FFontConstantBuffer
    {
        FVector2 AtlasSize = FVector2(2048.0f, 2980.0f);
        FVector2 GlyphSize = FVector2(20.0f, 24.0f);
        FVector2 Padding = FVector2(0.0f, 0.0f);
    };

    bool Initialize();
    void Release();

    void RenderText(FString InText, const FMatrix& InWorldMatrix, const FViewProjConstants& InViewProjectionCostants,
                    float InCenterY = 0.0f, float InStartZ = -2.5f, float InCharWidth = 0.05f, float InCharHeight = 0.05f) const;

	// Special member function
	UFontRenderer();
	~UFontRenderer();

private:
    ID3D11VertexShader* FontVertexShader = nullptr;
    ID3D11PixelShader* FontPixelShader = nullptr;
    ID3D11InputLayout* FontInputLayout = nullptr;

    /// @brief 텍스처 리소스
    ID3D11ShaderResourceView* FontAtlasTexture = nullptr;
    ID3D11SamplerState* FontSampler = nullptr;

    ID3D11Buffer* FontVertexBuffer = nullptr;
    ID3D11Buffer* FontConstantBuffer = nullptr;

    uint32 VertexCount = 0;
    FFontConstantBuffer ConstantBufferData;

    TMap<uint32, CharacterMetric> FontMetrics = {};
    float AtlasWidth = 0;
    float AtlasHeight = 0;

	// bool CreateVertexBufferForText(FString InText, float InStartX, float InStartY, float InCharWidth, float InCharHeight);
	bool CreateShaders();
	bool LoadFontTexture();
	bool CreateSamplerState();
	bool CreateConstantBuffer();

	CharacterMetric GetCharacterMetric(uint32 InUnicode) const;
};
