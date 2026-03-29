#pragma once

#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif

#include <memory>
#include <vector>

#include "nanovg_context.hpp"

#if defined(NANOVG_NO_FAST_SQRT)
#undef NANOVG_FAST_SQRT
#elif !defined(NANOVG_FAST_SQRT)
#define NANOVG_FAST_SQRT
#endif

#ifndef NVG_INIT_FONTIMAGE_SIZE
#define NVG_INIT_FONTIMAGE_SIZE  512
#define NVG_MAX_FONTIMAGE_SIZE   2048
#define NVG_MAX_FONTIMAGES       4
#endif

#ifndef NVG_INIT_COMMANDS_SIZE
#define NVG_INIT_COMMANDS_SIZE 256
#endif
#ifndef NVG_INIT_POINTS_SIZE
#define NVG_INIT_POINTS_SIZE 128
#endif
#ifndef NVG_INIT_PATHS_SIZE
#define NVG_INIT_PATHS_SIZE 16
#endif
#ifndef NVG_INIT_VERTS_SIZE
#define NVG_INIT_VERTS_SIZE 256
#endif
#ifndef NVG_MAX_STATES
#define NVG_MAX_STATES 32
#endif
#ifndef NVG_KAPPA90
#define NVG_KAPPA90 0.5522847493f
#endif
#ifndef NVG_COUNTOF
#define NVG_COUNTOF(arr) (sizeof(arr) / sizeof(0[arr]))
#endif

namespace nvg {

namespace detail {

float sqrtf(float a);
float modf(float a, float b);
float sinf(float a);
float cosf(float a);
float tanf(float a);
float atan2f(float a, float b);
float acosf(float a);
int mini(int a, int b);
int maxi(int a, int b);
int clampi(int a, int mn, int mx);
float minf(float a, float b);
float maxf(float a, float b);
float absf(float a);
float signf(float a);
float clampf(float a, float mn, float mx);
float cross(float dx0, float dy0, float dx1, float dy1);
float rsqrtf(float x);
float normalize(float& x, float& y);

std::shared_ptr<PathCache> allocPathCache();
CompositeOperationState compositeOperationState(int op);
float hue(float h, float m1, float m2);
void setPaintColor(Paint& p, const Color& color);
void isectRects(float* dst,
	float ax, float ay, float aw, float ah,
	float bx, float by, float bw, float bh);
int ptEquals(float x1, float y1, float x2, float y2, float tol);
float distPtSeg(float x, float y, float px, float py, float qx, float qy);

float getAverageScale(const float* t);
float triarea2(float ax, float ay, float bx, float by, float cx, float cy);
float polyArea(Point* pts, int npts);
void polyReverse(Point* pts, int npts);
void pushVertex(std::vector<Vertex>& vs, float x, float y, float u, float v, float s, float t);
int curveDivs(float r, float arc, float tol);
void chooseBevel(int bevel, Point* p0, Point* p1, float w,
	float* x0, float* y0, float* x1, float* y1);
void roundJoin(std::vector<Vertex>& out, Point* p0, Point* p1,
	float lw, float rw, float lu, float ru, int ncap,
	float fringe, float t);
void bevelJoin(std::vector<Vertex>& out, Point* p0, Point* p1,
	float lw, float rw, float lu, float ru, float fringe, float t);
void insertSpacer(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, float u0, float u1, float t);
void buttCapStart(std::vector<Vertex>& out, Point* p,
	float dx, float dy, float w, float d,
	float aa, float u0, float u1, float t, int dir);
void buttCapEnd(std::vector<Vertex>& out, Point* p,
	float dx, float dy, float w, float d,
	float aa, float u0, float u1, float t, int dir);
void roundCapStart(std::vector<Vertex>& out, Point* p,
	float dx, float dy, float w, int ncap,
	float aa, float u0, float u1, float t, int dir);
void roundCapEnd(std::vector<Vertex>& out, Point* p,
	float dx, float dy, float w, int ncap,
	float aa, float u0, float u1, float t, int dir);
float quantize(float a, float d);
float getFontScale(const State* state);
int isTransformFlipped(const float* xform);

} // namespace detail

} // namespace nvg
