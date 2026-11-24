// Stand-in for the After Effects SDK headers, ONLY for tests/host_sim (compile + smoke test of the
// plugin glue without After Effects). Signatures follow the SDK as documented; the real SDK build is
// authoritative. Never put this folder on the include path of the real plugin.
#pragma once
#include <cstdint>
#include <cstring>
typedef int32_t A_long; typedef uint32_t A_u_long; typedef int16_t A_short; typedef uint16_t A_u_short;
typedef char A_char; typedef unsigned char A_u_char; typedef unsigned char A_Boolean; typedef int32_t A_Err;
typedef double A_FpLong; typedef float A_FpShort; typedef double PF_FpLong; typedef float PF_FpShort;
typedef int32_t PF_Fixed; typedef int32_t PF_Err; typedef unsigned char PF_Boolean; typedef int32_t int32;
typedef struct _PF_ProgData* PF_ProgPtr; typedef char** PF_Handle; typedef A_long PF_PathID;
typedef struct _PF_PathOutline* PF_PathOutlinePtr; typedef struct _PF_PathSegPrep* PF_PathSegPrepPtr;
#define TRUE 1
#define FALSE 0
#ifdef _WIN32
#define DllExport __declspec(dllexport)
#else
#define DllExport __attribute__((visibility("default")))
#endif
enum { A_Err_NONE = 0 };
enum { PF_Err_NONE = 0, PF_Err_OUT_OF_MEMORY = 4, PF_Err_INTERNAL_STRUCT_DAMAGED = 512, PF_Err_BAD_CALLBACK_PARAM = 516 };
typedef A_long PF_Cmd;
enum { PF_Cmd_ABOUT, PF_Cmd_GLOBAL_SETUP, PF_Cmd_PARAMS_SETUP, PF_Cmd_SMART_PRE_RENDER, PF_Cmd_SMART_RENDER,
       PF_Cmd_GPU_DEVICE_SETUP, PF_Cmd_GPU_DEVICE_SETDOWN, PF_Cmd_SMART_RENDER_GPU, PF_Cmd_USER_CHANGED_PARAM };
typedef A_long PF_ParamIndex;
typedef struct { PF_ParamIndex param_index; } PF_UserChangedParamExtra;
enum { PF_ParamFlag_SUPERVISE = 1 << 6 };
enum { PF_ChangeFlag_CHANGED_VALUE = 1 };
enum { PF_OutFlag_REFRESH_UI = 1 << 3, PF_OutFlag_DISPLAY_ERROR_MESSAGE = 1 << 13, PF_OutFlag_FORCE_RERENDER = 1 << 22 };
typedef unsigned short A_UTF16Char;
typedef struct { A_long value; A_u_long scale; } A_Time;
typedef struct { A_FpLong mat[4][4]; } A_Matrix4;
typedef struct { A_u_char alpha, red, green, blue; } PF_Pixel; typedef PF_Pixel PF_Pixel8;
typedef struct { A_u_short alpha, red, green, blue; } PF_Pixel16;
typedef struct { PF_FpShort alpha, red, green, blue; } PF_PixelFloat;
#define PF_MAX_CHAN8 255
#define PF_MAX_CHAN16 32768
typedef struct { void* data; A_long rowbytes; A_long width, height; } PF_LayerDef; typedef PF_LayerDef PF_EffectWorld;
typedef struct { A_long num; A_u_long den; } PF_RationalScale;
typedef struct { A_long left, top, right, bottom; } PF_LRect;
#define PF_PathID_NONE 0
enum { PF_Param_LAYER, PF_Param_BUTTON = 23, PF_Param_PATH = 12 };
typedef struct {
  union { struct { PF_FpLong value; } fs_d; struct { A_long value; } sd; struct { A_long value; } bd; struct { PF_Fixed value; } ad;
          struct { A_long value; } pd; struct { PF_Pixel value, dephault; } cd; struct { PF_PathID path_id; A_long reserved0; A_long dephault; } path_d; } u;
  A_long param_type; A_char name_do_not_use_directly[32]; union { A_long id; A_long change_flags; } uu; A_long flags;
} PF_ParamDef;
typedef struct SPBasicSuite { int32 (*AcquireSuite)(const char*, int32, const void**); int32 (*ReleaseSuite)(const char*, int32); } SPBasicSuite;
enum { kSPNoError = 0 };
typedef struct { PF_Err (*checkout_param)(PF_ProgPtr, A_long, A_long, A_long, A_u_long, PF_ParamDef*);
                 PF_Err (*checkin_param)(PF_ProgPtr, PF_ParamDef*); PF_Err (*add_param)(PF_ProgPtr, A_long, PF_ParamDef*); } PF_InteractCallbacks;
enum { PF_Quality_LO = 0, PF_Quality_HI = 1 };
typedef struct { PF_InteractCallbacks inter; PF_ProgPtr effect_ref; A_long quality; A_long current_time, time_step; A_u_long time_scale;
                 A_long width, height; PF_RationalScale downsample_x, downsample_y; SPBasicSuite* pica_basicP; } PF_InData;
typedef struct { A_u_long my_version; A_long out_flags, out_flags2; A_long num_params; A_char return_msg[256]; } PF_OutData;
#define PF_ADD_PARAM(IN, IDX, DEF) ((*(IN)->inter.add_param)((IN)->effect_ref, (IDX), (DEF)))
#define PF_CHECKOUT_PARAM(IN, I, T, S, SC, P) ((*(IN)->inter.checkout_param)((IN)->effect_ref, (I), (T), (S), (SC), (P)))
#define PF_CHECKIN_PARAM(IN, P) ((*(IN)->inter.checkin_param)((IN)->effect_ref, (P)))
#define AEFX_CLR_STRUCT(S) do { std::memset(&(S), 0, sizeof(S)); } while (0)
#define ERR(F) do { if (!err) { err = (F); } } while (0)
#define ERR2(F) do { if (((err2 = (F)) != A_Err_NONE) && !err) err = err2; } while (0)
enum { PF_Precision_INTEGER, PF_Precision_TENTHS, PF_Precision_HUNDREDTHS, PF_Precision_THOUSANDTHS };
enum { PF_ValueDisplayFlag_NONE = 0, PF_ValueDisplayFlag_PERCENT = 1 };
enum { PF_LayerDefault_NONE = 0, PF_LayerDefault_MYSELF = -1 };
#define PF_DEF_NAME name_do_not_use_directly
#define PF_STUB_ADD(NAME, ID) do { std::strncpy(def.PF_DEF_NAME, NAME, 31); def.uu.id = (ID); ERR(PF_ADD_PARAM(in_data, -1, &def)); } while (0)
#define PF_ADD_SLIDER(N, a, b, c, d, DF, ID) do { def.u.sd.value = DF; PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_FLOAT_SLIDERX(N, a, b, c, d, DF, P, DISP, FL, ID) do { def.u.fs_d.value = DF; (void)(P); (void)(DISP); def.flags = FL; PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_ANGLE(N, DF, ID) do { def.u.ad.value = (DF) * 65536; PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_CHECKBOX(N, NB, DF, FL, ID) do { def.u.bd.value = DF; def.flags = FL; (void)(NB); PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_CHECKBOXX(N, DF, FL, ID) do { def.u.bd.value = DF; def.flags = FL; PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_COLOR(N, R, G, B, ID) do { def.u.cd.value.red = R; def.u.cd.value.green = G; def.u.cd.value.blue = B; PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_LAYER(N, DF, ID) do { def.param_type = PF_Param_LAYER; (void)(DF); PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_POPUP(N, C, DF, STR, ID) do { def.u.pd.value = DF; (void)(C); (void)(STR); PF_STUB_ADD(N, ID); } while (0)
#define PF_ADD_TOPIC(N, ID) PF_STUB_ADD(N, ID)
#define PF_ADD_BUTTON(N, BN, PUI, FL, ID) do { AEFX_CLR_STRUCT(def); def.param_type = PF_Param_BUTTON; def.flags = (FL); (void)(BN); (void)(PUI); PF_STUB_ADD(N, ID); } while (0)
#define PF_END_TOPIC(ID) PF_STUB_ADD("", ID)
enum { PF_Stage_DEVELOP, PF_Stage_ALPHA, PF_Stage_BETA, PF_Stage_RELEASE };
#define PF_VERSION(V, S, B, ST, BLD) (((V) << 19) | ((S) << 15) | ((B) << 11) | ((ST) << 9) | (BLD))
enum { PF_OutFlag_NON_PARAM_VARY = 1 << 2, PF_OutFlag_DEEP_COLOR_AWARE = 1 << 25 };
enum { PF_OutFlag2_I_USE_3D_CAMERA = 1 << 1, PF_OutFlag2_I_USE_3D_LIGHTS = 1 << 2, PF_OutFlag2_SUPPORTS_SMART_RENDER = 1 << 10,
       PF_OutFlag2_FLOAT_COLOR_AWARE = 1 << 12, PF_OutFlag2_I_MIX_GUID_DEPENDENCIES = 1 << 21, PF_OutFlag2_SUPPORTS_GPU_RENDER_F32 = 1 << 25,
       PF_OutFlag2_SUPPORTS_THREADED_RENDERING = 1 << 27 };
typedef A_long PF_GPU_Framework;
enum { PF_GPU_Framework_NONE = 0, PF_GPU_Framework_OPENCL, PF_GPU_Framework_METAL, PF_GPU_Framework_CUDA, PF_GPU_Framework_DIRECTX };
typedef struct { A_long field; PF_LRect rect; A_long channel_mask; PF_Boolean preserve_rgb_of_zero_alpha; } PF_RenderRequest;
typedef struct { PF_LRect result_rect, max_result_rect; PF_RationalScale par; PF_Boolean solid; } PF_CheckoutResult;
enum { PF_RenderOutputFlag_RETURNS_EXTRA_PIXELS = 1, PF_RenderOutputFlag_GPU_RENDER_POSSIBLE = 2 };
typedef void (*PF_DeletePreRenderDataFunc)(void*);
typedef struct { PF_RenderRequest output_request; short bitdepth; const void* gpu_data; PF_GPU_Framework what_gpu; A_u_long device_index; } PF_PreRenderInput;
typedef struct { PF_LRect result_rect, max_result_rect; PF_Boolean solid; PF_Boolean reserved; short flags; void* pre_render_data; PF_DeletePreRenderDataFunc delete_pre_render_data_func; } PF_PreRenderOutput;
typedef struct { PF_Err (*checkout_layer)(PF_ProgPtr, A_long, A_long, const PF_RenderRequest*, A_long, A_long, A_u_long, PF_CheckoutResult*);
                 PF_Err (*GuidMixInPtr)(PF_ProgPtr, A_u_long, const void*); } PF_PreRenderCallbacks;
typedef struct { PF_PreRenderInput* input; PF_PreRenderOutput* output; PF_PreRenderCallbacks* cb; } PF_PreRenderExtra;
typedef struct { PF_RenderRequest output_request; short bitdepth; void* pre_render_data; const void* gpu_data; PF_GPU_Framework what_gpu; A_u_long device_index; } PF_SmartRenderInput;
typedef struct { PF_Err (*checkout_layer_pixels)(PF_ProgPtr, A_long, PF_EffectWorld**); PF_Err (*checkin_layer_pixels)(PF_ProgPtr, A_long); PF_Err (*checkout_output)(PF_ProgPtr, PF_EffectWorld**); } PF_SmartRenderCallbacks;
typedef struct { PF_SmartRenderInput* input; PF_SmartRenderCallbacks* cb; } PF_SmartRenderExtra;
typedef struct { PF_GPU_Framework device_framework; PF_Boolean compatibleB; void *platformPV, *devicePV, *contextPV, *command_queuePV, *offscreen_opengl_contextPV, *offscreen_opengl_devicePV; } PF_GPUDeviceInfo;
typedef struct { PF_GPU_Framework what_gpu; A_u_long device_index; } PF_GPUDeviceSetupInput;
typedef struct { void* gpu_data; } PF_GPUDeviceSetupOutput;
typedef struct { PF_GPUDeviceSetupInput* input; PF_GPUDeviceSetupOutput* output; } PF_GPUDeviceSetupExtra;
typedef struct { void* gpu_data; PF_GPU_Framework what_gpu; A_u_long device_index; } PF_GPUDeviceSetdownInput;
typedef struct { PF_GPUDeviceSetdownInput* input; } PF_GPUDeviceSetdownExtra;
#define kPFGPUDeviceSuite "PF GPU Device Suite"
#define kPFGPUDeviceSuiteVersion1 1
typedef struct { PF_Err (*GetDeviceCount)(PF_ProgPtr, A_u_long*); PF_Err (*GetDeviceInfo)(PF_ProgPtr, A_u_long, PF_GPUDeviceInfo*);
                 PF_Err (*GetGPUWorldData)(PF_ProgPtr, PF_EffectWorld*, void**); } PF_GPUDeviceSuite1;
#define kPFHandleSuite "PF Handle Suite"
#define kPFHandleSuiteVersion1 1
typedef struct { PF_Handle (*host_new_handle)(A_u_long); void* (*host_lock_handle)(PF_Handle); void (*host_unlock_handle)(PF_Handle); void (*host_dispose_handle)(PF_Handle); } PF_HandleSuite1;
#define kPFPathQuerySuite "PF Path Query Suite"
#define kPFPathQuerySuiteVersion1 1
typedef struct { PF_Err (*PF_CheckoutPath)(PF_ProgPtr, PF_PathID, A_long, A_long, A_u_long, PF_PathOutlinePtr*);
                 PF_Err (*PF_CheckinPath)(PF_ProgPtr, PF_PathID, PF_Boolean, PF_PathOutlinePtr); } PF_PathQuerySuite1;
#define kPFPathDataSuite "PF Path Data Suite"
#define kPFPathDataSuiteVersion1 1
typedef struct { PF_FpLong x, y; PF_FpLong tan_in_x, tan_in_y; PF_FpLong tan_out_x, tan_out_y; } PF_PathVertex;
typedef struct { PF_Err (*PF_PathIsOpen)(PF_ProgPtr, PF_PathOutlinePtr, PF_Boolean*); PF_Err (*PF_PathNumSegments)(PF_ProgPtr, PF_PathOutlinePtr, A_long*);
  PF_Err (*PF_PathVertexInfo)(PF_ProgPtr, PF_PathOutlinePtr, A_long, PF_PathVertex*);
  PF_Err (*PF_PathPrepareSegLength)(PF_ProgPtr, PF_PathOutlinePtr, A_long, A_long, PF_PathSegPrepPtr*);
  PF_Err (*PF_PathGetSegLength)(PF_ProgPtr, PF_PathOutlinePtr, A_long, PF_PathSegPrepPtr*, PF_FpLong*);
  PF_Err (*PF_PathEvalSegLength)(PF_ProgPtr, PF_PathOutlinePtr, PF_PathSegPrepPtr*, A_long, PF_FpLong, PF_FpLong*, PF_FpLong*);
  PF_Err (*PF_PathCleanupSegLength)(PF_ProgPtr, PF_PathOutlinePtr, A_long, PF_PathSegPrepPtr*); } PF_PathDataSuite1;
typedef struct _AEGP_Layer* AEGP_LayerH; typedef struct _AEGP_Comp* AEGP_CompH; typedef struct _AEGP_Stream* AEGP_StreamRefH; typedef A_long AEGP_PluginID;
typedef A_long AEGP_ObjectType; enum { AEGP_ObjectType_NONE = 3, AEGP_ObjectType_AV = 0, AEGP_ObjectType_LIGHT = 1, AEGP_ObjectType_CAMERA = 2 };
typedef A_long AEGP_LayerFlags; enum { AEGP_LayerFlag_VIDEO_ACTIVE = 1, AEGP_LayerFlag_LAYER_IS_3D = 1 << 9 };
typedef struct { A_long num; A_u_long den; } A_Ratio; typedef struct _AEGP_Item* AEGP_ItemH;
typedef struct _AEGP_Effect* AEGP_EffectRefH; typedef A_long AEGP_LayerIDVal; typedef void* AEGP_GlobalRefcon;
typedef struct { A_FpLong x, y, z; } AEGP_ThreeDVal;
typedef A_long AEGP_LightType; enum { AEGP_LightType_PARALLEL, AEGP_LightType_SPOT, AEGP_LightType_POINT, AEGP_LightType_AMBIENT };
typedef A_long AEGP_LayerStream; enum { AEGP_LayerStream_ANCHORPOINT = 1, AEGP_LayerStream_POSITION, AEGP_LayerStream_SCALE, AEGP_LayerStream_ROTATION,
  AEGP_LayerStream_ZOOM = 20, AEGP_LayerStream_INTENSITY, AEGP_LayerStream_COLOR, AEGP_LayerStream_CONE_ANGLE, AEGP_LayerStream_CONE_FEATHER };
typedef A_long AEGP_LTimeMode; enum { AEGP_LTimeMode_LayerTime, AEGP_LTimeMode_CompTime };
typedef struct { A_FpLong alphaF, redF, greenF, blueF; } AEGP_ColorVal;
typedef union { A_FpLong one_d; AEGP_ColorVal color; AEGP_ThreeDVal three_d; AEGP_LayerIDVal layer_id; } AEGP_StreamVal2;
typedef struct { AEGP_StreamRefH streamH; AEGP_StreamVal2 val; } AEGP_StreamValue2;
#define kAEGPPFInterfaceSuite "AEGP PF Interface Suite"
#define kAEGPPFInterfaceSuiteVersion1 1
typedef struct { A_Err (*AEGP_GetEffectLayer)(PF_ProgPtr, AEGP_LayerH*); A_Err (*AEGP_GetNewEffectForEffect)(AEGP_PluginID, PF_ProgPtr, AEGP_EffectRefH*);
                 A_Err (*AEGP_ConvertEffectToCompTime)(PF_ProgPtr, A_long, A_u_long, A_Time*);
                 A_Err (*AEGP_GetEffectCamera)(PF_ProgPtr, const A_Time*, AEGP_LayerH*); } AEGP_PFInterfaceSuite1;
#define kAEGPLayerSuite "AEGP Layer Suite"
#define kAEGPLayerSuiteVersion8 8
typedef struct { A_Err (*AEGP_GetCompNumLayers)(AEGP_CompH, A_long*); A_Err (*AEGP_GetCompLayerByIndex)(AEGP_CompH, A_long, AEGP_LayerH*);
  A_Err (*AEGP_GetLayerParentComp)(AEGP_LayerH, AEGP_CompH*); A_Err (*AEGP_GetLayerFlags)(AEGP_LayerH, AEGP_LayerFlags*);
  A_Err (*AEGP_GetLayerToWorldXform)(AEGP_LayerH, const A_Time*, A_Matrix4*); A_Err (*AEGP_GetLayerObjectType)(AEGP_LayerH, AEGP_ObjectType*);
  A_Err (*AEGP_SetLayerFlag)(AEGP_LayerH, AEGP_LayerFlags, A_Boolean); A_Err (*AEGP_GetLayerID)(AEGP_LayerH, AEGP_LayerIDVal*);
  A_Err (*AEGP_GetLayerFromLayerID)(AEGP_CompH, AEGP_LayerIDVal, AEGP_LayerH*);
  A_Err (*AEGP_GetLayerSourceItem)(AEGP_LayerH, AEGP_ItemH*); } AEGP_LayerSuite8;
#define kAEGPStreamSuite "AEGP Stream Suite"
#define kAEGPStreamSuiteVersion5 5
typedef struct { A_Err (*AEGP_GetNewLayerStream)(AEGP_PluginID, AEGP_LayerH, AEGP_LayerStream, AEGP_StreamRefH*); A_Err (*AEGP_DisposeStream)(AEGP_StreamRefH);
  A_Err (*AEGP_GetNewStreamValue)(AEGP_PluginID, AEGP_StreamRefH, AEGP_LTimeMode, const A_Time*, A_Boolean, AEGP_StreamValue2*);
  A_Err (*AEGP_DisposeStreamValue)(AEGP_StreamValue2*);
  A_Err (*AEGP_GetNewEffectStreamByIndex)(AEGP_PluginID, AEGP_EffectRefH, PF_ParamIndex, AEGP_StreamRefH*);
  A_Err (*AEGP_SetStreamValue)(AEGP_PluginID, AEGP_StreamRefH, AEGP_StreamValue2*); } AEGP_StreamSuite5;
#define kAEGPCompSuite "AEGP Comp Suite"
#define kAEGPCompSuiteVersion11 25
typedef struct { A_Err (*AEGP_CreateNullInComp)(const A_UTF16Char*, AEGP_CompH, const A_Time*, AEGP_LayerH*);
                 A_Err (*AEGP_GetItemFromComp)(AEGP_CompH, AEGP_ItemH*); } AEGP_CompSuite11;
#define kAEGPItemSuite "AEGP Item Suite"
#define kAEGPItemSuiteVersion9 14
typedef struct { A_Err (*AEGP_GetItemDimensions)(AEGP_ItemH, A_long*, A_long*); A_Err (*AEGP_GetItemPixelAspectRatio)(AEGP_ItemH, A_Ratio*); } AEGP_ItemSuite9;
#define kAEGPUtilitySuite "AEGP Utility Suite"
#define kAEGPUtilitySuiteVersion5 11
typedef struct { A_Err (*AEGP_StartUndoGroup)(const A_char*); A_Err (*AEGP_EndUndoGroup)(void);
                 A_Err (*AEGP_RegisterWithAEGP)(AEGP_GlobalRefcon, const A_char*, AEGP_PluginID*); } AEGP_UtilitySuite5;
#define kAEGPEffectSuite "AEGP Effect Suite"
#define kAEGPEffectSuiteVersion4 4
typedef struct { A_Err (*AEGP_DisposeEffect)(AEGP_EffectRefH); } AEGP_EffectSuite4;
#define kAEGPLightSuite "AEGP Light Suite"
#define kAEGPLightSuiteVersion2 2
typedef struct { A_Err (*AEGP_GetLightType)(AEGP_LayerH, AEGP_LightType*); } AEGP_LightSuite2;
typedef struct _PF_PluginData* PF_PluginDataPtr; typedef PF_Err (*PF_PluginDataCB2)(PF_PluginDataPtr, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
#define AE_RESERVED_INFO 8
enum { PF_Err_INVALID_CALLBACK = 517 };
#define PF_REGISTER_EFFECT_EXT2(P, CB, N, M, C, R, E, U) result = (CB)((P), (N), (M), (C), (E), "", "", (U))
