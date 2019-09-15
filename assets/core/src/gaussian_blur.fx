
cbuffer BlurConstants : register(b0)
{
   float2 blur_dir_size;
}

Texture2D<float3> src_texture;

SamplerState samp;

float3 gaussian_blur_199_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 50;
   const float weights[step_count] =
      {0.01880, 0.02500, 0.02482, 0.02456, 0.02420, 0.02375, 0.02322, 0.02261,
       0.02193, 0.02119, 0.02039, 0.01955, 0.01866, 0.01775, 0.01681, 0.01587,
       0.01491, 0.01396, 0.01302, 0.01209, 0.01119, 0.01031, 0.00946, 0.00865,
       0.00788, 0.00715, 0.00646, 0.00582, 0.00521, 0.00466, 0.00414, 0.00367,
       0.00324, 0.00285, 0.00249, 0.00217, 0.00189, 0.00163, 0.00141, 0.00121,
       0.00103, 0.00088, 0.00075, 0.00063, 0.00053, 0.00045, 0.00037, 0.00031,
       0.00026, 0.00021};
   const float offsets[step_count] =
      {0.66656,  2.49938,  4.49889,  6.49840,  8.49791,  10.49742, 12.49692,
       14.49643, 16.49594, 18.49545, 20.49496, 22.49446, 24.49397, 26.49348,
       28.49299, 30.49250, 32.49200, 34.49151, 36.49102, 38.49053, 40.49004,
       42.48954, 44.48905, 46.48856, 48.48807, 50.48758, 52.48709, 54.48660,
       56.48610, 58.48561, 60.48512, 62.48463, 64.48414, 66.48364, 68.48315,
       70.48266, 72.48217, 74.48168, 76.48119, 78.48070, 80.48020, 82.47971,
       84.47923, 86.47873, 88.47824, 90.47775, 92.47726, 94.47677, 96.47627,
       98.47578};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_151_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 38;
   const float weights[step_count] =
      {0.02482, 0.03293, 0.03253, 0.03192, 0.03111, 0.03012, 0.02895, 0.02764,
       0.02621, 0.02468, 0.02309, 0.02144, 0.01978, 0.01813, 0.01649, 0.01491,
       0.01338, 0.01193, 0.01056, 0.00929, 0.00811, 0.00703, 0.00606, 0.00518,
       0.00440, 0.00372, 0.00311, 0.00259, 0.00214, 0.00176, 0.00143, 0.00116,
       0.00093, 0.00075, 0.00059, 0.00047, 0.00036, 0.00028};
   const float offsets[step_count] = {0.66648,  2.49893,  4.49807,  6.49721,
                                      8.49635,  10.49550, 12.49464, 14.49378,
                                      16.49292, 18.49206, 20.49121, 22.49035,
                                      24.48949, 26.48863, 28.48778, 30.48692,
                                      32.48606, 34.48520, 36.48435, 38.48349,
                                      40.48263, 42.48178, 44.48092, 46.48006,
                                      48.47921, 50.47835, 52.47749, 54.47664,
                                      56.47578, 58.47493, 60.47407, 62.47321,
                                      64.47236, 66.47150, 68.47065, 70.46980,
                                      72.46894, 74.46809};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_99_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 25;
   const float weights[step_count] = {0.03796, 0.05003, 0.04864, 0.04654,
                                      0.04382, 0.04060, 0.03701, 0.03321,
                                      0.02932, 0.02547, 0.02178, 0.01832,
                                      0.01517, 0.01236, 0.00991, 0.00782,
                                      0.00607, 0.00464, 0.00349, 0.00258,
                                      0.00188, 0.00135, 0.00095, 0.00066,
                                      0.00045};
   const float offsets[step_count] = {0.66622,  2.49749,  4.49548,  6.49346,
                                      8.49145,  10.48944, 12.48743, 14.48542,
                                      16.48342, 18.48141, 20.47940, 22.47739,
                                      24.47539, 26.47338, 28.47138, 30.46937,
                                      32.46737, 34.46537, 36.46337, 38.46137,
                                      40.45937, 42.45737, 44.45538, 46.45338,
                                      48.45139};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_75_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 19;
   const float weights[step_count] = {0.05022, 0.06560, 0.06244, 0.05779,
                                      0.05200, 0.04549, 0.03869, 0.03199,
                                      0.02572, 0.02010, 0.01528, 0.01129,
                                      0.00811, 0.00566, 0.00384, 0.00254,
                                      0.00163, 0.00102, 0.00062};
   const float offsets[step_count] = {0.66588,  2.49559,  4.49207,  6.48854,
                                      8.48502,  10.48150, 12.47797, 14.47446,
                                      16.47094, 18.46743, 20.46392, 22.46041,
                                      24.45691, 26.45341, 28.44992, 30.44643,
                                      32.44295, 34.43947, 36.43600};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_51_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 13;
   const float weights[step_count] = {0.07410, 0.09446, 0.08482, 0.07161,
                                      0.05686, 0.04245, 0.02980, 0.01967,
                                      0.01221, 0.00713, 0.00391, 0.00202,
                                      0.00098};
   const float offsets[step_count] = {0.66495,  2.49035,  4.48263,  6.47493,
                                      8.46723,  10.45955, 12.45189, 14.44425,
                                      16.43664, 18.42906, 20.42151, 22.41399,
                                      24.40652};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_39_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 10;
   const float weights[step_count] = {0.09722, 0.11994, 0.09956, 0.07429,
                                      0.04984, 0.03006, 0.01630, 0.00794,
                                      0.00348, 0.00137};
   const float offsets[step_count] = {0.66368,  2.48326,  4.46990,  6.45657,
                                      8.44331,  10.43013, 12.41705, 14.40408,
                                      16.39125, 18.37856};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}

float3 gaussian_blur_23_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 result = 0.0;

   const int step_count = 6;
   const float weights[step_count] = {0.16501, 0.17507, 0.10112,
                                      0.04268, 0.01316, 0.00296};
   const float offsets[step_count] = {0.65772, 2.45017, 4.41096,
                                      6.37285, 8.33626, 10.30153};

   for (int i = 0; i < step_count; ++i) {
      const float2 texcoord_offset = offsets[i] * blur_dir_size;
      const float3 samples =
         src_texture.SampleLevel(samp, texcoords + texcoord_offset, 0) +
         src_texture.SampleLevel(samp, texcoords - texcoord_offset, 0);
      result += weights[i] * samples;
   }

   return result;
}
