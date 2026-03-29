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

#include <stdio.h>
#include <array>
#include <memory>
#ifdef NANOVG_GLEW
#	include <GL/glew.h>
#endif
#if defined(NANOVG_USE_SDL3) && !defined(NANOVG_GLEW)
#	include <SDL3/SDL_opengl.h>
#elif !defined(NANOVG_USE_SDL3)
#	ifdef __APPLE__
#		define GLFW_INCLUDE_GLCOREARB
#	endif
#	include <GLFW/glfw3.h>
#endif
#include "nvg_window.hpp"
#include "nvg_input.hpp"
#include "nanovg.hpp"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.hpp"
#include "nanovg_gl_utils.hpp"
#include "perf.hpp"

void renderPattern(nvg::Context& vg, GlUtilsFramebuffer* fb, float t, float pxRatio)
{
	int winWidth, winHeight;
	int fboWidth, fboHeight;
	int pw, ph, x, y;
	float s = 20.0f;
	float sr = (cosf(t)+1)*0.5f;
	float r = s * 0.6f * (0.2f + 0.8f * sr);

	if (fb == NULL) return;

	vg.imageSize( fb->image, fboWidth, fboHeight);
	winWidth = (int)(fboWidth / pxRatio);
	winHeight = (int)(fboHeight / pxRatio);

	// Draw some stuff to an FBO as a test
	nvgluBindFramebuffer(fb);
	glViewport(0, 0, fboWidth, fboHeight);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	vg.beginFrame( static_cast<float>(winWidth), static_cast<float>(winHeight), pxRatio);

	pw = (int)ceilf(winWidth / s);
	ph = (int)ceilf(winHeight / s);

	vg.beginPath();
	for (y = 0; y < ph; y++) {
		for (x = 0; x < pw; x++) {
			float cx = (x+0.5f) * s;
			float cy = (y+0.5f) * s;
			vg.circle( cx,cy, r);
		}
	}
	vg.fillColor( nvg::rgba(220,160,0,200));
	vg.fill();

	vg.endFrame();
	nvgluBindFramebuffer(NULL);
}

int loadFonts(nvg::Context& vg)
{
	int font;
	font = vg.createFont( "sans", "../example/Roboto-Regular.ttf");
	if (font == -1) {
		printf("Could not add font regular.\n");
		return -1;
	}
	font = vg.createFont( "sans-bold", "../example/Roboto-Bold.ttf");
	if (font == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}
	return 0;
}

void errorcb(int error, const char* desc)
{
	printf("Window error %d: %s\n", error, desc);
}

static void key(NvgWindow* window, int key, int scancode, int action, int mods, void* user)
{
	UNUSED(user);
	UNUSED(scancode);
	UNUSED(mods);
	if (key == NVG_KEY_ESCAPE && action == NVG_PRESS)
		nvgwin_set_should_close(window, 1);
}

int main()
{
	NvgWindow* window;
	std::unique_ptr<nvg::Context> vgOwner;
	GPUtimer gpuTimer;
	PerfGraph fps, cpuGraph, gpuGraph;
	double prevt = 0, cpuTime = 0;
	GlUtilsFramebuffer* fb = NULL;
	int winWidth, winHeight;
	int fbWidth, fbHeight;
	float pxRatio;

	if (nvgwin_init() != 0) {
		printf("Failed to init window system.");
		return -1;
	}

	initGraph(fps, GRAPH_RENDER_FPS, "Frame Time");
	initGraph(cpuGraph, GRAPH_RENDER_MS, "CPU Time");
	initGraph(gpuGraph, GRAPH_RENDER_MS, "GPU Time");

	nvgwin_set_error_callback(errorcb);
#ifdef DEMO_MSAA
	const int msaa = 4;
#else
	const int msaa = 0;
#endif
	window = nvgwin_create(1000, 600, "NanoVG", NVGWIN_PROFILE_GL3, msaa, 1);
	if (!window) {
		nvgwin_shutdown();
		return -1;
	}

	nvgwin_set_key_callback(window, key, nullptr);

	nvgwin_make_current(window);
#ifdef NANOVG_GLEW
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) {
		printf("Could not init glew.\n");
		return -1;
	}
	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
#endif

#ifdef DEMO_MSAA
	vgOwner = nvg::createGL(static_cast<int>(nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#else
	vgOwner = nvg::createGL(static_cast<int>(nvg::CreateFlags::Antialias | nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#endif
	if (!vgOwner) {
		printf("Could not init nanovg.\n");
		return -1;
	}
	nvg::Context& vg = *vgOwner;

	// Create hi-dpi FBO for hi-dpi screens.
	nvgwin_get_window_size(window, &winWidth, &winHeight);
	nvgwin_get_framebuffer_size(window, &fbWidth, &fbHeight);
	// Calculate pixel ration for hi-dpi devices.
	pxRatio = (float)fbWidth / (float)winWidth;

	// The image pattern is tiled, set repeat on x and y.
	fb = nvgluCreateFramebuffer(vg, (int)(100*pxRatio), (int)(100*pxRatio), static_cast<int>(nvg::ImageFlags::RepeatX | nvg::ImageFlags::RepeatY));
	if (fb == NULL) {
		printf("Could not create FBO.\n");
		return -1;
	}

	if (loadFonts(vg) == -1) {
		printf("Could not load fonts\n");
		return -1;
	}

	nvgwin_swap_interval(0);

	initGPUTimer(&gpuTimer);

	nvgwin_set_time(0);
	prevt = nvgwin_get_time();

	while (!nvgwin_should_close(window))
	{
		double mx, my, t, dt;
		std::array<float, 3> gpuTimes{};
		int i, n;

		t = nvgwin_get_time();
		dt = t - prevt;
		prevt = t;

		startGPUTimer(&gpuTimer);

		nvgwin_get_cursor_pos(window, &mx, &my);
		nvgwin_get_window_size(window, &winWidth, &winHeight);
		nvgwin_get_framebuffer_size(window, &fbWidth, &fbHeight);
		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		renderPattern(vg, fb, static_cast<float>(t), pxRatio);

		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		vg.beginFrame( static_cast<float>(winWidth), static_cast<float>(winHeight), pxRatio);

		// Use the FBO as image pattern.
		if (fb != NULL) {
			nvg::Paint img = vg.imagePattern( 0, 0, 100, 100, 0, fb->image, 1.0f);
			vg.save();

			for (i = 0; i < 20; i++) {
				vg.beginPath();
				vg.rect( static_cast<float>(10 + i * 30), 10.f, 10.f, static_cast<float>(winHeight - 20));
				vg.fillColor( nvg::hsla(i/19.0f, 0.5f, 0.5f, 255));
				vg.fill();
			}

			vg.beginPath();
			const float animT = static_cast<float>(t);
			vg.roundedRect( 140.f + sinf(animT * 1.3f) * 100.f, 140.f + cosf(animT * 1.71244f) * 100.f, 250.f, 250.f, 20.f);
			vg.fillPaint( img);
			vg.fill();
			vg.strokeColor( nvg::rgba(220,160,0,255));
			vg.strokeWidth( 3.0f);
			vg.stroke();

			vg.restore();
		}

		renderGraph(vg, 5,5, fps);
		renderGraph(vg, 5+200+5,5, cpuGraph);
		if (gpuTimer.supported)
			renderGraph(vg, 5+200+5+200+5,5, gpuGraph);

		vg.endFrame();

		// Measure the CPU time taken excluding swap buffers (as the swap may wait for GPU)
		cpuTime = nvgwin_get_time() - t;

		updateGraph(fps, static_cast<float>(dt));
		updateGraph(cpuGraph, static_cast<float>(cpuTime));

		// We may get multiple results.
		n = stopGPUTimer(&gpuTimer, gpuTimes.data(), (int)gpuTimes.size());
		for (i = 0; i < n; i++)
			updateGraph(gpuGraph, gpuTimes[i]);

		nvgwin_swap_buffers(window);
		nvgwin_poll_events();
	}

	nvgluDeleteFramebuffer(fb);

	nvg::deleteGL(vgOwner);

	printf("Average Frame Time: %.2f ms\n", getGraphAverage(fps) * 1000.0f);
	printf("          CPU Time: %.2f ms\n", getGraphAverage(cpuGraph) * 1000.0f);
	printf("          GPU Time: %.2f ms\n", getGraphAverage(gpuGraph) * 1000.0f);

	nvgwin_destroy(window);
	nvgwin_shutdown();
	return 0;
}
