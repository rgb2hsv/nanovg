//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "nanovg.hpp"


#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <vector>
#include <algorithm>
#include <array>
#include <memory>
#include <exception>
#include <stdexcept>
#include <ranges>
#include <span>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define Expects(condition) \
	do { \
		if (!(condition)) { \
			printf("Precondition failed: %s\n  at %s:%d in %s()\n", \
				#condition, __FILE__, __LINE__, __func__); \
			std::terminate(); \
		} \
	} while (0)
	

#define NANOVG_FAST_SQRT
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#ifndef NVG_NO_STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4204)  // nonstandard extension used : non-constant aggregate initializer
#pragma warning(disable: 4706)  // assignment within conditional expression
#endif

#define NVG_INIT_FONTIMAGE_SIZE  512
#define NVG_MAX_FONTIMAGE_SIZE   2048
#define NVG_MAX_FONTIMAGES       4

#define NVG_INIT_COMMANDS_SIZE 256
#define NVG_INIT_POINTS_SIZE 128
#define NVG_INIT_PATHS_SIZE 16
#define NVG_INIT_VERTS_SIZE 256

#ifndef NVG_MAX_STATES
#define NVG_MAX_STATES 32
#endif

#define NVG_KAPPA90 0.5522847493f	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define NVG_COUNTOF(arr) (sizeof(arr) / sizeof(0[arr]))

namespace nvg {

enum class PathCommand : int {
	MoveTo = 0,
	LineTo = 1,
	BezierTo = 2,
	Close = 3,
	Winding = 4,
};

enum class PointFlags : unsigned char {
	Corner = 0x01,
	Left = 0x02,
	Bevel = 0x04,
	InnerBevel = 0x08,
};

inline constexpr unsigned char operator&(unsigned char a, PointFlags b) noexcept {
	return a & static_cast<unsigned char>(b);
}

inline constexpr unsigned char operator|(unsigned char a, PointFlags b) noexcept {
	return a | static_cast<unsigned char>(b);
}

inline constexpr unsigned char& operator|=(unsigned char& a, PointFlags b) noexcept {
	a |= static_cast<unsigned char>(b);
	return a;
}

inline constexpr PointFlags operator|(PointFlags a, PointFlags b) noexcept {
	return static_cast<PointFlags>(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

struct State {
	CompositeOperationState compositeOperation;
	int shapeAntiAlias;
	Paint fill;
	Paint stroke;
	float strokeWidth;
	float miterLimit;
	int lineJoin;
	int lineCap;
	int lineStyle;
	float alpha;
	std::array<float, 6> xform;
	Scissor scissor;
	float fontSize;
	float letterSpacing;
	float lineHeight;
	float fontBlur;
	float fontDilate;
	int textAlign;
	int fontId;
};

struct Point {
	float x,y;
	float dx, dy;
	float len;
	float dmx, dmy;
	unsigned char flags;
};

struct PathCache {
	std::vector<Point> points;
	std::vector<Path> paths;
	std::vector<Vertex> verts;
	std::array<float, 4> bounds;
};

struct Context {
	explicit Context(const Params& params);
	~Context();

	Params params;
	std::vector<float> commands;
	float commandx, commandy;
	float tessTol;
	float distTol;
	float fringeWidth;
	float devicePxRatio;
	struct FONScontext* fs=nullptr;
	std::array<int, NVG_MAX_FONTIMAGES> fontImages;
	size_t fontImageIdx;
	size_t drawCallCount;
	size_t fillTriCount;
	size_t strokeTriCount;
	size_t textTriCount;
	ScissorBounds scissor;
	std::vector<State> states;
	std::shared_ptr<PathCache> cache;
};

Context* createInternal(Params& params){
	return new Context(params);
}

void deleteInternal(Context* ctx){
	delete ctx;
}

namespace detail {

static float sqrtf(float a) { return std::sqrtf(a); } //TODO: change to use fast sqrt approximation
static float modf(float a, float b) { return std::fmodf(a, b); }
static float sinf(float a) { return std::sinf(a); }
static float cosf(float a) { return std::cosf(a); }
static float tanf(float a) { return std::tanf(a); }
static float atan2f(float a,float b) { return std::atan2f(a, b); }
static float acosf(float a) { return std::acosf(a); }
static int mini(int a, int b) { return a < b ? a : b; }
static int maxi(int a, int b) { return a > b ? a : b; }
static int clampi(int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float minf(float a, float b) { return a < b ? a : b; }
static float maxf(float a, float b) { return a > b ? a : b; }
static float absf(float a) { return a >= 0.0f ? a : -a; }
static float signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static float clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float cross(float dx0, float dy0, float dx1, float dy1) { return dx1*dy0 - dx0*dy1; }
static float rsqrtf(float x) {
	float xhalf = 0.5f * x;
    uint32_t i = std::bit_cast<uint32_t>(x);
    // The magic constant
    i = 0x5f3759df - (i >> 1);    
    float y = std::bit_cast<float>(i);
    // One iteration of Newton's Method
    y = y * (1.5f - (xhalf * y * y));
	return y;
}

#if defined(NANOVG_FAST_SQRT)
static float normalize(float& x, float& y)
{
	float id = rsqrtf(x*x + y*y);
	x *= id;
	y *= id;
	if(id<=std::numeric_limits<float>::epsilon())
		return std::numeric_limits<float>::max();

	return 1.0f/id;
}
#else
static float normalize(float& x, float& y)
{
	float d = sqrtf(x*x + y*y);
	if(d>std::numeric_limits<float>::epsilon()) {
		d = 1.0f/d;
		x *= d;
		y *= d;
	}
	return d;
}
#endif

static std::shared_ptr<PathCache> allocPathCache(void)
{
	auto c = std::make_shared<PathCache>();

	c->points.reserve(static_cast<size_t>(NVG_INIT_POINTS_SIZE));

	c->paths.reserve(static_cast<size_t>(NVG_INIT_PATHS_SIZE));

	c->verts.reserve(static_cast<size_t>(NVG_INIT_VERTS_SIZE));

	return c;
}
static void setDevicePixelRatio(Context& ctx, float ratio)
{
	ctx.tessTol = 0.25f / ratio;
	ctx.distTol = 0.01f / ratio;
	ctx.fringeWidth = 1.0f / ratio;
	ctx.devicePxRatio = ratio;
}
static CompositeOperationState compositeOperationState(int op)
{
	int sfactor, dfactor;

	if (op == static_cast<int>(CompositeOperation::SourceOver))
	{
		sfactor = static_cast<int>(BlendFactor::One);
		dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
	}
	else if (op == static_cast<int>(CompositeOperation::SourceIn))
	{
		sfactor = static_cast<int>(BlendFactor::DstAlpha);
		dfactor = static_cast<int>(BlendFactor::Zero);
	}
	else if (op == static_cast<int>(CompositeOperation::SourceOut))
	{
		sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
		dfactor = static_cast<int>(BlendFactor::Zero);
	}
	else if (op == static_cast<int>(CompositeOperation::Atop))
	{
		sfactor = static_cast<int>(BlendFactor::DstAlpha);
		dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
	}
	else if (op == static_cast<int>(CompositeOperation::DestinationOver))
	{
		sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
		dfactor = static_cast<int>(BlendFactor::One);
	}
	else if (op == static_cast<int>(CompositeOperation::DestinationIn))
	{
		sfactor = static_cast<int>(BlendFactor::Zero);
		dfactor = static_cast<int>(BlendFactor::SrcAlpha);
	}
	else if (op == static_cast<int>(CompositeOperation::DestinationOut))
	{
		sfactor = static_cast<int>(BlendFactor::Zero);
		dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
	}
	else if (op == static_cast<int>(CompositeOperation::DestinationAtop))
	{
		sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
		dfactor = static_cast<int>(BlendFactor::SrcAlpha);
	}
	else if (op == static_cast<int>(CompositeOperation::Lighter))
	{
		sfactor = static_cast<int>(BlendFactor::One);
		dfactor = static_cast<int>(BlendFactor::One);
	}
	else if (op == static_cast<int>(CompositeOperation::Copy))
	{
		sfactor = static_cast<int>(BlendFactor::One);
		dfactor = static_cast<int>(BlendFactor::Zero);
	}
	else if (op == static_cast<int>(CompositeOperation::Xor))
	{
		sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
		dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
	}
	else
	{
		sfactor = static_cast<int>(BlendFactor::One);
		dfactor = static_cast<int>(BlendFactor::Zero);
	}

	CompositeOperationState state;
	state.srcRGB = sfactor;
	state.dstRGB = dfactor;
	state.srcAlpha = sfactor;
	state.dstAlpha = dfactor;
	return state;
}
static State& getState(Context& ctx)
{
	return ctx.states.back();
}
static float hue(float h, float m1, float m2)
{
	if (h < 0) h += 1;
	if (h > 1) h -= 1;
	if (h < 1.0f/6.0f)
		return m1 + (m2 - m1) * h * 6.0f;
	else if (h < 3.0f/6.0f)
		return m2;
	else if (h < 4.0f/6.0f)
		return m1 + (m2 - m1) * (2.0f/3.0f - h) * 6.0f;
	return m1;
}
static void setPaintColor(Paint* p, Color color)
{
	memset(p, 0, sizeof(*p));
	transformIdentity(p->xform);
	p->radius = 0.0f;
	p->feather = 1.0f;
	p->innerColor = color;
	p->outerColor = color;
}
static void isectRects(float* dst,
							float ax, float ay, float aw, float ah,
							float bx, float by, float bw, float bh)
{
	float minx = maxf(ax, bx);
	float miny = maxf(ay, by);
	float maxx = minf(ax+aw, bx+bw);
	float maxy = minf(ay+ah, by+bh);
	dst[0] = minx;
	dst[1] = miny;
	dst[2] = maxf(0.0f, maxx - minx);
	dst[3] = maxf(0.0f, maxy - miny);
}
static int ptEquals(float x1, float y1, float x2, float y2, float tol)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return dx*dx + dy*dy < tol*tol;
}
static float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
	float pqx, pqy, dx, dy, d, t;
	pqx = qx-px;
	pqy = qy-py;
	dx = x-px;
	dy = y-py;
	d = pqx*pqx + pqy*pqy;
	t = pqx*dx + pqy*dy;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	dx = px + t*pqx - x;
	dy = py + t*pqy - y;
	return dx*dx + dy*dy;
}
template<class C>
requires std::ranges::range<C> && std::same_as<std::ranges::range_value_t<C>, float> 
static void appendCommands(Context& ctx, C& vals)
{
	auto& state = getState(ctx);
	auto nvals = std::ranges::size(vals);
	Expects(nvals>0);
	if (auto front=(int)vals.front(); front != static_cast<int>(PathCommand::Close) && front != static_cast<int>(PathCommand::Winding)) {
		auto iter=std::rbegin(vals);
		ctx.commandy = *iter++;
		ctx.commandx = *iter;
	}

	// transform commands
	size_t i=0;
	while (i < nvals) {
		int cmd = (int)(vals[i]);
		switch (cmd) {
		case static_cast<int>(PathCommand::MoveTo):
			transformPoint(vals[i+1], vals[i+2], state.xform.data(), vals[i+1], vals[i+2]);
			i += 3;
			break;
		case static_cast<int>(PathCommand::LineTo):
			transformPoint(vals[i+1], vals[i+2], state.xform.data(), vals[i+1], vals[i+2]);
			i += 3;
			break;
		case static_cast<int>(PathCommand::BezierTo):
			transformPoint(vals[i+1], vals[i+2], state.xform.data(), vals[i+1], vals[i+2]);
			transformPoint(vals[i+3], vals[i+4], state.xform.data(), vals[i+3], vals[i+4]);
			transformPoint(vals[i+5], vals[i+6], state.xform.data(), vals[i+5], vals[i+6]);
			i += 7;
			break;
		case static_cast<int>(PathCommand::Close):
			i++;
			break;
		case static_cast<int>(PathCommand::Winding):
			i += 2;
			break;
		default:
			i++;
		}
	}
	ctx.commands.insert(ctx.commands.end(), vals.begin(), vals.end());
}

static void clearPathCache(Context& ctx)
{
	ctx.cache->points.clear();
	ctx.cache->paths.clear();
}
static Path* lastPath(Context& ctx)
{
	std::vector<Path>& paths = ctx.cache->paths;
	if (!paths.empty())
		return &paths.back();
	return nullptr;
}
static void addPath(Context& ctx)
{
	std::vector<Path>& paths = ctx.cache->paths;
	const int nextCount = static_cast<int>(paths.size()) + 1;
	if (static_cast<size_t>(nextCount) > paths.capacity()) {
		const int newCap = nextCount + static_cast<int>(paths.capacity() / 2);
		paths.reserve(static_cast<size_t>(newCap));
	}
	paths.emplace_back();
	Path* path = &paths.back();
	std::memset(path, 0, sizeof(*path));
	path->first = static_cast<int>(ctx.cache->points.size());
	path->winding = static_cast<int>(Winding::CCW);
}
static Point* lastPoint(Context& ctx)
{
	std::vector<Point>& points = ctx.cache->points;
	if (!points.empty())
		return &points.back();
	return nullptr;
}
static void addPoint(Context& ctx, float x, float y, int flags)
{
	Path* path = lastPath(ctx);
	if (path == nullptr) return;

	std::vector<Point>& points = ctx.cache->points;

	if (path->count > 0 && !points.empty()) {
		Point* pt = lastPoint(ctx);
		if (ptEquals(pt->x,pt->y, x,y, ctx.distTol)) {
			pt->flags |= static_cast<unsigned char>(flags);
			return;
		}
	}

	const int nextCount = static_cast<int>(points.size()) + 1;
	if (static_cast<size_t>(nextCount) > points.capacity()) {
		const int newCap = nextCount + static_cast<int>(points.capacity() / 2);
		points.reserve(static_cast<size_t>(newCap));
	}

	points.emplace_back();
	Point* pt = &points.back();
	std::memset(pt, 0, sizeof(*pt));
	pt->x = x;
	pt->y = y;
	pt->flags = static_cast<unsigned char>(flags);

	path->count++;
}

static void closePath(Context& ctx)
{
	Path* path = lastPath(ctx);
	if (path == nullptr) return;
	path->closed = 1;
}
static void pathWinding(Context& ctx, int winding)
{
	Path* path = lastPath(ctx);
	if (path == nullptr) return;
	path->winding = winding;
}
static float getAverageScale(const float *t)
{
	float sx = sqrtf(t[0]*t[0] + t[2]*t[2]);
	float sy = sqrtf(t[1]*t[1] + t[3]*t[3]);
	return (sx + sy) * 0.5f;
}
static void prepareTempVerts(Context& ctx, int nverts)
{
	std::vector<Vertex>& verts = ctx.cache->verts;
	verts.clear();
	if (static_cast<size_t>(nverts) > verts.capacity()) {
		const int cverts = (nverts + 0xff) & ~0xff; // Round up to prevent allocations when things change just slightly.
		verts.reserve(static_cast<size_t>(cverts));
	}
}
static float triarea2(float ax, float ay, float bx, float by, float cx, float cy)
{
	float abx = bx - ax;
	float aby = by - ay;
	float acx = cx - ax;
	float acy = cy - ay;
	return acx*aby - abx*acy;
}
static float polyArea(Point* pts, int npts)
{
	int i;
	float area = 0;
	for (i = 2; i < npts; i++) {
		Point* a = &pts[0];
		Point* b = &pts[i-1];
		Point* c = &pts[i];
		area += triarea2(a->x,a->y, b->x,b->y, c->x,c->y);
	}
	return area * 0.5f;
}
static void polyReverse(Point* pts, int npts)
{
	Point tmp;
	int i = 0, j = npts-1;
	while (i < j) {
		tmp = pts[i];
		pts[i] = pts[j];
		pts[j] = tmp;
		i++;
		j--;
	}
}
template <typename A, typename B, typename C, typename D, typename E, typename F>
static void pushVertex(std::vector<Vertex>& vs, const A& x, const B& y, const C& u, const D& v, const E& s, const F& t)
{
	vs.push_back(Vertex{
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(u),
		static_cast<float>(v),
		static_cast<float>(s),
		static_cast<float>(t)});
}
static void tesselateBezier(Context& ctx,
								 float x1, float y1, float x2, float y2,
								 float x3, float y3, float x4, float y4,
								 int level, int type)
{
	float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;
	float dx,dy,d2,d3;

	if (level > 10) return;

	x12 = (x1+x2)*0.5f;
	y12 = (y1+y2)*0.5f;
	x23 = (x2+x3)*0.5f;
	y23 = (y2+y3)*0.5f;
	x34 = (x3+x4)*0.5f;
	y34 = (y3+y4)*0.5f;
	x123 = (x12+x23)*0.5f;
	y123 = (y12+y23)*0.5f;

	dx = x4 - x1;
	dy = y4 - y1;
	d2 = absf(((x2 - x4) * dy - (y2 - y4) * dx));
	d3 = absf(((x3 - x4) * dy - (y3 - y4) * dx));

	if ((d2 + d3)*(d2 + d3) < ctx.tessTol * (dx*dx + dy*dy)) {
		addPoint(ctx, x4, y4, type);
		return;
	}

/*	if (absf(x1+x3-x2-x2) + absf(y1+y3-y2-y2) + absf(x2+x4-x3-x3) + absf(y2+y4-y3-y3) < ctx.tessTol) {
		addPoint(ctx, x4, y4, type);
		return;
	}*/

	x234 = (x23+x34)*0.5f;
	y234 = (y23+y34)*0.5f;
	x1234 = (x123+x234)*0.5f;
	y1234 = (y123+y234)*0.5f;

	tesselateBezier(ctx, x1,y1, x12,y12, x123,y123, x1234,y1234, level+1, 0);
	tesselateBezier(ctx, x1234,y1234, x234,y234, x34,y34, x4,y4, level+1, type);
}
static void flattenPaths(Context& ctx)
{
	auto& cache = ctx.cache;
	// State* state = getState(ctx);
	Point* last;
	Point* p0;
	Point* p1;
	Point* pts;
	Path* path;
	int i, j;
	float* cp1;
	float* cp2;
	float* p;
	float area;

	if (!cache->paths.empty())
		return;

	// Flatten
	i = 0;
	while (i < ctx.commands.size()) {
		int cmd = (int)ctx.commands[i];
		switch (cmd) {
		case static_cast<int>(PathCommand::MoveTo):
			addPath(ctx);
			p = &ctx.commands[i+1];
			addPoint(ctx, p[0], p[1], static_cast<int>(PointFlags::Corner));
			i += 3;
			break;
		case static_cast<int>(PathCommand::LineTo):
			p = &ctx.commands[i+1];
			addPoint(ctx, p[0], p[1], static_cast<int>(PointFlags::Corner));
			i += 3;
			break;
		case static_cast<int>(PathCommand::BezierTo):
			last = lastPoint(ctx);
			if (last != NULL) {
				cp1 = &ctx.commands[i+1];
				cp2 = &ctx.commands[i+3];
				p = &ctx.commands[i+5];
				tesselateBezier(ctx, last->x,last->y, cp1[0],cp1[1], cp2[0],cp2[1], p[0],p[1], 0, static_cast<int>(PointFlags::Corner));
			}
			i += 7;
			break;
		case static_cast<int>(PathCommand::Close):
			detail::closePath(ctx);
			i++;
			break;
		case static_cast<int>(PathCommand::Winding):
			detail::pathWinding(ctx, (int)ctx.commands[i+1]);
			i += 2;
			break;
		default:
			i++;
		}
	}

	cache->bounds[0] = cache->bounds[1] = 1e6f;
	cache->bounds[2] = cache->bounds[3] = -1e6f;

	// Calculate the direction and length of line segments.
	for (j = 0; j < static_cast<int>(cache->paths.size()); j++) {
		path = &cache->paths[j];
		pts = &cache->points[path->first];

		// If the first and last points are the same, remove the last, mark as closed path.
		p0 = &pts[path->count-1];
		p1 = &pts[0];
		if (ptEquals(p0->x,p0->y, p1->x,p1->y, ctx.distTol)) {
			path->count--;
			p0 = &pts[path->count-1];
			path->closed = 1;
		}

		// Enforce winding.
		path->reversed = 0;
		if (path->count > 2) {
			area = polyArea(pts, path->count);
			if (path->winding == static_cast<int>(Winding::CCW) && area < 0.0f) {
				polyReverse(pts, path->count);
				path->reversed = 1;
			}
			if (path->winding == static_cast<int>(Winding::CW) && area > 0.0f) {
				polyReverse(pts, path->count);
				path->reversed = 1;
			}
		}

		for(i = 0; i < path->count; i++) {
			// Calculate segment direction and length
			p0->dx = p1->x - p0->x;
			p0->dy = p1->y - p0->y;
			p0->len = normalize(p0->dx, p0->dy);
			// Update bounds
			cache->bounds[0] = minf(cache->bounds[0], p0->x);
			cache->bounds[1] = minf(cache->bounds[1], p0->y);
			cache->bounds[2] = maxf(cache->bounds[2], p0->x);
			cache->bounds[3] = maxf(cache->bounds[3], p0->y);
			// Advance
			p0 = p1++;
		}
	}
}
static int curveDivs(float r, float arc, float tol)
{
	float da = acosf(r / (r + tol)) * 2.0f;
	return maxi(2, (int)ceilf(arc / da));
}
static void chooseBevel(int bevel, Point* p0, Point* p1, float w,
							float* x0, float* y0, float* x1, float* y1)
{
	if (bevel) {
		*x0 = p1->x + p0->dy * w;
		*y0 = p1->y - p0->dx * w;
		*x1 = p1->x + p1->dy * w;
		*y1 = p1->y - p1->dx * w;
	} else {
		*x0 = p1->x + p1->dmx * w;
		*y0 = p1->y + p1->dmy * w;
		*x1 = p1->x + p1->dmx * w;
		*y1 = p1->y + p1->dmy * w;
	}
}
static void roundJoin(std::vector<Vertex>& out, Point* p0, Point* p1,
								 float lw, float rw, float lu, float ru, int ncap,
								 float fringe, float t)
{
	int i, n;
	float dlx0 = p0->dy;
	float dly0 = -p0->dx;
	float dlx1 = p1->dy;
	float dly1 = -p1->dx;
	UNUSED(fringe);

	if (p1->flags & PointFlags::Left) {
		float lx0,ly0,lx1,ly1,a0,a1;
		chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, lw, &lx0,&ly0, &lx1,&ly1);
		a0 = atan2f(-dly0, -dlx0);
		a1 = atan2f(-dly1, -dlx1);
		if (a1 > a0) a1 -= (float)(M_PI * 2.0);

		pushVertex(out, lx0, ly0, lu, 1, -1, t);
		pushVertex(out, p1->x - dlx0*rw, p1->y - dly0*rw, ru, 1, 1, t);

		n = clampi((int)ceilf(((a0 - a1) / (float)M_PI) * ncap), 2, ncap);
		for (i = 0; i < n; i++) {
			float u = i/(float)(n-1);
			float a = a0 + u*(a1-a0);
			float rx = p1->x + cosf(a) * rw;
			float ry = p1->y + sinf(a) * rw;
			pushVertex(out, p1->x, p1->y, 0.5f, 1, 0, t);
			pushVertex(out, rx, ry, ru,1, 1, t);
		}

		pushVertex(out, lx1, ly1, lu, 1, -1, t);
		pushVertex(out, p1->x - dlx1*rw, p1->y - dly1*rw, ru, 1, 1, t);

	} else {
		float rx0,ry0,rx1,ry1,a0,a1;
		chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, -rw, &rx0,&ry0, &rx1,&ry1);
		a0 = atan2f(dly0, dlx0);
		a1 = atan2f(dly1, dlx1);
		if (a1 < a0) a1 += (float)(M_PI * 2.0);

		pushVertex(out, p1->x + dlx0*rw, p1->y + dly0*rw, lu,1, -1, t);
		pushVertex(out, rx0, ry0, ru, 1, 1, t);

		n = clampi((int)ceilf(((a1 - a0) / (float)M_PI) * ncap), 2, ncap);
		for (i = 0; i < n; i++) {
			float u = i/(float)(n-1);
			float a = a0 + u*(a1-a0);
			float lx = p1->x + cosf(a) * lw;
			float ly = p1->y + sinf(a) * lw;
			pushVertex(out, lx, ly, lu, 1, 1, t);
			pushVertex(out, p1->x, p1->y, 0.5f, 1,  0.0, t);
		}

		pushVertex(out, p1->x + dlx1*rw, p1->y + dly1*rw, lu, 1, -1, t);
		pushVertex(out, rx1, ry1, ru, 1, 1, t);

	}
	}
static void bevelJoin(std::vector<Vertex>& out, Point* p0, Point* p1,
										float lw, float rw, float lu, float ru, float fringe, float t)
{
	float rx0,ry0,rx1,ry1;
	float lx0,ly0,lx1,ly1;
	float dlx0 = p0->dy;
	float dly0 = -p0->dx;
	float dlx1 = p1->dy;
	float dly1 = -p1->dx;
	UNUSED(fringe);

	if (p1->flags & PointFlags::Left) {
		chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, lw, &lx0,&ly0, &lx1,&ly1);

		pushVertex(out, lx0, ly0, lu, 1, -1, t);
		pushVertex(out, p1->x - dlx0*rw, p1->y - dly0*rw, ru, 1, 1, t);

		if (p1->flags & PointFlags::Bevel) {
			pushVertex(out, lx0, ly0, lu, 1, -1, t);
			pushVertex(out, p1->x - dlx0*rw, p1->y - dly0*rw, ru, 1, 1, t);

			pushVertex(out, lx1, ly1, lu, 1, -1, t);
			pushVertex(out, p1->x - dlx1*rw, p1->y - dly1*rw, ru, 1, 1, t);
		} else {
			rx0 = p1->x - p1->dmx * rw;
			ry0 = p1->y - p1->dmy * rw;

			pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
			pushVertex(out, p1->x - dlx0*rw, p1->y - dly0*rw, ru, 1, 1, t);

			pushVertex(out, rx0, ry0, ru,1, -1, t);
			pushVertex(out, rx0, ry0, ru,1, 1, t);

			pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
			pushVertex(out, p1->x - dlx1*rw, p1->y - dly1*rw, ru, 1, 1, t);
		}

		pushVertex(out, lx1, ly1, lu, 1, -1, t);
		pushVertex(out, p1->x - dlx1*rw, p1->y - dly1*rw, ru,1, 1, t);

	} else {
		chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, -rw, &rx0,&ry0, &rx1,&ry1);

		pushVertex(out, p1->x + dlx0*lw, p1->y + dly0*lw, lu, 1, 1, t);
		pushVertex(out, rx0, ry0, ru, 1, -1, t);

		if (p1->flags & PointFlags::Bevel) {
			pushVertex(out, p1->x + dlx0*lw, p1->y + dly0*lw, lu, 1, 1, t);
			pushVertex(out, rx0, ry0, ru, 1, -1, t);

			pushVertex(out, p1->x + dlx1*lw, p1->y + dly1*lw, lu, 1, 1, t);
			pushVertex(out, rx1, ry1, ru, 1, -1, t);
		} else {
			lx0 = p1->x + p1->dmx * lw;
			ly0 = p1->y + p1->dmy * lw;

			pushVertex(out, p1->x + dlx0*lw, p1->y + dly0*lw, lu, 1, 1, t);
			pushVertex(out, p1->x, p1->y, 0.5f,1, -1, t);

			pushVertex(out, lx0, ly0, lu, 1, 1, t);
			pushVertex(out, lx0, ly0, lu, 1, -1, t);

			pushVertex(out, p1->x + dlx1*lw, p1->y + dly1*lw, lu, 1, 1, t);
			pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
		}

		pushVertex(out, p1->x + dlx1*lw, p1->y + dly1*lw, lu, 1, 1, t);
		pushVertex(out, rx1, ry1, ru, 1, -1, t);
	}

	}
static void insertSpacer(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, float u0, float u1, float t){
	float dlx = dy;
	float dly = -dx;
	float px = p->x;
	float py = p->y;
	pushVertex(out, px + dlx*w, py + dly*w, u0,1,-1, t);
	pushVertex(out, px - dlx*w, py - dly*w, u1,1, 1, t);
	}
static void buttCapStart(std::vector<Vertex>& out, Point* p,
									float dx, float dy, float w, float d,
									float aa, float u0, float u1, float t, int dir)
{
	float px = p->x - dx*d;
	float py = p->y - dy*d;
	float dlx = dy;
	float dly = -dx;
	pushVertex(out, px + dlx*w - dx*aa, py + dly*w - dy*aa, u0,0, -1, t - dir * aa / w);
	pushVertex(out, px - dlx*w - dx*aa, py - dly*w - dy*aa, u1,0, 1, t - dir * aa / w);
	pushVertex(out, px + dlx*w, py + dly*w, u0,1, -1, t);
	pushVertex(out, px - dlx*w, py - dly*w, u1,1, 1, t);
	}
static void buttCapEnd(std::vector<Vertex>& out, Point* p,
								  float dx, float dy, float w, float d,
								  float aa, float u0, float u1, float t, int dir)
{
	float px = p->x + dx*d;
	float py = p->y + dy*d;
	float dlx = dy;
	float dly = -dx;
	pushVertex(out, px + dlx*w, py + dly*w, u0, 1 , -1, t);
	pushVertex(out, px - dlx*w, py - dly*w, u1, 1 , 1, t);
	pushVertex(out, px + dlx*w + dx*aa, py + dly*w + dy*aa, u0, 0, -1, t + dir * aa / w);
	pushVertex(out, px - dlx*w + dx*aa, py - dly*w + dy*aa, u1, 0, 1, t + dir * aa / w);
	}
static void roundCapStart(std::vector<Vertex>& out, Point* p,
									 float dx, float dy, float w, int ncap,
									 float aa, float u0, float u1, float t, int dir)
{
	int i;
	float px = p->x;
	float py = p->y;
	float dlx = dy;
	float dly = -dx;
	UNUSED(aa);
	for (i = 0; i < ncap; i++) {
		const float a = i/(float)(ncap-1) * (float)M_PI;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		pushVertex(out, px - dlx*ax - dx*ay, py - dly*ax - dy*ay, u0, 1, ax / w, t - dir * ay / w);
		pushVertex(out, px, py, 0.5f, 1 , 0, t);
	}
	pushVertex(out, px + dlx*w, py + dly*w, u0, 1, 1, t);
	pushVertex(out, px - dlx*w, py - dly*w, u1, 1, -1, t);
	}
static void roundCapEnd(std::vector<Vertex>& out, Point* p,
								   float dx, float dy, float w, int ncap,
								   float aa, float u0, float u1, float t, int dir)
{
	int i;
	float px = p->x;
	float py = p->y;
	float dlx = dy;
	float dly = -dx;
	UNUSED(aa);
	pushVertex(out, px + dlx*w, py + dly*w, u0,1, w, t);
	pushVertex(out, px - dlx*w, py - dly*w, u1,1, -w, t);
	for (i = 0; i < ncap; i++) {
		float a = i/(float)(ncap-1) * (float)M_PI;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		pushVertex(out, px, py, 0.5f, 1, 0, t);
		pushVertex(out, px - dlx*ax + dx*ay, py - dly*ax + dy*ay, u0, 1, ax / w, t + dir * ay / w);
	}
	}
static void calculateJoins(Context& ctx, float w, int lineJoin, float miterLimit)
{
	auto& cache = ctx.cache;
	int i, j;
	float iw = 0.0f;

	if (w > 0.0f) iw = 1.0f / w;

	// Calculate which joins needs extra vertices to append, and gather vertex count.
	for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
		Path* path = &cache->paths[i];
		Point* pts = &cache->points[path->first];
		Point* p0 = &pts[path->count-1];
		Point* p1 = &pts[0];
		int nleft = 0;

		path->nbevel = 0;

		for (j = 0; j < path->count; j++) {
			float dlx0, dly0, dlx1, dly1, dmr2, cross, limit;
			dlx0 = p0->dy;
			dly0 = -p0->dx;
			dlx1 = p1->dy;
			dly1 = -p1->dx;
			// Calculate extrusions
			p1->dmx = (dlx0 + dlx1) * 0.5f;
			p1->dmy = (dly0 + dly1) * 0.5f;
			dmr2 = p1->dmx*p1->dmx + p1->dmy*p1->dmy;
			if (dmr2 > 0.000001f) {
				float scale = 1.0f / dmr2;
				if (scale > 600.0f) {
					scale = 600.0f;
				}
				p1->dmx *= scale;
				p1->dmy *= scale;
			}

			// Clear flags, but keep the corner.
			p1->flags = (p1->flags & PointFlags::Corner) ? static_cast<unsigned char>(PointFlags::Corner) : 0u;

			// Keep track of left turns.
			cross = p1->dx * p0->dy - p0->dx * p1->dy;
			if (cross > 0.0f) {
				nleft++;
				p1->flags |= PointFlags::Left;
			}

			// Calculate if we should use bevel or miter for inner join.
			limit = maxf(1.01f, minf(p0->len, p1->len) * iw);
			if ((dmr2 * limit*limit) < 1.0f)
				p1->flags |= PointFlags::InnerBevel;

			// Check to see if the corner needs to be beveled.
			if (p1->flags & PointFlags::Corner) {
				if ((dmr2 * miterLimit*miterLimit) < 1.0f || lineJoin == static_cast<int>(LineCap::Bevel) || lineJoin == static_cast<int>(LineCap::Round)) {
					p1->flags |= PointFlags::Bevel;
				}
			}

			if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0)
				path->nbevel++;

			p0 = p1++;
		}

		path->convex = (nleft == path->count) ? 1 : 0;
	}
}
static int expandStroke(Context& ctx, float w, float fringe, int lineCap, int lineJoin, int lineStyle, float miterLimit)
{
	auto& cache = ctx.cache;
	std::vector<Vertex>& buf = ctx.cache->verts;
	int cverts, i, j;
	float t;
	float aa = fringe;//ctx.fringeWidth;
	float u0 = 0.0f, u1 = 1.0f;
	int ncap = curveDivs(w, (float)M_PI, ctx.tessTol);	// Calculate divisions per half circle.

	w += aa * 0.5f;
	const float invStrokeWidth = 1.0f / w;
	// Disable the gradient used for antialiasing when antialiasing is not used.
	if (aa == 0.0f) {
		u0 = 0.5f;
		u1 = 0.5f;
	}

	// Force round join to minimize distortion
	if(lineStyle > 1) lineJoin = static_cast<int>(LineCap::Round);

	calculateJoins(ctx, w, lineJoin, miterLimit);
	// Calculate max vertex usage.
	cverts = 0;
	for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
		Path* path = &cache->paths[i];
		int loop = (path->closed == 0) ? 0 : 1;
		if (lineJoin == static_cast<int>(LineCap::Round)) {
			cverts += (path->count + path->nbevel*(ncap+2) + 1) * 2; // plus one for loop
		} else {
			cverts += (path->count + path->nbevel*5 + 1) * 2; // plus one for loop
		}
		if(lineStyle > 1) cverts += 4 * path->count; // extra vertices for spacers
		if (loop == 0) {
			// space for caps
			if (lineCap == static_cast<int>(LineCap::Round)) {
				cverts += (ncap*2 + 2)*2;
			} else {
				cverts += (3+3)*2;
			}
		}
	}

	prepareTempVerts(ctx, cverts);

	for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
		Path* path = &cache->paths[i];
		Point* pts = &cache->points[path->first];
		Point* p0;
		Point* p1;
		int s, e, loop;
		float dx, dy;

		path->fill = 0;
		path->nfill = 0;

		// Calculate fringe or stroke
		loop = (path->closed == 0) ? 0 : 1;
		const size_t strokeStart = buf.size();

		if (loop) {
			// Looping
			p0 = &pts[path->count-1];
			p1 = &pts[0];
			s = 0;
			e = path->count;
		} else {
			// Add cap
			p0 = &pts[0];
			p1 = &pts[1];
			s = 1;
			e = path->count-1;
		}

		t = 0;

		int dir = 1;
		if(lineStyle > 1 && path->reversed) {
			dir = -1;
			for (j = s; j < path->count; ++j) {
				dx = p1->x - p0->x;
				dy = p1->y - p0->y;
				t+=normalize(dx, dy) * invStrokeWidth;
				p0 = p1++;
			}
			if (loop) {
				// Looping
				p0 = &pts[path->count-1];
				p1 = &pts[0];
			} else {
				// Add cap
				p0 = &pts[0];
				p1 = &pts[1];
			}
		}

		if (loop == 0) {
			// Add cap
			dx = p1->x - p0->x;
			dy = p1->y - p0->y;
			normalize(dx, dy);
			if (lineCap == static_cast<int>(LineCap::Butt))
				buttCapStart(buf, p0, dx, dy, w, -aa*0.5f, aa, u0, u1, t, dir);
			else if (lineCap == static_cast<int>(LineCap::Butt) || lineCap == static_cast<int>(LineCap::Square))
				buttCapStart(buf, p0, dx, dy, w, w-aa, aa, u0, u1, t, dir);
			else if (lineCap == static_cast<int>(LineCap::Round))
				roundCapStart(buf, p0, dx, dy, w, ncap, aa, u0, u1, t, dir);
		}
		for (j = s; j < e; ++j) {
			if(lineStyle > 1){
				dx = p1->x - p0->x;
				dy = p1->y - p0->y;
				float dt=normalize(dx, dy);
				insertSpacer(buf, p0, dx, dy, w, u0, u1, t);
				t+=dir*dt*invStrokeWidth;
				insertSpacer(buf, p1, dx, dy, w, u0, u1, t);
			}
			if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0) {
				if (lineJoin == static_cast<int>(LineCap::Round)) {
					roundJoin(buf, p0, p1, w, w, u0, u1, ncap, aa, t);
				} else {
					bevelJoin(buf, p0, p1, w, w, u0, u1, aa, t);
				}
			} else {
				pushVertex(buf, p1->x + (p1->dmx * w), p1->y + (p1->dmy * w), u0, 1, -1, t);
				pushVertex(buf, p1->x - (p1->dmx * w), p1->y - (p1->dmy * w), u1, 1, 1, t);
			}
			p0 = p1++;
		}
		
		if (loop) {
			// Loop it
			pushVertex(buf, buf[strokeStart].x, buf[strokeStart].y, u0, 1, -1, t);
			pushVertex(buf, buf[strokeStart + 1].x, buf[strokeStart + 1].y, u1, 1, 1, t);
		} else {
			dx = p1->x - p0->x;
			dy = p1->y - p0->y;
			float dt = normalize(dx, dy);
			if(lineStyle > 1){
				insertSpacer(buf, p0, dx, dy, w, u0, u1, t);
				t+=dir*dt*invStrokeWidth;
				insertSpacer(buf, p1, dx, dy, w, u0, u1, t);
			}
			// Add cap
			if (lineCap == static_cast<int>(LineCap::Butt))
				buttCapEnd(buf, p1, dx, dy, w, -aa*0.5f, aa, u0, u1, t, dir);
			else if (lineCap == static_cast<int>(LineCap::Butt) || lineCap == static_cast<int>(LineCap::Square))
				buttCapEnd(buf, p1, dx, dy, w, w-aa, aa, u0, u1, t, dir);
			else if (lineCap == static_cast<int>(LineCap::Round))
				roundCapEnd(buf, p1, dx, dy, w, ncap, aa, u0, u1, t, dir);
		}
		path->stroke = buf.data() + strokeStart;
		path->nstroke = static_cast<int>(buf.size() - strokeStart);
	}

	return 1;
}
static int expandFill(Context& ctx, float w, int lineJoin, float miterLimit)
{
	auto& cache = ctx.cache;
	std::vector<Vertex>& buf = ctx.cache->verts;
	int cverts, convex, i, j;
	float aa = ctx.fringeWidth;
	int fringe = w > 0.0f;

	calculateJoins(ctx, w, lineJoin, miterLimit);

	// Calculate max vertex usage.
	cverts = 0;
	for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
		Path* path = &cache->paths[i];
		cverts += path->count + path->nbevel + 1;
		if (fringe)
			cverts += (path->count + path->nbevel*5 + 1) * 2; // plus one for loop
	}

	prepareTempVerts(ctx, cverts);

	convex = cache->paths.size() == 1 && cache->paths[0].convex;

	for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
		Path* path = &cache->paths[i];
		Point* pts = &cache->points[path->first];
		Point* p0;
		Point* p1;
		float rw, lw, woff;
		float ru, lu;

		// Calculate shape vertices.
		woff = 0.5f*aa;
		const size_t fillStart = buf.size();

		if (fringe) {
			// Looping
			p0 = &pts[path->count-1];
			p1 = &pts[0];
			for (j = 0; j < path->count; ++j) {
				if (p1->flags & PointFlags::Bevel) {
					float dlx0 = p0->dy;
					float dly0 = -p0->dx;
					float dlx1 = p1->dy;
					float dly1 = -p1->dx;
					if (p1->flags & PointFlags::Left) {
						float lx = p1->x + p1->dmx * woff;
						float ly = p1->y + p1->dmy * woff;
						pushVertex(buf, lx, ly, 0.5f, 1, 0, 0);
					} else {
						float lx0 = p1->x + dlx0 * woff;
						float ly0 = p1->y + dly0 * woff;
						float lx1 = p1->x + dlx1 * woff;
						float ly1 = p1->y + dly1 * woff;
						pushVertex(buf, lx0, ly0, 0.5f, 1, 0, 0);
						pushVertex(buf, lx1, ly1, 0.5f, 1, 0, 0);
					}
				} else {
					pushVertex(buf, p1->x + (p1->dmx * woff), p1->y + (p1->dmy * woff), 0.5f,1, 0, 0);
				}
				p0 = p1++;
			}
		} else {
			for (j = 0; j < path->count; ++j) {
				pushVertex(buf, pts[j].x, pts[j].y, 0.5f, 1, 0, 0);
			}
		}

		path->fill = buf.data() + fillStart;
		path->nfill = static_cast<int>(buf.size() - fillStart);

		// Calculate fringe
		if (fringe) {
			lw = w + woff;
			rw = w - woff;
			lu = 0;
			ru = 1;
			const size_t strokeStart = buf.size();

			// Create only half a fringe for convex shapes so that
			// the shape can be rendered without stenciling.
			if (convex) {
				lw = woff;	// This should generate the same vertex as fill inset above.
				lu = 0.5f;	// Set outline fade at middle.
			}

			// Looping
			p0 = &pts[path->count-1];
			p1 = &pts[0];

			for (j = 0; j < path->count; ++j) {
				if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0) {
					bevelJoin(buf, p0, p1, lw, rw, lu, ru, ctx.fringeWidth, 0);
				} else {
					pushVertex(buf, p1->x + (p1->dmx * lw), p1->y + (p1->dmy * lw), lu,1,0, 0);
					pushVertex(buf, p1->x - (p1->dmx * rw), p1->y - (p1->dmy * rw), ru,1,0, 0);
				}
				p0 = p1++;
			}

			// Loop it
			pushVertex(buf, buf[strokeStart].x, buf[strokeStart].y, lu,1,0, 0);
			pushVertex(buf, buf[strokeStart + 1].x, buf[strokeStart + 1].y, ru,1,0, 0);

			path->stroke = buf.data() + strokeStart;
			path->nstroke = static_cast<int>(buf.size() - strokeStart);
		} else {
			path->stroke = NULL;
			path->nstroke = 0;
		}
	}

	return 1;
}
static float quantize(float a, float d)
{
	return ((int)(a / d + 0.5f)) * d;
}
static float getFontScale(State* state)
{
	return minf(quantize(getAverageScale(state->xform.data()), 0.01f), 4.0f);
}
static void flushTextTexture(Context& ctx)
{
	std::array<int, 4> dirty{};

	if (fonsValidateTexture(ctx.fs, dirty.data())) {
		int fontImage = ctx.fontImages[ctx.fontImageIdx];
		// Update texture
		if (fontImage != 0) {
			int iw, ih;
			const unsigned char* data = fonsGetTextureData(ctx.fs, &iw, &ih);
			int x = dirty[0];
			int y = dirty[1];
			int w = dirty[2] - dirty[0];
			int h = dirty[3] - dirty[1];
			ctx.params.renderUpdateTexture(ctx.params.userPtr, fontImage, x,y, w,h, data);
		}
	}
}
static int allocTextAtlas(Context& ctx)
{
	int iw, ih;
	flushTextTexture(ctx);
	if (ctx.fontImageIdx >= NVG_MAX_FONTIMAGES-1)
		return 0;
	// if next fontImage already have a texture
	if (ctx.fontImages[ctx.fontImageIdx+1] != 0)
		imageSize(ctx, ctx.fontImages[ctx.fontImageIdx+1], iw, ih);
	else { // calculate the new font image size and create it.
		imageSize(ctx, ctx.fontImages[ctx.fontImageIdx], iw, ih);
		if (iw > ih)
			ih *= 2;
		else
			iw *= 2;
		if (iw > NVG_MAX_FONTIMAGE_SIZE || ih > NVG_MAX_FONTIMAGE_SIZE)
			iw = ih = NVG_MAX_FONTIMAGE_SIZE;
		ctx.fontImages[ctx.fontImageIdx+1] = ctx.params.renderCreateTexture(ctx.params.userPtr, static_cast<int>(Texture::Alpha), iw, ih, 0, NULL);
	}
	++ctx.fontImageIdx;
	fonsResetAtlas(ctx.fs, iw, ih);
	return 1;
}
static void renderText(Context& ctx, Vertex* verts, int nverts)
{
	State& state = getState(ctx);
	Paint paint = state.fill;

	// Render triangles.
	paint.image = ctx.fontImages[ctx.fontImageIdx];

	// Apply global alpha
	paint.innerColor.a *= state.alpha;
	paint.outerColor.a *= state.alpha;

	ctx.params.renderTriangles(ctx.params.userPtr, &paint, state.compositeOperation, &state.scissor, verts, nverts, ctx.fringeWidth);

	ctx.drawCallCount++;
	ctx.textTriCount += nverts/3;
}
static int isTransformFlipped(const float *xform)
{
	float det = xform[0] * xform[3] - xform[2] * xform[1];
	return( det < 0);
}

} // namespace detail

Context::~Context() {
	int i;
	if (fs)
		fonsDeleteInternal(fs);

	for (i = 0; i < NVG_MAX_FONTIMAGES; i++) {
		if (fontImages[i] != 0) {
			deleteImage(*this, fontImages[i]);
			fontImages[i] = 0;
		}
	}

	if (params.renderDelete != nullptr)
		params.renderDelete(params.userPtr);
}
Context::Context(const Params& params):params(params){
	int i;
	FONSparams fontParams;
	for (i = 0; i < NVG_MAX_FONTIMAGES; i++)
		fontImages[i] = 0;

	cache = detail::allocPathCache();

	save(*this);
	reset(*this);
	detail::setDevicePixelRatio(*this, 1.0f);

	if (params.renderCreate(params.userPtr) == 0)
		throw std::runtime_error("Could not init render context.");

	// Init font rendering
	std::memset(&fontParams, 0, sizeof(fontParams));
	fontParams.width = NVG_INIT_FONTIMAGE_SIZE;
	fontParams.height = NVG_INIT_FONTIMAGE_SIZE;
	fontParams.flags = FONS_ZERO_TOPLEFT;
	fontParams.renderCreate = nullptr;
	fontParams.renderUpdate = nullptr;
	fontParams.renderDraw = nullptr;
	fontParams.renderDelete = nullptr;
	fontParams.userPtr = nullptr;
	fs = fonsCreateInternal(&fontParams);
	if (fs == nullptr) throw std::runtime_error("Could not create font context");

	// Create font texture
	fontImages[0] = params.renderCreateTexture(params.userPtr, static_cast<int>(Texture::Alpha), fontParams.width, fontParams.height, 0, nullptr);
	if (fontImages[0] == 0) throw std::runtime_error("Could not create font texture");
	fontImageIdx = 0;
	scissor = ScissorBounds{0.0f, 0.0f, -1.0f, -1.0f};
}

int getImageTextureId(Context& ctx, int handle)
{
	return ctx.params.renderGetImageTextureId(ctx.params.userPtr, handle);
}

const Params& internalParams(Context& ctx)
{
    return ctx.params;
}

ScissorBounds currentScissor(Context& ctx) {
	return ctx.scissor;
}

void beginFrame(Context& ctx, float windowWidth, float windowHeight, float devicePixelRatio)
{
/*	printf("Tris: draws:%d  fill:%d  stroke:%d  text:%d  TOT:%d\n",
		ctx.drawCallCount, ctx.fillTriCount, ctx.strokeTriCount, ctx.textTriCount,
		ctx.fillTriCount+ctx.strokeTriCount+ctx.textTriCount);*/

	ctx.states.clear();
	save(ctx);
	reset(ctx);

	detail::setDevicePixelRatio(ctx, devicePixelRatio);

	ctx.params.renderViewport(ctx.params.userPtr, windowWidth, windowHeight, devicePixelRatio);

	ctx.drawCallCount = 0;
	ctx.fillTriCount = 0;
	ctx.strokeTriCount = 0;
	ctx.textTriCount = 0;
}

void cancelFrame(Context& ctx)
{
	ctx.params.renderCancel(ctx.params.userPtr);
}

void endFrame(Context& ctx)
{
	ctx.params.renderFlush(ctx.params.userPtr);
	if (ctx.fontImageIdx != 0) {
		int fontImage = ctx.fontImages[ctx.fontImageIdx];
		ctx.fontImages[ctx.fontImageIdx] = 0;
		int i, j, iw, ih;
		// delete images that smaller than current one
		if (fontImage == 0)
			return;
		imageSize(ctx, fontImage, iw, ih);
		for (i = j = 0; i < ctx.fontImageIdx; i++) {
			if (ctx.fontImages[i] != 0) {
				int nw, nh;
				int image = ctx.fontImages[i];
				ctx.fontImages[i] = 0;
				imageSize(ctx, image, nw, nh);
				if (nw < iw || nh < ih)
					deleteImage(ctx, image);
				else
					ctx.fontImages[j++] = image;
			}
		}
		// make current font image to first
		ctx.fontImages[j] = ctx.fontImages[0];
		ctx.fontImages[0] = fontImage;
		ctx.fontImageIdx = 0;
	}
}

Color rgb(unsigned char r, unsigned char g, unsigned char b)
{
	return rgba(r,g,b,255);
}

Color rgbf(float r, float g, float b)
{
	return rgbaf(r,g,b,1.0f);
}

Color rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	Color color;
	// Use longer initialization to suppress warning.
	color.r = r / 255.0f;
	color.g = g / 255.0f;
	color.b = b / 255.0f;
	color.a = a / 255.0f;
	return color;
}

Color rgbaf(float r, float g, float b, float a)
{
	Color color;
	// Use longer initialization to suppress warning.
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}

Color transRgba(Color c, unsigned char a)
{
	c.a = a / 255.0f;
	return c;
}

Color transRgbaf(Color c, float a)
{
	c.a = a;
	return c;
}

Color lerpRgba(Color c0, Color c1, float u)
{
	int i;
	float oneminu;
	Color cint = {{{0}}};

	u = detail::clampf(u, 0.0f, 1.0f);
	oneminu = 1.0f - u;
	for( i = 0; i <4; i++ )
	{
		cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
	}

	return cint;
}

Color hsl(float h, float s, float l)
{
	return hsla(h,s,l,255);
}


Color hsla(float h, float s, float l, unsigned char a)
{
	float m1, m2;
	Color col;
	h = detail::modf(h, 1.0f);
	if (h < 0.0f) h += 1.0f;
	s = detail::clampf(s, 0.0f, 1.0f);
	l = detail::clampf(l, 0.0f, 1.0f);
	m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
	m1 = 2 * l - m2;
	col.r = detail::clampf(detail::hue(h + 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
	col.g = detail::clampf(detail::hue(h, m1, m2), 0.0f, 1.0f);
	col.b = detail::clampf(detail::hue(h - 1.0f/3.0f, m1, m2), 0.0f, 1.0f);
	col.a = a/255.0f;
	return col;
}

void transformIdentity(float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void transformTranslate(float* t, float tx, float ty)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = tx; t[5] = ty;
}

void transformScale(float* t, float sx, float sy)
{
	t[0] = sx; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = sy;
	t[4] = 0.0f; t[5] = 0.0f;
}

void transformRotate(float* t, float a)
{
	float cs = cosf(a), sn = sinf(a);
	t[0] = cs; t[1] = sn;
	t[2] = -sn; t[3] = cs;
	t[4] = 0.0f; t[5] = 0.0f;
}

void transformSkewX(float* t, float a)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = tanf(a); t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void transformSkewY(float* t, float a)
{
	t[0] = 1.0f; t[1] = tanf(a);
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void transformMultiply(float* t, const float* s)
{
	float t0 = t[0] * s[0] + t[1] * s[2];
	float t2 = t[2] * s[0] + t[3] * s[2];
	float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
	t[1] = t[0] * s[1] + t[1] * s[3];
	t[3] = t[2] * s[1] + t[3] * s[3];
	t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

void transformPremultiply(float* t, const float* s)
{
	std::array<float, 6> s2{};
	memcpy(s2.data(), s, sizeof(float)*6);
	transformMultiply(s2.data(), t);
	memcpy(t, s2.data(), sizeof(float)*6);
}

int transformInverse(float* inv, const float* t)
{
	double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
	if (det > -1e-6 && det < 1e-6) {
		transformIdentity(inv);
		return 0;
	}
	invdet = 1.0 / det;
	inv[0] = (float)(t[3] * invdet);
	inv[2] = (float)(-t[2] * invdet);
	inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
	inv[1] = (float)(-t[1] * invdet);
	inv[3] = (float)(t[0] * invdet);
	inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
	return 1;
}

void transformPoint(float& dx, float& dy, const float* t, float sx, float sy)
{
	dx = sx*t[0] + sy*t[2] + t[4];
	dy = sx*t[1] + sy*t[3] + t[5];
}

float degToRad(float deg)
{
	return deg / 180.0f * (float)M_PI;
}

float radToDeg(float rad)
{
	return rad / (float)M_PI * 180.0f;
}



// State handling
void save(Context& ctx)
{
	if (!ctx.states.empty())
		ctx.states.push_back(ctx.states.back());
	else
		ctx.states.push_back({});
}

void restore(Context& ctx)
{
	if(ctx.states.size()<=1)
		return;

	ctx.states.pop_back();
}

void reset(Context& ctx)
{
	State& state = detail::getState(ctx);
	std::memset(&state, 0, sizeof(state));

	detail::setPaintColor(&state.fill, rgba(255,255,255,255));
	detail::setPaintColor(&state.stroke, rgba(0,0,0,255));
	state.compositeOperation = detail::compositeOperationState(static_cast<int>(CompositeOperation::SourceOver));
	state.shapeAntiAlias = 1;
	state.strokeWidth = 1.0f;
	state.miterLimit = 10.0f;
	state.lineCap = static_cast<int>(LineCap::Butt);
	state.lineJoin = static_cast<int>(LineCap::Miter);
	state.lineStyle = static_cast<int>(LineStyle::Solid);
	state.alpha = 1.0f;
	transformIdentity(state.xform.data());

	state.scissor.extent[0] = -1.0f;
	state.scissor.extent[1] = -1.0f;

	state.fontSize = 16.0f;
	state.letterSpacing = 0.0f;
	state.lineHeight = 1.0f;
	state.fontBlur = 0.0f;
	state.fontDilate = 0.0f;
	state.textAlign = static_cast<int>(Align::Left | Align::Baseline);
	state.fontId = 0;
}

// State setting
void shapeAntiAlias(Context& ctx, int enabled)
{
	State& state = detail::getState(ctx);
	state.shapeAntiAlias = enabled;
}

void strokeWidth(Context& ctx, float width)
{
	State& state = detail::getState(ctx);
	state.strokeWidth = width;
}

float getStrokeWidth(Context& ctx) {
	State& state = detail::getState(ctx);
	return state.strokeWidth;
}

int getTextAlign(Context& ctx) {
	State& state = detail::getState(ctx);
	return state.textAlign;
}

float getFontSize(Context& ctx) {
	State& state = detail::getState(ctx);
	return state.fontSize;
}
int getFontFaceId(Context& ctx) {
	State& state = detail::getState(ctx);
	return state.fontId;
}

void miterLimit(Context& ctx, float limit)
{
	State& state = detail::getState(ctx);
	state.miterLimit = limit;
}

void lineStyle(Context& ctx, int lineStyle) {
	State& state = detail::getState(ctx);
	state.lineStyle = lineStyle;
}

void lineCap(Context& ctx, int cap)
{
	State& state = detail::getState(ctx);
	state.lineCap = cap;
}

void lineJoin(Context& ctx, int join)
{
	State& state = detail::getState(ctx);
	state.lineJoin = join;
}

void globalAlpha(Context& ctx, float alpha)
{
	State& state = detail::getState(ctx);
	state.alpha = alpha;
}

void transform(Context& ctx, float a, float b, float c, float d, float e, float f)
{
	State& state = detail::getState(ctx);
	const std::array<float, 6> t = { a, b, c, d, e, f };
	transformPremultiply(state.xform.data(), t.data());
}

void resetTransform(Context& ctx)
{
	State& state = detail::getState(ctx);
	transformIdentity(state.xform.data());
}

void translate(Context& ctx, float x, float y)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> t{};
	transformTranslate(t.data(), x,y);
	transformPremultiply(state.xform.data(), t.data());
}

void rotate(Context& ctx, float angle)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> t{};
	transformRotate(t.data(), angle);
	transformPremultiply(state.xform.data(), t.data());
}

void skewX(Context& ctx, float angle)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> t{};
	transformSkewX(t.data(), angle);
	transformPremultiply(state.xform.data(), t.data());
}

void skewY(Context& ctx, float angle)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> t{};
	transformSkewY(t.data(), angle);
	transformPremultiply(state.xform.data(), t.data());
}

void scale(Context& ctx, float x, float y)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> t{};
	transformScale(t.data(), x,y);
	transformPremultiply(state.xform.data(), t.data());
}

void currentTransform(Context& ctx, float* xform)
{
	State& state = detail::getState(ctx);
	if (xform == NULL) return;
	memcpy(xform, state.xform.data(), sizeof(float)*6);
}

void strokeColor(Context& ctx, Color color)
{
	State& state = detail::getState(ctx);
	detail::setPaintColor(&state.stroke, color);
}

void strokePaint(Context& ctx, Paint paint)
{
	State& state = detail::getState(ctx);
	state.stroke = paint;
	transformMultiply(state.stroke.xform, state.xform.data());
}

void fillColor(Context& ctx, Color color)
{
	State& state = detail::getState(ctx);
	detail::setPaintColor(&state.fill, color);
}

void fillPaint(Context& ctx, Paint paint)
{
	State& state = detail::getState(ctx);
	state.fill = paint;
	transformMultiply(state.fill.xform, state.xform.data());
}

#ifndef NVG_NO_STB
int createImage(Context& ctx, const char* filename, int imageFlags)
{
	int w, h, n, image;
	unsigned char* img;
	stbi_set_unpremultiply_on_load(1);
	stbi_convert_iphone_png_to_rgb(1);
	img = stbi_load(filename, &w, &h, &n, 4);
	if (img == NULL) {
//		printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
		return 0;
	}
	image = createImageRGBA(ctx, w, h, imageFlags, img);
	stbi_image_free(img);
	return image;
}

int createImageMem(Context& ctx, int imageFlags, unsigned char* data, int ndata)
{
	int w, h, n, image;
	stbi_set_unpremultiply_on_load(1);
	stbi_convert_iphone_png_to_rgb(1);
	unsigned char* img = stbi_load_from_memory(data, ndata, &w, &h, &n, 4);
	if (img == NULL) {
//		printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
		return 0;
	}
	image = createImageRGBA(ctx, w, h, imageFlags, img);
	stbi_image_free(img);
	return image;
}
#endif

int createImageRGBA(Context& ctx, int w, int h, int imageFlags, const unsigned char* data)
{
	return ctx.params.renderCreateTexture(ctx.params.userPtr, static_cast<int>(Texture::Rgba), w, h, imageFlags, data);
}

void updateImage(Context& ctx, int image, const unsigned char* data)
{
	int w, h;
	ctx.params.renderGetTextureSize(ctx.params.userPtr, image, &w, &h);
	ctx.params.renderUpdateTexture(ctx.params.userPtr, image, 0,0, w,h, data);
}

void imageSize(Context& ctx, int image, int& w, int& h)
{
	ctx.params.renderGetTextureSize(ctx.params.userPtr, image, &w, &h);
}

void deleteImage(Context& ctx, int image)
{
	ctx.params.renderDeleteTexture(ctx.params.userPtr, image);
}

Paint linearGradient(Context& ctx,
								  float sx, float sy, float ex, float ey,
								  Color icol, Color ocol)
{
	Paint p;
	float dx, dy, d;
	const float large = 1e5;
	UNUSED(ctx);
	memset(&p, 0, sizeof(p));

	// Calculate transform aligned to the line
	dx = ex - sx;
	dy = ey - sy;
	d = sqrtf(dx*dx + dy*dy);
	if (d > 0.0001f) {
		dx /= d;
		dy /= d;
	} else {
		dx = 0;
		dy = 1;
	}

	p.xform[0] = dy; p.xform[1] = -dx;
	p.xform[2] = dx; p.xform[3] = dy;
	p.xform[4] = sx - dx*large; p.xform[5] = sy - dy*large;

	p.extent[0] = large;
	p.extent[1] = large + d*0.5f;

	p.radius = 0.0f;

	p.feather = detail::maxf(1.0f, d);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}

Paint radialGradient(Context& ctx,
								  float cx, float cy, float inr, float outr,
								  Color icol, Color ocol)
{
	Paint p;
	float r = (inr+outr)*0.5f;
	float f = (outr-inr);
	UNUSED(ctx);
	memset(&p, 0, sizeof(p));

	transformIdentity(p.xform);
	p.xform[4] = cx;
	p.xform[5] = cy;

	p.extent[0] = r;
	p.extent[1] = r;

	p.radius = r;

	p.feather = detail::maxf(1.0f, f);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}

Paint boxGradient(Context& ctx,
							   float x, float y, float w, float h, float r, float f,
							   Color icol, Color ocol)
{
	Paint p;
	UNUSED(ctx);
	memset(&p, 0, sizeof(p));

	transformIdentity(p.xform);
	p.xform[4] = x+w*0.5f;
	p.xform[5] = y+h*0.5f;

	p.extent[0] = w*0.5f;
	p.extent[1] = h*0.5f;

	p.radius = r;

	p.feather = detail::maxf(1.0f, f);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}


Paint imagePattern(Context& ctx,
								float cx, float cy, float w, float h, float angle,
								int image, float alpha)
{
	Paint p;
	UNUSED(ctx);
	memset(&p, 0, sizeof(p));

	transformRotate(p.xform, angle);
	p.xform[4] = cx;
	p.xform[5] = cy;

	p.extent[0] = w;
	p.extent[1] = h;

	p.image = image;

	p.innerColor = p.outerColor = rgbaf(1,1,1,alpha);

	return p;
}

// Scissoring
void scissor(Context& ctx, float x, float y, float w, float h)
{
	State& state = detail::getState(ctx);
	ctx.scissor = ScissorBounds{x, y, w, h};
	w = detail::maxf(0.0f, w);
	h = detail::maxf(0.0f, h);

	transformIdentity(state.scissor.xform);
	state.scissor.xform[4] = x+w*0.5f;
	state.scissor.xform[5] = y+h*0.5f;
	transformMultiply(state.scissor.xform, state.xform.data());

	state.scissor.extent[0] = w*0.5f;
	state.scissor.extent[1] = h*0.5f;
}


void intersectScissor(Context& ctx, float x, float y, float w, float h)
{
	State& state = detail::getState(ctx);
	std::array<float, 6> pxform{};
	std::array<float, 6> invxorm{};
	std::array<float, 4> rect{};
	float ex, ey, tex, tey;

	// If no previous scissor has been set, set the scissor as current scissor.
	if (state.scissor.extent[0] < 0) {
		scissor(ctx, x, y, w, h);
		return;
	}

	// Transform the current scissor rect into current transform space.
	// If there is difference in rotation, this will be approximation.
	memcpy(pxform.data(), state.scissor.xform, sizeof(float)*6);
	ex = state.scissor.extent[0];
	ey = state.scissor.extent[1];
	transformInverse(invxorm.data(), state.xform.data());
	transformMultiply(pxform.data(), invxorm.data());
	tex = ex*detail::absf(pxform[0]) + ey*detail::absf(pxform[2]);
	tey = ex*detail::absf(pxform[1]) + ey*detail::absf(pxform[3]);

	// Intersect rects.
	detail::isectRects(rect.data(), pxform[4]-tex,pxform[5]-tey,tex*2,tey*2, x,y,w,h);

	scissor(ctx, rect[0], rect[1], rect[2], rect[3]);
}

void resetScissor(Context& ctx)
{
	State& state = detail::getState(ctx);
	memset(state.scissor.xform, 0, sizeof(state.scissor.xform));
	state.scissor.extent[0] = -1.0f;
	state.scissor.extent[1] = -1.0f;
}

// Global composite operation.
void globalCompositeOperation(Context& ctx, int op)
{
	State& state = detail::getState(ctx);
	state.compositeOperation = detail::compositeOperationState(op);
}

void globalCompositeBlendFunc(Context& ctx, int sfactor, int dfactor)
{
	globalCompositeBlendFuncSeparate(ctx, sfactor, dfactor, sfactor, dfactor);
}

void globalCompositeBlendFuncSeparate(Context& ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha)
{
	CompositeOperationState op;
	op.srcRGB = srcRGB;
	op.dstRGB = dstRGB;
	op.srcAlpha = srcAlpha;
	op.dstAlpha = dstAlpha;

	State& state = detail::getState(ctx);
	state.compositeOperation = op;
}






































// Draw
void beginPath(Context& ctx)
{
	ctx.commands.clear();
	detail::clearPathCache(ctx);
}

void moveTo(Context& ctx, float x, float y)
{
	std::array<float, 3> vals = { static_cast<float>(to_underlying(PathCommand::MoveTo)), x, y };
	detail::appendCommands(ctx, vals);
}

void lineTo(Context& ctx, float x, float y)
{
	std::array<float, 3> vals = { static_cast<float>(to_underlying(PathCommand::LineTo)), x, y };
	detail::appendCommands(ctx, vals);
}

void bezierTo(Context& ctx, float c1x, float c1y, float c2x, float c2y, float x, float y)
{
	std::array<float, 7> vals = { static_cast<float>(to_underlying(PathCommand::BezierTo)), c1x, c1y, c2x, c2y, x, y };
	detail::appendCommands(ctx, vals);
}

void quadTo(Context& ctx, float cx, float cy, float x, float y)
{
    float x0 = ctx.commandx;
    float y0 = ctx.commandy;
    std::array<float, 7> vals = { static_cast<float>(to_underlying(PathCommand::BezierTo)),
        x0 + 2.0f/3.0f*(cx - x0), y0 + 2.0f/3.0f*(cy - y0),
        x + 2.0f/3.0f*(cx - x), y + 2.0f/3.0f*(cy - y),
        x, y };
   detail::appendCommands(ctx, vals);
}

void arcTo(Context& ctx, float x1, float y1, float x2, float y2, float radius)
{
	float x0 = ctx.commandx;
	float y0 = ctx.commandy;
	float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
	int dir;

	if (ctx.commands.empty()) {
		return;
	}

	// Handle degenerate cases.
	if (detail::ptEquals(x0,y0, x1,y1, ctx.distTol) ||
		detail::ptEquals(x1,y1, x2,y2, ctx.distTol) ||
		detail::distPtSeg(x1,y1, x0,y0, x2,y2) < ctx.distTol*ctx.distTol ||
		radius < ctx.distTol) {
		lineTo(ctx, x1,y1);
		return;
	}

	// Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
	dx0 = x0-x1;
	dy0 = y0-y1;
	dx1 = x2-x1;
	dy1 = y2-y1;
	detail::normalize(dx0, dy0);
	detail::normalize(dx1, dy1);
	a = acosf(dx0*dx1 + dy0*dy1);
	d = radius / tanf(a/2.0f);

//	printf("a=%f° d=%f\n", a/M_PI*180.0f, d);

	if (d > 10000.0f) {
		lineTo(ctx, x1,y1);
		return;
	}

	if (detail::cross(dx0,dy0, dx1,dy1) > 0.0f) {
		cx = x1 + dx0*d + dy0*radius;
		cy = y1 + dy0*d + -dx0*radius;
		a0 = atan2f(dx0, -dy0);
		a1 = atan2f(-dx1, dy1);
		dir = static_cast<int>(Winding::CW);
//		printf("CW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/M_PI*180.0f, a1/M_PI*180.0f);
	} else {
		cx = x1 + dx0*d + -dy0*radius;
		cy = y1 + dy0*d + dx0*radius;
		a0 = atan2f(-dx0, dy0);
		a1 = atan2f(dx1, -dy1);
		dir = static_cast<int>(Winding::CCW);
//		printf("CCW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/M_PI*180.0f, a1/M_PI*180.0f);
	}

	arc(ctx, cx, cy, radius, a0, a1, dir);
}

void closePath(Context& ctx)
{
	std::array<float, 1> vals = { static_cast<float>(to_underlying(PathCommand::Close)) };
	detail::appendCommands(ctx, vals);
}

void pathWinding(Context& ctx, int dir)
{
	std::array<float, 2> vals = { static_cast<float>(to_underlying(PathCommand::Winding)), (float)dir };
	detail::appendCommands(ctx, vals);
}

void arc(Context& ctx, float cx, float cy, float r, float a0, float a1, int dir)
{
	const float pi = (float)M_PI;
	const float twoPi = pi * 2.0f;
	float a = 0, da = 0, hda = 0, kappa = 0;
	float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
	float px = 0, py = 0, ptanx = 0, ptany = 0;
	std::array<float, 3 + 5*7 + 100> vals{};
	int i, ndivs, nvals;
	int move = !ctx.commands.empty() ? static_cast<int>(PathCommand::LineTo) : static_cast<int>(PathCommand::MoveTo);

	// Clamp angles
	da = a1 - a0;
	if (dir == static_cast<int>(Winding::CW)) {
		if (detail::absf(da) >= twoPi) {
			da = twoPi;
		} else {
			while (da < 0.0f) da += twoPi;
		}
	} else {
		if (detail::absf(da) >= twoPi) {
			da = -twoPi;
		} else {
			while (da > 0.0f) da -= twoPi;
		}
	}

	// Split arc into max 90 degree segments.
	ndivs = detail::maxi(1, detail::mini((int)(detail::absf(da) / (pi*0.5f) + 0.5f), 5));
	hda = (da / (float)ndivs) / 2.0f;
	kappa = detail::absf(4.0f / 3.0f * (1.0f - cosf(hda)) / sinf(hda));

	if (dir == static_cast<int>(Winding::CCW))
		kappa = -kappa;

	nvals = 0;
	for (i = 0; i <= ndivs; i++) {
		a = a0 + da * (i/(float)ndivs);
		dx = cosf(a);
		dy = sinf(a);
		x = cx + dx*r;
		y = cy + dy*r;
		tanx = -dy*r*kappa;
		tany = dx*r*kappa;

		if (i == 0) {
			vals[(size_t)nvals++] = (float)move;
			vals[nvals++] = x;
			vals[nvals++] = y;
		} else {
			vals[nvals++] = static_cast<float>(to_underlying(PathCommand::BezierTo));
			vals[nvals++] = px+ptanx;
			vals[nvals++] = py+ptany;
			vals[nvals++] = x-tanx;
			vals[nvals++] = y-tany;
			vals[nvals++] = x;
			vals[nvals++] = y;
		}
		px = x;
		py = y;
		ptanx = tanx;
		ptany = tany;
	}

	std::span<float> span(vals.data(), static_cast<size_t>(nvals));
	detail::appendCommands(ctx, span);
}

void rect(Context& ctx, float x, float y, float w, float h)
{
	std::array<float, 13> vals = {
		static_cast<float>(to_underlying(PathCommand::MoveTo)), x,y,
		static_cast<float>(to_underlying(PathCommand::LineTo)), x,y+h,
		static_cast<float>(to_underlying(PathCommand::LineTo)), x+w,y+h,
		static_cast<float>(to_underlying(PathCommand::LineTo)), x+w,y,
		static_cast<float>(to_underlying(PathCommand::Close))
	};
	detail::appendCommands(ctx, vals);
}

void roundedRect(Context& ctx, float x, float y, float w, float h, float r)
{
	roundedRectVarying(ctx, x, y, w, h, r, r, r, r);
}

void roundedRectVarying(Context& ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
{
	if(radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f) {
		rect(ctx, x, y, w, h);
		return;
	} else {
		float halfw = detail::absf(w)*0.5f;
		float halfh = detail::absf(h)*0.5f;
		float rxBL = detail::minf(radBottomLeft, halfw) * detail::signf(w), ryBL = detail::minf(radBottomLeft, halfh) * detail::signf(h);
		float rxBR = detail::minf(radBottomRight, halfw) * detail::signf(w), ryBR = detail::minf(radBottomRight, halfh) * detail::signf(h);
		float rxTR = detail::minf(radTopRight, halfw) * detail::signf(w), ryTR = detail::minf(radTopRight, halfh) * detail::signf(h);
		float rxTL = detail::minf(radTopLeft, halfw) * detail::signf(w), ryTL = detail::minf(radTopLeft, halfh) * detail::signf(h);
		std::array<float, 44> vals = {
			static_cast<float>(to_underlying(PathCommand::MoveTo)), x, y + ryTL,
			static_cast<float>(to_underlying(PathCommand::LineTo)), x, y + h - ryBL,
			static_cast<float>(to_underlying(PathCommand::BezierTo)), x, y + h - ryBL*(1 - NVG_KAPPA90), x + rxBL*(1 - NVG_KAPPA90), y + h, x + rxBL, y + h,
			static_cast<float>(to_underlying(PathCommand::LineTo)), x + w - rxBR, y + h,
			static_cast<float>(to_underlying(PathCommand::BezierTo)), x + w - rxBR*(1 - NVG_KAPPA90), y + h, x + w, y + h - ryBR*(1 - NVG_KAPPA90), x + w, y + h - ryBR,
			static_cast<float>(to_underlying(PathCommand::LineTo)), x + w, y + ryTR,
			static_cast<float>(to_underlying(PathCommand::BezierTo)), x + w, y + ryTR*(1 - NVG_KAPPA90), x + w - rxTR*(1 - NVG_KAPPA90), y, x + w - rxTR, y,
			static_cast<float>(to_underlying(PathCommand::LineTo)), x + rxTL, y,
			static_cast<float>(to_underlying(PathCommand::BezierTo)), x + rxTL*(1 - NVG_KAPPA90), y, x, y + ryTL*(1 - NVG_KAPPA90), x, y + ryTL,
			static_cast<float>(to_underlying(PathCommand::Close))
		};
		detail::appendCommands(ctx, vals);
	}
}

void ellipse(Context& ctx, float cx, float cy, float rx, float ry)
{
	std::array<float, 32> vals = {
		static_cast<float>(to_underlying(PathCommand::MoveTo)), cx-rx, cy,
		static_cast<float>(to_underlying(PathCommand::BezierTo)), cx-rx, cy+ry*NVG_KAPPA90, cx-rx*NVG_KAPPA90, cy+ry, cx, cy+ry,
		static_cast<float>(to_underlying(PathCommand::BezierTo)), cx+rx*NVG_KAPPA90, cy+ry, cx+rx, cy+ry*NVG_KAPPA90, cx+rx, cy,
		static_cast<float>(to_underlying(PathCommand::BezierTo)), cx+rx, cy-ry*NVG_KAPPA90, cx+rx*NVG_KAPPA90, cy-ry, cx, cy-ry,
		static_cast<float>(to_underlying(PathCommand::BezierTo)), cx-rx*NVG_KAPPA90, cy-ry, cx-rx, cy-ry*NVG_KAPPA90, cx-rx, cy,
		static_cast<float>(to_underlying(PathCommand::Close))
	};
	detail::appendCommands(ctx, vals);
}

void circle(Context& ctx, float cx, float cy, float r)
{
	ellipse(ctx, cx,cy, r,r);
}

void debugDumpPathCache(Context& ctx)
{
	const Path* path;
	int i, j;

	printf("Dumping %zu cached paths\n", ctx.cache->paths.size());
	for (i = 0; i < static_cast<int>(ctx.cache->paths.size()); i++) {
		path = &ctx.cache->paths[i];
		printf(" - Path %d\n", i);
		if (path->nfill) {
			printf("   - fill: %d\n", path->nfill);
			for (j = 0; j < path->nfill; j++)
				printf("%f\t%f\n", path->fill[j].x, path->fill[j].y);
		}
		if (path->nstroke) {
			printf("   - stroke: %d\n", path->nstroke);
			for (j = 0; j < path->nstroke; j++)
				printf("%f\t%f\n", path->stroke[j].x, path->stroke[j].y);
		}
	}
}

void fill(Context& ctx)
{
	State& state = detail::getState(ctx);
	const Path* path;
	Paint fillPaint = state.fill;
	int i;

	detail::flattenPaths(ctx);
	if (ctx.params.edgeAntiAlias && state.shapeAntiAlias)
		detail::expandFill(ctx, ctx.fringeWidth, static_cast<int>(LineCap::Miter), 2.4f);
	else
		detail::expandFill(ctx, 0.0f, static_cast<int>(LineCap::Miter), 2.4f);

	// Apply global alpha
	fillPaint.innerColor.a *= state.alpha;
	fillPaint.outerColor.a *= state.alpha;

	ctx.params.renderFill(ctx.params.userPtr, &fillPaint, state.compositeOperation, &state.scissor, ctx.fringeWidth,
						   ctx.cache->bounds.data(), ctx.cache->paths.data(), static_cast<int>(ctx.cache->paths.size()));

	// Count triangles
	for (i = 0; i < static_cast<int>(ctx.cache->paths.size()); i++) {
		path = &ctx.cache->paths[i];
		ctx.fillTriCount += path->nfill-2;
		ctx.fillTriCount += path->nstroke-2;
		ctx.drawCallCount += 2;
	}
}

void stroke(Context& ctx)
{
	State& state = detail::getState(ctx);
	const float scale = detail::getAverageScale(state.xform.data());
	float strokeWidth = detail::clampf(state.strokeWidth * scale, 0.0f, 1000.0f);
	Paint strokePaint = state.stroke;
	const Path* path;
	int i;


	if (strokeWidth < ctx.fringeWidth) {
		// If the stroke width is less than pixel size, use alpha to emulate coverage.
		// Since coverage is area, scale by alpha*alpha.
		float alpha = detail::clampf(strokeWidth / ctx.fringeWidth, 0.0f, 1.0f);
		strokePaint.innerColor.a *= alpha*alpha;
		strokePaint.outerColor.a *= alpha*alpha;
		strokeWidth = ctx.fringeWidth;
	}

	// Apply global alpha
	strokePaint.innerColor.a *= state.alpha;
	strokePaint.outerColor.a *= state.alpha;

	detail::flattenPaths(ctx);

	if (ctx.params.edgeAntiAlias && state.shapeAntiAlias)
		detail::expandStroke(ctx, strokeWidth*0.5f, ctx.fringeWidth, state.lineCap, state.lineJoin, state.lineStyle, state.miterLimit);
	else
		detail::expandStroke(ctx, strokeWidth*0.5f, 0.0f, state.lineCap, state.lineJoin, state.lineStyle, state.miterLimit);

	ctx.params.renderStroke(ctx.params.userPtr, &strokePaint, state.compositeOperation, &state.scissor, ctx.fringeWidth,
							 strokeWidth, state.lineStyle, ctx.cache->paths.data(), static_cast<int>(ctx.cache->paths.size()));

	// Count triangles
	for (i = 0; i < static_cast<int>(ctx.cache->paths.size()); i++) {
		path = &ctx.cache->paths[i];
		ctx.strokeTriCount += path->nstroke-2;
		ctx.drawCallCount++;
	}
}

// Add fonts
int createFont(Context& ctx, const char* name, const char* filename)
{
	return fonsAddFont(ctx.fs, name, filename, 0);
}

int createFontAtIndex(Context& ctx, const char* name, const char* filename, const int fontIndex)
{
	return fonsAddFont(ctx.fs, name, filename, fontIndex);
}

int createFontMem(Context& ctx, const char* name, unsigned char* data, int ndata, int freeData)
{
	return fonsAddFontMem(ctx.fs, name, data, ndata, freeData, 0);
}

int createFontMemAtIndex(Context& ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex)
{
	return fonsAddFontMem(ctx.fs, name, data, ndata, freeData, fontIndex);
}

int findFont(Context& ctx, const char* name)
{
	if (name == NULL) return -1;
	return fonsGetFontByName(ctx.fs, name);
}


int addFallbackFontId(Context& ctx, int baseFont, int fallbackFont)
{
	if(baseFont == -1 || fallbackFont == -1) return 0;
	return fonsAddFallbackFont(ctx.fs, baseFont, fallbackFont);
}

int addFallbackFont(Context& ctx, const char* baseFont, const char* fallbackFont)
{
	return addFallbackFontId(ctx, findFont(ctx, baseFont), findFont(ctx, fallbackFont));
}

void resetFallbackFontsId(Context& ctx, int baseFont)
{
	fonsResetFallbackFont(ctx.fs, baseFont);
}

void resetFallbackFonts(Context& ctx, const char* baseFont)
{
	resetFallbackFontsId(ctx, findFont(ctx, baseFont));
}

// State setting
void fontSize(Context& ctx, float size)
{
	State& state = detail::getState(ctx);
	state.fontSize = size;
}

void fontBlur(Context& ctx, float blur)
{
	State& state = detail::getState(ctx);
	state.fontBlur = blur;
}

void fontDilate(Context& ctx, float dilate)
{
	State& state = detail::getState(ctx);
	state.fontDilate = dilate;
}

void textLetterSpacing(Context& ctx, float spacing)
{
	State& state = detail::getState(ctx);
	state.letterSpacing = spacing;
}

void textLineHeight(Context& ctx, float lineHeight)
{
	State& state = detail::getState(ctx);
	state.lineHeight = lineHeight;
}

void textAlign(Context& ctx, int align)
{
	State& state = detail::getState(ctx);
	state.textAlign = align;
}

void fontFaceId(Context& ctx, int font)
{
	State& state = detail::getState(ctx);
	state.fontId = font;
}

void fontFace(Context& ctx, const char* font)
{
	State& state = detail::getState(ctx);
	state.fontId = fonsGetFontByName(ctx.fs, font);
}







float text(Context& ctx, float x, float y, const char* string, const char* end)
{
	State& state = detail::getState(ctx);
	FONStextIter iter, prevIter;
	FONSquad q;
	std::vector<Vertex>& buf = ctx.cache->verts;
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;
	int cverts = 0;
	int isFlipped = detail::isTransformFlipped(state.xform.data());

	if (end == NULL)
		end = string + strlen(string);

	if (state.fontId == FONS_INVALID) return x;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetDilate(ctx.fs, state.fontDilate*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);

	cverts = detail::maxi(2, (int)(end - string)) * 6; // conservative estimate.
	detail::prepareTempVerts(ctx, cverts);

	fonsTextIterInit(ctx.fs, &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_REQUIRED);
	prevIter = iter;
	while (fonsTextIterNext(ctx.fs, &iter, &q)) {
		std::array<float, 8> c{};
		if (iter.prevGlyphIndex == -1) { // can not retrieve glyph?
			if (!buf.empty()) {
				detail::renderText(ctx, buf.data(), static_cast<int>(buf.size()));
				buf.clear();
			}
			if (!detail::allocTextAtlas(ctx))
				break; // no memory :(
			iter = prevIter;
			fonsTextIterNext(ctx.fs, &iter, &q); // try again
			if (iter.prevGlyphIndex == -1) // still can not find glyph?
				break;
		}
		prevIter = iter;
		if(isFlipped) {
			float tmp;

			tmp = q.y0; q.y0 = q.y1; q.y1 = tmp;
			tmp = q.t0; q.t0 = q.t1; q.t1 = tmp;
		}
		// Transform corners.
		transformPoint(c[0], c[1], state.xform.data(), q.x0*invscale + x, q.y0*invscale + y);
		transformPoint(c[2], c[3], state.xform.data(), q.x1*invscale + x, q.y0*invscale + y);
		transformPoint(c[4], c[5], state.xform.data(), q.x1*invscale + x, q.y1*invscale + y);
		transformPoint(c[6], c[7], state.xform.data(), q.x0*invscale + x, q.y1*invscale + y);
		// Create triangles
		if (buf.size() + 6 <= buf.capacity()) {
			detail::pushVertex(buf, c[0], c[1], q.s0, q.t0, 0, 0);
			detail::pushVertex(buf, c[4], c[5], q.s1, q.t1, 0, 0);
			detail::pushVertex(buf, c[2], c[3], q.s1, q.t0, 0, 0);
			detail::pushVertex(buf, c[0], c[1], q.s0, q.t0, 0, 0);
			detail::pushVertex(buf, c[6], c[7], q.s0, q.t1, 0, 0);
			detail::pushVertex(buf, c[4], c[5], q.s1, q.t1, 0, 0);
		}
	}

	// TODO: add back-end bit to do this just once per frame.
	detail::flushTextTexture(ctx);

	detail::renderText(ctx, buf.data(), static_cast<int>(buf.size()));
	return iter.nextx * invscale + x;
}

void textBox(Context& ctx, float x, float y, float breakRowWidth, const char* string, const char* end)
{
	State& state = detail::getState(ctx);
	std::array<TextRow, 2> rows{};
	int nrows = 0, i;
	int oldAlign = state.textAlign;
	int halign = state.textAlign & static_cast<int>(Align::Left | Align::Center | Align::Right);
	int valign = state.textAlign & static_cast<int>(Align::Top | Align::Middle | Align::MiddleAscent | Align::Bottom | Align::Baseline);
	float lineh = 0;

	if (state.fontId == FONS_INVALID) return;

	textMetrics(ctx, NULL, NULL, &lineh);

	state.textAlign = static_cast<int>(Align::Left) | valign;

	while ((nrows = textBreakLines(ctx, string, end, breakRowWidth, rows.data(), (int)rows.size(), 0))) {
		for (i = 0; i < nrows; i++) {
			TextRow* row = &rows[(size_t)i];
			if (halign & Align::Left)
				text(ctx, x, y, row->start, row->end);
			else if (halign & Align::Center)
				text(ctx, x + breakRowWidth*0.5f - row->width*0.5f, y, row->start, row->end);
			else if (halign & Align::Right)
				text(ctx, x + breakRowWidth - row->width, y, row->start, row->end);
			y += lineh * state.lineHeight;
		}
		string = rows[(size_t)nrows-1].next;
	}

	state.textAlign = oldAlign;
}

int textGlyphPositions(Context& ctx, float x, float y, const char* string, const char* end, GlyphPosition* positions, int maxPositions)
{
	State& state = detail::getState(ctx);
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;
	FONStextIter iter, prevIter;
	FONSquad q;
	int npos = 0;

	if (state.fontId == FONS_INVALID) return 0;

	if (end == NULL)
		end = string + strlen(string);

	if (string == end)
		return 0;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);

	fonsTextIterInit(ctx.fs, &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
	prevIter = iter;
	while (fonsTextIterNext(ctx.fs, &iter, &q)) {
		if (iter.prevGlyphIndex < 0 && detail::allocTextAtlas(ctx)) { // can not retrieve glyph?
			iter = prevIter;
			fonsTextIterNext(ctx.fs, &iter, &q); // try again
		}
		prevIter = iter;
		positions[npos].str = iter.str;
		positions[npos].x = iter.x * invscale + x;
		positions[npos].minx = detail::minf(iter.x, q.x0) * invscale + x;
		positions[npos].maxx = detail::maxf(iter.nextx, q.x1) * invscale + x;
		npos++;
		if (npos >= maxPositions)
			break;
	}

	return npos;
}

enum class CodepointType : int {
	Space,
	Newline,
	Char,
	CjkChar,
};

int textBreakLines(Context& ctx, const char* string, const char* end, float breakRowWidth, TextRow* rows, int maxRows, int skipSpaces)
{
	State& state = detail::getState(ctx);
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;
	FONStextIter iter, prevIter;
	FONSquad q;
	int nrows = 0;
	float rowStartX = 0;
	float rowWidth = 0;
	float rowMinX = 0;
	float rowMaxX = 0;
	const char* rowStart = NULL;
	const char* rowEnd = NULL;
	const char* wordStart = NULL;
	float wordStartX = 0;
	float wordMinX = 0;
	const char* breakEnd = NULL;
	float breakWidth = 0;
	float breakMaxX = 0;
	CodepointType type = CodepointType::Space, ptype = CodepointType::Space;
	unsigned int pcodepoint = 0;

	if (maxRows == 0) return 0;
	if (state.fontId == FONS_INVALID) return 0;

	if (end == NULL)
		end = string + strlen(string);

	if (string == end) return 0;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetDilate(ctx.fs, state.fontDilate*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);

	breakRowWidth *= scale;

	fonsTextIterInit(ctx.fs, &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
	prevIter = iter;
	while (fonsTextIterNext(ctx.fs, &iter, &q)) {
		if (iter.prevGlyphIndex < 0 && detail::allocTextAtlas(ctx)) { // can not retrieve glyph?
			iter = prevIter;
			fonsTextIterNext(ctx.fs, &iter, &q); // try again
		}
		prevIter = iter;
		switch (iter.codepoint) {
			case 9:			// \t
			case 11:		// \v
			case 12:		// \f
			case 32:		// space
			case 0x00a0:	// NBSP
				type = CodepointType::Space;
				break;
			case 10:		// \n
				type = pcodepoint == 13 ? CodepointType::Space : CodepointType::Newline;
				break;
			case 13:		// \r
				type = pcodepoint == 10 ? CodepointType::Space : CodepointType::Newline;
				break;
			case 0x0085:	// NEL
				type = CodepointType::Newline;
				break;
			default:
				if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) ||
					(iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) ||
					(iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
					(iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) ||
					(iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) ||
					(iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
					type = CodepointType::CjkChar;
				else
					type = CodepointType::Char;
				break;
		}

		if (type == CodepointType::Newline) {
			// Always handle new lines.
			rows[nrows].start = rowStart != NULL ? rowStart : iter.str;
			rows[nrows].end = rowEnd != NULL ? rowEnd : iter.str;
			rows[nrows].width = rowWidth * invscale;
			rows[nrows].minx = rowMinX * invscale;
			rows[nrows].maxx = rowMaxX * invscale;
			rows[nrows].next = iter.next;
			nrows++;
			if (nrows >= maxRows)
				return nrows;
			// Set null break point
			breakEnd = rowStart;
			breakWidth = 0.0;
			breakMaxX = 0.0;
			// Indicate to skip the white space at the beginning of the row.
			rowStart = NULL;
			rowEnd = NULL;
			rowWidth = 0;
			rowMinX = rowMaxX = 0;
		} else {
			if (rowStart == NULL) {
				// Skip white space until the beginning of the line
				if (type == CodepointType::Char || type == CodepointType::CjkChar) {
					// The current char is the row so far
					rowStartX = iter.x;
					rowStart = iter.str;
					rowEnd = iter.next;
					rowWidth = iter.nextx - rowStartX;
					rowMinX = q.x0 - rowStartX;
					rowMaxX = q.x1 - rowStartX;
					wordStart = iter.str;
					wordStartX = iter.x;
					wordMinX = q.x0 - rowStartX;
					// Set null break point
					breakEnd = rowStart;
					breakWidth = 0.0;
					breakMaxX = 0.0;
				}
			} else {
				float nextWidth = iter.nextx - rowStartX;

				// track last non-white space character
				if (type == CodepointType::Char || type == CodepointType::CjkChar) {
					rowEnd = iter.next;
					rowWidth = iter.nextx - rowStartX;
					rowMaxX = q.x1 - rowStartX;
				}
				// track last end of a word
				if (((ptype == CodepointType::Char || ptype == CodepointType::CjkChar) && type == CodepointType::Space) || type == CodepointType::CjkChar) {
					breakEnd = iter.str;
					breakWidth = rowWidth;
					breakMaxX = rowMaxX;
				}
				// track last beginning of a word
				if ((ptype == CodepointType::Space && (type == CodepointType::Char || type == CodepointType::CjkChar)) || type == CodepointType::CjkChar) {
					wordStart = iter.str;
					wordStartX = iter.x;
					wordMinX = q.x0;
				}

				// Break to new line when a character is beyond break width.
				if ((type == CodepointType::Char || type == CodepointType::CjkChar) && nextWidth > breakRowWidth) {
					// The run length is too long, need to break to new line.
					if (breakEnd == rowStart) {
						// The current word is longer than the row length, just break it from here.
						rows[nrows].start = rowStart;
						rows[nrows].end = iter.str;
						rows[nrows].width = rowWidth * invscale;
						rows[nrows].minx = rowMinX * invscale;
						rows[nrows].maxx = rowMaxX * invscale;
						rows[nrows].next = iter.str;
						nrows++;
						if (nrows >= maxRows)
							return nrows;
						rowStartX = iter.x;
						rowStart = iter.str;
						rowEnd = iter.next;
						rowWidth = iter.nextx - rowStartX;
						rowMinX = q.x0 - rowStartX;
						rowMaxX = q.x1 - rowStartX;
						wordStart = iter.str;
						wordStartX = iter.x;
						wordMinX = q.x0 - rowStartX;
					} else {
						// Break the line from the end of the last word, and start new line from the beginning of the new.
						rows[nrows].start = rowStart;
						rows[nrows].end = breakEnd;
						rows[nrows].width = breakWidth * invscale;
						rows[nrows].minx = rowMinX * invscale;
						rows[nrows].maxx = breakMaxX * invscale;
						rows[nrows].next = wordStart;
						nrows++;
						if (nrows >= maxRows)
							return nrows;
						// Update row
						rowStartX = wordStartX;
						rowStart = wordStart;
						rowEnd = iter.next;
						rowWidth = iter.nextx - rowStartX;
						rowMinX = wordMinX - rowStartX;
						rowMaxX = q.x1 - rowStartX;
					}
					// Set null break point
					breakEnd = rowStart;
					breakWidth = 0.0;
					breakMaxX = 0.0;
				}
			}
		}

		pcodepoint = iter.codepoint;
		ptype = type;
	}

	// Break the line from the end of the last word, and start new line from the beginning of the new.
	if (rowStart != NULL) {
		rows[nrows].start = rowStart;
		rows[nrows].end = rowEnd;
		rows[nrows].width = rowWidth * invscale;
		rows[nrows].minx = rowMinX * invscale;
		rows[nrows].maxx = rowMaxX * invscale;
		rows[nrows].next = end;
		nrows++;
	} else if (!skipSpaces) {
		rows[nrows].start = end;
		rows[nrows].end = end;
		rows[nrows].width = rowWidth * invscale;
		rows[nrows].minx = rowMinX * invscale;
		rows[nrows].maxx = rowMaxX * invscale;
		rows[nrows].next = end;
		nrows++;
	}

	return nrows;
}

float textBounds(Context& ctx, float x, float y, const char* string, const char* end, float* bounds)
{
	State& state = detail::getState(ctx);
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;
	float width;

	if (state.fontId == FONS_INVALID) return 0;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetDilate(ctx.fs, state.fontDilate*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);

	width = fonsTextBounds(ctx.fs, 0, 0, string, end, bounds);
	if (bounds != NULL) {
		// Use line bounds for height.
		fonsLineBounds(ctx.fs, 0, &bounds[1], &bounds[3]);
		bounds[0] = bounds[0] * invscale + x;
		bounds[1] = bounds[1] * invscale + y;
		bounds[2] = bounds[2] * invscale + x;
		bounds[3] = bounds[3] * invscale + y;
	}
	return width * invscale;
}

void textBoxBounds(Context& ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{
	State& state = detail::getState(ctx);
	std::array<TextRow, 2> rows{};
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;
	float yoff = 0;
	int nrows = 0, i;
	int oldAlign = state.textAlign;
	int halign = state.textAlign & static_cast<int>(Align::Left | Align::Center | Align::Right);
	int valign = state.textAlign & static_cast<int>(Align::Top | Align::Middle | Align::MiddleAscent | Align::Bottom | Align::Baseline);
	float lineh = 0, rminy = 0, rmaxy = 0;
	float minx, miny, maxx, maxy;

	if (state.fontId == FONS_INVALID) {
		if (bounds != NULL)
			bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		return;
	}

	textMetrics(ctx, NULL, NULL, &lineh);

	state.textAlign = static_cast<int>(Align::Left) | valign;

	minx = maxx = 0;
	miny = maxy = 0;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetDilate(ctx.fs, state.fontDilate*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);
	fonsLineBounds(ctx.fs, 0, &rminy, &rmaxy);
	rminy *= invscale;
	rmaxy *= invscale;

	while ((nrows = textBreakLines(ctx, string, end, breakRowWidth, rows.data(), (int)rows.size(), 0))) {
		for (i = 0; i < nrows; i++) {
			TextRow* row = &rows[(size_t)i];
			float rminx, rmaxx, dx = 0;
			// Horizontal bounds
			if (halign & Align::Left)
				dx = 0;
			else if (halign & Align::Center)
				dx = breakRowWidth*0.5f - row->width*0.5f;
			else if (halign & Align::Right)
				dx = breakRowWidth - row->width;
			rminx = row->minx + dx;
			rmaxx = row->maxx + dx;
			minx = detail::minf(minx, rminx);
			maxx = detail::maxf(maxx, rmaxx);
			// Vertical bounds.
			miny = detail::minf(miny, yoff + rminy);
			maxy = detail::maxf(maxy, yoff + rmaxy);

			yoff += lineh * state.lineHeight;
		}
		string = rows[(size_t)nrows-1].next;
	}

	state.textAlign = oldAlign;

	if (bounds != NULL) {
		bounds[0] = minx + x;
		bounds[1] = miny + y;
		bounds[2] = maxx + x;
		bounds[3] = maxy + y;
	}
}

void textMetrics(Context& ctx, float* ascender, float* descender, float* lineh)
{
	State& state = detail::getState(ctx);
	float scale = detail::getFontScale(&state) * ctx.devicePxRatio;
	float invscale = 1.0f / scale;

	if (state.fontId == FONS_INVALID) return;

	fonsSetSize(ctx.fs, state.fontSize*scale);
	fonsSetSpacing(ctx.fs, state.letterSpacing*scale);
	fonsSetBlur(ctx.fs, state.fontBlur*scale);
	fonsSetDilate(ctx.fs, state.fontDilate*scale);
	fonsSetAlign(ctx.fs, state.textAlign);
	fonsSetFont(ctx.fs, state.fontId);

	fonsVertMetrics(ctx.fs, ascender, descender, lineh);
	if (ascender != NULL)
		*ascender *= invscale;
	if (descender != NULL)
		*descender *= invscale;
	if (lineh != NULL)
		*lineh *= invscale;
}
}
