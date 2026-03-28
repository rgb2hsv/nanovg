NanoVG++
==========

NanoVG++ is small antialiased vector graphics rendering library for OpenGL. It has lean API modeled after HTML5 canvas API. It is aimed to be a practical and fun toolset for building scalable user interfaces and visualizations.

This repository’s **`modernize-cpp`** branch extends upstream NanoVG with a C++20 implementation while keeping the original C API available for legacy examples and bindings. The notes below under **Changes since upstream** summarize what differs from the merge-base with [memononen/nanovg](https://github.com/memononen/nanovg) `master` (commit `6a8a2e7`).

## Changes since upstream

- **C++20 core** — Drawing logic lives under [`src/nvg/`](src/nvg/): [`nanovg.hpp`](src/nvg/nanovg.hpp) is the primary C++ API; implementation is split across `nanovg.cpp`, `nanovg_context.*`, `nanovg_detail.*`, and related translation units instead of a single `nanovg.c`.
- **C API compatibility** — The classic `nvg*` entry points from [`nanovg.h`](src/nvg/legacy/nanovg.h) are implemented in [`nanovg_adapter.cpp`](src/nvg/nanovg_adapter.cpp) on top of `nvg::Context`. Legacy C headers live in [`src/nvg/legacy/`](src/nvg/legacy/) (`nanovg.h`, `nanovg_gl.h`, `nanovg_gl_utils.h`).
- **OpenGL backends** — C++ headers: [`nanovg_gl.hpp`](src/nvg/nanovg_gl.hpp), [`nanovg_gl_utils.hpp`](src/nvg/nanovg_gl_utils.hpp). Legacy C includes remain the `.h` variants in `legacy/`.
- **Third-party headers** — `fontstash.h`, `stb_truetype.h`, and the tree’s `stb_image.h` are under [`src/thirdParty/`](src/thirdParty/).
- **Build** — **CMake** is the supported route: requires **GLFW** and **GLEW** via `find_package` or the `external/glfw` and `external/glew` git submodules. **Premake** (`premake4.lua`) is still present but secondary. Example targets: `example_gl2`, `example_gl3`, `example_fbo` (and GLES variants on non-Windows), plus **`example_legacy_*`** for the original C sources.
- **Example layout** — C++ sources and shared `demo.cpp` / `perf.cpp` stay in [`example/`](example/). Original C examples, `demo.c` / `perf.c`, and local `stb_image*.h` copies are under [`example/legacy/`](example/legacy/).
- **Removed** — Monolithic `src/nanovg.c` and the old split `obsolete/nanovg_gl2.h` / `obsolete/nanovg_gl3.h` tree; premake-focused layout assumptions no longer match the default build.

### Headless check

The `example_gl3` binary accepts `--test N` (and `--test=N`) to run a fixed-frame capture path useful for CI (see `.cursor/skills/verify` in this repo).

## Screenshot

The sample PNGs that shipped with upstream are not present in this branch; run the `example_gl3` (or other) demo locally to view output.

Usage
=====

The NanoVG API is modeled loosely on HTML5 canvas API. If you know canvas, you're up to speed with NanoVG in no time.

## Creating drawing context

### C++ (default in this branch)

Use the OpenGL backend define matching your API, then `nvg::createGL` with `nvg::CreateFlags` (declared in [`nanovg_gl.hpp`](src/nvg/nanovg_gl.hpp)):

```cpp
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.hpp"
// After GL context is current:
auto vgOwner = nvg::createGL(static_cast<int>(
    nvg::CreateFlags::Antialias | nvg::CreateFlags::StencilStrokes));
if (!vgOwner) { /* error */ }
nvg::Context& vg = *vgOwner;
```

Flags correspond to the old C names: **Antialias** ≈ `NVG_ANTIALIAS`, **StencilStrokes** ≈ `NVG_STENCIL_STROKES`, **Debug** ≈ `NVG_DEBUG`.

### Legacy C API

C examples under [`example/legacy/`](example/legacy/) still use the upstream-style constructors, for example:

```C
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
```

The OpenGL backend is selected with the same `NANOVG_GL2_IMPLEMENTATION` / `NANOVG_GL3_IMPLEMENTATION` / GLES defines as before; headers for C are in [`src/nvg/legacy/`](src/nvg/legacy/). See [nanovg_gl.hpp](src/nvg/nanovg_gl.hpp) (C++) or `nanovg_gl.h` (C) and the examples for details. 

*NOTE:* The render target you're rendering to must have stencil buffer.

## Drawing shapes with NanoVG

Drawing a simple shape using NanoVG consists of four steps: 1) begin a new shape, 2) define the path to draw, 3) set fill or stroke, 4) and finally fill or stroke the path.

```C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

Calling `nvgBeginPath()` will clear any existing paths and start drawing from blank slate. There are number of number of functions to define the path to draw, such as rectangle, rounded rectangle and ellipse, or you can use the common moveTo, lineTo, bezierTo and arcTo API to compose the paths step by step.

In the C++ API, the same operations are methods on `nvg::Context` (for example `vg.beginPath()`, `vg.rect(...)`, `vg.fill()`).

## Understanding Composite Paths

Because of the way the rendering backend is build in NanoVG, drawing a composite path, that is path consisting from multiple paths defining holes and fills, is a bit more involved. NanoVG uses even-odd filling rule and by default the paths are wound in counter clockwise order. Keep that in mind when drawing using the low level draw API. In order to wind one of the predefined shapes as a hole, you should call `nvgPathWinding(vg, NVG_HOLE)`, or `nvgPathWinding(vg, NVG_CW)` _after_ defining the path.

``` C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgCircle(vg, 120,120, 5);
nvgPathWinding(vg, NVG_HOLE);	// Mark circle as a hole.
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

## Rendering is wrong, what to do?

- make sure you have created NanoVG context using one of the `nvgCreatexxx()` calls (C) or `nvg::createGL` (C++)
- make sure you have initialised OpenGL with *stencil buffer*
- make sure you have cleared stencil buffer
- make sure all rendering calls happen between `nvgBeginFrame()` and `nvgEndFrame()`
- to enable more checks for OpenGL errors, add `NVG_DEBUG` flag to `nvgCreatexxx()`
- if the problem still persists, please report an issue!

## OpenGL state touched by the backend

The OpenGL back-end touches following states:

When textures are uploaded or updated, the following pixel store is set to defaults: `GL_UNPACK_ALIGNMENT`, `GL_UNPACK_ROW_LENGTH`, `GL_UNPACK_SKIP_PIXELS`, `GL_UNPACK_SKIP_ROWS`. Texture binding is also affected. Texture updates can happen when the user loads images, or when new font glyphs are added. Glyphs are added as needed between calls to  `nvgBeginFrame()` and `nvgEndFrame()`.

The data for the whole frame is buffered and flushed in `nvgEndFrame()`. The following code illustrates the OpenGL state touched by the rendering code:
```C
	glUseProgram(prog);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	glBindBuffer(GL_UNIFORM_BUFFER, buf);
	glBindVertexArray(arr);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniformBlockBinding(... , GLNVG_FRAG_BINDING);
```

## API Reference

See the header file [nanovg.hpp](src/nvg/nanovg.hpp) for the C++ API. The C surface is declared in [nanovg.h](src/nvg/legacy/nanovg.h).

## Ports

- [DX11 port](https://github.com/cmaughan/nanovg) by [Chris Maughan](https://github.com/cmaughan)
- [Metal port](https://github.com/ollix/MetalNanoVG) by [Olli Wang](https://github.com/olliwang)
- [bgfx port](https://github.com/bkaradzic/bgfx/tree/master/examples/20-nanovg) by [Branimir Karadžić](https://github.com/bkaradzic)

## Projects using NanoVG

- [Processing API simulation by vinjn](https://github.com/island-org/island/blob/master/include/sketch2d.h)
- [NanoVG for .NET, C# P/Invoke binding](https://github.com/sbarisic/nanovg_dotnet)

## License
The library is licensed under [zlib license](LICENSE.txt)
Fonts used in examples:
- Roboto licensed under [Apache license](http://www.apache.org/licenses/LICENSE-2.0)
- Entypo licensed under CC BY-SA 4.0.
- Noto Emoji licensed under [SIL Open Font License, Version 1.1](http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL)

## Discussions
[NanoVG mailing list](https://groups.google.com/forum/#!forum/nanovg)

## Links
Uses [stb_truetype](http://nothings.org) (or, optionally, [freetype](http://freetype.org)) for font rendering.
Uses [stb_image](http://nothings.org) for image loading.
