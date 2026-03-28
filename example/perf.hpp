#pragma once

#include "nanovg.hpp"
#include <array>
#include <string>

enum GraphrenderStyle {
    GRAPH_RENDER_FPS,
    GRAPH_RENDER_MS,
    GRAPH_RENDER_PERCENT,
};

#define GRAPH_HISTORY_COUNT 100
struct PerfGraph {
	int style;
	std::array<char, 32> name{};
	std::array<float, GRAPH_HISTORY_COUNT> values{};
	int head;
};
typedef struct PerfGraph PerfGraph;

void initGraph(PerfGraph* fps, int style, const std::string& name);
void updateGraph(PerfGraph* fps, float frameTime);
void renderGraph(nvg::Context& vg, float x, float y, PerfGraph* fps);
float getGraphAverage(PerfGraph* fps);

#define GPU_QUERY_COUNT 5
struct GPUtimer {
	int supported;
	int cur, ret;
	std::array<unsigned int, GPU_QUERY_COUNT> queries{};
};
typedef struct GPUtimer GPUtimer;

void initGPUTimer(GPUtimer* timer);
void startGPUTimer(GPUtimer* timer);
int stopGPUTimer(GPUtimer* timer, float* times, int maxTimes);