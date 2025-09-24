// 상수 버퍼 정의
cbuffer WorldMatrixBuffer : register(b0)
{
	row_major matrix WorldMatrix;
};

cbuffer ViewProjectionBuffer : register(b1)
{
	row_major float4x4 View; // View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection; // Projection Matrix Calculation of MVP Matrix
};

cbuffer FontDataBuffer : register(b2)
{
    float2 AtlasSize;
    float2 GlyphSize;
    float2 Padding;
};

// 입력 구조체
struct VSInput
{
	float3 position : POSITION;     // FVector (3 floats)
	float2 texCoord : TEXCOORD0;    // FVector2 (2 floats)
	uint charIndex : TEXCOORD1;     // uint32 문자 인덱스
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	uint charIndex : TEXCOORD1;
};

// Texture and Sampler
Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

// Vertex shader
PSInput mainVS(VSInput Input)
{
	PSInput Output;

	// 월드 좌표계로 변환
	float4 WorldPosition = mul(float4(Input.position, 1.0f), WorldMatrix);

	// 뷰-프로젝션 변환
	Output.position = mul(WorldPosition, View);
	Output.position = mul(Output.position, Projection);

	// FontRenderer에서 이미 정확한 UV 좌표를 계산해서 보내기 때문에 그대로 사용
	Output.texCoord = Input.texCoord;
	Output.charIndex = Input.charIndex;

	return Output;
}

// Pixel shader
float4 mainPS(PSInput Input) : SV_TARGET
{
	// 폰트 텍스처에서 색상 샘플링
	float4 AtlasColor = FontAtlas.Sample(FontSampler, Input.texCoord);

	// 임계값 0.5를 기준으로 0.0 또는 1.0으로 만듦
	float Alpha = step(0.5f, AtlasColor.r);

	// 알파 값이 0 또는 1이기 때문에 0에 가까운 경우 픽셀을 폐기
	if (Alpha < 0.1f)
		discard;

	// 텍스트 부분은 무조건 명확한 흰색으로 출력
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
