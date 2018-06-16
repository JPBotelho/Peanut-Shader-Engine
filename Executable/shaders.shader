cbuffer CameraBuffer : register( b0 ) {
    float dTime;
	float time;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

VOut VShader(float4 position : POSITION, float4 color : COLOR)
{
    VOut output;

    output.position = position;
    output.color = color;
    //Position is in clip space.
    output.uv = (position.xy + 1) / 2;

    return output;
}


float3 PShader(VOut input) : SV_TARGET
{
    return (sin(time/1000)+ 1) / 2;
}
