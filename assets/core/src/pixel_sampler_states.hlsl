#ifndef PIXEL_SAMPLER_STATES
#define PIXEL_SAMPLER_STATES

SamplerState aniso_wrap_sampler : register(s0);
SamplerState linear_clamp_sampler : register(s1);
SamplerState linear_wrap_sampler : register(s2);
SamplerState linear_mirror_sampler : register(s3);
SamplerState text_sampler : register(s4);
SamplerState projtex_sampler : register(s5);

#endif