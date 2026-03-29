#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NvgWindow NvgWindow;

typedef enum NvgwinProfile {
    NVGWIN_PROFILE_GL2,
    NVGWIN_PROFILE_GL3,
    NVGWIN_PROFILE_GLES2,
    NVGWIN_PROFILE_GLES3,
} NvgwinProfile;

int nvgwin_init(void);
void nvgwin_shutdown(void);
typedef void (*NvgwinErrorFn)(int code, const char* desc);
void nvgwin_set_error_callback(NvgwinErrorFn fn);

NvgWindow* nvgwin_create(int w, int h, const char* title, NvgwinProfile profile, int msaa_samples, int debug_context);
void nvgwin_destroy(NvgWindow* win);

void nvgwin_make_current(NvgWindow* win);
void nvgwin_swap_buffers(NvgWindow* win);
void nvgwin_poll_events(void);
int nvgwin_should_close(const NvgWindow* win);
void nvgwin_set_should_close(NvgWindow* win, int close);

double nvgwin_get_time(void);
void nvgwin_set_time(double seconds);

void nvgwin_get_cursor_pos(NvgWindow* win, double* x, double* y);
void nvgwin_get_window_size(NvgWindow* win, int* width, int* height);
void nvgwin_get_framebuffer_size(NvgWindow* win, int* width, int* height);
void nvgwin_swap_interval(int interval);

typedef void (*NvgwinKeyFn)(NvgWindow* win, int key, int scancode, int action, int mods, void* user);
void nvgwin_set_key_callback(NvgWindow* win, NvgwinKeyFn fn, void* user);

#ifdef __cplusplus
}
#endif
