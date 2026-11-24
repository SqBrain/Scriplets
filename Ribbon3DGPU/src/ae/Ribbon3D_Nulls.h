// Ribbon3D GPU - control nulls: "Create Nulls from Mask" button and reading the nulls at render time.
//
// Each null is an oriented spline point: its position is the point, its X axis the tangent
// direction, its Z axis the ribbon's up (so the ribbon width lies along the null's Y axis and
// rotating the null around X rolls the ribbon), its X scale the handle length (100 % = smooth,
// 0 % = sharp corner).
#pragma once

#include "Ribbon3D.h"
#include "core/RibbonGeometry.h"

// PF_Cmd_USER_CHANGED_PARAM for RB_MAKE_NULLS: creates 3D nulls at the mask vertices (one undo
// step), fills the "Point" layer slots and switches Path Source to Control Nulls.
PF_Err RibbonMakeNullsFromMask(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]);

// Oriented points from the layers in the "Point" slots at comp time (world space).
bool RibbonReadControlPoints(PF_InData* in_data, const A_Time& compTime, std::vector<rb::ControlPoint>& out);

// Spline through the layers in the "Point" slots at comp time. False when fewer than 2 are set.
bool RibbonReadControlNulls(PF_InData* in_data, const A_Time& compTime, bool closed, rb::Polyline& out);

// PreRender: mixes the control nulls' transforms into AE's cache key, so moving, rotating or
// scaling a null re-renders the effect (AE does not track other layers' transforms for us).
PF_Err RibbonMixInControlNulls(PF_InData* in_data, PF_PreRenderExtra* extra);

// Handle-length ratio and tangent direction of a mask vertex (exposed for tests).
struct RibbonMaskVertex {
    double x, y;              // position
    double inX, inY, outX, outY;  // handle vectors relative to the vertex
};
void RibbonVertexToNull(const std::vector<RibbonMaskVertex>& v, size_t i, bool closed, double& rotationDeg,
                        double& scalePercent);
