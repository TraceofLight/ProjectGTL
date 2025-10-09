struct PS_INPUT
{
	float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
	float4 color : COLOR;          // Color from vertex shader
};


float4 PSMain(PS_INPUT input) : SV_TARGET
{
	// 버텍스에서 전달된 색상 사용
	return input.color;
}
