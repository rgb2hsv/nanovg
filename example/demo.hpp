#pragma once
#include "nanovg.hpp"
#include <array>
#include <string>

struct DemoData {
    int fontNormal, fontBold, fontIcons, fontEmoji;
    std::array<int, 12> images{};
};

int loadDemoData(nvg::Context& vg, DemoData& data);
void freeDemoData(nvg::Context& vg, DemoData& data);
void renderDemo(nvg::Context& vg, float mx, float my, float width, float height, float t, int blowup, DemoData& data);

bool saveScreenShot(int w, int h, int premult, const std::string& name, bool compare = false);
