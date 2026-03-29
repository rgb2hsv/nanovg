#include "perf.hpp"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <array>
#ifdef NANOVG_GLEW
#  include <GL/glew.h>
#endif
#if !defined(NANOVG_USE_SDL3)
#  include <GLFW/glfw3.h>
#endif
#include "nanovg.hpp"
#ifdef _MSC_VER
#define snprintf _snprintf
#elif !defined(__MINGW32__)
#include <iconv.h>
#endif

// timer query support
#ifndef GL_ARB_timer_query
#define GL_TIME_ELAPSED                   0x88BF
//typedef void (APIENTRY *pfnGLGETQUERYOBJECTUI64V)(GLuint id, GLenum pname, GLuint64* params);
//pfnGLGETQUERYOBJECTUI64V glGetQueryObjectui64v = 0;
#endif

void initGPUTimer(GPUtimer* timer)
{
	timer->supported = 0;
	timer->cur = 0;
	timer->ret = 0;
	timer->queries.fill(0u);

/*	timer->supported = glfwExtensionSupported("GL_ARB_timer_query");
	if (timer->supported) {
#ifndef GL_ARB_timer_query
		glGetQueryObjectui64v = (pfnGLGETQUERYOBJECTUI64V)glfwGetProcAddress("glGetQueryObjectui64v");
		printf("glGetQueryObjectui64v=%p\n", glGetQueryObjectui64v);
		if (!glGetQueryObjectui64v) {
			timer->supported = GL_FALSE;
			return;
		}
#endif
		glGenQueries(GPU_QUERY_COUNT, timer->queries.data());
	}*/
}

void startGPUTimer(GPUtimer* timer)
{
	if (!timer->supported)
		return;
	glBeginQuery(GL_TIME_ELAPSED, timer->queries[timer->cur % GPU_QUERY_COUNT] );
	timer->cur++;
}

int stopGPUTimer(GPUtimer* timer, float* times, int maxTimes)
{
	UNUSED(times);
	UNUSED(maxTimes);
	GLint available = 1;
	int n = 0;
	if (!timer->supported)
		return 0;

	glEndQuery(GL_TIME_ELAPSED);
	while (available && timer->ret <= timer->cur) {
		// check for results if there are any
		glGetQueryObjectiv(timer->queries[timer->ret % GPU_QUERY_COUNT], GL_QUERY_RESULT_AVAILABLE, &available);
		if (available) {
/*			GLuint64 timeElapsed = 0;
			glGetQueryObjectui64v(timer->queries[timer->ret % GPU_QUERY_COUNT], GL_QUERY_RESULT, &timeElapsed);
			timer->ret++;
			if (n < maxTimes) {
				times[n] = (float)((double)timeElapsed * 1e-9);
				n++;
			}*/
		}
	}
	return n;
}


void initGraph(PerfGraph& graph, int style, const std::string& name)
{
	graph.style = style;
	graph.head = 0;
	graph.values.fill(0.0f);
	graph.name.fill('\0');
	strncpy(graph.name.data(), name.c_str(), graph.name.size());
	graph.name[graph.name.size()-1] = '\0';
}

void updateGraph(PerfGraph& graph, float frameTime)
{
	graph.head = (graph.head+1) % GRAPH_HISTORY_COUNT;
	graph.values[graph.head] = frameTime;
}

float getGraphAverage(PerfGraph& graph)
{
	int i;
	float avg = 0;
	for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
		avg += graph.values[i];
	}
	return avg / (float)GRAPH_HISTORY_COUNT;
}

void renderGraph(nvg::Context& vg, float x, float y, PerfGraph& graph)
{
	int i;
	float avg, w, h;
	std::array<char, 64> str{};

	avg = getGraphAverage(graph);

	w = 200;
	h = 35;

	vg.beginPath();
	vg.rect( x,y, w,h);
	vg.fillColor( nvg::rgba(0,0,0,128));
	vg.fill();

	vg.beginPath();
	vg.moveTo( x, y+h);
	if (graph.style == GRAPH_RENDER_FPS) {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = 1.0f / (0.00001f + graph.values[(graph.head+i) % GRAPH_HISTORY_COUNT]);
			float vx, vy;
			if (v > 80.0f) v = 80.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 80.0f) * h);
			vg.lineTo( vx, vy);
		}
	} else if (graph.style == GRAPH_RENDER_PERCENT) {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = graph.values[(graph.head+i) % GRAPH_HISTORY_COUNT] * 1.0f;
			float vx, vy;
			if (v > 100.0f) v = 100.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 100.0f) * h);
			vg.lineTo( vx, vy);
		}
	} else {
		for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
			float v = graph.values[(graph.head+i) % GRAPH_HISTORY_COUNT] * 1000.0f;
			float vx, vy;
			if (v > 20.0f) v = 20.0f;
			vx = x + ((float)i/(GRAPH_HISTORY_COUNT-1)) * w;
			vy = y + h - ((v / 20.0f) * h);
			vg.lineTo( vx, vy);
		}
	}
	vg.lineTo( x+w, y+h);
	vg.fillColor( nvg::rgba(255,192,0,128));
	vg.fill();

	vg.fontFace( "sans");

	if (graph.name[0] != '\0') {
		vg.fontSize( 12.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Left | nvg::Align::Top));
		vg.fillColor( nvg::rgba(240,240,240,192));
		vg.text( x+3,y+3, graph.name.data(), NULL);
	}

	if (graph.style == GRAPH_RENDER_FPS) {
		vg.fontSize( 15.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Right | nvg::Align::Top));
		vg.fillColor( nvg::rgba(240,240,240,255));
		sprintf(str.data(), "%.2f FPS", 1.0f / avg);
		vg.text( x+w-3,y+3, str.data(), NULL);

		vg.fontSize( 13.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Right | nvg::Align::Baseline));
		vg.fillColor( nvg::rgba(240,240,240,160));
		sprintf(str.data(), "%.2f ms", avg * 1000.0f);
		vg.text( x+w-3,y+h-3, str.data(), NULL);
	}
	else if (graph.style == GRAPH_RENDER_PERCENT) {
		vg.fontSize( 15.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Right | nvg::Align::Top));
		vg.fillColor( nvg::rgba(240,240,240,255));
		sprintf(str.data(), "%.1f %%", avg * 1.0f);
		vg.text( x+w-3,y+3, str.data(), NULL);
	} else {
		vg.fontSize( 15.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Right | nvg::Align::Top));
		vg.fillColor( nvg::rgba(240,240,240,255));
		sprintf(str.data(), "%.2f ms", avg * 1000.0f);
		vg.text( x+w-3,y+3, str.data(), NULL);
	}
}