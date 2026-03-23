#ifndef DEMO_H
#define DEMO_H

#include "nanovg.hpp"

struct DemoData {
	int fontNormal, fontBold, fontIcons, fontEmoji;
	int images[12];
};
typedef struct DemoData DemoData;

int loadDemoData(nvg::NVGcontext* vg, DemoData* data);
void freeDemoData(nvg::NVGcontext* vg, DemoData* data);
void renderDemo(nvg::NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup, DemoData* data);

void saveScreenShot(int w, int h, int premult, const char* name);

#endif // DEMO_H
