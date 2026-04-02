//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//

#include "nanovg_context.hpp"
#include "nanovg_detail.hpp"

#include <array>
#include <vector>

#include "../thirdParty/fontstash.h"

namespace nvg
{

State& ContextImpl::topState()
{
    return states.back();
}

const State& ContextImpl::topState() const
{
    return states.back();
}

void ContextImpl::setDevicePixelRatio(float ratio)
{
    tessTol = 0.25f / ratio;
    distTol = 0.01f / ratio;
    fringeWidth = 1.0f / ratio;
    devicePxRatio = ratio;
}

void ContextImpl::clearPathCache()
{
    cache->points.clear();
    cache->paths.clear();
}

Path* ContextImpl::lastPath()
{
    std::vector<Path>& paths = cache->paths;
    if (!paths.empty()) return &paths.back();
    return nullptr;
}

void ContextImpl::appendPath()
{
    std::vector<Path>& paths = cache->paths;
    const int nextCount = static_cast<int>(paths.size()) + 1;
    if (static_cast<size_t>(nextCount) > paths.capacity()) {
        const int newCap = nextCount + static_cast<int>(paths.capacity() / 2);
        paths.reserve(static_cast<size_t>(newCap));
    }
    paths.emplace_back();
    Path* path = &paths.back();
    *path = Path{};
    path->first = static_cast<int>(cache->points.size());
    path->winding = static_cast<int>(Winding::CCW);
}

Point* ContextImpl::lastPoint()
{
    std::vector<Point>& points = cache->points;
    if (!points.empty()) return &points.back();
    return nullptr;
}

void ContextImpl::addPoint(float x, float y, int flags)
{
    Path* path = lastPath();
    if (path == nullptr) return;

    std::vector<Point>& points = cache->points;

    if (path->count > 0 && !points.empty()) {
        Point* pt = lastPoint();
        if (detail::ptEquals(pt->x, pt->y, x, y, distTol)) {
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
    *pt = Point{};
    pt->x = x;
    pt->y = y;
    pt->flags = static_cast<unsigned char>(flags);

    path->count++;
}

void ContextImpl::markCachedPathClosed()
{
    Path* path = lastPath();
    if (path == nullptr) return;
    path->closed = 1;
}

void ContextImpl::setCachedPathWinding(int winding)
{
    Path* path = lastPath();
    if (path == nullptr) return;
    path->winding = winding;
}

void ContextImpl::prepareTempVerts(int nverts)
{
    std::vector<Vertex>& verts = cache->verts;
    verts.clear();
    if (static_cast<size_t>(nverts) > verts.capacity()) {
        const int cverts = (nverts + 0xff) & ~0xff;  // Round up to prevent allocations when things change just slightly.
        verts.reserve(static_cast<size_t>(cverts));
    }
}

void ContextImpl::tesselateBezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int level, int type)
{
    float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
    float dx, dy, d2, d3;

    if (level > 10) return;

    x12 = (x1 + x2) * 0.5f;
    y12 = (y1 + y2) * 0.5f;
    x23 = (x2 + x3) * 0.5f;
    y23 = (y2 + y3) * 0.5f;
    x34 = (x3 + x4) * 0.5f;
    y34 = (y3 + y4) * 0.5f;
    x123 = (x12 + x23) * 0.5f;
    y123 = (y12 + y23) * 0.5f;

    dx = x4 - x1;
    dy = y4 - y1;
    d2 = detail::absf(((x2 - x4) * dy - (y2 - y4) * dx));
    d3 = detail::absf(((x3 - x4) * dy - (y3 - y4) * dx));

    if ((d2 + d3) * (d2 + d3) < tessTol * (dx * dx + dy * dy)) {
        addPoint(x4, y4, type);
        return;
    }

    /*	if (absf(x1+x3-x2-x2) + absf(y1+y3-y2-y2) + absf(x2+x4-x3-x3) + absf(y2+y4-y3-y3) < tessTol) {
                    addPoint(x4, y4, type);
                    return;
            }*/

    x234 = (x23 + x34) * 0.5f;
    y234 = (y23 + y34) * 0.5f;
    x1234 = (x123 + x234) * 0.5f;
    y1234 = (y123 + y234) * 0.5f;

    tesselateBezier(x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0);
    tesselateBezier(x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type);
}

void ContextImpl::flattenPaths()
{
    // State* state = topState();
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

    if (!cache->paths.empty()) return;

    // Flatten
    i = 0;
    while (static_cast<size_t>(i) < commands.size()) {
        int cmd = (int)commands[i];
        switch (cmd) {
            case static_cast<int>(PathCommand::MoveTo):
                appendPath();
                p = &commands[i + 1];
                addPoint(p[0], p[1], static_cast<int>(PointFlags::Corner));
                i += 3;
                break;
            case static_cast<int>(PathCommand::LineTo):
                p = &commands[i + 1];
                addPoint(p[0], p[1], static_cast<int>(PointFlags::Corner));
                i += 3;
                break;
            case static_cast<int>(PathCommand::BezierTo):
                last = lastPoint();
                if (last != NULL) {
                    cp1 = &commands[i + 1];
                    cp2 = &commands[i + 3];
                    p = &commands[i + 5];
                    tesselateBezier(last->x, last->y, cp1[0], cp1[1], cp2[0], cp2[1], p[0], p[1], 0, static_cast<int>(PointFlags::Corner));
                }
                i += 7;
                break;
            case static_cast<int>(PathCommand::Close):
                markCachedPathClosed();
                i++;
                break;
            case static_cast<int>(PathCommand::Winding):
                setCachedPathWinding((int)commands[i + 1]);
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
        p0 = &pts[path->count - 1];
        p1 = &pts[0];
        if (detail::ptEquals(p0->x, p0->y, p1->x, p1->y, distTol)) {
            path->count--;
            p0 = &pts[path->count - 1];
            path->closed = 1;
        }

        // Enforce winding.
        path->reversed = 0;
        if (path->count > 2) {
            area = detail::polyArea(pts, path->count);
            if (path->winding == static_cast<int>(Winding::CCW) && area < 0.0f) {
                detail::polyReverse(pts, path->count);
                path->reversed = 1;
            }
            if (path->winding == static_cast<int>(Winding::CW) && area > 0.0f) {
                detail::polyReverse(pts, path->count);
                path->reversed = 1;
            }
        }

        for (i = 0; i < path->count; i++) {
            // Calculate segment direction and length
            p0->dx = p1->x - p0->x;
            p0->dy = p1->y - p0->y;
            p0->len = detail::normalize(p0->dx, p0->dy);
            // Update bounds
            cache->bounds[0] = detail::minf(cache->bounds[0], p0->x);
            cache->bounds[1] = detail::minf(cache->bounds[1], p0->y);
            cache->bounds[2] = detail::maxf(cache->bounds[2], p0->x);
            cache->bounds[3] = detail::maxf(cache->bounds[3], p0->y);
            // Advance
            p0 = p1++;
        }
    }
}

void ContextImpl::calculateJoins(float w, int lineJoin, float miterLimit)
{
    int i, j;
    float iw = 0.0f;

    if (w > 0.0f) iw = 1.0f / w;

    // Calculate which joins needs extra vertices to append, and gather vertex count.
    for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
        Path* path = &cache->paths[i];
        Point* pts = &cache->points[path->first];
        Point* p0 = &pts[path->count - 1];
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
            dmr2 = p1->dmx * p1->dmx + p1->dmy * p1->dmy;
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
            limit = detail::maxf(1.01f, detail::minf(p0->len, p1->len) * iw);
            if ((dmr2 * limit * limit) < 1.0f) p1->flags |= PointFlags::InnerBevel;

            // Check to see if the corner needs to be beveled.
            if (p1->flags & PointFlags::Corner) {
                if ((dmr2 * miterLimit * miterLimit) < 1.0f || lineJoin == static_cast<int>(LineCap::Bevel) || lineJoin == static_cast<int>(LineCap::Round)) {
                    p1->flags |= PointFlags::Bevel;
                }
            }

            if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0) path->nbevel++;

            p0 = p1++;
        }

        path->convex = (nleft == path->count) ? 1 : 0;
    }
}

int ContextImpl::expandStroke(float w, float fringe, int lineCap, int lineJoin, int lineStyle, float miterLimit)
{
    std::vector<Vertex>& buf = cache->verts;
    int cverts, i, j;
    float t;
    float aa = fringe;  // fringeWidth;
    float u0 = 0.0f, u1 = 1.0f;
    int ncap = detail::curveDivs(w, nvgPi, tessTol);  // Calculate divisions per half circle.

    w += aa * 0.5f;
    const float invStrokeWidth = 1.0f / w;
    // Disable the gradient used for antialiasing when antialiasing is not used.
    if (aa == 0.0f) {
        u0 = 0.5f;
        u1 = 0.5f;
    }

    // Force round join to minimize distortion
    if (lineStyle > 1) lineJoin = static_cast<int>(LineCap::Round);

    calculateJoins(w, lineJoin, miterLimit);
    // Calculate max vertex usage.
    cverts = 0;
    for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
        Path* path = &cache->paths[i];
        int loop = (path->closed == 0) ? 0 : 1;
        if (lineJoin == static_cast<int>(LineCap::Round)) {
            cverts += (path->count + path->nbevel * (ncap + 2) + 1) * 2;  // plus one for loop
        } else {
            cverts += (path->count + path->nbevel * 5 + 1) * 2;  // plus one for loop
        }
        if (lineStyle > 1) cverts += 4 * path->count;  // extra vertices for spacers
        if (loop == 0) {
            // space for caps
            if (lineCap == static_cast<int>(LineCap::Round)) {
                cverts += (ncap * 2 + 2) * 2;
            } else {
                cverts += (3 + 3) * 2;
            }
        }
    }

    prepareTempVerts(cverts);

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
            p0 = &pts[path->count - 1];
            p1 = &pts[0];
            s = 0;
            e = path->count;
        } else {
            // Add cap
            p0 = &pts[0];
            p1 = &pts[1];
            s = 1;
            e = path->count - 1;
        }

        t = 0;

        int dir = 1;
        if (lineStyle > 1 && path->reversed) {
            dir = -1;
            for (j = s; j < path->count; ++j) {
                dx = p1->x - p0->x;
                dy = p1->y - p0->y;
                t += detail::normalize(dx, dy) * invStrokeWidth;
                p0 = p1++;
            }
            if (loop) {
                // Looping
                p0 = &pts[path->count - 1];
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
            detail::normalize(dx, dy);
            if (lineCap == static_cast<int>(LineCap::Butt))
                detail::buttCapStart(buf, p0, dx, dy, w, -aa * 0.5f, aa, u0, u1, t, dir);
            else if (lineCap == static_cast<int>(LineCap::Butt) || lineCap == static_cast<int>(LineCap::Square))
                detail::buttCapStart(buf, p0, dx, dy, w, w - aa, aa, u0, u1, t, dir);
            else if (lineCap == static_cast<int>(LineCap::Round))
                detail::roundCapStart(buf, p0, dx, dy, w, ncap, aa, u0, u1, t, dir);
        }
        for (j = s; j < e; ++j) {
            if (lineStyle > 1) {
                dx = p1->x - p0->x;
                dy = p1->y - p0->y;
                float dt = detail::normalize(dx, dy);
                detail::insertSpacer(buf, p0, dx, dy, w, u0, u1, t);
                t += dir * dt * invStrokeWidth;
                detail::insertSpacer(buf, p1, dx, dy, w, u0, u1, t);
            }
            if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0) {
                if (lineJoin == static_cast<int>(LineCap::Round)) {
                    detail::roundJoin(buf, p0, p1, w, w, u0, u1, ncap, aa, t);
                } else {
                    detail::bevelJoin(buf, p0, p1, w, w, u0, u1, aa, t);
                }
            } else {
                detail::pushVertex(buf, p1->x + (p1->dmx * w), p1->y + (p1->dmy * w), u0, 1, -1, t);
                detail::pushVertex(buf, p1->x - (p1->dmx * w), p1->y - (p1->dmy * w), u1, 1, 1, t);
            }
            p0 = p1++;
        }

        if (loop) {
            // Loop it
            detail::pushVertex(buf, buf[strokeStart].x, buf[strokeStart].y, u0, 1, -1, t);
            detail::pushVertex(buf, buf[strokeStart + 1].x, buf[strokeStart + 1].y, u1, 1, 1, t);
        } else {
            dx = p1->x - p0->x;
            dy = p1->y - p0->y;
            float dt = detail::normalize(dx, dy);
            if (lineStyle > 1) {
                detail::insertSpacer(buf, p0, dx, dy, w, u0, u1, t);
                t += dir * dt * invStrokeWidth;
                detail::insertSpacer(buf, p1, dx, dy, w, u0, u1, t);
            }
            // Add cap
            if (lineCap == static_cast<int>(LineCap::Butt))
                detail::buttCapEnd(buf, p1, dx, dy, w, -aa * 0.5f, aa, u0, u1, t, dir);
            else if (lineCap == static_cast<int>(LineCap::Butt) || lineCap == static_cast<int>(LineCap::Square))
                detail::buttCapEnd(buf, p1, dx, dy, w, w - aa, aa, u0, u1, t, dir);
            else if (lineCap == static_cast<int>(LineCap::Round))
                detail::roundCapEnd(buf, p1, dx, dy, w, ncap, aa, u0, u1, t, dir);
        }
        path->stroke = buf.data() + strokeStart;
        path->nstroke = static_cast<int>(buf.size() - strokeStart);
    }

    return 1;
}

int ContextImpl::expandFill(float w, int lineJoin, float miterLimit)
{
    std::vector<Vertex>& buf = cache->verts;
    int cverts, convex, i, j;
    float aa = fringeWidth;
    int fringe = w > 0.0f;

    calculateJoins(w, lineJoin, miterLimit);

    // Calculate max vertex usage.
    cverts = 0;
    for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
        Path* path = &cache->paths[i];
        cverts += path->count + path->nbevel + 1;
        if (fringe) cverts += (path->count + path->nbevel * 5 + 1) * 2;  // plus one for loop
    }

    prepareTempVerts(cverts);

    convex = cache->paths.size() == 1 && cache->paths[0].convex;

    for (i = 0; i < static_cast<int>(cache->paths.size()); i++) {
        Path* path = &cache->paths[i];
        Point* pts = &cache->points[path->first];
        Point* p0;
        Point* p1;
        float rw, lw, woff;
        float ru, lu;

        // Calculate shape vertices.
        woff = 0.5f * aa;
        const size_t fillStart = buf.size();

        if (fringe) {
            // Looping
            p0 = &pts[path->count - 1];
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
                        detail::pushVertex(buf, lx, ly, 0.5f, 1, 0, 0);
                    } else {
                        float lx0 = p1->x + dlx0 * woff;
                        float ly0 = p1->y + dly0 * woff;
                        float lx1 = p1->x + dlx1 * woff;
                        float ly1 = p1->y + dly1 * woff;
                        detail::pushVertex(buf, lx0, ly0, 0.5f, 1, 0, 0);
                        detail::pushVertex(buf, lx1, ly1, 0.5f, 1, 0, 0);
                    }
                } else {
                    detail::pushVertex(buf, p1->x + (p1->dmx * woff), p1->y + (p1->dmy * woff), 0.5f, 1, 0, 0);
                }
                p0 = p1++;
            }
        } else {
            for (j = 0; j < path->count; ++j) {
                detail::pushVertex(buf, pts[j].x, pts[j].y, 0.5f, 1, 0, 0);
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
                lw = woff;  // This should generate the same vertex as fill inset above.
                lu = 0.5f;  // Set outline fade at middle.
            }

            // Looping
            p0 = &pts[path->count - 1];
            p1 = &pts[0];

            for (j = 0; j < path->count; ++j) {
                if ((p1->flags & (PointFlags::Bevel | PointFlags::InnerBevel)) != 0) {
                    detail::bevelJoin(buf, p0, p1, lw, rw, lu, ru, fringeWidth, 0);
                } else {
                    detail::pushVertex(buf, p1->x + (p1->dmx * lw), p1->y + (p1->dmy * lw), lu, 1, 0, 0);
                    detail::pushVertex(buf, p1->x - (p1->dmx * rw), p1->y - (p1->dmy * rw), ru, 1, 0, 0);
                }
                p0 = p1++;
            }

            // Loop it
            detail::pushVertex(buf, buf[strokeStart].x, buf[strokeStart].y, lu, 1, 0, 0);
            detail::pushVertex(buf, buf[strokeStart + 1].x, buf[strokeStart + 1].y, ru, 1, 0, 0);

            path->stroke = buf.data() + strokeStart;
            path->nstroke = static_cast<int>(buf.size() - strokeStart);
        } else {
            path->stroke = NULL;
            path->nstroke = 0;
        }
    }

    return 1;
}

void ContextImpl::flushTextTexture()
{
    std::array<int, 4> dirty{};

    if (fonsValidateTexture(fs.get(), dirty.data())) {
        int fontImage = fontImages[fontImageIdx];
        // Update texture
        if (fontImage != 0) {
            int iw, ih;
            const unsigned char* data = fonsGetTextureData(fs.get(), &iw, &ih);
            int x = dirty[0];
            int y = dirty[1];
            int w = dirty[2] - dirty[0];
            int h = dirty[3] - dirty[1];
            params.renderUpdateTexture(params.userPtr, fontImage, x, y, w, h, data);
        }
    }
}

int ContextImpl::allocTextAtlas()
{
    int iw, ih;
    flushTextTexture();
    if (fontImageIdx >= NVG_MAX_FONTIMAGES - 1) return 0;
    // if next fontImage already have a texture
    if (fontImages[fontImageIdx + 1] != 0)
        params.renderGetTextureSize(params.userPtr, fontImages[fontImageIdx + 1], &iw, &ih);
    else {  // calculate the new font image size and create it.
        params.renderGetTextureSize(params.userPtr, fontImages[fontImageIdx], &iw, &ih);
        if (iw > ih)
            ih *= 2;
        else
            iw *= 2;
        if (iw > NVG_MAX_FONTIMAGE_SIZE || ih > NVG_MAX_FONTIMAGE_SIZE) iw = ih = NVG_MAX_FONTIMAGE_SIZE;
        fontImages[fontImageIdx + 1] = params.renderCreateTexture(params.userPtr, static_cast<int>(Texture::Alpha), iw, ih, 0, NULL);
    }
    ++fontImageIdx;
    fonsResetAtlas(fs.get(), iw, ih);
    return 1;
}

void ContextImpl::renderText(Vertex* verts, int nverts)
{
    State& state = topState();
    Paint paint = state.fill;

    // Render triangles.
    paint.image = fontImages[fontImageIdx];

    // Apply global alpha
    paint.innerColor.a *= state.alpha;
    paint.outerColor.a *= state.alpha;

    params.renderTriangles(params.userPtr, &paint, state.compositeOperation, &state.scissor, verts, nverts, fringeWidth);

    drawCallCount++;
    textTriCount += nverts / 3;
}

}  // namespace nvg
