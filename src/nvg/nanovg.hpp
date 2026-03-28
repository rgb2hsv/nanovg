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

#pragma once
#include <type_traits>
#include <memory>

#ifndef UNUSED
#define UNUSED(v) (void)(v)
#endif

namespace nvg {

struct ContextImpl;

template<typename E>
	requires std::is_enum_v<E>
[[nodiscard]] constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // nonstandard extension used : nameless struct/union
#endif

struct Color {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};
};

constexpr float PI=3.14159265358979323846264338327f;

struct Paint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	Color innerColor;
	Color outerColor;
	int image;
};

enum class Winding : int {
	CCW = 1, // Winding for solid shapes
	CW = 2,  // Winding for holes
};

enum class Solidity : int {
	Solid = 1, // CCW
	Hole = 2,  // CW
};

enum class LineStyle : int {
	Solid = 1,
	Dashed = 2,
	Dotted = 3,
	Glow = 4
};

enum class LineCap : int {
	Butt = 0,
	Round = 1,
	Square = 2,
	Bevel = 3,
	Miter = 4,
};

enum class Align : int {
	Left = 1 << 0,
	Center = 1 << 1,
	Right = 1 << 2,
	Top = 1 << 3,
	Middle = 1 << 4,
	MiddleAscent = 1 << 5,
	Bottom = 1 << 6,
	Baseline = 1 << 7,
};

inline constexpr Align operator|(Align a, Align b) noexcept {
	return static_cast<Align>(to_underlying(a) | to_underlying(b));
}

inline constexpr int operator&(int a, Align b) noexcept {
	return a & to_underlying(b);
}

enum class BlendFactor : int {
	Zero = 1 << 0,
	One = 1 << 1,
	SrcColor = 1 << 2,
	OneMinusSrcColor = 1 << 3,
	DstColor = 1 << 4,
	OneMinusDstColor = 1 << 5,
	SrcAlpha = 1 << 6,
	OneMinusSrcAlpha = 1 << 7,
	DstAlpha = 1 << 8,
	OneMinusDstAlpha = 1 << 9,
	SrcAlphaSaturate = 1 << 10,
};

enum class CompositeOperation : int {
	SourceOver,
	SourceIn,
	SourceOut,
	Atop,
	DestinationOver,
	DestinationIn,
	DestinationOut,
	DestinationAtop,
	Lighter,
	Copy,
	Xor,
};

struct CompositeOperationState {
	int srcRGB;
	int dstRGB;
	int srcAlpha;
	int dstAlpha;
};

struct GlyphPosition {
	const char* str;	// Position of the glyph in the input string.
	float x;			// The x-coordinate of the logical glyph position.
	float minx, maxx;	// The bounds of the glyph shape.
};

struct TextRow {
	const char* start;	// Pointer to the input text where the row starts.
	const char* end;	// Pointer to the input text where the row ends (one past the last character).
	const char* next;	// Pointer to the beginning of the next row.
	float width;		// Logical width of the row.
	float minx, maxx;	// Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
};

enum class ImageFlags : int {
	GenerateMipmaps = 1 << 0,
	RepeatX = 1 << 1,
	RepeatY = 1 << 2,
	Flipy = 1 << 3,
	Premultiplied = 1 << 4,
	Nearest = 1 << 5,
};

inline constexpr ImageFlags operator|(ImageFlags a, ImageFlags b) noexcept {
	return static_cast<ImageFlags>(to_underlying(a) | to_underlying(b));
}

inline constexpr int operator&(int a, ImageFlags b) noexcept { return a & to_underlying(b); }

// Internal Render API
//
enum class Texture : int {
	Alpha = 0x01,
	Rgba = 0x02,
};

struct Scissor {
	float xform[6];
	float extent[2];
};

struct ScissorBounds {
	float x;
	float y;
	float w;
	float h;
};

struct Vertex {
	float x,y,u,v,s,t;
};

struct Path {
	int first;
	int count;
	int reversed;
	unsigned char closed;
	int nbevel;
	Vertex* fill;
	int nfill;
	Vertex* stroke;
	int nstroke;
	int winding;
	int convex;
};

struct Params {
	void* userPtr;
	int edgeAntiAlias;
	int (*renderCreate)(void* uptr);
	int (*renderCreateTexture)(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
	int (*renderDeleteTexture)(void* uptr, int image);
	int (*renderUpdateTexture)(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
	int (*renderGetTextureSize)(void* uptr, int image, int* w, int* h);
	int (*renderGetImageTextureId)(void* uptr, int handle);
	void (*renderViewport)(void* uptr, float width, float height, float devicePixelRatio);
	void (*renderCancel)(void* uptr);
	void (*renderFlush)(void* uptr);
	void (*renderFill)(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor, float fringe, const float* bounds, const Path* paths, int npaths);
	void (*renderStroke)(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor, float fringe, float strokeWidth, int lineStyle, const Path* paths, int npaths);
	void (*renderTriangles)(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor, const Vertex* verts, int nverts, float fringe);
	void (*renderDelete)(void* uptr);
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

class Context {
public:
	explicit Context(const Params& params);
	~Context();
	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	ContextImpl& operator*() noexcept;
	const ContextImpl& operator*() const noexcept;
	ContextImpl* operator->() noexcept;
	const ContextImpl* operator->() const noexcept;

	void beginFrame(float windowWidth, float windowHeight, float devicePixelRatio);
	void cancelFrame();
	void endFrame();
	void globalCompositeOperation(int op);
	void globalCompositeBlendFunc(int sfactor, int dfactor);
	void globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);
	void save();
	void restore();
	void reset();
	ScissorBounds currentScissor() const;
	void shapeAntiAlias(int enabled);
	void strokeColor(const Color& color);
	void strokePaint(const Paint& paint);
	void fillColor(const Color& color);
	void fillPaint(const Paint& paint);
	void miterLimit(float limit);
	void strokeWidth(float size);
	void lineStyle(int lineStyle);
	void lineCap(int cap);
	void lineJoin(int join);
	void globalAlpha(float alpha);
	void resetTransform();
	void transform(float a, float b, float c, float d, float e, float f);
	void translate(float x, float y);
	void rotate(float angle);
	void skewX(float angle);
	void skewY(float angle);
	void scale(float x, float y);
	void currentTransform(float* xform) const;
	int createImage(const char* filename, int imageFlags);
	int createImageMem(int imageFlags, unsigned char* data, int ndata);
	int createImageRGBA(int w, int h, int imageFlags, const unsigned char* data);
	void updateImage(int image, const unsigned char* data);
	void imageSize(int image, int& w, int& h) const;
	void deleteImage(int image);
	Paint linearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol) const;
	Paint boxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol) const;
	Paint radialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol) const;
	Paint imagePattern(float ox, float oy, float ex, float ey, float angle, int image, float alpha) const;
	void scissor(float x, float y, float w, float h);
	void intersectScissor(float x, float y, float w, float h);
	void resetScissor();
	void beginPath();
	void moveTo(float x, float y);
	void lineTo(float x, float y);
	void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
	void quadTo(float cx, float cy, float x, float y);
	void arcTo(float x1, float y1, float x2, float y2, float radius);
	void closePath();
	void pathWinding(int dir);
	void arc(float cx, float cy, float r, float a0, float a1, int dir);
	void rect(float x, float y, float w, float h);
	void roundedRect(float x, float y, float w, float h, float r);
	void roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
	void ellipse(float cx, float cy, float rx, float ry);
	void circle(float cx, float cy, float r);
	void fill();
	void stroke();
	int createFont(const char* name, const char* filename);
	int createFontAtIndex(const char* name, const char* filename, const int fontIndex);
	int createFontMem(const char* name, unsigned char* data, int ndata, int freeData);
	int createFontMemAtIndex(const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);
	int findFont(const char* name) const;
	int addFallbackFontId(int baseFont, int fallbackFont);
	int addFallbackFont(const char* baseFont, const char* fallbackFont);
	void resetFallbackFontsId(int baseFont);
	void resetFallbackFonts(const char* baseFont);
	void fontSize(float size);
	void fontBlur(float blur);
	void fontDilate(float dilate);
	void textLetterSpacing(float spacing);
	void textLineHeight(float lineHeight);
	void textAlign(int align);
	void fontFaceId(int font);
	void fontFace(const char* font);
	int getFontFaceId() const;
	float getFontSize() const;
	float getStrokeWidth() const;
	int getTextAlign() const;
	float text(float x, float y, const char* string, const char* end);
	void textBox(float x, float y, float breakRowWidth, const char* string, const char* end);
	float textBounds(float x, float y, const char* string, const char* end, float* bounds);
	void textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);
	int textGlyphPositions(float x, float y, const char* string, const char* end, GlyphPosition* positions, int maxPositions);
	void textMetrics(float* ascender, float* descender, float* lineh);
	int textBreakLines(const char* string, const char* end, float breakRowWidth, TextRow* rows, int maxRows, int skipSpaces);
	const Params& internalParams() const;
	void debugDumpPathCache() const;

protected:
	std::shared_ptr<ContextImpl> mImpl;

};

// Frame drawing, paths, paints, text, images, and state: use Context members, e.g. ctx.beginFrame(...)
// or ctx->fill() when using a pointer or smart pointer to Context.

//
// Color utils
//
// Colors in NanoVG are stored as unsigned ints in ABGR format.

// Returns a color value from red, green, blue values. Alpha will be set to 255 (1.0f).
Color rgb(unsigned char r, unsigned char g, unsigned char b);

// Returns a color value from red, green, blue values. Alpha will be set to 1.0f.
Color rgbf(float r, float g, float b);


// Returns a color value from red, green, blue and alpha values.
Color rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

// Returns a color value from red, green, blue and alpha values.
Color rgbaf(float r, float g, float b, float a);


// Linearly interpolates from color c0 to c1, and returns resulting color value.
Color lerpRgba(Color c0, Color c1, float u);

// Sets transparency of a color value.
Color transRgba(Color c0, unsigned char a);

// Sets transparency of a color value.
Color transRgbaf(Color c0, float a);

// Returns color value specified by hue, saturation and lightness.
// HSL values are all in range [0..1], alpha will be set to 255.
Color hsl(float h, float s, float l);

// Returns color value specified by hue, saturation and lightness and alpha.
// HSL values are all in range [0..1], alpha in range [0..255]
Color hsla(float h, float s, float l, unsigned char a);

//
// Transforms (matrix utilities; not tied to Context)
// A 2x3 matrix is represented as float[6].

// Sets the transform to identity matrix.
void transformIdentity(float* dst);

// Sets the transform to translation matrix matrix.
void transformTranslate(float* dst, float tx, float ty);

// Sets the transform to scale matrix.
void transformScale(float* dst, float sx, float sy);

// Sets the transform to rotate matrix. Angle is specified in radians.
void transformRotate(float* dst, float a);

// Sets the transform to skew-x matrix. Angle is specified in radians.
void transformSkewX(float* dst, float a);

// Sets the transform to skew-y matrix. Angle is specified in radians.
void transformSkewY(float* dst, float a);

// Sets the transform to the result of multiplication of two transforms, of A = A*B.
void transformMultiply(float* dst, const float* src);

// Sets the transform to the result of multiplication of two transforms, of A = B*A.
void transformPremultiply(float* dst, const float* src);

// Sets the destination to inverse of specified transform.
// Returns 1 if the inverse could be calculated, else 0.
int transformInverse(float* dst, const float* src);

// Transform a point by given transform.
void transformPoint(float& dstx, float& dsty, const float* xform, float srcx, float srcy);

// Converts degrees to radians and vice versa.
float degToRad(float deg);
float radToDeg(float rad);

} // namespace nvg
