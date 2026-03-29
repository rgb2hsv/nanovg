#include "nanovg_detail.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace nvg
{
namespace detail
{

float modf(float a, float b)
{
    return ::fmodf(a, b);
}
float sinf(float a)
{
    return ::sinf(a);
}
float cosf(float a)
{
    return ::cosf(a);
}
float tanf(float a)
{
    return ::tanf(a);
}
float atan2f(float a, float b)
{
    return ::atan2f(a, b);
}
float acosf(float a)
{
    return ::acosf(a);
}
int mini(int a, int b)
{
    return a < b ? a : b;
}
int maxi(int a, int b)
{
    return a > b ? a : b;
}
int clampi(int a, int mn, int mx)
{
    return a < mn ? mn : (a > mx ? mx : a);
}
float minf(float a, float b)
{
    return a < b ? a : b;
}
float maxf(float a, float b)
{
    return a > b ? a : b;
}
float absf(float a)
{
    return a >= 0.0f ? a : -a;
}
float signf(float a)
{
    return a >= 0.0f ? 1.0f : -1.0f;
}
float clampf(float a, float mn, float mx)
{
    return a < mn ? mn : (a > mx ? mx : a);
}
float cross(float dx0, float dy0, float dx1, float dy1)
{
    return dx1 * dy0 - dx0 * dy1;
}

float rsqrtf(float x)
{
    float xhalf = 0.5f * x;
    uint32_t i = std::bit_cast<uint32_t>(x);
    i = 0x5f3759df - (i >> 1);
    float y = std::bit_cast<float>(i);
    y = y * (1.5f - (xhalf * y * y));
    return y;
}

float sqrtf(float a)
{
#if defined(NANOVG_FAST_SQRT)
    return 1.0f / std::max(std::numeric_limits<float>::epsilon(), rsqrtf(a));
#else
    return std::sqrt(a);
#endif
}

float normalize(float& x, float& y)
{
#if defined(NANOVG_FAST_SQRT)
    float id = rsqrtf(x * x + y * y);
    x *= id;
    y *= id;
    return 1.0f / std::max(std::numeric_limits<float>::epsilon(), id);
#else
    float d = sqrtf(x * x + y * y);
    if (d > std::numeric_limits<float>::epsilon()) {
        d = 1.0f / d;
        x *= d;
        y *= d;
    }
    return d;
#endif
}

std::shared_ptr<PathCache> allocPathCache()
{
    auto c = std::make_shared<PathCache>();

    c->points.reserve(static_cast<size_t>(NVG_INIT_POINTS_SIZE));

    c->paths.reserve(static_cast<size_t>(NVG_INIT_PATHS_SIZE));

    c->verts.reserve(static_cast<size_t>(NVG_INIT_VERTS_SIZE));

    return c;
}

CompositeOperationState compositeOperationState(int op)
{
    int sfactor, dfactor;

    if (op == static_cast<int>(CompositeOperation::SourceOver)) {
        sfactor = static_cast<int>(BlendFactor::One);
        dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
    } else if (op == static_cast<int>(CompositeOperation::SourceIn)) {
        sfactor = static_cast<int>(BlendFactor::DstAlpha);
        dfactor = static_cast<int>(BlendFactor::Zero);
    } else if (op == static_cast<int>(CompositeOperation::SourceOut)) {
        sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
        dfactor = static_cast<int>(BlendFactor::Zero);
    } else if (op == static_cast<int>(CompositeOperation::Atop)) {
        sfactor = static_cast<int>(BlendFactor::DstAlpha);
        dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
    } else if (op == static_cast<int>(CompositeOperation::DestinationOver)) {
        sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
        dfactor = static_cast<int>(BlendFactor::One);
    } else if (op == static_cast<int>(CompositeOperation::DestinationIn)) {
        sfactor = static_cast<int>(BlendFactor::Zero);
        dfactor = static_cast<int>(BlendFactor::SrcAlpha);
    } else if (op == static_cast<int>(CompositeOperation::DestinationOut)) {
        sfactor = static_cast<int>(BlendFactor::Zero);
        dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
    } else if (op == static_cast<int>(CompositeOperation::DestinationAtop)) {
        sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
        dfactor = static_cast<int>(BlendFactor::SrcAlpha);
    } else if (op == static_cast<int>(CompositeOperation::Lighter)) {
        sfactor = static_cast<int>(BlendFactor::One);
        dfactor = static_cast<int>(BlendFactor::One);
    } else if (op == static_cast<int>(CompositeOperation::Copy)) {
        sfactor = static_cast<int>(BlendFactor::One);
        dfactor = static_cast<int>(BlendFactor::Zero);
    } else if (op == static_cast<int>(CompositeOperation::Xor)) {
        sfactor = static_cast<int>(BlendFactor::OneMinusDstAlpha);
        dfactor = static_cast<int>(BlendFactor::OneMinusSrcAlpha);
    } else {
        sfactor = static_cast<int>(BlendFactor::One);
        dfactor = static_cast<int>(BlendFactor::Zero);
    }

    CompositeOperationState state{};
    state.srcRGB = sfactor;
    state.dstRGB = dfactor;
    state.srcAlpha = sfactor;
    state.dstAlpha = dfactor;
    return state;
}

float hue(float h, float m1, float m2)
{
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    if (h < 1.0f / 6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    else if (h < 3.0f / 6.0f)
        return m2;
    else if (h < 4.0f / 6.0f)
        return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
    return m1;
}

void setPaintColor(Paint& p, const Color& color)
{
    p = Paint{};
    transformIdentity(p.xform);
    p.radius = 0.0f;
    p.feather = 1.0f;
    p.innerColor = color;
    p.outerColor = color;
}

void isectRects(float* dst, float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    float minx = maxf(ax, bx);
    float miny = maxf(ay, by);
    float maxx = minf(ax + aw, bx + bw);
    float maxy = minf(ay + ah, by + bh);
    dst[0] = minx;
    dst[1] = miny;
    dst[2] = maxf(0.0f, maxx - minx);
    dst[3] = maxf(0.0f, maxy - miny);
}

int ptEquals(float x1, float y1, float x2, float y2, float tol)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy < tol * tol;
}

float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
    float pqx, pqy, dx, dy, d, t;
    pqx = qx - px;
    pqy = qy - py;
    dx = x - px;
    dy = y - py;
    d = pqx * pqx + pqy * pqy;
    t = pqx * dx + pqy * dy;
    if (d > 0) t /= d;
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;
    dx = px + t * pqx - x;
    dy = py + t * pqy - y;
    return dx * dx + dy * dy;
}

float getAverageScale(const float* t)
{
    float sx = sqrtf(t[0] * t[0] + t[2] * t[2]);
    float sy = sqrtf(t[1] * t[1] + t[3] * t[3]);
    return (sx + sy) * 0.5f;
}

float triarea2(float ax, float ay, float bx, float by, float cx, float cy)
{
    float abx = bx - ax;
    float aby = by - ay;
    float acx = cx - ax;
    float acy = cy - ay;
    return acx * aby - abx * acy;
}

float polyArea(Point* pts, int npts)
{
    int i;
    float area = 0;
    for (i = 2; i < npts; i++) {
        Point* a = &pts[0];
        Point* b = &pts[i - 1];
        Point* c = &pts[i];
        area += triarea2(a->x, a->y, b->x, b->y, c->x, c->y);
    }
    return area * 0.5f;
}

void polyReverse(Point* pts, int npts)
{
    Point tmp;
    int i = 0, j = npts - 1;
    while (i < j) {
        tmp = pts[i];
        pts[i] = pts[j];
        pts[j] = tmp;
        i++;
        j--;
    }
}

void pushVertex(std::vector<Vertex>& vs, float x, float y, float u, float v, float s, float t)
{
    vs.push_back(Vertex{x, y, u, v, s, t});
}

int curveDivs(float r, float arc, float tol)
{
    float da = acosf(r / (r + tol)) * 2.0f;
    return maxi(2, static_cast<int>(std::ceil(arc / da)));
}

void chooseBevel(int bevel, Point* p0, Point* p1, float w, float* x0, float* y0, float* x1, float* y1)
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

void roundJoin(std::vector<Vertex>& out, Point* p0, Point* p1, float lw, float rw, float lu, float ru, int ncap, float fringe, float t)
{
    int i, n;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;
    UNUSED(fringe);

    if (p1->flags & PointFlags::Left) {
        float lx0, ly0, lx1, ly1, a0, a1;
        chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);
        a0 = atan2f(-dly0, -dlx0);
        a1 = atan2f(-dly1, -dlx1);
        if (a1 > a0) a1 -= nvgPi * 2.0f;

        pushVertex(out, lx0, ly0, lu, 1, -1, t);
        pushVertex(out, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1, 1, t);

        n = clampi(static_cast<int>(std::ceil(((a0 - a1) / nvgPi) * ncap)), 2, ncap);
        for (i = 0; i < n; i++) {
            float u = i / (float)(n - 1);
            float a = a0 + u * (a1 - a0);
            float rx = p1->x + cosf(a) * rw;
            float ry = p1->y + sinf(a) * rw;
            pushVertex(out, p1->x, p1->y, 0.5f, 1, 0, t);
            pushVertex(out, rx, ry, ru, 1, 1, t);
        }

        pushVertex(out, lx1, ly1, lu, 1, -1, t);
        pushVertex(out, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1, 1, t);

    } else {
        float rx0, ry0, rx1, ry1, a0, a1;
        chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);
        a0 = atan2f(dly0, dlx0);
        a1 = atan2f(dly1, dlx1);
        if (a1 < a0) a1 += nvgPi * 2.0f;

        pushVertex(out, p1->x + dlx0 * rw, p1->y + dly0 * rw, lu, 1, -1, t);
        pushVertex(out, rx0, ry0, ru, 1, 1, t);

        n = clampi(static_cast<int>(std::ceil(((a1 - a0) / nvgPi) * ncap)), 2, ncap);
        for (i = 0; i < n; i++) {
            float u = i / (float)(n - 1);
            float a = a0 + u * (a1 - a0);
            float lx = p1->x + cosf(a) * lw;
            float ly = p1->y + sinf(a) * lw;
            pushVertex(out, lx, ly, lu, 1, 1, t);
            pushVertex(out, p1->x, p1->y, 0.5f, 1, 0.0f, t);
        }

        pushVertex(out, p1->x + dlx1 * rw, p1->y + dly1 * rw, lu, 1, -1, t);
        pushVertex(out, rx1, ry1, ru, 1, 1, t);
    }
}

void bevelJoin(std::vector<Vertex>& out, Point* p0, Point* p1, float lw, float rw, float lu, float ru, float fringe, float t)
{
    float rx0, ry0, rx1, ry1;
    float lx0, ly0, lx1, ly1;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;
    UNUSED(fringe);

    if (p1->flags & PointFlags::Left) {
        chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);

        pushVertex(out, lx0, ly0, lu, 1, -1, t);
        pushVertex(out, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1, 1, t);

        if (p1->flags & PointFlags::Bevel) {
            pushVertex(out, lx0, ly0, lu, 1, -1, t);
            pushVertex(out, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1, 1, t);

            pushVertex(out, lx1, ly1, lu, 1, -1, t);
            pushVertex(out, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1, 1, t);
        } else {
            rx0 = p1->x - p1->dmx * rw;
            ry0 = p1->y - p1->dmy * rw;

            pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
            pushVertex(out, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1, 1, t);

            pushVertex(out, rx0, ry0, ru, 1, -1, t);
            pushVertex(out, rx0, ry0, ru, 1, 1, t);

            pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
            pushVertex(out, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1, 1, t);
        }

        pushVertex(out, lx1, ly1, lu, 1, -1, t);
        pushVertex(out, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1, 1, t);

    } else {
        chooseBevel(p1->flags & PointFlags::InnerBevel, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);

        pushVertex(out, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1, 1, t);
        pushVertex(out, rx0, ry0, ru, 1, -1, t);

        if (p1->flags & PointFlags::Bevel) {
            pushVertex(out, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1, 1, t);
            pushVertex(out, rx0, ry0, ru, 1, -1, t);

            pushVertex(out, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1, 1, t);
            pushVertex(out, rx1, ry1, ru, 1, -1, t);
        } else {
            lx0 = p1->x + p1->dmx * lw;
            ly0 = p1->y + p1->dmy * lw;

            pushVertex(out, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1, 1, t);
            pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);

            pushVertex(out, lx0, ly0, lu, 1, 1, t);
            pushVertex(out, lx0, ly0, lu, 1, -1, t);

            pushVertex(out, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1, 1, t);
            pushVertex(out, p1->x, p1->y, 0.5f, 1, -1, t);
        }

        pushVertex(out, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1, 1, t);
        pushVertex(out, rx1, ry1, ru, 1, -1, t);
    }
}

void insertSpacer(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, float u0, float u1, float t)
{
    float dlx = dy;
    float dly = -dx;
    float px = p->x;
    float py = p->y;
    pushVertex(out, px + dlx * w, py + dly * w, u0, 1, -1, t);
    pushVertex(out, px - dlx * w, py - dly * w, u1, 1, 1, t);
}

void buttCapStart(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, float d, float aa, float u0, float u1, float t, int dir)
{
    float px = p->x - dx * d;
    float py = p->y - dy * d;
    float dlx = dy;
    float dly = -dx;
    pushVertex(out, px + dlx * w - dx * aa, py + dly * w - dy * aa, u0, 0, -1, t - dir * aa / w);
    pushVertex(out, px - dlx * w - dx * aa, py - dly * w - dy * aa, u1, 0, 1, t - dir * aa / w);
    pushVertex(out, px + dlx * w, py + dly * w, u0, 1, -1, t);
    pushVertex(out, px - dlx * w, py - dly * w, u1, 1, 1, t);
}

void buttCapEnd(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, float d, float aa, float u0, float u1, float t, int dir)
{
    float px = p->x + dx * d;
    float py = p->y + dy * d;
    float dlx = dy;
    float dly = -dx;
    pushVertex(out, px + dlx * w, py + dly * w, u0, 1, -1, t);
    pushVertex(out, px - dlx * w, py - dly * w, u1, 1, 1, t);
    pushVertex(out, px + dlx * w + dx * aa, py + dly * w + dy * aa, u0, 0, -1, t + dir * aa / w);
    pushVertex(out, px - dlx * w + dx * aa, py - dly * w + dy * aa, u1, 0, 1, t + dir * aa / w);
}

void roundCapStart(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1, float t, int dir)
{
    int i;
    float px = p->x;
    float py = p->y;
    float dlx = dy;
    float dly = -dx;
    UNUSED(aa);
    for (i = 0; i < ncap; i++) {
        const float a = i / (float)(ncap - 1) * nvgPi;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        pushVertex(out, px - dlx * ax - dx * ay, py - dly * ax - dy * ay, u0, 1, ax / w, t - dir * ay / w);
        pushVertex(out, px, py, 0.5f, 1, 0, t);
    }
    pushVertex(out, px + dlx * w, py + dly * w, u0, 1, 1, t);
    pushVertex(out, px - dlx * w, py - dly * w, u1, 1, -1, t);
}

void roundCapEnd(std::vector<Vertex>& out, Point* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1, float t, int dir)
{
    int i;
    float px = p->x;
    float py = p->y;
    float dlx = dy;
    float dly = -dx;
    UNUSED(aa);
    pushVertex(out, px + dlx * w, py + dly * w, u0, 1, w, t);
    pushVertex(out, px - dlx * w, py - dly * w, u1, 1, -w, t);
    for (i = 0; i < ncap; i++) {
        float a = i / (float)(ncap - 1) * nvgPi;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        pushVertex(out, px, py, 0.5f, 1, 0, t);
        pushVertex(out, px - dlx * ax + dx * ay, py - dly * ax + dy * ay, u0, 1, ax / w, t + dir * ay / w);
    }
}

float quantize(float a, float d)
{
    return (static_cast<int>(a / d + 0.5f)) * d;
}

float getFontScale(const State* state)
{
    return minf(quantize(getAverageScale(state->xform.data()), 0.01f), 4.0f);
}

int isTransformFlipped(const float* xform)
{
    float det = xform[0] * xform[3] - xform[2] * xform[1];
    return (det < 0);
}

}  // namespace detail
}  // namespace nvg
