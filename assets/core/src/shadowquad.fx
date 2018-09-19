
float4 shadowquad_vs(float3 position : POSITION) : SV_Position
{
   return float4(position.xy, 0.5, 1.0);

}

float4 shadowquad_ps(uniform float4 intensity : register(ps, c[0])) : SV_Target0
{
   return intensity.a;
}