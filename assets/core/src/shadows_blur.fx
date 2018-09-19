
#include "fullscreen_tri_vs.hlsl"
#include "pixel_utilities.hlsl"

Texture2D<float4> source_texture : register(ps_3_0, s4);
Texture2D<float2> buffer_texture : register(ps_3_0, s4);
Texture2D<float1> depth_texture : register(ps_3_0, s5);

SamplerState point_clamp_sampler;

static const uint SAMPLE_NUM = 32;

static const float2 POISSON_SAMPLES[SAMPLE_NUM] =
{
   float2(-0.2791822891137698f, 0.4062567546708375f),
   float2(-0.19995295555583473f, 0.7240894131651954f),
   float2(-0.3897554154525151f, 0.048752433671575515f),
   float2(-0.7941653846046869f, 0.5226389266597269f),
   float2(-0.7087398274068326f, 0.23699678345357753f),
   float2(-0.0005315934230399086f, 0.3030196331178548f),
   float2(-0.5960809749199095f, 0.7972953391738834f),
   float2(0.8648539007043492f, 0.4483595541939763f),
   float2(0.028584831515869054f, 0.9272382872682036f),
   float2(0.5277787255290286f, 0.24396028966174835f),
   float2(0.5871579716943437f, 0.5151228941064709f),
   float2(0.34554700147319906f, 0.9289079363110085f),
   float2(0.19350865400912212f, 0.5589927483532224f),
   float2(0.9880997081599587f, 0.005023901022158773f),
   float2(0.6072341504857947f, 0.7925756070526573f),
   float2(-0.19942705854068615f, -0.5344510598757078f),
   float2(-0.9429809317213993f, -0.00705991893637376f),
   float2(-0.26479654839094224f, -0.8934290673340078f),
   float2(-0.40667576792951926f, -0.3567416884013182f),
   float2(-0.9345727918104664f, -0.31846690517409726f),
   float2(-0.6764870070709892f, -0.2391133913513607f),
   float2(-0.6961889285559116f, -0.7118709870423396f),
   float2(0.8621173443188956f, -0.33994046124535626f),
   float2(0.7085030604646408f, -0.07884950018190454f),
   float2(0.3938922154477268f, -0.5926090392958616f),
   float2(0.11819781034701447f, -0.9111503913361952f),
   float2(0.7845337749306952f, -0.6173695031985156f),
   float2(0.08340128466639019f, -0.558717684474538f),
   float2(0.559570819915191f, -0.33204162605826915f),
   float2(0.4735060371441681f, -0.8748025691220752f),
   float2(0.4105952859714034f, -0.07989419329213943f),
   float2(0.19455459584340226f, -0.26937094700460246f),
};

float2 scale : register(ps, c[60]);

float4 blur_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float2 centre_sample = buffer_texture.SampleLevel(point_clamp_sampler, texcoords, 0);
   float centre_v = centre_sample.r;
   float centre_d = centre_sample.g;

   float v_total = centre_v;
   float w_total = 1.0;

   [unroll] for (uint i = 0; i < SAMPLE_NUM; i += 4) {
      float2 samples[4];

      samples[0] = buffer_texture.SampleLevel(point_clamp_sampler, texcoords + (POISSON_SAMPLES[i] * scale), 0);
      samples[1] = buffer_texture.SampleLevel(point_clamp_sampler, texcoords + (POISSON_SAMPLES[i + 1] * scale), 0);
      samples[2] = buffer_texture.SampleLevel(point_clamp_sampler, texcoords + (POISSON_SAMPLES[i + 2] * scale), 0);
      samples[3] = buffer_texture.SampleLevel(point_clamp_sampler, texcoords + (POISSON_SAMPLES[i + 3] * scale), 0);

      float4 v;
      float4 d;
      
      [unroll] for (uint s = 0; s < 4; ++s) {
         v[s] = samples[s].r;
         d[s] = samples[s].g;
      }

      float4 w = saturate(1.0 - abs(centre_d - d));

      v_total += dot(v, w);
      w_total += (w.x + w.y + w.z + w.w);
   }
   
   return v_total / w_total;
}

float4 combine_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float v = source_texture.SampleLevel(point_clamp_sampler, texcoords, 0).a;
   float d = depth_texture.SampleLevel(point_clamp_sampler, texcoords, 0);

   const static float near_plane = 0.5;
   const static float far_plane = 10000.0;

   const float proj_a = far_plane / (far_plane - near_plane);
   const float proj_b = (-far_plane * near_plane) / (far_plane - near_plane);

   d = proj_b / (d - proj_a);

   return float4(v, d, v, d);
}