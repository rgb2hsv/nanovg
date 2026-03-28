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
#include <stdlib.h>
#include <string.h>
#include <array>
#include <memory>
#ifdef NANOVG_GLEW
#	include <GL/glew.h>
#endif
#ifdef __APPLE__
#	define GLFW_INCLUDE_GLCOREARB
#endif
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include "nanovg.hpp"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.hpp"
#include "demo.hpp"
#include "perf.h"


void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

int blowup = 0;
int screenshot = 0;
int premult = 0;

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	UNUSED(scancode);
	UNUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		blowup = !blowup;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		screenshot = 1;
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		premult = !premult;
}

int main(int argc, char** argv)
{
	GLFWwindow* window;
	DemoData data;
	std::shared_ptr<nvg::Context> vgOwner;
	GPUtimer gpuTimer;
	PerfGraph fps, cpuGraph, gpuGraph;
	double prevt = 0, cpuTime = 0;
	int testCount = 0;
	bool testSpecified = false;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--test")) {
			if (i + 1 >= argc) {
				printf("Missing value for --test N.\n");
				return -1;
			}
			char* endptr = NULL;
			long v = strtol(argv[i + 1], &endptr, 10);
			if (endptr == argv[i + 1] || (endptr && *endptr != '\0') || v < 0) {
				printf("Invalid --test N value.\n");
				return -1;
			}
			testCount = static_cast<int>(v);
			testSpecified = true;
			i++;
		} else if (!strncmp(argv[i], "--test=", 7)) {
			char* endptr = NULL;
			long v = strtol(argv[i] + 7, &endptr, 10);
			if (endptr == argv[i] + 7 || (endptr && *endptr != '\0') || v < 0) {
				printf("Invalid --test N value.\n");
				return -1;
			}
			testCount = static_cast<int>(v);
			testSpecified = true;
		}
	}
	int testRemaining = testCount;

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");
	initGraph(&cpuGraph, GRAPH_RENDER_MS, "CPU Time");
	initGraph(&gpuGraph, GRAPH_RENDER_MS, "GPU Time");

	glfwSetErrorCallback(errorcb);
#ifndef _WIN32 // don't require this on win32, and works with more cards
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

#ifdef DEMO_MSAA
	glfwWindowHint(GLFW_SAMPLES, 4);
#endif
	window = glfwCreateWindow(1000, 600, "NanoVG", NULL, NULL);
//	window = glfwCreateWindow(1000, 600, "NanoVG", glfwGetPrimaryMonitor(), NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);

	glfwMakeContextCurrent(window);
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
	vgOwner = nvg::createGL3(static_cast<int>(nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#else
	vgOwner = nvg::createGL3(static_cast<int>(nvg::CreateFlags::Antialias | nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#endif
	if (!vgOwner) {
		printf("Could not init nanovg.\n");
		return -1;
	}
	nvg::Context& vg = *vgOwner;

	if (loadDemoData(vg, &data) == -1) {
		printf("Could not load demo data.\n");
		return -1;
	}

	if (testSpecified && testCount == 0)
		glfwSetWindowShouldClose(window, GL_TRUE);

	glfwSwapInterval(0);

	initGPUTimer(&gpuTimer);

	glfwSetTime(0);
	long long frameCounter = 0;
	int sampleRate = 5;
	bool success= true;
	if(testSpecified)
		prevt = 0.0f;
	else
		prevt = glfwGetTime();
	
	while (!glfwWindowShouldClose(window))
	{
		double mx, my, t, dt;
		int winWidth, winHeight;
		int fbWidth, fbHeight;
		float pxRatio;
		std::array<float, 3> gpuTimes{};
		int i, n;

		if(testSpecified)
			t=frameCounter/30.0f;
		else
			t = glfwGetTime();
		
		dt = t - prevt;
		prevt = t;

		startGPUTimer(&gpuTimer);


		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		if(testSpecified){
			double angle = 2.0 * nvg::PI * testRemaining / (double)testCount;
			mx=winWidth/2.0f+cos(angle)*winWidth/4.0f;
			my=winHeight/2.0f+sin(angle)*winHeight/4.0f;
		} else {
			glfwGetCursorPos(window, &mx, &my);
		}
		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		if (premult)
			glClearColor(0,0,0,0);
		else
			glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		nvg::beginFrame(vg, (float)winWidth, (float)winHeight, pxRatio);

		renderDemo(vg, (float)mx, (float)my, (float)winWidth, (float)winHeight, (float)t, blowup, &data);

		if(!testSpecified){
			renderGraph(vg, 5,5, &fps);
			renderGraph(vg, 5+200+5,5, &cpuGraph);
			if (gpuTimer.supported) {
				renderGraph(vg, 5+200+5+200+5,5, &gpuGraph);
			}
		}
		nvg::endFrame(vg);

		// Measure the CPU time taken excluding swap buffers (as the swap may wait for GPU)
		cpuTime = glfwGetTime() - t;

		updateGraph(&fps, (float)dt);
		updateGraph(&cpuGraph, (float)cpuTime);

		// We may get multiple results.
		n = stopGPUTimer(&gpuTimer, gpuTimes.data(), (int)gpuTimes.size());
		for (i = 0; i < n; i++)
			updateGraph(&gpuGraph, gpuTimes[i]);

		if (testRemaining > 0 && frameCounter % sampleRate ==0) {
			std::array<char, 256> fileName{};
			printf("Capturing %d/%d at t = %0.2f sec\n", testCount - testRemaining + 1, testCount,t);
			snprintf(fileName.data(), (int)fileName.size(), "screenshot%03d.png", testCount - testRemaining + 1);
			success&=saveScreenShot(fbWidth, fbHeight, false, fileName.data(), true);
			testRemaining--;
			if (testRemaining == 0)
				glfwSetWindowShouldClose(window, GL_TRUE);
		} else if (screenshot) {
			screenshot = 0;
			success&=saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
		}
		frameCounter++;
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	freeDemoData(vg, &data);

	nvg::deleteGL3(std::move(vgOwner));

	if (!testSpecified) {
		printf("Average Frame Time: %.2f ms\n", getGraphAverage(&fps) * 1000.0f);
		printf("          CPU Time: %.2f ms\n", getGraphAverage(&cpuGraph) * 1000.0f);
		printf("          GPU Time: %.2f ms\n", getGraphAverage(&gpuGraph) * 1000.0f);
	} else {
		if(success)
			printf("Test passed!\n");
		else
			printf("Test failed!\n");
	}

	glfwTerminate();
	return (success)?EXIT_SUCCESS:EXIT_FAILURE;
}



