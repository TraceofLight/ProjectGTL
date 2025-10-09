cbuffer PerFrame : register(b0)
{
	row_major float4x4 Model;      // Model/World Matrix
	row_major float4x4 View;       // View Matrix
	row_major float4x4 Projection; // Projection Matrix
};

struct VS_INPUT
{
	float3 position : POSITION; // Input position from vertex buffer
	float4 color : COLOR;       // Input color from vertex buffer
};

struct PS_INPUT
{
	float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
	float4 color : COLOR;          // Color to pass to pixel shader
};

PS_INPUT VSMain(VS_INPUT input)
{
	PS_INPUT output;
	float4 tmp = float4(input.position, 1.0f);
	
	// Model is Identity for debug lines, so skip it
	// tmp = mul(tmp, Model);
	tmp = mul(tmp, View);
	tmp = mul(tmp, Projection);

	output.position = tmp;
	output.color = input.color;

	return output;
}
