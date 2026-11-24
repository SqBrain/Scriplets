// Ribbon3D GPU - data shared by the host (C++) and the render kernel (C++ / OpenCL C / CUDA / Metal).
//
// Every struct holds only 32-bit ints and floats, no vector types, so the memory layout is
// identical in all four languages and buffers can be uploaded as raw bytes.
#ifndef RIBBON_SHARED_H
#define RIBBON_SHARED_H

#define RK_TILE 16          /* screen tile size for triangle binning, pixels */
#define RK_MAX_LIGHTS 16

#define RK_LIGHT_AMBIENT  0
#define RK_LIGHT_PARALLEL 1
#define RK_LIGHT_POINT    2
#define RK_LIGHT_SPOT     3

#define RK_COMPOSITE_OVER   0   /* ribbon over the layer */
#define RK_COMPOSITE_BEHIND 1   /* ribbon behind the layer */
#define RK_COMPOSITE_ALONE  2   /* ribbon only */

/* Projected triangle, ready for rasterization in buffer pixel space. */
typedef struct {
    float px[3], py[3];     /* vertex positions, buffer pixels */
    float iw[3];            /* 1 / camera-space depth per vertex (perspective-correct interpolation) */
    int   v[3];             /* indices into the vertex buffer */
    float ia;               /* 1 / signed doubled area */
    float minx, miny, maxx, maxy;
    float front;            /* 1: the front side faces the camera, 0: the back side */
    float pad0, pad1;
} RK_Tri;

/* Ribbon vertex in world space. */
typedef struct {
    float pos[3];
    float nrm[3];           /* unit normal of the FRONT side */
    float tan[3];           /* unit tangent along the ribbon (for the satin highlight) */
    float uv[2];            /* u along the ribbon 0..1, v across 0..1 */
    float pad;
} RK_Vertex;

typedef struct {
    int   type;             /* RK_LIGHT_* */
    float pos[3];           /* world position (point / spot) */
    float dir[3];           /* unit direction the light shines to (parallel / spot) */
    float color[3];         /* linear color * intensity */
    float cosOuter, cosInner;
    float pad0, pad1;
} RK_Light;

typedef struct {
    int   width, height;    /* output buffer, pixels */
    int   srcPitch, dstPitch, texPitch;   /* row pitch in pixels (4 floats per pixel, BGRA) */
    int   tilesX, tilesY;
    int   samples;          /* supersampling per axis: 1..4 */
    int   numLights;
    int   hasTexture, texWidth, texHeight;
    int   composite;        /* RK_COMPOSITE_* */
    int   srcX, srcY;       /* position of the source buffer inside the output buffer */
    int   srcW, srcH;       /* source buffer size (0 x 0: no source, transparent) */
    int   pad0;
    float camPos[3];
    float frontColor[3], backColor[3];    /* linear */
    float ambient, diffuse, specular, gloss;
    float satin, sheen, translucency, metal;
    float texRepeat, texOffset;
    float opacity;
    float gamma;            /* 1: linear working space, 2.2: gamma-encoded colors */
} RK_Params;

#endif
