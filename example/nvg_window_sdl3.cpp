#include "nvg_window.hpp"
#include "nvg_input.hpp"
#include <SDL3/SDL.h>
#include <stdlib.h>

struct NvgWindow {
	SDL_Window* window;
	SDL_GLContext gl_ctx;
	bool should_close;
	NvgwinKeyFn key_cb;
	void* key_user;
};

static NvgwinErrorFn g_error_fn = nullptr;
static NvgWindow* g_active_window = nullptr;
static uint64_t g_time_origin_ms = 0;

static int map_sdl_key_to_nvg(SDL_Keycode k)
{
	switch (k) {
	case SDLK_ESCAPE: return NVG_KEY_ESCAPE;
	case SDLK_SPACE: return NVG_KEY_SPACE;
	case SDLK_S: return NVG_KEY_S;
	case SDLK_P: return NVG_KEY_P;
	default: return -1;
	}
}

static void report_sdl_error(const char* what)
{
	if (g_error_fn)
		g_error_fn(0, SDL_GetError());
	(void)what;
}

int nvgwin_init(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		report_sdl_error("SDL_Init");
		return -1;
	}
	return 0;
}

void nvgwin_shutdown(void)
{
	SDL_Quit();
}

void nvgwin_set_error_callback(NvgwinErrorFn fn)
{
	g_error_fn = fn;
}

NvgWindow* nvgwin_create(int w, int h, const char* title, NvgwinProfile profile, int msaa_samples, int debug_context)
{
	SDL_GL_ResetAttributes();

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	switch (profile) {
	case NVGWIN_PROFILE_GL2:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		if (msaa_samples > 0) {
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa_samples);
		}
		break;
	case NVGWIN_PROFILE_GL3:
#ifndef _WIN32
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		{
			int glflags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
			if (debug_context)
				glflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, glflags);
		}
#else
		if (debug_context)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
		if (msaa_samples > 0) {
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa_samples);
		}
		break;
	case NVGWIN_PROFILE_GLES2:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		break;
	case NVGWIN_PROFILE_GLES3:
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		break;
	}

	SDL_Window* win = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL);
	if (!win) {
		report_sdl_error("SDL_CreateWindow");
		return nullptr;
	}

	SDL_GLContext ctx = SDL_GL_CreateContext(win);
	if (!ctx) {
		report_sdl_error("SDL_GL_CreateContext");
		SDL_DestroyWindow(win);
		return nullptr;
	}

	NvgWindow* nw = new NvgWindow{};
	nw->window = win;
	nw->gl_ctx = ctx;
	nw->should_close = false;
	nw->key_cb = nullptr;
	nw->key_user = nullptr;
	g_active_window = nw;
	g_time_origin_ms = SDL_GetTicks();
	return nw;
}

void nvgwin_destroy(NvgWindow* win)
{
	if (!win)
		return;
	if (g_active_window == win)
		g_active_window = nullptr;
	SDL_GL_DestroyContext(win->gl_ctx);
	SDL_DestroyWindow(win->window);
	delete win;
}

void nvgwin_make_current(NvgWindow* win)
{
	SDL_GL_MakeCurrent(win->window, win->gl_ctx);
}

void nvgwin_swap_buffers(NvgWindow* win)
{
	SDL_GL_SwapWindow(win->window);
}

void nvgwin_poll_events(void)
{
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		NvgWindow* nw = g_active_window;
		switch (e.type) {
		case SDL_EVENT_QUIT:
			if (nw)
				nw->should_close = true;
			break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (nw)
				nw->should_close = true;
			break;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			if (nw && nw->key_cb) {
				int key = map_sdl_key_to_nvg(e.key.key);
				if (key >= 0) {
					int action = (e.type == SDL_EVENT_KEY_DOWN) ? NVG_PRESS : NVG_RELEASE;
					if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat)
						action = 2;
					nw->key_cb(nw, key, (int)e.key.scancode, action, (int)e.key.mod, nw->key_user);
				}
			}
			break;
		default:
			break;
		}
	}
}

int nvgwin_should_close(const NvgWindow* win)
{
	return win->should_close ? 1 : 0;
}

void nvgwin_set_should_close(NvgWindow* win, int close)
{
	win->should_close = close != 0;
}

double nvgwin_get_time(void)
{
	return (SDL_GetTicks() - g_time_origin_ms) / 1000.0;
}

void nvgwin_set_time(double seconds)
{
	g_time_origin_ms = SDL_GetTicks() - (uint64_t)(seconds * 1000.0);
}

void nvgwin_get_cursor_pos(NvgWindow* win, double* x, double* y)
{
	(void)win;
	float fx = 0.f, fy = 0.f;
	SDL_GetMouseState(&fx, &fy);
	if (x)
		*x = fx;
	if (y)
		*y = fy;
}

void nvgwin_get_window_size(NvgWindow* win, int* width, int* height)
{
	SDL_GetWindowSize(win->window, width, height);
}

void nvgwin_get_framebuffer_size(NvgWindow* win, int* width, int* height)
{
	SDL_GetWindowSizeInPixels(win->window, width, height);
}

void nvgwin_swap_interval(int interval)
{
	SDL_GL_SetSwapInterval(interval);
}

void nvgwin_set_key_callback(NvgWindow* win, NvgwinKeyFn fn, void* user)
{
	win->key_cb = fn;
	win->key_user = user;
}
