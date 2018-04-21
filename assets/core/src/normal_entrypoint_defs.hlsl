#ifndef NORMAL_ENTRYPOINT_DEFS
#define NORMAL_ENTRYPOINT_DEFS

struct Normal_state
{
   bool projected_tex;
   bool shadowed;
   bool hardedged;
};

#define DEFINE_NORMAL_QPAQUE_PS_ENTRYPOINTS(INPUT_TYPE, FUNC_NAME)            \
                                                                              \
float4 near_opaque_ps(INPUT_TYPE input) : COLOR                               \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = false;                                               \
   state.shadowed = false;                                                    \
   state.hardedged = false;                                                   \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_hardedged_ps(INPUT_TYPE input) : COLOR                     \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = false;                                               \
   state.shadowed = false;                                                    \
   state.hardedged = true;                                                    \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_projectedtex_ps(INPUT_TYPE input) : COLOR                  \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = true;                                                \
   state.shadowed = false;                                                    \
   state.hardedged = false;                                                   \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_hardedged_projectedtex_ps(INPUT_TYPE input) : COLOR        \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = true;                                                \
   state.shadowed = false;                                                    \
   state.hardedged = true;                                                    \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_shadow_ps(INPUT_TYPE input) : COLOR                        \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = false;                                               \
   state.shadowed = true;                                                     \
   state.hardedged = false;                                                   \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_hardedged_shadow_ps(INPUT_TYPE input) : COLOR              \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = false;                                               \
   state.shadowed = true;                                                     \
   state.hardedged = true;                                                    \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_shadow_projectedtex_ps(INPUT_TYPE input) : COLOR           \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = true;                                                \
   state.shadowed = true;                                                     \
   state.hardedged = false;                                                   \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}                                                                             \
                                                                              \
float4 near_opaque_hardedged_shadow_projectedtex_ps(INPUT_TYPE input) : COLOR \
{                                                                             \
   Normal_state state;                                                        \
   state.projected_tex = true;                                                \
   state.shadowed = true;                                                     \
   state.hardedged = true;                                                    \
                                                                              \
   return FUNC_NAME(input, state);                                            \
}

#define DEFINE_NORMAL_TRANSPARENT_PS_ENTRYPOINTS(INPUT_TYPE, FUNC_NAME)            \
float4 near_transparent_ps(INPUT_TYPE input) : COLOR                               \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = false;                                                    \
   state.shadowed = false;                                                         \
   state.hardedged = false;                                                        \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_hardedged_ps(INPUT_TYPE input) : COLOR                     \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = false;                                                    \
   state.shadowed = false;                                                         \
   state.hardedged = true;                                                         \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_projectedtex_ps(INPUT_TYPE input) : COLOR                  \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = true;                                                     \
   state.shadowed = false;                                                         \
   state.hardedged = false;                                                        \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_hardedged_projectedtex_ps(INPUT_TYPE input) : COLOR        \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = true;                                                     \
   state.shadowed = false;                                                         \
   state.hardedged = true;                                                         \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_shadow_ps(INPUT_TYPE input) : COLOR                        \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = false;                                                    \
   state.shadowed = true;                                                          \
   state.hardedged = false;                                                        \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_hardedged_shadow_ps(INPUT_TYPE input) : COLOR              \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = false;                                                    \
   state.shadowed = true;                                                          \
   state.hardedged = true;                                                         \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_shadow_projectedtex_ps(INPUT_TYPE input) : COLOR           \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = true;                                                     \
   state.shadowed = true;                                                          \
   state.hardedged = false;                                                        \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}                                                                                  \
                                                                                   \
float4 near_transparent_hardedged_shadow_projectedtex_ps(INPUT_TYPE input) : COLOR \
{                                                                                  \
   Normal_state state;                                                             \
   state.projected_tex = true;                                                     \
   state.shadowed = true;                                                          \
   state.hardedged = true;                                                         \
                                                                                   \
   return FUNC_NAME(input, state);                                                 \
}

#endif