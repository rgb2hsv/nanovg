//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
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
#include "nanovg.hpp"
#include <array>
#include <memory>

namespace nvg {

enum class CreateFlags : int {
	Antialias = 1 << 0,
	StencilStrokes = 1 << 1,
	Debug = 1 << 2,
};

inline constexpr CreateFlags operator|(CreateFlags a, CreateFlags b) noexcept {
	return static_cast<CreateFlags>(to_underlying(a) | to_underlying(b));
}

inline constexpr int operator&(int a, CreateFlags b) noexcept { return a & to_underlying(b); }

} // namespace nvg

#if defined NANOVG_GL2_IMPLEMENTATION
#  define NANOVG_GL2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GL3_IMPLEMENTATION
#  define NANOVG_GL3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#  define NANOVG_GL_USE_UNIFORMBUFFER 1
#elif defined NANOVG_GLES2_IMPLEMENTATION
#  define NANOVG_GLES2 1
#  define NANOVG_GL_IMPLEMENTATION 1
#elif defined NANOVG_GLES3_IMPLEMENTATION
#  define NANOVG_GLES3 1
#  define NANOVG_GL_IMPLEMENTATION 1
#endif

#define NANOVG_GL_USE_STATE_FILTER (1)

// Creates NanoVG contexts for different OpenGL (ES) versions.
// Flags should be combination of the create flags above.

namespace nvg {
std::unique_ptr<Context> createGL(int flags);
void deleteGL(std::unique_ptr<Context>& ctx);

#if defined NANOVG_GL2
int nvglCreateImageFromHandleGL2(Context& ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandleGL2(Context& ctx, int image);
#endif

#if defined NANOVG_GL3
int nvglCreateImageFromHandleGL3(Context& ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandleGL3(Context& ctx, int image);
#endif

#if defined NANOVG_GLES2
int nvglCreateImageFromHandleGLES2(Context& ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandleGLES2(Context& ctx, int image);
#endif

#if defined NANOVG_GLES3
int nvglCreateImageFromHandleGLES3(Context& ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandleGLES3(Context& ctx, int image);
#endif
} // namespace nvg

namespace nvg {

// Additional flags on top of ImageFlags (combined in the same int).
enum class ImageFlagsGl : int {
	NoDelete = 1 << 16,
};

inline constexpr int operator|(int a, ImageFlagsGl b) noexcept { return a | to_underlying(b); }
inline constexpr int operator|(ImageFlags a, ImageFlagsGl b) noexcept {
	return to_underlying(a) | to_underlying(b);
}
inline constexpr int operator|(ImageFlagsGl a, ImageFlags b) noexcept {
	return to_underlying(a) | to_underlying(b);
}
inline constexpr int operator&(int a, ImageFlagsGl b) noexcept { return a & to_underlying(b); }

} // namespace nvg

#ifdef NANOVG_GL_IMPLEMENTATION

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "nanovg.hpp"

namespace nvg {

enum GlUniformLoc {
	GlUniformLocViewSize,
	GlUniformLocTex,
	GlUniformLocFrag,
	GlUniformLocCount
};

enum GlShaderType {
	NSVG_SHADER_FILLGRAD,
	NSVG_SHADER_FILLIMG,
	NSVG_SHADER_SIMPLE,
	NSVG_SHADER_IMG
};

#if NANOVG_GL_USE_UNIFORMBUFFER
enum GlUniformBindings {
	GlFragBinding = 0,
};
#endif

struct GlShader {
	GLuint prog;
	GLuint frag;
	GLuint vert;
	std::array<GLint, GlUniformLocCount> loc{};
};
typedef struct GlShader GlShader;

struct GlTexture {
	int id;
	GLuint tex;
	int width, height;
	int type;
	int flags;
};
typedef struct GlTexture GlTexture;

struct GlBlend
{
	GLenum srcRGB;
	GLenum dstRGB;
	GLenum srcAlpha;
	GLenum dstAlpha;
};
typedef struct GlBlend GlBlend;

enum GlCallType {
	None = 0,
	Fill,
	ConvexFill,
	Stroke,
	Triangles,
};

struct GlCall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
	GlBlend blendFunc;
};
typedef struct GlCall GlCall;

struct GLPath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};
typedef struct GLPath GLPath;

struct GlFragUniforms {
	#if NANOVG_GL_USE_UNIFORMBUFFER
		float scissorMat[12]; // matrices are actually 3 vec4s
		float paintMat[12];
		struct Color innerCol;
		struct Color outerCol;
		float scissorExt[2];
		float scissorScale[2];
		float extent[2];
		float radius;
		float feather;
		float strokeMult;
		float strokeThr;
		int lineStyle;
		int texType;
		int type;
	#else
		// note: after modifying layout or size of uniform array,
		// don't forget to also update the fragment shader source!
		#define NANOVG_GL_UNIFORMARRAY_SIZE 12
		#ifdef _MSC_VER
		#pragma warning(push)
		#pragma warning(disable: 4201)  // nonstandard extension: nameless struct/union
		#endif
		union {
			struct {
				float scissorMat[12]; // matrices are actually 3 vec4s
				float paintMat[12];
				struct Color innerCol;
				struct Color outerCol;
				float scissorExt[2];
				float scissorScale[2];
				float extent[2];
				float radius;
				float feather;
				float strokeMult;
				float strokeThr;
				float lineStyle;
				float texType;
				float type;
				float unused1;
				float unused2;
				float unused3;
			};
			float uniformArray[NANOVG_GL_UNIFORMARRAY_SIZE][4];
		};
		#ifdef _MSC_VER
		#pragma warning(pop)
		#endif
	#endif
};
typedef struct GlFragUniforms GlFragUniforms;

struct GLContext {
	GlShader shader;
	GlTexture* textures;
	std::array<float, 2> view{};
	int ntextures;
	int ctextures;
	int textureId;
	GLuint vertBuf;
#if defined NANOVG_GL3
	GLuint vertArr;
#endif
#if NANOVG_GL_USE_UNIFORMBUFFER
	GLuint fragBuf;
#endif
	int fragSize;
	int flags;

	// Per frame buffers
	GlCall* calls;
	int ccalls;
	int ncalls;
	GLPath* paths;
	int cpaths;
	int npaths;
	struct Vertex* verts;
	int cverts;
	int nverts;
	unsigned char* uniforms;
	int cuniforms;
	int nuniforms;

	// cached state
	#if NANOVG_GL_USE_STATE_FILTER
	GLuint boundTexture;
	GLuint stencilMask;
	GLenum stencilFunc;
	GLint stencilFuncRef;
	GLuint stencilFuncMask;
	GlBlend blendFunc;
	#endif

	int dummyTex;
};
typedef struct GLContext GLContext;

static int glnvg__maxi(int a, int b) { return std::max(a, b); }

#ifdef NANOVG_GLES2
static unsigned int glnvg__nearestPow2(unsigned int num)
{
	unsigned n = num > 0 ? num - 1 : 0;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}
#endif

static void glnvg__bindTexture(GLContext* gl, GLuint tex)
{
#if NANOVG_GL_USE_STATE_FILTER
	if (gl->boundTexture != tex) {
		gl->boundTexture = tex;
		glBindTexture(GL_TEXTURE_2D, tex);
	}
#else
	glBindTexture(GL_TEXTURE_2D, tex);
#endif
}

static void glnvg__stencilMask(GLContext* gl, GLuint mask)
{
#if NANOVG_GL_USE_STATE_FILTER
	if (gl->stencilMask != mask) {
		gl->stencilMask = mask;
		glStencilMask(mask);
	}
#else
	glStencilMask(mask);
#endif
}

static void glnvg__stencilFunc(GLContext* gl, GLenum func, GLint ref, GLuint mask)
{
#if NANOVG_GL_USE_STATE_FILTER
	if ((gl->stencilFunc != func) ||
		(gl->stencilFuncRef != ref) ||
		(gl->stencilFuncMask != mask)) {

		gl->stencilFunc = func;
		gl->stencilFuncRef = ref;
		gl->stencilFuncMask = mask;
		glStencilFunc(func, ref, mask);
	}
#else
	glStencilFunc(func, ref, mask);
#endif
}
static void glnvg__blendFuncSeparate(GLContext* gl, const GlBlend* blend)
{
#if NANOVG_GL_USE_STATE_FILTER
	if ((gl->blendFunc.srcRGB != blend->srcRGB) ||
		(gl->blendFunc.dstRGB != blend->dstRGB) ||
		(gl->blendFunc.srcAlpha != blend->srcAlpha) ||
		(gl->blendFunc.dstAlpha != blend->dstAlpha)) {

		gl->blendFunc = *blend;
		glBlendFuncSeparate(blend->srcRGB, blend->dstRGB, blend->srcAlpha,blend->dstAlpha);
	}
#else
	glBlendFuncSeparate(blend->srcRGB, blend->dstRGB, blend->srcAlpha,blend->dstAlpha);
#endif
}

static GlTexture* glnvg__allocTexture(GLContext* gl)
{
	GlTexture* tex = nullptr;
	int i;

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == 0) {
			tex = &gl->textures[i];
			break;
		}
	}
	if (tex == nullptr) {
		if (gl->ntextures+1 > gl->ctextures) {
			GlTexture* textures;
			int ctextures = glnvg__maxi(gl->ntextures+1, 4) +  gl->ctextures/2; // 1.5x Overallocate
			textures = static_cast<GlTexture*>(std::realloc(gl->textures, sizeof(GlTexture)*ctextures));
			if (textures == nullptr) return nullptr;
			gl->textures = textures;
			gl->ctextures = ctextures;
		}
		tex = &gl->textures[gl->ntextures++];
	}

	*tex = GlTexture{};
	tex->id = ++gl->textureId;

	return tex;
}

static GlTexture* glnvg__findTexture(GLContext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++)
		if (gl->textures[i].id == id)
			return &gl->textures[i];
	return nullptr;
}

static int glnvg__deleteTexture(GLContext* gl, int id)
{
	int i;
	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].id == id) {
			if (gl->textures[i].tex != 0 && (gl->textures[i].flags & ImageFlagsGl::NoDelete) == 0)
				glDeleteTextures(1, &gl->textures[i].tex);
			std::memset(&gl->textures[i], 0, sizeof(gl->textures[i]));
			return 1;
		}
	}
	return 0;
}

static void glnvg__dumpShaderError(GLuint shader, const char* name, const char* type)
{
	std::array<GLchar, 512+1> str{};
	GLsizei len = 0;
	glGetShaderInfoLog(shader, 512, &len, str.data());
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Shader %s/%s error:\n%s\n", name, type, str.data());
}

static void glnvg__dumpProgramError(GLuint prog, const char* name)
{
	std::array<GLchar, 512+1> str{};
	GLsizei len = 0;
	glGetProgramInfoLog(prog, 512, &len, str.data());
	if (len > 512) len = 512;
	str[len] = '\0';
	printf("Program %s error:\n%s\n", name, str.data());
}

static void glnvg__checkError(GLContext* gl, const char* str)
{
	GLenum err;
	if ((gl->flags & CreateFlags::Debug) == 0) return;
	err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("Error %08x after %s\n", err, str);
		return;
	}
}

static int glnvg__createShader(GlShader* shader, const char* name, const char* header, const char* opts, const char* vshader, const char* fshader)
{
	GLint status;
	GLuint prog, vert, frag;
	const char* str[3];
	str[0] = header;
	str[1] = opts != NULL ? opts : "";

	*shader = GlShader{};

	prog = glCreateProgram();
	vert = glCreateShader(GL_VERTEX_SHADER);
	frag = glCreateShader(GL_FRAGMENT_SHADER);
	str[2] = vshader;
	glShaderSource(vert, 3, str, 0);
	str[2] = fshader;
	glShaderSource(frag, 3, str, 0);

	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(vert, name, "vert");
		return 0;
	}

	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpShaderError(frag, name, "frag");
		return 0;
	}

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);

	glBindAttribLocation(prog, 0, "vertex");
	glBindAttribLocation(prog, 1, "tcoord");

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		glnvg__dumpProgramError(prog, name);
		return 0;
	}

	shader->prog = prog;
	shader->vert = vert;
	shader->frag = frag;

	return 1;
}

static void glnvg__deleteShader(GlShader* shader)
{
	if (shader->prog != 0)
		glDeleteProgram(shader->prog);
	if (shader->vert != 0)
		glDeleteShader(shader->vert);
	if (shader->frag != 0)
		glDeleteShader(shader->frag);
}

static void glnvg__getUniforms(GlShader* shader)
{
	shader->loc[GlUniformLocViewSize] = glGetUniformLocation(shader->prog, "viewSize");
	shader->loc[GlUniformLocTex] = glGetUniformLocation(shader->prog, "tex");

#if NANOVG_GL_USE_UNIFORMBUFFER
	shader->loc[GlUniformLocFrag] = glGetUniformBlockIndex(shader->prog, "frag");
#else
	shader->loc[GlUniformLocFrag] = glGetUniformLocation(shader->prog, "frag");
#endif
}

static int glnvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);

static int glnvg__renderCreate(void* uptr)
{
	GLContext* gl = (GLContext*)uptr;
	int align = 4;

	// TODO: mediump float may not be enough for GLES2 in iOS.
	// see the following discussion: https://github.com/memononen/nanovg/issues/46
	static const char* shaderHeader =
#if defined NANOVG_GL2
		R"NVG(#define NANOVG_GL2 1
)NVG"
#elif defined NANOVG_GL3
		R"NVG(#version 150 core
#define NANOVG_GL3 1
)NVG"
#elif defined NANOVG_GLES2
		R"NVG(#version 100
#define NANOVG_GL2 1
)NVG"
#elif defined NANOVG_GLES3
		R"NVG(#version 300 es
#define NANOVG_GL3 1
)NVG"
#endif

#if NANOVG_GL_USE_UNIFORMBUFFER
	R"NVG(#define USE_UNIFORMBUFFER 1
)NVG"
#else
	R"NVG(#define UNIFORMARRAY_SIZE 12
)NVG"
#endif
	R"NVG(
)NVG";

	static const char* fillVertShader = R"NVG(
#ifdef NANOVG_GL3
	uniform vec2 viewSize;
	in vec2 vertex;
	in vec4 tcoord;
	out vec2 ftcoord;
	out vec2 fpos;
	smooth out vec2 uv;
#else
	uniform vec2 viewSize;
	attribute vec2 vertex;
	attribute vec4 tcoord;
	varying vec2 ftcoord;
	varying vec2 fpos;
	varying vec2 uv;
#endif
void main(void) {
	ftcoord = tcoord.xy;
	uv = 0.5 * tcoord.zw;
	fpos = vertex;
	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);
}
)NVG";

	static const char* fillFragShader = R"NVG(
#ifdef GL_ES
#if defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NANOVG_GL3)
 precision highp float;
#else
 precision mediump float;
#endif
#endif
#ifdef NANOVG_GL3
#ifdef USE_UNIFORMBUFFER
	layout(std140) uniform frag {
		mat3 scissorMat;
		mat3 paintMat;
		vec4 innerCol;
		vec4 outerCol;
		vec2 scissorExt;
		vec2 scissorScale;
		vec2 extent;
		float radius;
		float feather;
		float strokeMult;
		float strokeThr;
   int lineStyle;
		int texType;
		int type;
	};
#else
	uniform vec4 frag[UNIFORMARRAY_SIZE];
#endif
	uniform sampler2D tex;
	in vec2 ftcoord;
	in vec2 fpos;
	smooth in vec2 uv;
	out vec4 outColor;
#else
	uniform vec4 frag[UNIFORMARRAY_SIZE];
	uniform sampler2D tex;
	varying vec2 ftcoord;
	varying vec2 fpos;
	varying vec2 uv;
#endif
#ifndef USE_UNIFORMBUFFER
	#define scissorMat mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)
	#define paintMat mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)
	#define innerCol frag[6]
	#define outerCol frag[7]
	#define scissorExt frag[8].xy
	#define scissorScale frag[8].zw
	#define extent frag[9].xy
	#define radius frag[9].z
	#define feather frag[9].w
	#define strokeMult frag[10].x
	#define strokeThr frag[10].y
	#define lineStyle int(frag[10].z)
	#define texType int(frag[10].w)
	#define type int(frag[11].x)
#endif

float sdroundrect(vec2 pt, vec2 ext, float rad) {
	vec2 ext2 = ext - vec2(rad,rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
}

// Scissoring
float scissorMask(vec2 p) {
	vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
	sc = vec2(0.5,0.5) - sc * scissorScale;
	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}
float glow(vec2 uv){
  return smoothstep(0.0, 1.0, 1.0 - 2.0 * abs(uv.x));
}
float dashed(vec2 uv){
	float fy = fract(uv.y / 4.0);
	float w = step(fy, 0.5);
	fy *= 4.0;
	if(fy >= 1.5){
		fy -= 1.5;
	} else if(fy <= 0.5) {
	fy = 0.5 - fy;
	} else {
	fy = 0.0;
	}
w *= smoothstep(0.0, 1.0, 6.0 * (0.25 - (uv.x * uv.x  + fy * fy)));
	return w;
}
float dotted(vec2 uv){
	float fy = 4.0 * fract(uv.y / (4.0)) - 0.5;
	return smoothstep(0.0, 1.0, 6.0 * (0.25 - (uv.x * uv.x  + fy * fy)));
}
#ifdef EDGE_AA
// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask() {
	return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);
}
#endif

void main(void) {
   vec4 result;
	float scissor = scissorMask(fpos);
#ifdef EDGE_AA
	float strokeAlpha = strokeMask();
if(lineStyle == 2) strokeAlpha*=dashed(uv);
if(lineStyle == 3) strokeAlpha*=dotted(uv);
if(lineStyle == 4) strokeAlpha*=glow(uv);
	if (strokeAlpha < strokeThr) discard;
#else
	float strokeAlpha = 1.0;
if(lineStyle == 2) strokeAlpha*=dashed(uv);
if(lineStyle == 3) strokeAlpha*=dotted(uv);
if(lineStyle == 4) strokeAlpha*=glow(uv);
if (lineStyle > 1 && strokeAlpha < strokeThr) discard;
#endif
	if (type == 0) {			// Gradient
		// Calculate gradient color using box gradient
		vec2 pt = (paintMat * vec3(fpos,1.0)).xy;
		float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
		vec4 color = mix(innerCol,outerCol,d);
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	} else if (type == 1) {		// Image
		// Calculate color fron texture
		vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;
#ifdef NANOVG_GL3
		vec4 color = texture(tex, pt);
#else
		vec4 color = texture2D(tex, pt);
#endif
		if (texType == 1) color = vec4(color.xyz*color.w,color.w);
		if (texType == 2) color = vec4(color.x);
		// Apply color tint and alpha.
		color *= innerCol;
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	} else if (type == 2) {		// Stencil fill
		result = vec4(1,1,1,1);
	} else if (type == 3) {		// Textured tris
#ifdef NANOVG_GL3
		vec4 color = texture(tex, ftcoord);
#else
		vec4 color = texture2D(tex, ftcoord);
#endif
		if (texType == 1) color = vec4(color.xyz*color.w,color.w);
		if (texType == 2) color = vec4(color.x);
		color *= scissor;
		result = color * innerCol;
	}
#ifdef NANOVG_GL3
	outColor = result;
#else
	gl_FragColor = result;
#endif
}
)NVG";

	glnvg__checkError(gl, "init");

	if (gl->flags & CreateFlags::Antialias) {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, "#define EDGE_AA 1\n", fillVertShader, fillFragShader) == 0)
			return 0;
	} else {
		if (glnvg__createShader(&gl->shader, "shader", shaderHeader, NULL, fillVertShader, fillFragShader) == 0)
			return 0;
	}

	glnvg__checkError(gl, "uniform locations");
	glnvg__getUniforms(&gl->shader);

	// Create dynamic vertex array
#if defined NANOVG_GL3
	glGenVertexArrays(1, &gl->vertArr);
#endif
	glGenBuffers(1, &gl->vertBuf);

#if NANOVG_GL_USE_UNIFORMBUFFER
	// Create UBOs
	glUniformBlockBinding(gl->shader.prog, gl->shader.loc[GlUniformLocFrag], GlFragBinding);
	glGenBuffers(1, &gl->fragBuf);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
#endif
	gl->fragSize = sizeof(GlFragUniforms) + align - sizeof(GlFragUniforms) % align;

	// Some platforms does not allow to have samples to unset textures.
	// Create empty one which is bound when there's no texture specified.
	gl->dummyTex = glnvg__renderCreateTexture(gl, static_cast<int>(Texture::Alpha), 1, 1, 0, NULL);

	glnvg__checkError(gl, "create done");

	glFinish();

	return 1;
}

static int glnvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	GLContext* gl = (GLContext*)uptr;
	GlTexture* tex = glnvg__allocTexture(gl);

	if (tex == NULL) return 0;

#ifdef NANOVG_GLES2
	// Check for non-power of 2.
	if (glnvg__nearestPow2(w) != (unsigned int)w || glnvg__nearestPow2(h) != (unsigned int)h) {
		// No repeat
		if ((imageFlags & ImageFlags::RepeatX) != 0 || (imageFlags & ImageFlags::RepeatY) != 0) {
			printf("Repeat X/Y is not supported for non power-of-two textures (%d x %d)\n", w, h);
			imageFlags &= ~(static_cast<int>(ImageFlags::RepeatX) | static_cast<int>(ImageFlags::RepeatY));
		}
		// No mips.
		if (imageFlags & ImageFlags::GenerateMipmaps) {
			printf("Mip-maps is not support for non power-of-two textures (%d x %d)\n", w, h);
			imageFlags &= ~static_cast<int>(ImageFlags::GenerateMipmaps);
		}
	}
#endif

	glGenTextures(1, &tex->tex);
	tex->width = w;
	tex->height = h;
	tex->type = type;
	tex->flags = imageFlags;
	glnvg__bindTexture(gl, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

#if defined (NANOVG_GL2)
	// GL 1.4 and later has support for generating mipmaps using a tex parameter.
	if (imageFlags & ImageFlags::GenerateMipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
#endif

	if (type == static_cast<int>(Texture::Rgba))
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
#if defined(NANOVG_GLES2) || defined (NANOVG_GL2)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
#elif defined(NANOVG_GLES3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#endif

	if (imageFlags & ImageFlags::GenerateMipmaps) {
		if (imageFlags & ImageFlags::Nearest) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
	} else {
		if (imageFlags & ImageFlags::Nearest) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}

	if (imageFlags & ImageFlags::Nearest) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (imageFlags & ImageFlags::RepeatX)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	if (imageFlags & ImageFlags::RepeatY)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

	// The new way to build mipmaps on GLES and GL3
#if !defined(NANOVG_GL2)
	if (imageFlags & ImageFlags::GenerateMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
#endif

	glnvg__checkError(gl, "create tex");
	glnvg__bindTexture(gl, 0);

	return tex->id;
}


static int glnvg__renderDeleteTexture(void* uptr, int image)
{
	GLContext* gl = (GLContext*)uptr;
	return glnvg__deleteTexture(gl, image);
}

static int glnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
	GLContext* gl = (GLContext*)uptr;
	GlTexture* tex = glnvg__findTexture(gl, image);

	if (tex == NULL) return 0;
	glnvg__bindTexture(gl, tex->tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, y);
#else
	// No support for all of skip, need to update a whole row at a time.
	if (tex->type == static_cast<int>(Texture::Rgba))
		data += y*tex->width*4;
	else
		data += y*tex->width;
	x = 0;
	w = tex->width;
#endif

	if (tex->type == static_cast<int>(Texture::Rgba))
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
#if defined(NANOVG_GLES2) || defined(NANOVG_GL2)
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
#else
		glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, w,h, GL_RED, GL_UNSIGNED_BYTE, data);
#endif

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifndef NANOVG_GLES2
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#endif

	glnvg__bindTexture(gl, 0);

	return 1;
}

static int glnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
	GLContext* gl = (GLContext*)uptr;
	GlTexture* tex = glnvg__findTexture(gl, image);
	if (tex == NULL) return 0;
	*w = tex->width;
	*h = tex->height;
	return 1;
}

static int glnvg__renderGetImageTextureId(void* uptr, int handle)
{
	GLContext* gl = (GLContext*)uptr;
	GlTexture* tex = glnvg__findTexture(gl, handle);
	if(tex) {
	    return tex->tex;
	} else {
	    return -1;
	}
}

static void glnvg__xformToMat3x4(float* m3, float* t)
{
	m3[0] = t[0];
	m3[1] = t[1];
	m3[2] = 0.0f;
	m3[3] = 0.0f;
	m3[4] = t[2];
	m3[5] = t[3];
	m3[6] = 0.0f;
	m3[7] = 0.0f;
	m3[8] = t[4];
	m3[9] = t[5];
	m3[10] = 1.0f;
	m3[11] = 0.0f;
}

static Color glnvg__premulColor(Color c)
{
	c.r *= c.a;
	c.g *= c.a;
	c.b *= c.a;
	return c;
}

static int glnvg__convertPaint(GLContext* gl, GlFragUniforms* frag, Paint* paint,
							   Scissor* scissor, float width, float fringe, float strokeThr, int lineStyle)
{
	GlTexture* tex = NULL;
	std::array<float, 6> invxform{};

	std::memset(frag, 0, sizeof(*frag));

	frag->innerCol = glnvg__premulColor(paint->innerColor);
	frag->outerCol = glnvg__premulColor(paint->outerColor);
#if NANOVG_GL_USE_UNIFORMBUFFER
	frag->lineStyle = lineStyle;
#else
	frag->lineStyle = static_cast<float>(lineStyle);
#endif
	if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
		std::memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
		frag->scissorExt[0] = 1.0f;
		frag->scissorExt[1] = 1.0f;
		frag->scissorScale[0] = 1.0f;
		frag->scissorScale[1] = 1.0f;
	} else {
		transformInverse(invxform.data(), scissor->xform);
		glnvg__xformToMat3x4(frag->scissorMat, invxform.data());
		frag->scissorExt[0] = scissor->extent[0];
		frag->scissorExt[1] = scissor->extent[1];
		frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]) / fringe;
		frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]) / fringe;
	}

	std::memcpy(frag->extent, paint->extent, sizeof(frag->extent));
	frag->strokeMult = (width*0.5f + fringe*0.5f) / fringe;
	frag->strokeThr = strokeThr;

	if (paint->image != 0) {
		tex = glnvg__findTexture(gl, paint->image);
		if (tex == NULL) return 0;
		if ((tex->flags & ImageFlags::Flipy) != 0) {
			std::array<float, 6> m1{};
			std::array<float, 6> m2{};
			transformTranslate(m1.data(), 0.0f, frag->extent[1] * 0.5f);
			transformMultiply(m1.data(), paint->xform);
			transformScale(m2.data(), 1.0f, -1.0f);
			transformMultiply(m2.data(), m1.data());
			transformTranslate(m1.data(), 0.0f, -frag->extent[1] * 0.5f);
			transformMultiply(m1.data(), m2.data());
			transformInverse(invxform.data(), m1.data());
		} else {
			transformInverse(invxform.data(), paint->xform);
		}
		frag->type = NSVG_SHADER_FILLIMG;

		#if NANOVG_GL_USE_UNIFORMBUFFER
		if (tex->type == static_cast<int>(Texture::Rgba))
			frag->texType = (tex->flags & ImageFlags::Premultiplied) ? 0 : 1;
		else
			frag->texType = 2;
		#else
		if (tex->type == static_cast<int>(Texture::Rgba))
			frag->texType = (tex->flags & ImageFlags::Premultiplied) ? 0.0f : 1.0f;
		else
			frag->texType = 2.0f;
		#endif
//		printf("frag->texType = %d\n", frag->texType);
	} else {
		frag->type = NSVG_SHADER_FILLGRAD;
		frag->radius = paint->radius;
		frag->feather = paint->feather;
		transformInverse(invxform.data(), paint->xform);
	}

	glnvg__xformToMat3x4(frag->paintMat, invxform.data());

	return 1;
}

namespace detail {
static GlFragUniforms* fragUniformPtr(GLContext* gl, int i);
} // namespace detail

static void glnvg__setUniforms(GLContext* gl, int uniformOffset, int image)
{
	GlTexture* tex = NULL;
#if NANOVG_GL_USE_UNIFORMBUFFER
	glBindBufferRange(GL_UNIFORM_BUFFER, GlFragBinding, gl->fragBuf, uniformOffset, sizeof(GlFragUniforms));
#else
	GlFragUniforms* frag = nvg::detail::fragUniformPtr(gl, uniformOffset);
	glUniform4fv(gl->shader.loc[GlUniformLocFrag], NANOVG_GL_UNIFORMARRAY_SIZE, &(frag->uniformArray[0][0]));
#endif

	if (image != 0) {
		tex = glnvg__findTexture(gl, image);
	}
	// If no image is set, use empty texture
	if (tex == NULL) {
		tex = glnvg__findTexture(gl, gl->dummyTex);
	}
	glnvg__bindTexture(gl, tex != NULL ? tex->tex : 0);
	glnvg__checkError(gl, "tex paint tex");
}

static void glnvg__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
	UNUSED(devicePixelRatio);
	GLContext* gl = (GLContext*)uptr;
	gl->view[0] = width;
	gl->view[1] = height;
}

static void glnvg__fill(GLContext* gl, GlCall* call)
{
	GLPath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	// Draw shapes
	glEnable(GL_STENCIL_TEST);
	glnvg__stencilMask(gl, 0xff);
	glnvg__stencilFunc(gl, GL_ALWAYS, 0, 0xff);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// set bindpoint for solid loc
	glnvg__setUniforms(gl, call->uniformOffset, 0);
	glnvg__checkError(gl, "fill simple");

	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
	glDisable(GL_CULL_FACE);
	for (i = 0; i < npaths; i++)
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
	glEnable(GL_CULL_FACE);

	// Draw anti-aliased pixels
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
	glnvg__checkError(gl, "fill fill");

	if (gl->flags & CreateFlags::Antialias) {
		glnvg__stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		// Draw fringes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}

	// Draw fill
	glnvg__stencilFunc(gl, GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glDrawArrays(GL_TRIANGLE_STRIP, call->triangleOffset, call->triangleCount);

	glDisable(GL_STENCIL_TEST);
}

static void glnvg__convexFill(GLContext* gl, GlCall* call)
{
	GLPath* paths = &gl->paths[call->pathOffset];
	int i, npaths = call->pathCount;

	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError(gl, "convex fill");

	for (i = 0; i < npaths; i++) {
		glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
		// Draw fringes
		if (paths[i].strokeCount > 0) {
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
		}
	}
}

static void glnvg__stroke(GLContext* gl, GlCall* call)
{
	GLPath* paths = &gl->paths[call->pathOffset];
	int npaths = call->pathCount, i;

	if (gl->flags & CreateFlags::StencilStrokes) {

		glEnable(GL_STENCIL_TEST);
		glnvg__stencilMask(gl, 0xff);

		// Fill the stroke base without overlap
		glnvg__stencilFunc(gl, GL_EQUAL, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		glnvg__setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
		glnvg__checkError(gl, "stroke fill 0");
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

		// Draw anti-aliased pixels.
		glnvg__setUniforms(gl, call->uniformOffset, call->image);
		glnvg__stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

		// Clear stencil buffer.
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glnvg__stencilFunc(gl, GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
		glnvg__checkError(gl, "stroke fill 1");
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glDisable(GL_STENCIL_TEST);

//		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);

	} else {
		glnvg__setUniforms(gl, call->uniformOffset, call->image);
		glnvg__checkError(gl, "stroke fill");
		// Draw Strokes
		for (i = 0; i < npaths; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
	}
}

static void glnvg__triangles(GLContext* gl, GlCall* call)
{
	glnvg__setUniforms(gl, call->uniformOffset, call->image);
	glnvg__checkError(gl, "triangles fill");

	glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void glnvg__renderCancel(void* uptr) {
	GLContext* gl = (GLContext*)uptr;
	gl->nverts = 0;
	gl->npaths = 0;
	gl->ncalls = 0;
	gl->nuniforms = 0;
}

static GLenum glnvg_convertBlendFuncFactor(int factor)
{
	if (factor == static_cast<int>(BlendFactor::Zero))
		return GL_ZERO;
	if (factor == static_cast<int>(BlendFactor::One))
		return GL_ONE;
	if (factor == static_cast<int>(BlendFactor::SrcColor))
		return GL_SRC_COLOR;
	if (factor == static_cast<int>(BlendFactor::OneMinusSrcColor))
		return GL_ONE_MINUS_SRC_COLOR;
	if (factor == static_cast<int>(BlendFactor::DstColor))
		return GL_DST_COLOR;
	if (factor == static_cast<int>(BlendFactor::OneMinusDstColor))
		return GL_ONE_MINUS_DST_COLOR;
	if (factor == static_cast<int>(BlendFactor::SrcAlpha))
		return GL_SRC_ALPHA;
	if (factor == static_cast<int>(BlendFactor::OneMinusSrcAlpha))
		return GL_ONE_MINUS_SRC_ALPHA;
	if (factor == static_cast<int>(BlendFactor::DstAlpha))
		return GL_DST_ALPHA;
	if (factor == static_cast<int>(BlendFactor::OneMinusDstAlpha))
		return GL_ONE_MINUS_DST_ALPHA;
	if (factor == static_cast<int>(BlendFactor::SrcAlphaSaturate))
		return GL_SRC_ALPHA_SATURATE;
	return GL_INVALID_ENUM;
}

static GlBlend glnvg__blendCompositeOperation(CompositeOperationState op)
{
	GlBlend blend;
	blend.srcRGB = glnvg_convertBlendFuncFactor(op.srcRGB);
	blend.dstRGB = glnvg_convertBlendFuncFactor(op.dstRGB);
	blend.srcAlpha = glnvg_convertBlendFuncFactor(op.srcAlpha);
	blend.dstAlpha = glnvg_convertBlendFuncFactor(op.dstAlpha);
	if (blend.srcRGB == GL_INVALID_ENUM || blend.dstRGB == GL_INVALID_ENUM || blend.srcAlpha == GL_INVALID_ENUM || blend.dstAlpha == GL_INVALID_ENUM)
	{
		blend.srcRGB = GL_ONE;
		blend.dstRGB = GL_ONE_MINUS_SRC_ALPHA;
		blend.srcAlpha = GL_ONE;
		blend.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
	}
	return blend;
}

static void glnvg__renderFlush(void* uptr)
{
	GLContext* gl = (GLContext*)uptr;
	int i;

	if (gl->ncalls > 0) {

		// Setup require GL state.
		glUseProgram(gl->shader.prog);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilMask(0xffffffff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		#if NANOVG_GL_USE_STATE_FILTER
		gl->boundTexture = 0;
		gl->stencilMask = 0xffffffff;
		gl->stencilFunc = GL_ALWAYS;
		gl->stencilFuncRef = 0;
		gl->stencilFuncMask = 0xffffffff;
		gl->blendFunc.srcRGB = GL_INVALID_ENUM;
		gl->blendFunc.srcAlpha = GL_INVALID_ENUM;
		gl->blendFunc.dstRGB = GL_INVALID_ENUM;
		gl->blendFunc.dstAlpha = GL_INVALID_ENUM;
		#endif

#if NANOVG_GL_USE_UNIFORMBUFFER
		// Upload ubo for frag shaders
		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
		glBufferData(GL_UNIFORM_BUFFER, gl->nuniforms * gl->fragSize, gl->uniforms, GL_STREAM_DRAW);
#endif

		// Upload vertex data
#if defined NANOVG_GL3
		glBindVertexArray(gl->vertArr);
#endif
		glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
		glBufferData(GL_ARRAY_BUFFER, gl->nverts * sizeof(Vertex), gl->verts, GL_STREAM_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(size_t)0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(0 + 2*sizeof(float)));

		// Set view and texture just once per frame.
		glUniform1i(gl->shader.loc[GlUniformLocTex], 0);
		glUniform2fv(gl->shader.loc[GlUniformLocViewSize], 1, gl->view.data());

#if NANOVG_GL_USE_UNIFORMBUFFER
		glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
#endif

		for (i = 0; i < gl->ncalls; i++) {
			GlCall* call = &gl->calls[i];
			glnvg__blendFuncSeparate(gl,&call->blendFunc);
			if (call->type == Fill)
				glnvg__fill(gl, call);
			else if (call->type == ConvexFill)
				glnvg__convexFill(gl, call);
			else if (call->type == Stroke)
				glnvg__stroke(gl, call);
			else if (call->type == Triangles)
				glnvg__triangles(gl, call);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
#if defined NANOVG_GL3
		glBindVertexArray(0);
#endif
		glDisable(GL_CULL_FACE);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
		glnvg__bindTexture(gl, 0);
	}

	// Reset calls
	gl->nverts = 0;
	gl->npaths = 0;
	gl->ncalls = 0;
	gl->nuniforms = 0;
}

static int glnvg__maxVertCount(const Path* paths, int npaths)
{
	int i, count = 0;
	for (i = 0; i < npaths; i++) {
		count += paths[i].nfill;
		count += paths[i].nstroke;
	}
	return count;
}

static GlCall* glnvg__allocCall(GLContext* gl)
{
	GlCall* ret = NULL;
	if (gl->ncalls+1 > gl->ccalls) {
		GlCall* calls;
		int ccalls = glnvg__maxi(gl->ncalls+1, 128) + gl->ccalls/2; // 1.5x Overallocate
		calls = static_cast<GlCall*>(std::realloc(gl->calls, sizeof(GlCall) * ccalls));
		if (calls == nullptr) return nullptr;
		gl->calls = calls;
		gl->ccalls = ccalls;
	}
	ret = &gl->calls[gl->ncalls++];
	std::memset(ret, 0, sizeof(GlCall));
	return ret;
}

static int glnvg__allocPaths(GLContext* gl, int n)
{
	int ret = 0;
	if (gl->npaths+n > gl->cpaths) {
		GLPath* paths;
		int cpaths = glnvg__maxi(gl->npaths + n, 128) + gl->cpaths/2; // 1.5x Overallocate
		paths = static_cast<GLPath*>(std::realloc(gl->paths, sizeof(GLPath) * cpaths));
		if (paths == nullptr) return -1;
		gl->paths = paths;
		gl->cpaths = cpaths;
	}
	ret = gl->npaths;
	gl->npaths += n;
	return ret;
}

static int glnvg__allocVerts(GLContext* gl, int n)
{
	int ret = 0;
	if (gl->nverts+n > gl->cverts) {
		Vertex* verts;
		int cverts = glnvg__maxi(gl->nverts + n, 4096) + gl->cverts/2; // 1.5x Overallocate
		verts = static_cast<Vertex*>(std::realloc(gl->verts, sizeof(Vertex) * cverts));
		if (verts == nullptr) return -1;
		gl->verts = verts;
		gl->cverts = cverts;
	}
	ret = gl->nverts;
	gl->nverts += n;
	return ret;
}

static int glnvg__allocFragUniforms(GLContext* gl, int n)
{
	int ret = 0, structSize = gl->fragSize;
	if (gl->nuniforms+n > gl->cuniforms) {
		unsigned char* uniforms;
		int cuniforms = glnvg__maxi(gl->nuniforms+n, 128) + gl->cuniforms/2; // 1.5x Overallocate
		uniforms = static_cast<unsigned char*>(std::realloc(gl->uniforms, structSize * cuniforms));
		if (uniforms == nullptr) return -1;
		gl->uniforms = uniforms;
		gl->cuniforms = cuniforms;
	}
	ret = gl->nuniforms * structSize;
	gl->nuniforms += n;
	return ret;
}

namespace detail {
static GlFragUniforms* fragUniformPtr(GLContext* gl, int i)
{
	return reinterpret_cast<GlFragUniforms*>(&gl->uniforms[i]);
}
} // namespace detail

static void glnvg__vset(Vertex* vtx, float x, float y, float u, float v)
{
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
}

static void glnvg__renderFill(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor, float fringe,
							  const float* bounds, const Path* paths, int npaths)
{
	GLContext* gl = static_cast<GLContext*>(uptr);
	GlCall* call = glnvg__allocCall(gl);
	Vertex* quad;
	GlFragUniforms* frag;
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = Fill;
	call->triangleCount = 4;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;
	call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

	if (npaths == 1 && paths[0].convex)
	{
		call->type = ConvexFill;
		call->triangleCount = 0;	// Bounding box fill quad not needed for convex fill
	}

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths) + call->triangleCount;
	offset = glnvg__allocVerts(gl, maxverts);
	if (offset == -1) goto error;

	for (i = 0; i < npaths; i++) {
		GLPath* copy = &gl->paths[call->pathOffset + i];
		const Path* path = &paths[i];
		std::memset(copy, 0, sizeof(GLPath));
		if (path->nfill > 0) {
			copy->fillOffset = offset;
			copy->fillCount = path->nfill;
			std::memcpy(&gl->verts[offset], path->fill, sizeof(Vertex) * path->nfill);
			offset += path->nfill;
		}
		if (path->nstroke > 0) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			std::memcpy(&gl->verts[offset], path->stroke, sizeof(Vertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	// Setup uniforms for draw calls
	if (call->type == Fill) {
		// Quad
		call->triangleOffset = offset;
		quad = &gl->verts[call->triangleOffset];
		glnvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
		glnvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
		glnvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
		glnvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

		call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
		if (call->uniformOffset == -1) goto error;
		// Simple shader for stencil
		frag = nvg::detail::fragUniformPtr(gl, call->uniformOffset);
		std::memset(frag, 0, sizeof(*frag));
		frag->strokeThr = -1.0f;
		frag->type = NSVG_SHADER_SIMPLE;
		// Fill shader
		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, fringe, fringe, -1.0f, 0);
	} else {
		call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
		if (call->uniformOffset == -1) goto error;
		// Fill shader
		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset), paint, scissor, fringe, fringe, -1.0f, 0);
	}

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderStroke(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor, float fringe,
								float strokeWidth, int lineStyle, const Path* paths, int npaths)
{
	GLContext* gl = (GLContext*)uptr;
	GlCall* call = glnvg__allocCall(gl);
	int i, maxverts, offset;

	if (call == NULL) return;

	call->type = Stroke;
	call->pathOffset = glnvg__allocPaths(gl, npaths);
	if (call->pathOffset == -1) goto error;
	call->pathCount = npaths;
	call->image = paint->image;
	call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

	// Allocate vertices for all the paths.
	maxverts = glnvg__maxVertCount(paths, npaths);
	offset = glnvg__allocVerts(gl, maxverts);
	if (offset == -1) goto error;

	for (i = 0; i < npaths; i++) {
		GLPath* copy = &gl->paths[call->pathOffset + i];
		const Path* path = &paths[i];
		std::memset(copy, 0, sizeof(GLPath));
		if (path->nstroke) {
			copy->strokeOffset = offset;
			copy->strokeCount = path->nstroke;
			std::memcpy(&gl->verts[offset], path->stroke, sizeof(Vertex) * path->nstroke);
			offset += path->nstroke;
		}
	}

	if (gl->flags & CreateFlags::StencilStrokes) {
		// Fill shader
		call->uniformOffset = glnvg__allocFragUniforms(gl, 2);
		if (call->uniformOffset == -1) goto error;

		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f, lineStyle);
		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f, lineStyle);

	} else {
		// Fill shader
		call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
		if (call->uniformOffset == -1) goto error;
		glnvg__convertPaint(gl, nvg::detail::fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f, lineStyle);
	}

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderTriangles(void* uptr, Paint* paint, CompositeOperationState compositeOperation, Scissor* scissor,
								   const Vertex* verts, int nverts, float fringe)
{
	GLContext* gl = (GLContext*)uptr;
	GlCall* call = glnvg__allocCall(gl);
	GlFragUniforms* frag;

	if (call == NULL) return;

	call->type = Triangles;
	call->image = paint->image;
	call->blendFunc = glnvg__blendCompositeOperation(compositeOperation);

	// Allocate vertices for all the paths.
	call->triangleOffset = glnvg__allocVerts(gl, nverts);
	if (call->triangleOffset == -1) goto error;
	call->triangleCount = nverts;

	std::memcpy(&gl->verts[call->triangleOffset], verts, sizeof(Vertex) * nverts);

	// Fill shader
	call->uniformOffset = glnvg__allocFragUniforms(gl, 1);
	if (call->uniformOffset == -1) goto error;
	frag = nvg::detail::fragUniformPtr(gl, call->uniformOffset);
	glnvg__convertPaint(gl, frag, paint, scissor, 1.0f, fringe, -1.0f, 0);
	frag->type = NSVG_SHADER_IMG;

	return;

error:
	// We get here if call alloc was ok, but something else is not.
	// Roll back the last call to prevent drawing it.
	if (gl->ncalls > 0) gl->ncalls--;
}

static void glnvg__renderDelete(void* uptr)
{
	GLContext* gl = (GLContext*)uptr;
	int i;
	if (gl == NULL) return;

	glnvg__deleteShader(&gl->shader);

#if NANOVG_GL3
#if NANOVG_GL_USE_UNIFORMBUFFER
	if (gl->fragBuf != 0)
		glDeleteBuffers(1, &gl->fragBuf);
#endif
	if (gl->vertArr != 0)
		glDeleteVertexArrays(1, &gl->vertArr);
#endif
	if (gl->vertBuf != 0)
		glDeleteBuffers(1, &gl->vertBuf);

	for (i = 0; i < gl->ntextures; i++) {
		if (gl->textures[i].tex != 0 && (gl->textures[i].flags & ImageFlagsGl::NoDelete) == 0)
			glDeleteTextures(1, &gl->textures[i].tex);
	}
	std::free(gl->textures);

	std::free(gl->paths);
	std::free(gl->verts);
	std::free(gl->uniforms);
	std::free(gl->calls);

	std::free(gl);
}


std::unique_ptr<Context> createGL(int flags)
{
	Params params{};
	GLContext* gl = static_cast<GLContext*>(std::malloc(sizeof(GLContext)));
	if (gl == nullptr) return nullptr;
	*gl = GLContext{};

	params.renderCreate = glnvg__renderCreate;
	params.renderCreateTexture = glnvg__renderCreateTexture;
	params.renderDeleteTexture = glnvg__renderDeleteTexture;
	params.renderUpdateTexture = glnvg__renderUpdateTexture;
	params.renderGetTextureSize = glnvg__renderGetTextureSize;
	params.renderGetImageTextureId = glnvg__renderGetImageTextureId;
	params.renderViewport = glnvg__renderViewport;
	params.renderCancel = glnvg__renderCancel;
	params.renderFlush = glnvg__renderFlush;
	params.renderFill = glnvg__renderFill;
	params.renderStroke = glnvg__renderStroke;
	params.renderTriangles = glnvg__renderTriangles;
	params.renderDelete = glnvg__renderDelete;
	params.userPtr = gl;
	params.edgeAntiAlias = flags & CreateFlags::Antialias ? 1 : 0;

	gl->flags = flags;
	return std::make_unique<Context>(params);
}

inline void deleteGL(std::unique_ptr<Context>& ctx){
	ctx.reset();
}

#if defined NANOVG_GL2
int nvglCreateImageFromHandleGL2(Context& ctx, GLuint textureId, int w, int h, int imageFlags)
#elif defined NANOVG_GL3
int nvglCreateImageFromHandleGL3(Context& ctx, GLuint textureId, int w, int h, int imageFlags)
#elif defined NANOVG_GLES2
int nvglCreateImageFromHandleGLES2(Context& ctx, GLuint textureId, int w, int h, int imageFlags)
#elif defined NANOVG_GLES3
int nvglCreateImageFromHandleGLES3(Context& ctx, GLuint textureId, int w, int h, int imageFlags)
#endif
{
	GLContext* gl = (GLContext*)ctx.internalParams().userPtr;
	GlTexture* tex = glnvg__allocTexture(gl);

	if (tex == NULL) return 0;

	tex->type = static_cast<int>(Texture::Rgba);
	tex->tex = textureId;
	tex->flags = imageFlags;
	tex->width = w;
	tex->height = h;

	return tex->id;
}

#if defined NANOVG_GL2
GLuint nvglImageHandleGL2(Context& ctx, int image)
#elif defined NANOVG_GL3
GLuint nvglImageHandleGL3(Context& ctx, int image)
#elif defined NANOVG_GLES2
GLuint nvglImageHandleGLES2(Context& ctx, int image)
#elif defined NANOVG_GLES3
GLuint nvglImageHandleGLES3(Context& ctx, int image)
#endif
{
	GLContext* gl = (GLContext*)ctx.internalParams().userPtr;
	GlTexture* tex = glnvg__findTexture(gl, image);
	return tex->tex;
}

} // namespace nvg

#endif // NANOVG_GL_IMPLEMENTATION
