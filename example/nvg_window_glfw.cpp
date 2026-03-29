#include "nvg_window.hpp"
#include <stdlib.h>
#ifdef NANOVG_GLEW
#include <GL/glew.h>
#endif
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

struct NvgWindow {
    GLFWwindow* glfw;
    NvgwinKeyFn key_cb;
    void* key_user;
};

static NvgwinErrorFn g_error_fn = nullptr;

static void glfw_error_cb(int code, const char* desc)
{
    if (g_error_fn) g_error_fn(code, desc);
}

static void glfw_key_cb(GLFWwindow* w, int key, int scancode, int action, int mods)
{
    NvgWindow* nw = static_cast<NvgWindow*>(glfwGetWindowUserPointer(w));
    if (nw && nw->key_cb) nw->key_cb(nw, key, scancode, action, mods, nw->key_user);
}

int nvgwin_init(void)
{
    return glfwInit() ? 0 : -1;
}

void nvgwin_shutdown(void)
{
    glfwTerminate();
}

void nvgwin_set_error_callback(NvgwinErrorFn fn)
{
    g_error_fn = fn;
    glfwSetErrorCallback(glfw_error_cb);
}

NvgWindow* nvgwin_create(int w, int h, const char* title, NvgwinProfile profile, int msaa_samples, int debug_context)
{
    glfwDefaultWindowHints();
    switch (profile) {
        case NVGWIN_PROFILE_GL2:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            if (msaa_samples > 0) glfwWindowHint(GLFW_SAMPLES, msaa_samples);
            break;
        case NVGWIN_PROFILE_GL3:
#ifndef _WIN32
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debug_context ? GLFW_TRUE : GLFW_FALSE);
            if (msaa_samples > 0) glfwWindowHint(GLFW_SAMPLES, msaa_samples);
            break;
        case NVGWIN_PROFILE_GLES2:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            break;
        case NVGWIN_PROFILE_GLES3:
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            break;
    }

    GLFWwindow* gw = glfwCreateWindow(w, h, title, NULL, NULL);
    if (!gw) return nullptr;
    NvgWindow* nw = new NvgWindow{};
    nw->glfw = gw;
    nw->key_cb = nullptr;
    nw->key_user = nullptr;
    glfwSetWindowUserPointer(gw, nw);
    return nw;
}

void nvgwin_destroy(NvgWindow* win)
{
    if (!win) return;
    glfwDestroyWindow(win->glfw);
    delete win;
}

void nvgwin_make_current(NvgWindow* win)
{
    glfwMakeContextCurrent(win->glfw);
}

void nvgwin_swap_buffers(NvgWindow* win)
{
    glfwSwapBuffers(win->glfw);
}

void nvgwin_poll_events(void)
{
    glfwPollEvents();
}

int nvgwin_should_close(const NvgWindow* win)
{
    return glfwWindowShouldClose(win->glfw) ? 1 : 0;
}

void nvgwin_set_should_close(NvgWindow* win, int close)
{
    glfwSetWindowShouldClose(win->glfw, close ? GLFW_TRUE : GLFW_FALSE);
}

double nvgwin_get_time(void)
{
    return glfwGetTime();
}

void nvgwin_set_time(double seconds)
{
    glfwSetTime(seconds);
}

void nvgwin_get_cursor_pos(NvgWindow* win, double* x, double* y)
{
    glfwGetCursorPos(win->glfw, x, y);
}

void nvgwin_get_window_size(NvgWindow* win, int* width, int* height)
{
    glfwGetWindowSize(win->glfw, width, height);
}

void nvgwin_get_framebuffer_size(NvgWindow* win, int* width, int* height)
{
    glfwGetFramebufferSize(win->glfw, width, height);
}

void nvgwin_swap_interval(int interval)
{
    glfwSwapInterval(interval);
}

void nvgwin_set_key_callback(NvgWindow* win, NvgwinKeyFn fn, void* user)
{
    win->key_cb = fn;
    win->key_user = user;
    glfwSetKeyCallback(win->glfw, glfw_key_cb);
}
