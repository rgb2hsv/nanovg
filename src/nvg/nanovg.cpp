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
#include <concepts>
#include <span>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define Expects(condition)                                                                                     \
    do {                                                                                                       \
        if (!(condition)) {                                                                                    \
            printf("Precondition failed: %s\n  at %s:%d in %s()\n", #condition, __FILE__, __LINE__, __func__); \
            std::terminate();                                                                                  \
        }                                                                                                      \
    } while (0)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4127)  // conditional expression is constant
#pragma warning(disable : 4204)  // nonstandard extension used : non-constant aggregate initializer
#pragma warning(disable : 4706)  // assignment within conditional expression
#endif
#define FONTSTASH_IMPLEMENTATION
#include "../thirdParty/fontstash.h"

#ifndef NVG_NO_STB
#define STB_IMAGE_IMPLEMENTATION
#include "../thirdParty/stb_image.h"
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "nanovg_detail.hpp"

namespace nvg
{

Context::Context(const Params& params) : mImpl(std::make_unique<ContextImpl>(params))
{
}

Context::~Context() = default;

ContextImpl& Context::operator*() noexcept
{
    return *mImpl;
}

const ContextImpl& Context::operator*() const noexcept
{
    return *mImpl;
}

ContextImpl* Context::operator->() noexcept
{
    return mImpl.get();
}

const ContextImpl* Context::operator->() const noexcept
{
    return mImpl.get();
}

ContextImpl::~ContextImpl()
{
    for (size_t i = 0; i < fontImages.size(); ++i) {
        if (fontImages[i] != 0) {
            params.renderDeleteTexture(params.userPtr, fontImages[i]);
            fontImages[i] = 0;
        }
    }

    if (params.renderDelete != nullptr) params.renderDelete(params.userPtr);
}
ContextImpl::ContextImpl(const Params& params) : params(params)
{
    FONSparams fontParams{};
    fontImages.resize(NVG_MAX_FONTIMAGES, 0);

    cache = detail::allocPathCache();

    setDevicePixelRatio(1.0f);

    if (params.renderCreate(params.userPtr) == 0) throw std::runtime_error("Could not init render context.");

    // Init font rendering
    fontParams.width = NVG_INIT_FONTIMAGE_SIZE;
    fontParams.height = NVG_INIT_FONTIMAGE_SIZE;
    fontParams.flags = FONS_ZERO_TOPLEFT;
    fontParams.renderCreate = nullptr;
    fontParams.renderUpdate = nullptr;
    fontParams.renderDraw = nullptr;
    fontParams.renderDelete = nullptr;
    fontParams.userPtr = nullptr;
    FONScontext* rawFs = fonsCreateInternal(&fontParams);
    if (rawFs == nullptr) throw std::runtime_error("Could not create font context");
    fs = std::shared_ptr<FONScontext>(rawFs, [](FONScontext* p) { fonsDeleteInternal(p); });

    // Create font texture
    fontImages[0] = params.renderCreateTexture(params.userPtr, static_cast<int>(Texture::Alpha), fontParams.width, fontParams.height, 0, nullptr);
    if (fontImages[0] == 0) throw std::runtime_error("Could not create font texture");
    fontImageIdx = 0;
    scissor = ScissorBounds{0.0f, 0.0f, -1.0f, -1.0f};
}

Color rgb(unsigned char r, unsigned char g, unsigned char b)
{
    return rgba(r, g, b, 255);
}

Color rgbf(float r, float g, float b)
{
    return rgbaf(r, g, b, 1.0f);
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
    for (i = 0; i < 4; i++) {
        cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
    }

    return cint;
}

Color hsl(float h, float s, float l)
{
    return hsla(h, s, l, 255);
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
    col.r = detail::clampf(detail::hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
    col.g = detail::clampf(detail::hue(h, m1, m2), 0.0f, 1.0f);
    col.b = detail::clampf(detail::hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
    col.a = a / 255.0f;
    return col;
}

void transformIdentity(float* t)
{
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void transformTranslate(float* t, float tx, float ty)
{
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = tx;
    t[5] = ty;
}

void transformScale(float* t, float sx, float sy)
{
    t[0] = sx;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = sy;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void transformRotate(float* t, float a)
{
    float cs = cosf(a), sn = sinf(a);
    t[0] = cs;
    t[1] = sn;
    t[2] = -sn;
    t[3] = cs;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void transformSkewX(float* t, float a)
{
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = tanf(a);
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void transformSkewY(float* t, float a)
{
    t[0] = 1.0f;
    t[1] = tanf(a);
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
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
    memcpy(s2.data(), s, sizeof(float) * 6);
    transformMultiply(s2.data(), t);
    memcpy(t, s2.data(), sizeof(float) * 6);
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
    dx = sx * t[0] + sy * t[2] + t[4];
    dy = sx * t[1] + sy * t[3] + t[5];
}

float degToRad(float deg)
{
    return deg / 180.0f * (float)M_PI;
}

float radToDeg(float rad)
{
    return rad / (float)M_PI * 180.0f;
}

void Context::beginFrame(float windowWidth, float windowHeight, float devicePixelRatio)
{
    mImpl->states.clear();
    save();
    reset();

    mImpl->setDevicePixelRatio(devicePixelRatio);

    mImpl->params.renderViewport(mImpl->params.userPtr, windowWidth, windowHeight, devicePixelRatio);

    mImpl->drawCallCount = 0;
    mImpl->fillTriCount = 0;
    mImpl->strokeTriCount = 0;
    mImpl->textTriCount = 0;
}

void Context::cancelFrame()
{
    mImpl->params.renderCancel(mImpl->params.userPtr);
}

void Context::endFrame()
{
    mImpl->params.renderFlush(mImpl->params.userPtr);
    if (mImpl->fontImageIdx != 0) {
        int fontImage = mImpl->fontImages[mImpl->fontImageIdx];
        mImpl->fontImages[mImpl->fontImageIdx] = 0;
        int iw, ih;
        if (fontImage == 0) return;
        imageSize(fontImage, iw, ih);
        size_t j = 0;
        for (size_t i = 0; i < mImpl->fontImageIdx; i++) {
            if (mImpl->fontImages[i] != 0) {
                int nw, nh;
                int image = mImpl->fontImages[i];
                mImpl->fontImages[i] = 0;
                imageSize(image, nw, nh);
                if (nw < iw || nh < ih)
                    deleteImage(image);
                else
                    mImpl->fontImages[j++] = image;
            }
        }
        mImpl->fontImages[j] = mImpl->fontImages[0];
        mImpl->fontImages[0] = fontImage;
        mImpl->fontImageIdx = 0;
    }
}

void Context::save()
{
    if (!mImpl->states.empty())
        mImpl->states.push_back(mImpl->states.back());
    else
        mImpl->states.push_back({});
}

void Context::restore()
{
    if (mImpl->states.size() <= 1) return;

    mImpl->states.pop_back();
}

void Context::reset()
{
    State& state = mImpl->topState();
    detail::setPaintColor(state.fill, rgba(255, 255, 255, 255));
    detail::setPaintColor(state.stroke, rgba(0, 0, 0, 255));
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
const ScissorBounds& Context::currentScissor() const
{
    return mImpl->scissor;
}

void Context::shapeAntiAlias(int enabled)
{
    State& state = mImpl->topState();
    state.shapeAntiAlias = enabled;
}

void Context::strokeWidth(float width)
{
    State& state = mImpl->topState();
    state.strokeWidth = width;
}

float Context::getStrokeWidth() const
{
    const State& state = (*this)->topState();
    return state.strokeWidth;
}

int Context::getTextAlign() const
{
    const State& state = (*this)->topState();
    return state.textAlign;
}

float Context::getFontSize() const
{
    const State& state = (*this)->topState();
    return state.fontSize;
}
int Context::getFontFaceId() const
{
    const State& state = (*this)->topState();
    return state.fontId;
}

void Context::miterLimit(float limit)
{
    State& state = mImpl->topState();
    state.miterLimit = limit;
}

void Context::lineStyle(int lineStyle)
{
    State& state = mImpl->topState();
    state.lineStyle = lineStyle;
}

void Context::lineCap(int cap)
{
    State& state = mImpl->topState();
    state.lineCap = cap;
}

void Context::lineJoin(int join)
{
    State& state = mImpl->topState();
    state.lineJoin = join;
}

void Context::globalAlpha(float alpha)
{
    State& state = mImpl->topState();
    state.alpha = alpha;
}

void Context::transform(float a, float b, float c, float d, float e, float f)
{
    State& state = mImpl->topState();
    const std::array<float, 6> t = {a, b, c, d, e, f};
    transformPremultiply(state.xform.data(), t.data());
}

void Context::resetTransform()
{
    State& state = mImpl->topState();
    transformIdentity(state.xform.data());
}

void Context::translate(float x, float y)
{
    State& state = mImpl->topState();
    std::array<float, 6> t{};
    transformTranslate(t.data(), x, y);
    transformPremultiply(state.xform.data(), t.data());
}

void Context::rotate(float angle)
{
    State& state = mImpl->topState();
    std::array<float, 6> t{};
    transformRotate(t.data(), angle);
    transformPremultiply(state.xform.data(), t.data());
}

void Context::skewX(float angle)
{
    State& state = mImpl->topState();
    std::array<float, 6> t{};
    transformSkewX(t.data(), angle);
    transformPremultiply(state.xform.data(), t.data());
}

void Context::skewY(float angle)
{
    State& state = mImpl->topState();
    std::array<float, 6> t{};
    transformSkewY(t.data(), angle);
    transformPremultiply(state.xform.data(), t.data());
}

void Context::scale(float x, float y)
{
    State& state = mImpl->topState();
    std::array<float, 6> t{};
    transformScale(t.data(), x, y);
    transformPremultiply(state.xform.data(), t.data());
}

void Context::currentTransform(float* xform) const
{
    const State& state = (*this)->topState();
    if (xform == NULL) return;
    memcpy(xform, state.xform.data(), sizeof(float) * 6);
}

void Context::strokeColor(const Color& color)
{
    State& state = mImpl->topState();
    detail::setPaintColor(state.stroke, color);
}

void Context::strokePaint(const Paint& paint)
{
    State& state = mImpl->topState();
    state.stroke = paint;
    transformMultiply(state.stroke.xform, state.xform.data());
}

void Context::fillColor(const Color& color)
{
    State& state = mImpl->topState();
    detail::setPaintColor(state.fill, color);
}

void Context::fillPaint(const Paint& paint)
{
    State& state = mImpl->topState();
    state.fill = paint;
    transformMultiply(state.fill.xform, state.xform.data());
}

#ifndef NVG_NO_STB
int Context::createImage(const char* filename, int imageFlags)
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
    image = createImageRGBA(w, h, imageFlags, img);
    stbi_image_free(img);
    return image;
}

int Context::createImageMem(int imageFlags, unsigned char* data, int ndata)
{
    int w, h, n, image;
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);
    unsigned char* img = stbi_load_from_memory(data, ndata, &w, &h, &n, 4);
    if (img == NULL) {
        //		printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
        return 0;
    }
    image = createImageRGBA(w, h, imageFlags, img);
    stbi_image_free(img);
    return image;
}
#endif

int Context::createImageRGBA(int w, int h, int imageFlags, const unsigned char* data)
{
    return mImpl->params.renderCreateTexture(mImpl->params.userPtr, static_cast<int>(Texture::Rgba), w, h, imageFlags, data);
}

void Context::updateImage(int image, const unsigned char* data)
{
    int w, h;
    mImpl->params.renderGetTextureSize(mImpl->params.userPtr, image, &w, &h);
    mImpl->params.renderUpdateTexture(mImpl->params.userPtr, image, 0, 0, w, h, data);
}

void Context::imageSize(int image, int& w, int& h) const
{
    (*this)->params.renderGetTextureSize((*this)->params.userPtr, image, &w, &h);
}

void Context::deleteImage(int image)
{
    mImpl->params.renderDeleteTexture(mImpl->params.userPtr, image);
}

Paint Context::linearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol) const
{
    Paint p{};
    float dx, dy, d;
    const float large = 1e5;

    // Calculate transform aligned to the line
    dx = ex - sx;
    dy = ey - sy;
    d = sqrtf(dx * dx + dy * dy);
    if (d > 0.0001f) {
        dx /= d;
        dy /= d;
    } else {
        dx = 0;
        dy = 1;
    }

    p.xform[0] = dy;
    p.xform[1] = -dx;
    p.xform[2] = dx;
    p.xform[3] = dy;
    p.xform[4] = sx - dx * large;
    p.xform[5] = sy - dy * large;

    p.extent[0] = large;
    p.extent[1] = large + d * 0.5f;

    p.radius = 0.0f;

    p.feather = detail::maxf(1.0f, d);

    p.innerColor = icol;
    p.outerColor = ocol;

    return p;
}

Paint Context::radialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol) const
{
    Paint p{};
    float r = (inr + outr) * 0.5f;
    float f = (outr - inr);

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

Paint Context::boxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol) const
{
    Paint p{};

    transformIdentity(p.xform);
    p.xform[4] = x + w * 0.5f;
    p.xform[5] = y + h * 0.5f;

    p.extent[0] = w * 0.5f;
    p.extent[1] = h * 0.5f;

    p.radius = r;

    p.feather = detail::maxf(1.0f, f);

    p.innerColor = icol;
    p.outerColor = ocol;

    return p;
}

Paint Context::imagePattern(float cx, float cy, float w, float h, float angle, int image, float alpha) const
{
    Paint p{};

    transformRotate(p.xform, angle);
    p.xform[4] = cx;
    p.xform[5] = cy;

    p.extent[0] = w;
    p.extent[1] = h;

    p.image = image;

    p.innerColor = p.outerColor = rgbaf(1, 1, 1, alpha);

    return p;
}

// Scissoring
void Context::scissor(float x, float y, float w, float h)
{
    State& state = mImpl->topState();
    mImpl->scissor = ScissorBounds{x, y, w, h};
    w = detail::maxf(0.0f, w);
    h = detail::maxf(0.0f, h);

    transformIdentity(state.scissor.xform);
    state.scissor.xform[4] = x + w * 0.5f;
    state.scissor.xform[5] = y + h * 0.5f;
    transformMultiply(state.scissor.xform, state.xform.data());

    state.scissor.extent[0] = w * 0.5f;
    state.scissor.extent[1] = h * 0.5f;
}

void Context::intersectScissor(float x, float y, float w, float h)
{
    State& state = mImpl->topState();
    std::array<float, 6> pxform{};
    std::array<float, 6> invxorm{};
    std::array<float, 4> rect{};
    float ex, ey, tex, tey;

    // If no previous scissor has been set, set the scissor as current scissor.
    if (state.scissor.extent[0] < 0) {
        scissor(x, y, w, h);
        return;
    }

    // Transform the current scissor rect into current transform space.
    // If there is difference in rotation, this will be approximation.
    memcpy(pxform.data(), state.scissor.xform, sizeof(float) * 6);
    ex = state.scissor.extent[0];
    ey = state.scissor.extent[1];
    transformInverse(invxorm.data(), state.xform.data());
    transformMultiply(pxform.data(), invxorm.data());
    tex = ex * detail::absf(pxform[0]) + ey * detail::absf(pxform[2]);
    tey = ex * detail::absf(pxform[1]) + ey * detail::absf(pxform[3]);

    // Intersect rects.
    detail::isectRects(rect.data(), pxform[4] - tex, pxform[5] - tey, tex * 2, tey * 2, x, y, w, h);

    scissor(rect[0], rect[1], rect[2], rect[3]);
}

void Context::resetScissor()
{
    State& state = mImpl->topState();
    state.scissor = Scissor{};
    state.scissor.extent[0] = -1.0f;
    state.scissor.extent[1] = -1.0f;
}

// Global composite operation.
void Context::globalCompositeOperation(int op)
{
    State& state = mImpl->topState();
    state.compositeOperation = detail::compositeOperationState(op);
}

void Context::globalCompositeBlendFunc(int sfactor, int dfactor)
{
    globalCompositeBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void Context::globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha)
{
    CompositeOperationState op;
    op.srcRGB = srcRGB;
    op.dstRGB = dstRGB;
    op.srcAlpha = srcAlpha;
    op.dstAlpha = dstAlpha;

    State& state = mImpl->topState();
    state.compositeOperation = op;
}

// Draw
void Context::beginPath()
{
    mImpl->commands.clear();
    mImpl->clearPathCache();
}

void Context::moveTo(float x, float y)
{
    std::array<float, 3> vals = {static_cast<float>(to_underlying(PathCommand::MoveTo)), x, y};
    mImpl->appendCommands(vals);
}

void Context::lineTo(float x, float y)
{
    std::array<float, 3> vals = {static_cast<float>(to_underlying(PathCommand::LineTo)), x, y};
    mImpl->appendCommands(vals);
}

void Context::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    std::array<float, 7> vals = {static_cast<float>(to_underlying(PathCommand::BezierTo)), c1x, c1y, c2x, c2y, x, y};
    mImpl->appendCommands(vals);
}

void Context::quadTo(float cx, float cy, float x, float y)
{
    float x0 = mImpl->commandx;
    float y0 = mImpl->commandy;
    std::array<float, 7> vals = {
        static_cast<float>(to_underlying(PathCommand::BezierTo)), x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0), x + 2.0f / 3.0f * (cx - x), y + 2.0f / 3.0f * (cy - y), x, y};
    mImpl->appendCommands(vals);
}

void Context::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    float x0 = mImpl->commandx;
    float y0 = mImpl->commandy;
    float dx0, dy0, dx1, dy1, a, d, cx, cy, a0, a1;
    int dir;

    if (mImpl->commands.empty()) {
        return;
    }

    // Handle degenerate cases.
    if (detail::ptEquals(x0, y0, x1, y1, mImpl->distTol) || detail::ptEquals(x1, y1, x2, y2, mImpl->distTol) || detail::distPtSeg(x1, y1, x0, y0, x2, y2) < mImpl->distTol * mImpl->distTol ||
        radius < mImpl->distTol) {
        lineTo(x1, y1);
        return;
    }

    // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
    dx0 = x0 - x1;
    dy0 = y0 - y1;
    dx1 = x2 - x1;
    dy1 = y2 - y1;
    detail::normalize(dx0, dy0);
    detail::normalize(dx1, dy1);
    a = acosf(dx0 * dx1 + dy0 * dy1);
    d = radius / tanf(a / 2.0f);

    //	printf("a=%f° d=%f\n", a/M_PI*180.0f, d);

    if (d > 10000.0f) {
        lineTo(x1, y1);
        return;
    }

    if (detail::cross(dx0, dy0, dx1, dy1) > 0.0f) {
        cx = x1 + dx0 * d + dy0 * radius;
        cy = y1 + dy0 * d + -dx0 * radius;
        a0 = atan2f(dx0, -dy0);
        a1 = atan2f(-dx1, dy1);
        dir = static_cast<int>(Winding::CW);
        //		printf("CW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/M_PI*180.0f, a1/M_PI*180.0f);
    } else {
        cx = x1 + dx0 * d + -dy0 * radius;
        cy = y1 + dy0 * d + dx0 * radius;
        a0 = atan2f(-dx0, dy0);
        a1 = atan2f(dx1, -dy1);
        dir = static_cast<int>(Winding::CCW);
        //		printf("CCW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/M_PI*180.0f, a1/M_PI*180.0f);
    }

    arc(cx, cy, radius, a0, a1, dir);
}

void Context::closePath()
{
    std::array<float, 1> vals = {static_cast<float>(to_underlying(PathCommand::Close))};
    mImpl->appendCommands(vals);
}

void Context::pathWinding(int dir)
{
    std::array<float, 2> vals = {static_cast<float>(to_underlying(PathCommand::Winding)), (float)dir};
    mImpl->appendCommands(vals);
}

void Context::arc(float cx, float cy, float r, float a0, float a1, int dir)
{
    const float pi = (float)M_PI;
    const float twoPi = pi * 2.0f;
    float a = 0, da = 0, hda = 0, kappa = 0;
    float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    std::array<float, 3 + 5 * 7 + 100> vals{};
    int i, ndivs, nvals;
    int move = !mImpl->commands.empty() ? static_cast<int>(PathCommand::LineTo) : static_cast<int>(PathCommand::MoveTo);

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
    ndivs = detail::maxi(1, detail::mini((int)(detail::absf(da) / (pi * 0.5f) + 0.5f), 5));
    hda = (da / (float)ndivs) / 2.0f;
    kappa = detail::absf(4.0f / 3.0f * (1.0f - cosf(hda)) / sinf(hda));

    if (dir == static_cast<int>(Winding::CCW)) kappa = -kappa;

    nvals = 0;
    for (i = 0; i <= ndivs; i++) {
        a = a0 + da * (i / (float)ndivs);
        dx = cosf(a);
        dy = sinf(a);
        x = cx + dx * r;
        y = cy + dy * r;
        tanx = -dy * r * kappa;
        tany = dx * r * kappa;

        if (i == 0) {
            vals[(size_t)nvals++] = (float)move;
            vals[nvals++] = x;
            vals[nvals++] = y;
        } else {
            vals[nvals++] = static_cast<float>(to_underlying(PathCommand::BezierTo));
            vals[nvals++] = px + ptanx;
            vals[nvals++] = py + ptany;
            vals[nvals++] = x - tanx;
            vals[nvals++] = y - tany;
            vals[nvals++] = x;
            vals[nvals++] = y;
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    std::span<float> span(vals.data(), static_cast<size_t>(nvals));
    mImpl->appendCommands(span);
}

void Context::rect(float x, float y, float w, float h)
{
    std::array<float, 13> vals = {static_cast<float>(to_underlying(PathCommand::MoveTo)), x,     y,     static_cast<float>(to_underlying(PathCommand::LineTo)), x,     y + h,
                                  static_cast<float>(to_underlying(PathCommand::LineTo)), x + w, y + h, static_cast<float>(to_underlying(PathCommand::LineTo)), x + w, y,
                                  static_cast<float>(to_underlying(PathCommand::Close))};
    mImpl->appendCommands(vals);
}

void Context::roundedRect(float x, float y, float w, float h, float r)
{
    roundedRectVarying(x, y, w, h, r, r, r, r);
}

void Context::roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
{
    if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f) {
        rect(x, y, w, h);
        return;
    } else {
        float halfw = detail::absf(w) * 0.5f;
        float halfh = detail::absf(h) * 0.5f;
        float rxBL = detail::minf(radBottomLeft, halfw) * detail::signf(w), ryBL = detail::minf(radBottomLeft, halfh) * detail::signf(h);
        float rxBR = detail::minf(radBottomRight, halfw) * detail::signf(w), ryBR = detail::minf(radBottomRight, halfh) * detail::signf(h);
        float rxTR = detail::minf(radTopRight, halfw) * detail::signf(w), ryTR = detail::minf(radTopRight, halfh) * detail::signf(h);
        float rxTL = detail::minf(radTopLeft, halfw) * detail::signf(w), ryTL = detail::minf(radTopLeft, halfh) * detail::signf(h);
        std::array<float, 44> vals = {static_cast<float>(to_underlying(PathCommand::MoveTo)),
                                      x,
                                      y + ryTL,
                                      static_cast<float>(to_underlying(PathCommand::LineTo)),
                                      x,
                                      y + h - ryBL,
                                      static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                      x,
                                      y + h - ryBL * (1 - NVG_KAPPA90),
                                      x + rxBL * (1 - NVG_KAPPA90),
                                      y + h,
                                      x + rxBL,
                                      y + h,
                                      static_cast<float>(to_underlying(PathCommand::LineTo)),
                                      x + w - rxBR,
                                      y + h,
                                      static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                      x + w - rxBR * (1 - NVG_KAPPA90),
                                      y + h,
                                      x + w,
                                      y + h - ryBR * (1 - NVG_KAPPA90),
                                      x + w,
                                      y + h - ryBR,
                                      static_cast<float>(to_underlying(PathCommand::LineTo)),
                                      x + w,
                                      y + ryTR,
                                      static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                      x + w,
                                      y + ryTR * (1 - NVG_KAPPA90),
                                      x + w - rxTR * (1 - NVG_KAPPA90),
                                      y,
                                      x + w - rxTR,
                                      y,
                                      static_cast<float>(to_underlying(PathCommand::LineTo)),
                                      x + rxTL,
                                      y,
                                      static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                      x + rxTL * (1 - NVG_KAPPA90),
                                      y,
                                      x,
                                      y + ryTL * (1 - NVG_KAPPA90),
                                      x,
                                      y + ryTL,
                                      static_cast<float>(to_underlying(PathCommand::Close))};
        mImpl->appendCommands(vals);
    }
}

void Context::ellipse(float cx, float cy, float rx, float ry)
{
    std::array<float, 32> vals = {static_cast<float>(to_underlying(PathCommand::MoveTo)),
                                  cx - rx,
                                  cy,
                                  static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                  cx - rx,
                                  cy + ry * NVG_KAPPA90,
                                  cx - rx * NVG_KAPPA90,
                                  cy + ry,
                                  cx,
                                  cy + ry,
                                  static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                  cx + rx * NVG_KAPPA90,
                                  cy + ry,
                                  cx + rx,
                                  cy + ry * NVG_KAPPA90,
                                  cx + rx,
                                  cy,
                                  static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                  cx + rx,
                                  cy - ry * NVG_KAPPA90,
                                  cx + rx * NVG_KAPPA90,
                                  cy - ry,
                                  cx,
                                  cy - ry,
                                  static_cast<float>(to_underlying(PathCommand::BezierTo)),
                                  cx - rx * NVG_KAPPA90,
                                  cy - ry,
                                  cx - rx,
                                  cy - ry * NVG_KAPPA90,
                                  cx - rx,
                                  cy,
                                  static_cast<float>(to_underlying(PathCommand::Close))};
    mImpl->appendCommands(vals);
}

void Context::circle(float cx, float cy, float r)
{
    ellipse(cx, cy, r, r);
}

void Context::debugDumpPathCache() const
{
    const Path* path;
    int i, j;

    printf("Dumping %zu cached paths\n", (*this)->cache->paths.size());
    for (i = 0; i < static_cast<int>((*this)->cache->paths.size()); i++) {
        path = &(*this)->cache->paths[i];
        printf(" - Path %d\n", i);
        if (path->nfill) {
            printf("   - fill: %d\n", path->nfill);
            for (j = 0; j < path->nfill; j++) printf("%f\t%f\n", path->fill[j].x, path->fill[j].y);
        }
        if (path->nstroke) {
            printf("   - stroke: %d\n", path->nstroke);
            for (j = 0; j < path->nstroke; j++) printf("%f\t%f\n", path->stroke[j].x, path->stroke[j].y);
        }
    }
}

void Context::fill()
{
    State& state = mImpl->topState();
    const Path* path;
    Paint fillPaint = state.fill;
    int i;

    mImpl->flattenPaths();
    if (mImpl->params.edgeAntiAlias && state.shapeAntiAlias)
        mImpl->expandFill(mImpl->fringeWidth, static_cast<int>(LineCap::Miter), 2.4f);
    else
        mImpl->expandFill(0.0f, static_cast<int>(LineCap::Miter), 2.4f);

    // Apply global alpha
    fillPaint.innerColor.a *= state.alpha;
    fillPaint.outerColor.a *= state.alpha;

    mImpl->params.renderFill(mImpl->params.userPtr, &fillPaint, state.compositeOperation, &state.scissor, mImpl->fringeWidth, mImpl->cache->bounds.data(), mImpl->cache->paths.data(),
                             static_cast<int>(mImpl->cache->paths.size()));

    // Count triangles
    for (i = 0; i < static_cast<int>(mImpl->cache->paths.size()); i++) {
        path = &mImpl->cache->paths[i];
        mImpl->fillTriCount += path->nfill - 2;
        mImpl->fillTriCount += path->nstroke - 2;
        mImpl->drawCallCount += 2;
    }
}

void Context::stroke()
{
    State& state = mImpl->topState();
    const float scale = detail::getAverageScale(state.xform.data());
    float strokeWidth = detail::clampf(state.strokeWidth * scale, 0.0f, 1000.0f);
    Paint strokePaint = state.stroke;
    const Path* path;
    int i;

    if (strokeWidth < mImpl->fringeWidth) {
        // If the stroke width is less than pixel size, use alpha to emulate coverage.
        // Since coverage is area, scale by alpha*alpha.
        float alpha = detail::clampf(strokeWidth / mImpl->fringeWidth, 0.0f, 1.0f);
        strokePaint.innerColor.a *= alpha * alpha;
        strokePaint.outerColor.a *= alpha * alpha;
        strokeWidth = mImpl->fringeWidth;
    }

    // Apply global alpha
    strokePaint.innerColor.a *= state.alpha;
    strokePaint.outerColor.a *= state.alpha;

    mImpl->flattenPaths();

    if (mImpl->params.edgeAntiAlias && state.shapeAntiAlias)
        mImpl->expandStroke(strokeWidth * 0.5f, mImpl->fringeWidth, state.lineCap, state.lineJoin, state.lineStyle, state.miterLimit);
    else
        mImpl->expandStroke(strokeWidth * 0.5f, 0.0f, state.lineCap, state.lineJoin, state.lineStyle, state.miterLimit);

    mImpl->params.renderStroke(mImpl->params.userPtr, &strokePaint, state.compositeOperation, &state.scissor, mImpl->fringeWidth, strokeWidth, state.lineStyle, mImpl->cache->paths.data(),
                               static_cast<int>(mImpl->cache->paths.size()));

    // Count triangles
    for (i = 0; i < static_cast<int>(mImpl->cache->paths.size()); i++) {
        path = &mImpl->cache->paths[i];
        mImpl->strokeTriCount += path->nstroke - 2;
        mImpl->drawCallCount++;
    }
}

// Add fonts
int Context::createFont(const char* name, const char* filename)
{
    return fonsAddFont(mImpl->fs.get(), name, filename, 0);
}

int Context::createFontAtIndex(const char* name, const char* filename, const int fontIndex)
{
    return fonsAddFont(mImpl->fs.get(), name, filename, fontIndex);
}

int Context::createFontMem(const char* name, unsigned char* data, int ndata, int freeData)
{
    return fonsAddFontMem(mImpl->fs.get(), name, data, ndata, freeData, 0);
}

int Context::createFontMemAtIndex(const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex)
{
    return fonsAddFontMem(mImpl->fs.get(), name, data, ndata, freeData, fontIndex);
}

int Context::findFont(const char* name) const
{
    if (name == NULL) return -1;
    return fonsGetFontByName((*this)->fs.get(), name);
}

int Context::addFallbackFontId(int baseFont, int fallbackFont)
{
    if (baseFont == -1 || fallbackFont == -1) return 0;
    return fonsAddFallbackFont(mImpl->fs.get(), baseFont, fallbackFont);
}

int Context::addFallbackFont(const char* baseFont, const char* fallbackFont)
{
    return addFallbackFontId(findFont(baseFont), findFont(fallbackFont));
}

void Context::resetFallbackFontsId(int baseFont)
{
    fonsResetFallbackFont(mImpl->fs.get(), baseFont);
}

void Context::resetFallbackFonts(const char* baseFont)
{
    resetFallbackFontsId(findFont(baseFont));
}

// State setting
void Context::fontSize(float size)
{
    State& state = mImpl->topState();
    state.fontSize = size;
}

void Context::fontBlur(float blur)
{
    State& state = mImpl->topState();
    state.fontBlur = blur;
}

void Context::fontDilate(float dilate)
{
    State& state = mImpl->topState();
    state.fontDilate = dilate;
}

void Context::textLetterSpacing(float spacing)
{
    State& state = mImpl->topState();
    state.letterSpacing = spacing;
}

void Context::textLineHeight(float lineHeight)
{
    State& state = mImpl->topState();
    state.lineHeight = lineHeight;
}

void Context::textAlign(int align)
{
    State& state = mImpl->topState();
    state.textAlign = align;
}

void Context::fontFaceId(int font)
{
    State& state = mImpl->topState();
    state.fontId = font;
}

void Context::fontFace(const char* font)
{
    State& state = mImpl->topState();
    state.fontId = fonsGetFontByName(mImpl->fs.get(), font);
}

float Context::text(float x, float y, const char* string, const char* end)
{
    State& state = mImpl->topState();
    FONStextIter iter, prevIter;
    FONSquad q;
    std::vector<Vertex>& buf = mImpl->cache->verts;
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
    float invscale = 1.0f / scale;
    int cverts = 0;
    int isFlipped = detail::isTransformFlipped(state.xform.data());

    if (end == NULL) end = string + strlen(string);

    if (state.fontId == FONS_INVALID) return x;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetDilate(mImpl->fs.get(), state.fontDilate * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);

    cverts = detail::maxi(2, (int)(end - string)) * 6;  // conservative estimate.
    mImpl->prepareTempVerts(cverts);

    fonsTextIterInit(mImpl->fs.get(), &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_REQUIRED);
    prevIter = iter;
    while (fonsTextIterNext(mImpl->fs.get(), &iter, &q)) {
        std::array<float, 8> c{};
        if (iter.prevGlyphIndex == -1) {  // can not retrieve glyph?
            if (!buf.empty()) {
                mImpl->renderText(buf.data(), static_cast<int>(buf.size()));
                buf.clear();
            }
            if (!mImpl->allocTextAtlas()) break;  // no memory :(
            iter = prevIter;
            fonsTextIterNext(mImpl->fs.get(), &iter, &q);  // try again
            if (iter.prevGlyphIndex == -1)                 // still can not find glyph?
                break;
        }
        prevIter = iter;
        if (isFlipped) {
            float tmp;

            tmp = q.y0;
            q.y0 = q.y1;
            q.y1 = tmp;
            tmp = q.t0;
            q.t0 = q.t1;
            q.t1 = tmp;
        }
        // Transform corners.
        transformPoint(c[0], c[1], state.xform.data(), q.x0 * invscale + x, q.y0 * invscale + y);
        transformPoint(c[2], c[3], state.xform.data(), q.x1 * invscale + x, q.y0 * invscale + y);
        transformPoint(c[4], c[5], state.xform.data(), q.x1 * invscale + x, q.y1 * invscale + y);
        transformPoint(c[6], c[7], state.xform.data(), q.x0 * invscale + x, q.y1 * invscale + y);
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
    mImpl->flushTextTexture();

    mImpl->renderText(buf.data(), static_cast<int>(buf.size()));
    return iter.nextx * invscale + x;
}

void Context::textBox(float x, float y, float breakRowWidth, const char* string, const char* end)
{
    State& state = mImpl->topState();
    std::array<TextRow, 2> rows{};
    int nrows = 0, i;
    int oldAlign = state.textAlign;
    int halign = state.textAlign & static_cast<int>(Align::Left | Align::Center | Align::Right);
    int valign = state.textAlign & static_cast<int>(Align::Top | Align::Middle | Align::MiddleAscent | Align::Bottom | Align::Baseline);
    float lineh = 0;

    if (state.fontId == FONS_INVALID) return;

    textMetrics(NULL, NULL, &lineh);

    state.textAlign = static_cast<int>(Align::Left) | valign;

    while ((nrows = textBreakLines(string, end, breakRowWidth, rows.data(), (int)rows.size(), 0))) {
        for (i = 0; i < nrows; i++) {
            TextRow* row = &rows[(size_t)i];
            if (halign & Align::Left)
                text(x, y, row->start, row->end);
            else if (halign & Align::Center)
                text(x + breakRowWidth * 0.5f - row->width * 0.5f, y, row->start, row->end);
            else if (halign & Align::Right)
                text(x + breakRowWidth - row->width, y, row->start, row->end);
            y += lineh * state.lineHeight;
        }
        string = rows[(size_t)nrows - 1].next;
    }

    state.textAlign = oldAlign;
}

int Context::textGlyphPositions(float x, float y, const char* string, const char* end, GlyphPosition* positions, int maxPositions)
{
    UNUSED(y);
    State& state = mImpl->topState();
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
    float invscale = 1.0f / scale;
    FONStextIter iter, prevIter;
    FONSquad q;
    int npos = 0;

    if (state.fontId == FONS_INVALID) return 0;

    if (end == NULL) end = string + strlen(string);

    if (string == end) return 0;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);

    fonsTextIterInit(mImpl->fs.get(), &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(mImpl->fs.get(), &iter, &q)) {
        if (iter.prevGlyphIndex < 0 && mImpl->allocTextAtlas()) {  // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(mImpl->fs.get(), &iter, &q);  // try again
        }
        prevIter = iter;
        positions[npos].str = iter.str;
        positions[npos].x = iter.x * invscale + x;
        positions[npos].minx = detail::minf(iter.x, q.x0) * invscale + x;
        positions[npos].maxx = detail::maxf(iter.nextx, q.x1) * invscale + x;
        npos++;
        if (npos >= maxPositions) break;
    }

    return npos;
}

enum class CodepointType : int {
    Space,
    Newline,
    Char,
    CjkChar,
};

int Context::textBreakLines(const char* string, const char* end, float breakRowWidth, TextRow* rows, int maxRows, int skipSpaces)
{
    State& state = mImpl->topState();
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
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

    if (end == NULL) end = string + strlen(string);

    if (string == end) return 0;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetDilate(mImpl->fs.get(), state.fontDilate * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);

    breakRowWidth *= scale;

    fonsTextIterInit(mImpl->fs.get(), &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(mImpl->fs.get(), &iter, &q)) {
        if (iter.prevGlyphIndex < 0 && mImpl->allocTextAtlas()) {  // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(mImpl->fs.get(), &iter, &q);  // try again
        }
        prevIter = iter;
        switch (iter.codepoint) {
            case 9:       // \t
            case 11:      // \v
            case 12:      // \f
            case 32:      // space
            case 0x00a0:  // NBSP
                type = CodepointType::Space;
                break;
            case 10:  // \n
                type = pcodepoint == 13 ? CodepointType::Space : CodepointType::Newline;
                break;
            case 13:  // \r
                type = pcodepoint == 10 ? CodepointType::Space : CodepointType::Newline;
                break;
            case 0x0085:  // NEL
                type = CodepointType::Newline;
                break;
            default:
                if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) || (iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) || (iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
                    (iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) || (iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) || (iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
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
            if (nrows >= maxRows) return nrows;
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
                        if (nrows >= maxRows) return nrows;
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
                        if (nrows >= maxRows) return nrows;
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

float Context::textBounds(float x, float y, const char* string, const char* end, float* bounds)
{
    State& state = mImpl->topState();
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
    float invscale = 1.0f / scale;
    float width;

    if (state.fontId == FONS_INVALID) return 0;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetDilate(mImpl->fs.get(), state.fontDilate * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);

    width = fonsTextBounds(mImpl->fs.get(), 0, 0, string, end, bounds);
    if (bounds != NULL) {
        // Use line bounds for height.
        fonsLineBounds(mImpl->fs.get(), 0, &bounds[1], &bounds[3]);
        bounds[0] = bounds[0] * invscale + x;
        bounds[1] = bounds[1] * invscale + y;
        bounds[2] = bounds[2] * invscale + x;
        bounds[3] = bounds[3] * invscale + y;
    }
    return width * invscale;
}

void Context::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{
    State& state = mImpl->topState();
    std::array<TextRow, 2> rows{};
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
    float invscale = 1.0f / scale;
    float yoff = 0;
    int nrows = 0, i;
    int oldAlign = state.textAlign;
    int halign = state.textAlign & static_cast<int>(Align::Left | Align::Center | Align::Right);
    int valign = state.textAlign & static_cast<int>(Align::Top | Align::Middle | Align::MiddleAscent | Align::Bottom | Align::Baseline);
    float lineh = 0, rminy = 0, rmaxy = 0;
    float minx, miny, maxx, maxy;

    if (state.fontId == FONS_INVALID) {
        if (bounds != NULL) bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
        return;
    }

    textMetrics(NULL, NULL, &lineh);

    state.textAlign = static_cast<int>(Align::Left) | valign;

    minx = maxx = 0;
    miny = maxy = 0;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetDilate(mImpl->fs.get(), state.fontDilate * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);
    fonsLineBounds(mImpl->fs.get(), 0, &rminy, &rmaxy);
    rminy *= invscale;
    rmaxy *= invscale;

    while ((nrows = textBreakLines(string, end, breakRowWidth, rows.data(), (int)rows.size(), 0))) {
        for (i = 0; i < nrows; i++) {
            TextRow* row = &rows[(size_t)i];
            float rminx, rmaxx, dx = 0;
            // Horizontal bounds
            if (halign & Align::Left)
                dx = 0;
            else if (halign & Align::Center)
                dx = breakRowWidth * 0.5f - row->width * 0.5f;
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
        string = rows[(size_t)nrows - 1].next;
    }

    state.textAlign = oldAlign;

    if (bounds != NULL) {
        bounds[0] = minx + x;
        bounds[1] = miny + y;
        bounds[2] = maxx + x;
        bounds[3] = maxy + y;
    }
}

void Context::textMetrics(float* ascender, float* descender, float* lineh)
{
    State& state = mImpl->topState();
    float scale = detail::getFontScale(&state) * mImpl->devicePxRatio;
    float invscale = 1.0f / scale;

    if (state.fontId == FONS_INVALID) return;

    fonsSetSize(mImpl->fs.get(), state.fontSize * scale);
    fonsSetSpacing(mImpl->fs.get(), state.letterSpacing * scale);
    fonsSetBlur(mImpl->fs.get(), state.fontBlur * scale);
    fonsSetDilate(mImpl->fs.get(), state.fontDilate * scale);
    fonsSetAlign(mImpl->fs.get(), state.textAlign);
    fonsSetFont(mImpl->fs.get(), state.fontId);

    fonsVertMetrics(mImpl->fs.get(), ascender, descender, lineh);
    if (ascender != NULL) *ascender *= invscale;
    if (descender != NULL) *descender *= invscale;
    if (lineh != NULL) *lineh *= invscale;
}
const Params& Context::internalParams() const
{
    return (*this)->params;
}

}  // namespace nvg
