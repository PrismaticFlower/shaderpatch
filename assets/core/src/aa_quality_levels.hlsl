#define FXAA_PC 1
#define FXAA_HLSL_3 1

// Quality level names are intended to be user friendly and not 
// reflectively of if this is actually as fast or as slow as FXAA goes.
#ifdef AA_FASTEST
#define FXAA_QUALITY__PRESET 12
const static float fxaa_quality_subpix = 0.75;
const static float fxaa_quality_edge_threshold = 0.166;
const static float fxaa_quality_edge_threshold_min = 0.0833;
const static bool fxaa_enabled = true;
#elif defined(AA_FAST)
#define FXAA_QUALITY__PRESET 15
const static float fxaa_quality_subpix = 0.75;
const static float fxaa_quality_edge_threshold = 0.125;
const static float fxaa_quality_edge_threshold_min = 0.0833;
const static bool fxaa_enabled = true;
#elif defined(AA_SLOWER)
#define FXAA_QUALITY__PRESET 20
const static float fxaa_quality_subpix = 0.75;
const static float fxaa_quality_edge_threshold = 0.125;
const static float fxaa_quality_edge_threshold_min = 0.0833;
const static bool fxaa_enabled = true;
#elif defined(AA_SLOWEST)
#define FXAA_QUALITY__PRESET 29
const static float fxaa_quality_subpix = 0.75;
const static float fxaa_quality_edge_threshold = 0.125;
const static float fxaa_quality_edge_threshold_min = 0.0833;
const static bool fxaa_enabled = true;
#else
#define FXAA_QUALITY__PRESET 12
const static float fxaa_quality_subpix = 0.75;
const static float fxaa_quality_edge_threshold = 0.166;
const static float fxaa_quality_edge_threshold_min = 0.0833;
const static bool fxaa_enabled = false;
#endif