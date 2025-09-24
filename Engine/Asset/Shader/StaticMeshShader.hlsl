// StaticMesh 렌더링을 위한 셰이더
// 정점, 텍스처 좌표, 노멀을 처리하여 3D 모델을 렌더링

cbuffer PerObject : register(b0)
{
	row_major float4x4 World;        // 월드 변환 행렬
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;         // 뷰 행렬
	row_major float4x4 Projection;   // 투영 행렬
}

cbuffer Material : register(b2)
{
	float4 DiffuseColor;	// 디퓨즈 색상
	float4 MaterialUsage;	// x: 버텍스 컬러 사용 여부, y: 디퓨즈 텍스처 사용 여부, z: UV 스크롤 사용 여부, w: 델타 타임
}

// 텍스처 및 샘플러
Texture2D DiffuseTexture : register(t0);
SamplerState DefaultSampler : register(s0);

struct VS_INPUT
{
	float3 Position : POSITION;		// 정점 위치
	float4 Color : COLOR;
	float2 TexCoord : TEXCOORD0;	// 텍스처 좌표
	float3 Normal : NORMAL;			// 정점 노멀
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;  // 클립 공간 위치
	float4 Color : COLOR;
	float2 TexCoord : TEXCOORD0;    // 텍스처 좌표
};

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	// 정점을 월드 공간으로 변환
	float4 worldPos = mul(float4(input.Position, 1.0f), World);

	// 뷰 및 투영 변환
	float4 viewPos = mul(worldPos, View);
	output.Position = mul(viewPos, Projection);

	// 색상 전달
	output.Color = input.Color;

	float2 texCoord = input.TexCoord;
	const float bUseUVScroll = MaterialUsage.z;
	const float AccTime = MaterialUsage.w;
	if (bUseUVScroll > 0.5f)
	{
		// X축 방향으로 무한 스크롤링
		float scrollSpeed = 0.25f; // 스크롤 속도 조정
		texCoord.x += scrollSpeed * AccTime;

		// 1.0을 넘어가면 0.0으로 순환
		texCoord.x = frac(texCoord.x);
	}

	output.TexCoord = texCoord;

	return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	const float bUseVertexColor = MaterialUsage.x;
	const float bUseDiffuseTexture = MaterialUsage.y;

	float4 finalColor = DiffuseColor;

	if (bUseDiffuseTexture > 0.5f)
	{
		const float4 diffuseTexColor = DiffuseTexture.Sample(DefaultSampler, input.TexCoord);
		finalColor = diffuseTexColor;
	}
	
	if (bUseVertexColor > 0.5f)
	{
		finalColor = input.Color;
	}

	

	return finalColor;
}

