
#include "normal.hlsl"

float4 near_opaque_ps(Ps_input input) : COLOR
{  
   State state;
   state.projected_tex = false;
   state.shadowed = false;
   state.hardedged = false;

   return main_opaque_ps(input, state);
}

float4 near_opaque_hardedged_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = false;
   state.hardedged = true;

   return main_opaque_ps(input, state);
}

float4 near_opaque_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = false;
   state.hardedged = false;

   return main_opaque_ps(input, state);
}

float4 near_opaque_hardedged_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = false;
   state.hardedged = true;

   return main_opaque_ps(input, state);
}

float4 near_opaque_shadow_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = true;
   state.hardedged = false;

   return main_opaque_ps(input, state);
}

float4 near_opaque_hardedged_shadow_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = true;
   state.hardedged = true;

   return main_opaque_ps(input, state);
}

float4 near_opaque_shadow_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = true;
   state.hardedged = false;

   return main_opaque_ps(input, state);
}

float4 near_opaque_hardedged_shadow_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = true;
   state.hardedged = true;

   return main_opaque_ps(input, state);
}

float4 near_transparent_ps(Ps_input input) : COLOR
{  
   State state;
   state.projected_tex = false;
   state.shadowed = false;
   state.hardedged = false;

   return main_transparent_ps(input, state);
}

float4 near_transparent_hardedged_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = false;
   state.hardedged = true;

   return main_transparent_ps(input, state);
}

float4 near_transparent_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = false;
   state.hardedged = false;

   return main_transparent_ps(input, state);
}

float4 near_transparent_hardedged_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = false;
   state.hardedged = true;

   return main_transparent_ps(input, state);
}

float4 near_transparent_shadow_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = true;
   state.hardedged = false;

   return main_transparent_ps(input, state);
}

float4 near_transparent_hardedged_shadow_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = false;
   state.shadowed = true;
   state.hardedged = true;

   return main_transparent_ps(input, state);
}

float4 near_transparent_shadow_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = true;
   state.hardedged = false;

   return main_transparent_ps(input, state);
}

float4 near_transparent_hardedged_shadow_projectedtex_ps(Ps_input input) : COLOR
{
   State state;
   state.projected_tex = true;
   state.shadowed = true;
   state.hardedged = true;

   return main_transparent_ps(input, state);
}
