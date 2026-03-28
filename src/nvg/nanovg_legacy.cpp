//
// C API wrapper for nvg::Context (see nanovg.hpp). Implements declarations in nanovg_legacy.hpp.
//

#include "nanovg_legacy.hpp"

#include "nanovg.hpp"

#include <cstring>
#include <memory>
#include <new>

struct NVGcontext {
	std::unique_ptr<nvg::Context> impl;
};

namespace {

nvg::Context* ctxPtr(NVGcontext* ctx) noexcept {
	return (ctx && ctx->impl) ? ctx->impl.get() : nullptr;
}

nvg::Params copyParams(const NVGparams& p) noexcept {
	nvg::Params out{};
	static_assert(sizeof(NVGparams) == sizeof(nvg::Params));
	static_assert(alignof(NVGparams) == alignof(nvg::Params));
	std::memcpy(&out, &p, sizeof(out));
	return out;
}

nvg::Color toCpp(NVGcolor c) noexcept {
	nvg::Color r;
	std::memcpy(&r, &c, sizeof(r));
	return r;
}

nvg::Paint toCpp(NVGpaint p) noexcept {
	nvg::Paint r;
	std::memcpy(&r, &p, sizeof(r));
	return r;
}

NVGcolor toLegacy(const nvg::Color& c) noexcept {
	NVGcolor r;
	std::memcpy(&r, &c, sizeof(r));
	return r;
}

NVGpaint toLegacy(const nvg::Paint& p) noexcept {
	NVGpaint r;
	std::memcpy(&r, &p, sizeof(r));
	return r;
}

NVGscissorBounds toLegacy(const nvg::ScissorBounds& b) noexcept {
	NVGscissorBounds r;
	std::memcpy(&r, &b, sizeof(r));
	return r;
}

} // namespace

extern "C" {

void nvgBeginFrame(NVGcontext* ctx, float windowWidth, float windowHeight, float devicePixelRatio) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->beginFrame(windowWidth, windowHeight, devicePixelRatio);
}

void nvgCancelFrame(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->cancelFrame();
}

void nvgEndFrame(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->endFrame();
}

void nvgGlobalCompositeOperation(NVGcontext* ctx, int op) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->globalCompositeOperation(op);
}

void nvgGlobalCompositeBlendFunc(NVGcontext* ctx, int sfactor, int dfactor) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->globalCompositeBlendFunc(sfactor, dfactor);
}

void nvgGlobalCompositeBlendFuncSeparate(NVGcontext* ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->globalCompositeBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

NVGcolor nvgRGB(unsigned char r, unsigned char g, unsigned char b) {
	return toLegacy(nvg::rgb(r, g, b));
}

NVGcolor nvgRGBf(float r, float g, float b) {
	return toLegacy(nvg::rgbf(r, g, b));
}

NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return toLegacy(nvg::rgba(r, g, b, a));
}

NVGcolor nvgRGBAf(float r, float g, float b, float a) {
	return toLegacy(nvg::rgbaf(r, g, b, a));
}

NVGcolor nvgLerpRGBA(NVGcolor c0, NVGcolor c1, float u) {
	return toLegacy(nvg::lerpRgba(toCpp(c0), toCpp(c1), u));
}

NVGcolor nvgTransRGBA(NVGcolor c0, unsigned char a) {
	return toLegacy(nvg::transRgba(toCpp(c0), a));
}

NVGcolor nvgTransRGBAf(NVGcolor c0, float a) {
	return toLegacy(nvg::transRgbaf(toCpp(c0), a));
}

NVGcolor nvgHSL(float h, float s, float l) {
	return toLegacy(nvg::hsl(h, s, l));
}

NVGcolor nvgHSLA(float h, float s, float l, unsigned char a) {
	return toLegacy(nvg::hsla(h, s, l, a));
}

void nvgSave(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->save();
}

void nvgRestore(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->restore();
}

void nvgReset(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->reset();
}

NVGscissorBounds nvgCurrentScissor(NVGcontext* ctx) {
	NVGscissorBounds z{};
	if (nvg::Context* c = ctxPtr(ctx))
		return toLegacy(c->currentScissor());
	return z;
}

void nvgShapeAntiAlias(NVGcontext* ctx, int enabled) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->shapeAntiAlias(enabled);
}

void nvgStrokeColor(NVGcontext* ctx, NVGcolor color) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->strokeColor(toCpp(color));
}

void nvgStrokePaint(NVGcontext* ctx, NVGpaint paint) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->strokePaint(toCpp(paint));
}

void nvgFillColor(NVGcontext* ctx, NVGcolor color) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fillColor(toCpp(color));
}

void nvgFillPaint(NVGcontext* ctx, NVGpaint paint) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fillPaint(toCpp(paint));
}

void nvgMiterLimit(NVGcontext* ctx, float limit) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->miterLimit(limit);
}

void nvgStrokeWidth(NVGcontext* ctx, float size) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->strokeWidth(size);
}

void nvgLineStyle(NVGcontext* ctx, int lineStyle) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->lineStyle(lineStyle);
}

void nvgLineCap(NVGcontext* ctx, int cap) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->lineCap(cap);
}

void nvgLineJoin(NVGcontext* ctx, int join) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->lineJoin(join);
}

void nvgGlobalAlpha(NVGcontext* ctx, float alpha) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->globalAlpha(alpha);
}

void nvgResetTransform(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->resetTransform();
}

void nvgTransform(NVGcontext* ctx, float a, float b, float c, float d, float e, float f) {
	if (nvg::Context* cx = ctxPtr(ctx))
		cx->transform(a, b, c, d, e, f);
}

void nvgTranslate(NVGcontext* ctx, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->translate(x, y);
}

void nvgRotate(NVGcontext* ctx, float angle) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->rotate(angle);
}

void nvgSkewX(NVGcontext* ctx, float angle) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->skewX(angle);
}

void nvgSkewY(NVGcontext* ctx, float angle) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->skewY(angle);
}

void nvgScale(NVGcontext* ctx, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->scale(x, y);
}

void nvgCurrentTransform(NVGcontext* ctx, float* xform) {
	if (!xform)
		return;
	if (nvg::Context* c = ctxPtr(ctx))
		c->currentTransform(xform);
}

void nvgTransformIdentity(float* dst) {
	nvg::transformIdentity(dst);
}

void nvgTransformTranslate(float* dst, float tx, float ty) {
	nvg::transformTranslate(dst, tx, ty);
}

void nvgTransformScale(float* dst, float sx, float sy) {
	nvg::transformScale(dst, sx, sy);
}

void nvgTransformRotate(float* dst, float a) {
	nvg::transformRotate(dst, a);
}

void nvgTransformSkewX(float* dst, float a) {
	nvg::transformSkewX(dst, a);
}

void nvgTransformSkewY(float* dst, float a) {
	nvg::transformSkewY(dst, a);
}

void nvgTransformMultiply(float* dst, const float* src) {
	nvg::transformMultiply(dst, src);
}

void nvgTransformPremultiply(float* dst, const float* src) {
	nvg::transformPremultiply(dst, src);
}

int nvgTransformInverse(float* dst, const float* src) {
	return nvg::transformInverse(dst, src);
}

void nvgTransformPoint(float* dstx, float* dsty, const float* xform, float srcx, float srcy) {
	if (!dstx || !dsty)
		return;
	nvg::transformPoint(*dstx, *dsty, xform, srcx, srcy);
}

float nvgDegToRad(float deg) {
	return nvg::degToRad(deg);
}

float nvgRadToDeg(float rad) {
	return nvg::radToDeg(rad);
}

int nvgCreateImage(NVGcontext* ctx, const char* filename, int imageFlags) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createImage(filename, imageFlags);
	return 0;
}

int nvgCreateImageMem(NVGcontext* ctx, int imageFlags, unsigned char* data, int ndata) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createImageMem(imageFlags, data, ndata);
	return 0;
}

int nvgCreateImageRGBA(NVGcontext* ctx, int w, int h, int imageFlags, const unsigned char* data) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createImageRGBA(w, h, imageFlags, data);
	return 0;
}

void nvgUpdateImage(NVGcontext* ctx, int image, const unsigned char* data) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->updateImage(image, data);
}

void nvgImageSize(NVGcontext* ctx, int image, int* w, int* h) {
	if (!w || !h)
		return;
	if (nvg::Context* c = ctxPtr(ctx))
		c->imageSize(image, *w, *h);
}

void nvgDeleteImage(NVGcontext* ctx, int image) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->deleteImage(image);
}

NVGpaint nvgLinearGradient(NVGcontext* ctx, float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol) {
	if (nvg::Context* c = ctxPtr(ctx))
		return toLegacy(c->linearGradient(sx, sy, ex, ey, toCpp(icol), toCpp(ocol)));
	return NVGpaint{};
}

NVGpaint nvgBoxGradient(NVGcontext* ctx, float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol) {
	if (nvg::Context* c = ctxPtr(ctx))
		return toLegacy(c->boxGradient(x, y, w, h, r, f, toCpp(icol), toCpp(ocol)));
	return NVGpaint{};
}

NVGpaint nvgRadialGradient(NVGcontext* ctx, float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol) {
	if (nvg::Context* c = ctxPtr(ctx))
		return toLegacy(c->radialGradient(cx, cy, inr, outr, toCpp(icol), toCpp(ocol)));
	return NVGpaint{};
}

NVGpaint nvgImagePattern(NVGcontext* ctx, float ox, float oy, float ex, float ey, float angle, int image, float alpha) {
	if (nvg::Context* c = ctxPtr(ctx))
		return toLegacy(c->imagePattern(ox, oy, ex, ey, angle, image, alpha));
	return NVGpaint{};
}

void nvgScissor(NVGcontext* ctx, float x, float y, float w, float h) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->scissor(x, y, w, h);
}

void nvgIntersectScissor(NVGcontext* ctx, float x, float y, float w, float h) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->intersectScissor(x, y, w, h);
}

void nvgResetScissor(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->resetScissor();
}

void nvgBeginPath(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->beginPath();
}

void nvgMoveTo(NVGcontext* ctx, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->moveTo(x, y);
}

void nvgLineTo(NVGcontext* ctx, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->lineTo(x, y);
}

void nvgBezierTo(NVGcontext* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->bezierTo(c1x, c1y, c2x, c2y, x, y);
}

void nvgQuadTo(NVGcontext* ctx, float cx, float cy, float x, float y) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->quadTo(cx, cy, x, y);
}

void nvgArcTo(NVGcontext* ctx, float x1, float y1, float x2, float y2, float radius) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->arcTo(x1, y1, x2, y2, radius);
}

void nvgClosePath(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->closePath();
}

void nvgPathWinding(NVGcontext* ctx, int dir) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->pathWinding(dir);
}

void nvgArc(NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->arc(cx, cy, r, a0, a1, dir);
}

void nvgRect(NVGcontext* ctx, float x, float y, float w, float h) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->rect(x, y, w, h);
}

void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->roundedRect(x, y, w, h, r);
}

void nvgRoundedRectVarying(NVGcontext* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->roundedRectVarying(x, y, w, h, radTopLeft, radTopRight, radBottomRight, radBottomLeft);
}

void nvgEllipse(NVGcontext* ctx, float cx, float cy, float rx, float ry) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->ellipse(cx, cy, rx, ry);
}

void nvgCircle(NVGcontext* ctx, float cx, float cy, float r) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->circle(cx, cy, r);
}

void nvgFill(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fill();
}

void nvgStroke(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->stroke();
}

int nvgCreateFont(NVGcontext* ctx, const char* name, const char* filename) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createFont(name, filename);
	return -1;
}

int nvgCreateFontAtIndex(NVGcontext* ctx, const char* name, const char* filename, const int fontIndex) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createFontAtIndex(name, filename, fontIndex);
	return -1;
}

int nvgCreateFontMem(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createFontMem(name, data, ndata, freeData);
	return -1;
}

int nvgCreateFontMemAtIndex(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->createFontMemAtIndex(name, data, ndata, freeData, fontIndex);
	return -1;
}

int nvgFindFont(NVGcontext* ctx, const char* name) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->findFont(name);
	return -1;
}

int nvgAddFallbackFontId(NVGcontext* ctx, int baseFont, int fallbackFont) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->addFallbackFontId(baseFont, fallbackFont);
	return 0;
}

int nvgAddFallbackFont(NVGcontext* ctx, const char* baseFont, const char* fallbackFont) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->addFallbackFont(baseFont, fallbackFont);
	return 0;
}

void nvgResetFallbackFontsId(NVGcontext* ctx, int baseFont) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->resetFallbackFontsId(baseFont);
}

void nvgResetFallbackFonts(NVGcontext* ctx, const char* baseFont) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->resetFallbackFonts(baseFont);
}

void nvgFontSize(NVGcontext* ctx, float size) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fontSize(size);
}

void nvgFontBlur(NVGcontext* ctx, float blur) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fontBlur(blur);
}

void nvgFontDilate(NVGcontext* ctx, float dilate) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fontDilate(dilate);
}

void nvgTextLetterSpacing(NVGcontext* ctx, float spacing) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textLetterSpacing(spacing);
}

void nvgTextLineHeight(NVGcontext* ctx, float lineHeight) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textLineHeight(lineHeight);
}

void nvgTextAlign(NVGcontext* ctx, int align) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textAlign(align);
}

void nvgFontFaceId(NVGcontext* ctx, int font) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fontFaceId(font);
}

void nvgFontFace(NVGcontext* ctx, const char* font) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->fontFace(font);
}

int nvgGetFontFaceId(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->getFontFaceId();
	return -1;
}

float nvgGetFontSize(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->getFontSize();
	return 0.0f;
}

float nvgGetStrokeWidth(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->getStrokeWidth();
	return 0.0f;
}

int nvgGetTextAlign(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->getTextAlign();
	return 0;
}

float nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->text(x, y, string, end);
	return 0.0f;
}

void nvgTextBox(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textBox(x, y, breakRowWidth, string, end);
}

float nvgTextBounds(NVGcontext* ctx, float x, float y, const char* string, const char* end, float* bounds) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->textBounds(x, y, string, end, bounds);
	return 0.0f;
}

void nvgTextBoxBounds(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textBoxBounds(x, y, breakRowWidth, string, end, bounds);
}

int nvgTextGlyphPositions(NVGcontext* ctx, float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->textGlyphPositions(x, y, string, end, reinterpret_cast<nvg::GlyphPosition*>(positions), maxPositions);
	return 0;
}

void nvgTextMetrics(NVGcontext* ctx, float* ascender, float* descender, float* lineh) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->textMetrics(ascender, descender, lineh);
}

int nvgTextBreakLines(NVGcontext* ctx, const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows, int skipSpaces) {
	if (nvg::Context* c = ctxPtr(ctx))
		return c->textBreakLines(string, end, breakRowWidth, reinterpret_cast<nvg::TextRow*>(rows), maxRows, skipSpaces);
	return 0;
}

int nvgGetImageTextureId(NVGcontext* ctx, int handle) {
	nvg::Context* c = ctxPtr(ctx);
	if (!c)
		return 0;
	const nvg::Params& p = c->internalParams();
	if (!p.renderGetImageTextureId)
		return 0;
	return p.renderGetImageTextureId(p.userPtr, handle);
}

NVGcontext* nvgCreateInternal(NVGparams* params) {
	if (!params)
		return nullptr;
	try {
		nvg::Params p = copyParams(*params);
		auto* wrap = new NVGcontext;
		wrap->impl = std::make_unique<nvg::Context>(p);
		return wrap;
	} catch (...) {
		return nullptr;
	}
}

void nvgDeleteInternal(NVGcontext* ctx) {
	delete ctx;
}

NVGparams* nvgInternalParams(NVGcontext* ctx) {
	nvg::Context* c = ctxPtr(ctx);
	if (!c)
		return nullptr;
	return reinterpret_cast<NVGparams*>(const_cast<nvg::Params*>(&c->internalParams()));
}

void nvgDebugDumpPathCache(NVGcontext* ctx) {
	if (nvg::Context* c = ctxPtr(ctx))
		c->debugDumpPathCache();
}

} // extern "C"
