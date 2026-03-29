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
#include <memory>
#ifdef NANOVG_GLEW
#include <GL/glew.h>
#endif
#if defined(NANOVG_USE_SDL3) && !defined(NANOVG_GLEW)
#include <SDL3/SDL_opengl.h>
#elif !defined(NANOVG_USE_SDL3)
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#endif
#include "nvg_window.hpp"
#include "nvg_input.hpp"
#include "nanovg.hpp"
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg_gl.hpp"
#include "demo.hpp"
#include "perf.hpp"

void errorcb(int error, const char* desc)
{
    printf("Window error %d: %s\n", error, desc);
}

int blowup = 0;
int screenshot = 0;
int premult = 0;

static void key(NvgWindow* window, int key, int scancode, int action, int mods, void* user)
{
    UNUSED(user);
    UNUSED(scancode);
    UNUSED(mods);
    if (key == NVG_KEY_ESCAPE && action == NVG_PRESS) nvgwin_set_should_close(window, 1);
    if (key == NVG_KEY_SPACE && action == NVG_PRESS) blowup = !blowup;
    if (key == NVG_KEY_S && action == NVG_PRESS) screenshot = 1;
    if (key == NVG_KEY_P && action == NVG_PRESS) premult = !premult;
}

int main()
{
    NvgWindow* window;
    DemoData data;
    PerfGraph fps;
    double prevt = 0;

    if (nvgwin_init() != 0) {
        printf("Failed to init window system.");
        return -1;
    }

    initGraph(fps, GRAPH_RENDER_FPS, "Frame Time");

    nvgwin_set_error_callback(errorcb);
#ifdef DEMO_MSAA
    const int msaa = 4;
#else
    const int msaa = 0;
#endif

    window = nvgwin_create(1000, 600, "NanoVG", NVGWIN_PROFILE_GL2, msaa, 0);
    if (!window) {
        nvgwin_shutdown();
        return -1;
    }

    nvgwin_set_key_callback(window, key, nullptr);

    nvgwin_make_current(window);
#ifdef NANOVG_GLEW
    if (glewInit() != GLEW_OK) {
        printf("Could not init glew.\n");
        return -1;
    }
#endif

#ifdef DEMO_MSAA
    auto vgOwner = nvg::createGL(static_cast<int>(nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#else
    auto vgOwner = nvg::createGL(static_cast<int>(nvg::CreateFlags::Antialias | nvg::CreateFlags::StencilStrokes | nvg::CreateFlags::Debug));
#endif
    if (!vgOwner) {
        printf("Could not init nanovg.\n");
        return -1;
    }
    nvg::Context& vg = *vgOwner;

    if (loadDemoData(vg, data) == -1) return -1;

    nvgwin_swap_interval(0);

    nvgwin_set_time(0);
    prevt = nvgwin_get_time();

    while (!nvgwin_should_close(window)) {
        double mx, my, t, dt;
        int winWidth, winHeight;
        int fbWidth, fbHeight;
        float pxRatio;

        t = nvgwin_get_time();
        dt = t - prevt;
        prevt = t;
        updateGraph(fps, static_cast<float>(dt));

        nvgwin_get_cursor_pos(window, &mx, &my);
        nvgwin_get_window_size(window, &winWidth, &winHeight);
        nvgwin_get_framebuffer_size(window, &fbWidth, &fbHeight);

        // Calculate pixel ration for hi-dpi devices.
        pxRatio = (float)fbWidth / (float)winWidth;

        // Update and render
        glViewport(0, 0, fbWidth, fbHeight);
        if (premult)
            glClearColor(0, 0, 0, 0);
        else
            glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        vg.beginFrame(static_cast<float>(winWidth), static_cast<float>(winHeight), pxRatio);

        renderDemo(vg, static_cast<float>(mx), static_cast<float>(my), static_cast<float>(winWidth), static_cast<float>(winHeight), static_cast<float>(t), blowup, data);
        renderGraph(vg, 5, 5, fps);

        vg.endFrame();

        if (screenshot) {
            screenshot = 0;
            saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
        }

        nvgwin_swap_buffers(window);
        nvgwin_poll_events();
    }

    freeDemoData(vg, data);

    nvg::deleteGL(vgOwner);

    nvgwin_destroy(window);
    nvgwin_shutdown();
    return 0;
}
