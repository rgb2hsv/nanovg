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

template<typename E>
	requires std::is_enum_v<E>
[[nodiscard]] constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // nonstandard extension used : nameless struct/union
#endif

typedef struct Context Context;

struct Color {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};
};
typedef struct Color Color;
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
typedef struct Paint Paint;

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
typedef struct CompositeOperationState CompositeOperationState;

struct GlyphPosition {
	const char* str;	// Position of the glyph in the input string.
	float x;			// The x-coordinate of the logical glyph position.
	float minx, maxx;	// The bounds of the glyph shape.
};
typedef struct GlyphPosition GlyphPosition;

struct TextRow {
	const char* start;	// Pointer to the input text where the row starts.
	const char* end;	// Pointer to the input text where the row ends (one past the last character).
	const char* next;	// Pointer to the beginning of the next row.
	float width;		// Logical width of the row.
	float minx, maxx;	// Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
};
typedef struct TextRow TextRow;

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
typedef struct Scissor Scissor;

struct ScissorBounds {
	float x;
	float y;
	float w;
	float h;
};
typedef struct ScissorBounds ScissorBounds;

struct Vertex {
	float x,y,u,v,s,t;
};
typedef struct Vertex Vertex;

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
typedef struct Path Path;

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
typedef struct Params Params;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Begin drawing a new frame
// Calls to nanovg drawing API should be wrapped in beginFrame() & endFrame()
// beginFrame() defines the size of the window to render to in relation currently
// set viewport (i.e. glViewport on GL backends). Device pixel ration allows to
// control the rendering on Hi-DPI devices.
// For example, GLFW returns two dimension for an opened window: window size and
// frame buffer size. In that case you would set windowWidth/Height to the window size
// devicePixelRatio to: frameBufferWidth / windowWidth.
void beginFrame(Context& ctx, float windowWidth, float windowHeight, float devicePixelRatio);

// Cancels drawing the current frame.
void cancelFrame(Context& ctx);

// Ends drawing flushing remaining render state.
void endFrame(Context& ctx);

//
// Composite operation
//
// The composite operations in NanoVG are modeled after HTML Canvas API, and
// the blend func is based on OpenGL (see corresponding manuals for more info).
// The colors in the blending state have premultiplied alpha.

// Sets the composite operation. The op parameter should be one of CompositeOperation.
void globalCompositeOperation(Context& ctx, int op);

// Sets the composite operation with custom pixel arithmetic. The parameters should be one of BlendFactor.
void globalCompositeBlendFunc(Context& ctx, int sfactor, int dfactor);

// Sets the composite operation with custom pixel arithmetic for RGB and alpha components separately. The parameters should be one of BlendFactor.
void globalCompositeBlendFuncSeparate(Context& ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

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
// State Handling
//
// NanoVG contains state which represents how paths will be rendered.
// The state contains transform, fill and stroke styles, text and font styles,
// and scissor clipping.

// Pushes and saves the current render state into a state stack.
// A matching restore() must be used to restore the state.
void save(Context& ctx);

// Pops and restores current render state.
void restore(Context& ctx);

// Resets current render state to default values. Does not affect the render state stack.
void reset(Context& ctx);

// Gets the current scissor bounds
ScissorBounds currentScissor(Context& ctx);

//
// Render styles
//
// Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
// Solid color is simply defined as a color value, different kinds of paints can be created
// using linearGradient(), boxGradient(), radialGradient() and imagePattern().
//
// Current render style can be saved and restored using save() and restore().

// Sets whether to draw antialias for stroke() and fill(). It's enabled by default.
void shapeAntiAlias(Context& ctx, int enabled);

// Sets current stroke style to a solid color.
void strokeColor(Context& ctx, Color color);

// Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
void strokePaint(Context& ctx, Paint paint);

// Sets current fill style to a solid color.
void fillColor(Context& ctx, Color color);

// Sets current fill style to a paint, which can be a one of the gradients or a pattern.
void fillPaint(Context& ctx, Paint paint);

// Sets the miter limit of the stroke style.
// Miter limit controls when a sharp corner is beveled.
void miterLimit(Context& ctx, float limit);

// Sets the stroke width of the stroke style.
void strokeWidth(Context& ctx, float size);

// Sets how line is drawn.
// Can be one of LineStyle::Solid (default), LineStyle::Glow, LineStyle::Dashed, LineStyle::Dotted
void lineStyle(Context& ctx, int lineStyle);

// Sets how the end of the line (cap) is drawn,
// Can be one of: LineCap::Butt (default), LineCap::Round, LineCap::Square.
void lineCap(Context& ctx, int cap);

// Sets how sharp path corners are drawn.
// Can be one of LineCap::Miter (default), LineCap::Round, LineCap::Bevel.
void lineJoin(Context& ctx, int join);

// Sets the transparency applied to all rendered shapes.
// Already transparent paths will get proportionally more transparent as well.
void globalAlpha(Context& ctx, float alpha);

//
// Transforms
//
// The paths, gradients, patterns and scissor region are transformed by an transformation
// matrix at the time when they are passed to the API.
// The current transformation matrix is a affine matrix:
//   [sx kx tx]
//   [ky sy ty]
//   [ 0  0  1]
// Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.
// The last row is assumed to be 0,0,1 and is not stored.
//
// Apart from resetTransform(), each transformation function first creates
// specific transformation matrix and pre-multiplies the current transformation by it.
//
// Current coordinate system (transformation) can be saved and restored using save() and restore().

// Resets current transform to a identity matrix.
void resetTransform(Context& ctx);

// Premultiplies current coordinate system by specified matrix.
// The parameters are interpreted as matrix as follows:
//   [a c e]
//   [b d f]
//   [0 0 1]
void transform(Context& ctx, float a, float b, float c, float d, float e, float f);

// Translates current coordinate system.
void translate(Context& ctx, float x, float y);

// Rotates current coordinate system. Angle is specified in radians.
void rotate(Context& ctx, float angle);

// Skews the current coordinate system along X axis. Angle is specified in radians.
void skewX(Context& ctx, float angle);

// Skews the current coordinate system along Y axis. Angle is specified in radians.
void skewY(Context& ctx, float angle);

// Scales the current coordinate system.
void scale(Context& ctx, float x, float y);

// Stores the top part (a-f) of the current transformation matrix in to the specified buffer.
//   [a c e]
//   [b d f]
//   [0 0 1]
// There should be space for 6 floats in the return buffer for the values a-f.
void currentTransform(Context& ctx, float* xform);


// The following functions can be used to make calculations on 2x3 transformation matrices.
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

//
// Images
//
// NanoVG allows you to load jpg, png, psd, tga, pic and gif files to be used for rendering.
// In addition you can upload your own image. The image loading is provided by stb_image.
// The parameter imageFlags is combination of flags defined in ImageFlags.

// Creates image by loading it from the disk from specified file name.
// Returns handle to the image.
int createImage(Context& ctx, const char* filename, int imageFlags);

// Creates image by loading it from the specified chunk of memory.
// Returns handle to the image.
int createImageMem(Context& ctx, int imageFlags, unsigned char* data, int ndata);

// Creates image from specified image data.
// Returns handle to the image.
int createImageRGBA(Context& ctx, int w, int h, int imageFlags, const unsigned char* data);

// Updates image data specified by image handle.
void updateImage(Context& ctx, int image, const unsigned char* data);

// Returns the dimensions of a created image.
void imageSize(Context& ctx, int image, int& w, int& h);

// Deletes created image.
void deleteImage(Context& ctx, int image);

//
// Paints
//
// NanoVG supports four types of paints: linear gradient, box gradient, radial gradient and image pattern.
// These can be used as paints for strokes and fills.

// Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
// of the linear gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
Paint linearGradient(Context& ctx, float sx, float sy, float ex, float ey,
						   Color icol, Color ocol);

// Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
// drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
// (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
// the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
Paint boxGradient(Context& ctx, float x, float y, float w, float h,
						float r, float f, Color icol, Color ocol);

// Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
// the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
Paint radialGradient(Context& ctx, float cx, float cy, float inr, float outr,
						   Color icol, Color ocol);

// Creates and returns an image pattern. Parameters (ox,oy) specify the left-top location of the image pattern,
// (ex,ey) the size of one image, angle rotation around the top-left corner, image is handle to the image to render.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
Paint imagePattern(Context& ctx, float ox, float oy, float ex, float ey,
						 float angle, int image, float alpha);

//
// Scissoring
//
// Scissoring allows you to clip the rendering into a rectangle. This is useful for various
// user interface cases like rendering a text edit or a timeline.

// Sets the current scissor rectangle.
// The scissor rectangle is transformed by the current transform.
void scissor(Context& ctx, float x, float y, float w, float h);

// Intersects current scissor rectangle with the specified rectangle.
// The scissor rectangle is transformed by the current transform.
// Note: in case the rotation of previous scissor rect differs from
// the current one, the intersection will be done between the specified
// rectangle and the previous scissor rectangle transformed in the current
// transform space. The resulting shape is always rectangle.
void intersectScissor(Context& ctx, float x, float y, float w, float h);

// Reset and disables scissoring.
void resetScissor(Context& ctx);

//
// Paths
//
// Drawing a new shape starts with beginPath(), it clears all the currently defined paths.
// Then you define one or more paths and sub-paths which describe the shape. The are functions
// to draw common shapes like rectangles and circles, and lower level step-by-step functions,
// which allow to define a path curve by curve.
//
// NanoVG uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
// winding and holes should have counter clockwise order. To specify winding of a path you can
// call pathWinding(). This is useful especially for the common shapes, which are drawn CCW.
//
// Finally you can fill the path using current fill style by calling fill(), and stroke it
// with current stroke style by calling stroke().
//
// The curve segments and sub-paths are transformed by the current transform.

// Clears the current path and sub-paths.
void beginPath(Context& ctx);

// Starts new sub-path with specified point as first point.
void moveTo(Context& ctx, float x, float y);

// Adds line segment from the last point in the path to the specified point.
void lineTo(Context& ctx, float x, float y);

// Adds cubic bezier segment from last point in the path via two control points to the specified point.
void bezierTo(Context& ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);

// Adds quadratic bezier segment from last point in the path via a control point to the specified point.
void quadTo(Context& ctx, float cx, float cy, float x, float y);

// Adds an arc segment at the corner defined by the last path point, and two specified points.
void arcTo(Context& ctx, float x1, float y1, float x2, float y2, float radius);

// Closes current sub-path with a line segment.
void closePath(Context& ctx);

// Sets the current sub-path winding, see Winding and Solidity.
void pathWinding(Context& ctx, int dir);

// Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
// and the arc is drawn from angle a0 to a1, and swept in direction dir (Winding::CCW, or Winding::CW).
// Angles are specified in radians.
void arc(Context& ctx, float cx, float cy, float r, float a0, float a1, int dir);

// Creates new rectangle shaped sub-path.
void rect(Context& ctx, float x, float y, float w, float h);

// Creates new rounded rectangle shaped sub-path.
void roundedRect(Context& ctx, float x, float y, float w, float h, float r);

// Creates new rounded rectangle shaped sub-path with varying radii for each corner.
void roundedRectVarying(Context& ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

// Creates new ellipse shaped sub-path.
void ellipse(Context& ctx, float cx, float cy, float rx, float ry);

// Creates new circle shaped sub-path.
void circle(Context& ctx, float cx, float cy, float r);

// Fills the current path with current fill style.
void fill(Context& ctx);

// Fills the current path with current stroke style.
void stroke(Context& ctx);


//
// Text
//
// NanoVG allows you to load .ttf files and use the font to render text.
//
// The appearance of the text can be defined by setting the current text style
// and by specifying the fill color. Common text and font settings such as
// font size, letter spacing and text align are supported. Font blur allows you
// to create simple text effects such as drop shadows.
//
// At render time the font face can be set based on the font handles or name.
//
// Font measure functions return values in local space, the calculations are
// carried in the same resolution as the final rendering. This is done because
// the text glyph positions are snapped to the nearest pixels sharp rendering.
//
// The local space means that values are not rotated or scale as per the current
// transformation. For example if you set font size to 12, which would mean that
// line height is 16, then regardless of the current scaling and rotation, the
// returned line height is always 16. Some measures may vary because of the scaling
// since aforementioned pixel snapping.
//
// While this may sound a little odd, the setup allows you to always render the
// same way regardless of scaling. I.e. following works regardless of scaling:
//
//		const char* txt = "Text me up.";
//		textBounds(vg, x,y, txt, NULL, bounds);
//		beginPath(vg);
//		rect(vg, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
//		fill(vg);
//
// Note: currently only solid color fill is supported for text.

// Creates font by loading it from the disk from specified file name.
// Returns handle to the font.
int createFont(Context& ctx, const char* name, const char* filename);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int createFontAtIndex(Context& ctx, const char* name, const char* filename, const int fontIndex);

// Creates font by loading it from the specified memory chunk.
// Returns handle to the font.
int createFontMem(Context& ctx, const char* name, unsigned char* data, int ndata, int freeData);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int createFontMemAtIndex(Context& ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);

// Finds a loaded font of specified name, and returns handle to it, or -1 if the font is not found.
int findFont(Context& ctx, const char* name);

// Adds a fallback font by handle.
int addFallbackFontId(Context& ctx, int baseFont, int fallbackFont);

// Adds a fallback font by name.
int addFallbackFont(Context& ctx, const char* baseFont, const char* fallbackFont);

// Resets fallback fonts by handle.
void resetFallbackFontsId(Context& ctx, int baseFont);

// Resets fallback fonts by name.
void resetFallbackFonts(Context& ctx, const char* baseFont);

// Sets the font size of current text style.
void fontSize(Context& ctx, float size);

// Sets the blur of current text style.
void fontBlur(Context& ctx, float blur);

// Sets the dilation of current text style.
void fontDilate(Context& ctx, float dilate);

// Sets the letter spacing of current text style.
void textLetterSpacing(Context& ctx, float spacing);

// Sets the proportional line height of current text style. The line height is specified as multiple of font size.
void textLineHeight(Context& ctx, float lineHeight);

// Sets the text align of current text style, see Align for options.
void textAlign(Context& ctx, int align);

// Sets the font face based on specified id of current text style.
void fontFaceId(Context& ctx, int font);

// Sets the font face based on specified name of current text style.
void fontFace(Context& ctx, const char* font);

// Gets the font size of current text style.
int getFontFaceId(Context& ctx);

// Get the font size
float getFontSize(Context& ctx);

//Get Stroke width
float getStrokeWidth(Context& ctx);

// Get text alignment
int getTextAlign(Context& ctx);

// Draws text string at specified location. If end is specified only the sub-string up to the end is drawn.
float text(Context& ctx, float x, float y, const char* string, const char* end);

// Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the sub-string up to the end is drawn.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
void textBox(Context& ctx, float x, float y, float breakRowWidth, const char* string, const char* end);

// Measures the specified text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Returns the horizontal advance of the measured text (i.e. where the next character should drawn).
// Measured values are returned in local coordinate space.
float textBounds(Context& ctx, float x, float y, const char* string, const char* end, float* bounds);

// Measures the specified multi-text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Measured values are returned in local coordinate space.
void textBoxBounds(Context& ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);

// Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
// Measured values are returned in local coordinate space.
int textGlyphPositions(Context& ctx, float x, float y, const char* string, const char* end, GlyphPosition* positions, int maxPositions);

// Returns the vertical metrics based on the current text style.
// Measured values are returned in local coordinate space.
void textMetrics(Context& ctx, float* ascender, float* descender, float* lineh);

// Breaks the specified text into lines. If end is specified only the sub-string will be used.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
int textBreakLines(Context& ctx, const char* string, const char* end, float breakRowWidth, TextRow* rows, int maxRows, int skipSpaces);

// Get image texture Id
int getImageTextureId(Context& ctx, int handle);

//
// Constructor and destructor, called by the render back-end.
std::shared_ptr<Context> createInternal(Params* params);
void deleteInternal(const std::shared_ptr<Context>& ctx);

Params* internalParams(Context& ctx);

// Debug function to dump cached path data.
void debugDumpPathCache(Context& ctx);

} // namespace nvg
