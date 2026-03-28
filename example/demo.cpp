#include "demo.hpp"
#include <stdio.h>
#include <string.h>
#include <string>
#include <array>
#include <vector>

#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifdef NANOVG_GLEW
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>


#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#include "nanovg.hpp"
#ifdef _MSC_VER
#define snprintf _snprintf
#elif !defined(__MINGW32__)
#include <iconv.h>
#endif

#define ICON_SEARCH 0x1F50D
#define ICON_CIRCLED_CROSS 0x2716
#define ICON_CHEVRON_RIGHT 0xE75E
#define ICON_CHECK 0x2713
#define ICON_LOGIN 0xE740
#define ICON_TRASH 0xE729

//static float minf(float a, float b) { return a < b ? a : b; }
//static float maxf(float a, float b) { return a > b ? a : b; }
//static float absf(float a) { return a >= 0.0f ? a : -a; }
static float clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }

// Returns 1 if col.rgba is 0.0f,0.0f,0.0f,0.0f, 0 otherwise
int isBlack(nvg::Color col)
{
	if( col.r == 0.0f && col.g == 0.0f && col.b == 0.0f && col.a == 0.0f )
	{
		return 1;
	}
	return 0;
}

static char* cpToUTF8(int cp, char* str)
{
	int n = 0;
	if (cp < 0x80) n = 1;
	else if (cp < 0x800) n = 2;
	else if (cp < 0x10000) n = 3;
	else if (cp < 0x200000) n = 4;
	else if (cp < 0x4000000) n = 5;
	else if (cp <= 0x7fffffff) n = 6;
	str[n] = '\0';
	switch (n) {
	case 6: str[5] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x4000000;
	case 5: str[4] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x200000;
	case 4: str[3] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x10000;
	case 3: str[2] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x800;
	case 2: str[1] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0xc0;
	case 1: str[0] = cp;
	}
	return str;
}


void drawWindow(nvg::Context& vg, const char* title, float x, float y, float w, float h)
{
	float cornerRadius = 3.0f;
	nvg::Paint shadowPaint;
	nvg::Paint headerPaint;

	nvg::save(vg);
//	clearState(vg);

	// Window
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x,y, w,h, cornerRadius);
	nvg::fillColor(vg, nvg::rgba(28,30,34,192));
//	nvg::fillColor(vg, nvg::rgba(0,0,0,128));
	nvg::fill(vg);

	// Drop shadow
	shadowPaint = nvg::boxGradient(vg, x,y+2, w,h, cornerRadius*2, 10, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	nvg::beginPath(vg);
	nvg::rect(vg, x-10,y-10, w+20,h+30);
	nvg::roundedRect(vg, x,y, w,h, cornerRadius);
	nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
	nvg::fillPaint(vg, shadowPaint);
	nvg::fill(vg);

	// Header
	headerPaint = nvg::linearGradient(vg, x,y,x,y+15, nvg::rgba(255,255,255,8), nvg::rgba(0,0,0,16));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+1,y+1, w-2,30, cornerRadius-1);
	nvg::fillPaint(vg, headerPaint);
	nvg::fill(vg);
	nvg::beginPath(vg);
	nvg::moveTo(vg, x+0.5f, y+0.5f+30);
	nvg::lineTo(vg, x+0.5f+w-1, y+0.5f+30);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,32));
	nvg::stroke(vg);

	nvg::fontSize(vg, 15.0f);
	nvg::fontFace(vg, "sans-bold");
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));

	nvg::fontBlur(vg,2);
	nvg::fillColor(vg, nvg::rgba(0,0,0,128));
	nvg::text(vg, x+w/2,y+16+1, title, NULL);

	nvg::fontBlur(vg,0);
	nvg::fillColor(vg, nvg::rgba(220,220,220,160));
	nvg::text(vg, x+w/2,y+16, title, NULL);

	nvg::restore(vg);
}

void drawSearchBox(nvg::Context& vg, const char* text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = h/2-1;

	// Edit
	bg = nvg::boxGradient(vg, x,y+1.5f, w,h, h/2,5, nvg::rgba(0,0,0,16), nvg::rgba(0,0,0,92));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x,y, w,h, cornerRadius);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

/*	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,48));
	nvg::stroke(vg);*/

	nvg::fontSize(vg, h*1.3f);
	nvg::fontFace(vg, "icons");
	nvg::fillColor(vg, nvg::rgba(255,255,255,64));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	nvg::text(vg, x+h*0.55f, y+h*0.55f, cpToUTF8(ICON_SEARCH, icon.data()), NULL);

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,32));

	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::text(vg, x+h*1.05f,y+h*0.5f,text, NULL);

	nvg::fontSize(vg, h*1.3f);
	nvg::fontFace(vg, "icons");
	nvg::fillColor(vg, nvg::rgba(255,255,255,32));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	nvg::text(vg, x+w-h*0.55f, y+h*0.55f, cpToUTF8(ICON_CIRCLED_CROSS, icon.data()), NULL);
}

void drawDropDown(nvg::Context& vg, const char* text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = 4.0f;

	bg = nvg::linearGradient(vg, x,y,x,y+h, nvg::rgba(255,255,255,16), nvg::rgba(0,0,0,16));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,48));
	nvg::stroke(vg);

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,160));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::text(vg, x+h*0.3f,y+h*0.5f,text, NULL);

	nvg::fontSize(vg, h*1.3f);
	nvg::fontFace(vg, "icons");
	nvg::fillColor(vg, nvg::rgba(255,255,255,64));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	nvg::text(vg, x+w-h*0.5f, y+h*0.5f, cpToUTF8(ICON_CHEVRON_RIGHT, icon.data()), NULL);
}

void drawLabel(nvg::Context& vg, const char* text, float x, float y, float w, float h)
{
	UNUSED(w);

	nvg::fontSize(vg, 15.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,128));

	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::text(vg, x,y+h*0.5f,text, NULL);
}

void drawEditBoxBase(nvg::Context& vg, float x, float y, float w, float h)
{
	nvg::Paint bg;
	// Edit
	bg = nvg::boxGradient(vg, x+1,y+1+1.5f, w-2,h-2, 3,4, nvg::rgba(255,255,255,32), nvg::rgba(32,32,32,32));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+1,y+1, w-2,h-2, 4-1);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, 4-0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,48));
	nvg::stroke(vg);
}

void drawEditBox(nvg::Context& vg, const char* text, float x, float y, float w, float h)
{

	drawEditBoxBase(vg, x,y, w,h);

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,64));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::text(vg, x+h*0.3f,y+h*0.5f,text, NULL);
}

void drawEditBoxNum(nvg::Context& vg,
					const char* text, const char* units, float x, float y, float w, float h)
{
	float uw;

	drawEditBoxBase(vg, x,y, w,h);

	uw = nvg::textBounds(vg, 0,0, units, NULL, NULL);

	nvg::fontSize(vg, 15.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,64));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Right | nvg::Align::Middle));
	nvg::text(vg, x+w-h*0.3f,y+h*0.5f,units, NULL);

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,128));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Right | nvg::Align::Middle));
	nvg::text(vg, x+w-uw-h*0.5f,y+h*0.5f,text, NULL);
}

void drawCheckBox(nvg::Context& vg, const char* text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	UNUSED(w);

	nvg::fontSize(vg, 15.0f);
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgba(255,255,255,160));

	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::text(vg, x+28,y+h*0.5f,text, NULL);

	bg = nvg::boxGradient(vg, x+1,y+(int)(h*0.5f)-9+1, 18,18, 3,3, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,92));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+1,y+(int)(h*0.5f)-9, 18,18, 3);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	nvg::fontSize(vg, 33);
	nvg::fontFace(vg, "icons");
	nvg::fillColor(vg, nvg::rgba(255,255,255,128));
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	nvg::text(vg, x+9+2, y+h*0.5f, cpToUTF8(ICON_CHECK, icon.data()), NULL);
}

void drawButton(nvg::Context& vg, int preicon, const char* text, float x, float y, float w, float h, nvg::Color col)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = 4.0f;
	float tw = 0, iw = 0;

	bg = nvg::linearGradient(vg, x,y,x,y+h, nvg::rgba(255,255,255,isBlack(col)?16:32), nvg::rgba(0,0,0,isBlack(col)?16:32));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
	if (!isBlack(col)) {
		nvg::fillColor(vg, col);
		nvg::fill(vg);
	}
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,48));
	nvg::stroke(vg);

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans-bold");
	tw = nvg::textBounds(vg, 0,0, text, NULL, NULL);
	if (preicon != 0) {
		nvg::fontSize(vg, h*1.3f);
		nvg::fontFace(vg, "icons");
		iw = nvg::textBounds(vg, 0,0, cpToUTF8(preicon, icon.data()), NULL, NULL);
		iw += h*0.15f;
	}

	if (preicon != 0) {
		nvg::fontSize(vg, h*1.3f);
		nvg::fontFace(vg, "icons");
		nvg::fillColor(vg, nvg::rgba(255,255,255,96));
		nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
		nvg::text(vg, x+w*0.5f-tw*0.5f-iw*0.75f, y+h*0.5f, cpToUTF8(preicon, icon.data()), NULL);
	}

	nvg::fontSize(vg, 17.0f);
	nvg::fontFace(vg, "sans-bold");
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	nvg::fillColor(vg, nvg::rgba(0,0,0,160));
	nvg::text(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f-1,text, NULL);
	nvg::fillColor(vg, nvg::rgba(255,255,255,160));
	nvg::text(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f,text, NULL);
}

void drawSlider(nvg::Context& vg, float pos, float x, float y, float w, float h)
{
	nvg::Paint bg, knob;
	float cy = y + h*0.5f;
	float kr = h*0.25f;

	nvg::save(vg);
//	clearState(vg);

	// Slot
	bg = nvg::boxGradient(vg, x,cy-2+1, w,4, 2,2, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,128));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x,cy-2, w,4, 2);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	// Knob Shadow
	bg = nvg::radialGradient(vg, x+(int)(pos*w),cy+1, kr-3,kr+3, nvg::rgba(0,0,0,64), nvg::rgba(0,0,0,0));
	nvg::beginPath(vg);
	nvg::rect(vg, x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
	nvg::circle(vg, x+(int)(pos*w),cy, kr);
	nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	// Knob
	knob = nvg::linearGradient(vg, x,cy-kr,x,cy+kr, nvg::rgba(255,255,255,16), nvg::rgba(0,0,0,16));
	nvg::beginPath(vg);
	nvg::circle(vg, x+(int)(pos*w),cy, kr-1);
	nvg::fillColor(vg, nvg::rgba(40,43,48,255));
	nvg::fill(vg);
	nvg::fillPaint(vg, knob);
	nvg::fill(vg);

	nvg::beginPath(vg);
	nvg::circle(vg, x+(int)(pos*w),cy, kr-0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,92));
	nvg::stroke(vg);

	nvg::restore(vg);
}

void drawFancyText(nvg::Context& vg, float x, float y){
	nvg::fontSize(vg, 30.0f);
	nvg::fontFace(vg, "sans-bold");
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));

	nvg::fontBlur(vg, 10);
	nvg::fillColor(vg, nvg::rgb(255, 0, 102));
	nvg::text(vg, x, y, "Font Blur", NULL);
	nvg::fontBlur(vg,0);
	nvg::fillColor(vg, nvg::rgb(255,255,255));
	nvg::text(vg, x, y, "Font Blur", NULL);

	fontDilate(vg, 2);
	nvg::fillColor(vg, nvg::rgb(255, 0, 102));
	nvg::text(vg, x, y+40, "Font Outline", NULL);
	fontDilate(vg,0);
	nvg::fillColor(vg, nvg::rgb(255,255,255));
	nvg::text(vg, x, y+40, "Font Outline", NULL);

	fontDilate(vg, 3); // Dilate will always be applied before blur
	nvg::fontBlur(vg, 2);
	nvg::fillColor(vg, nvg::rgb(255, 0, 102));
	nvg::text(vg, x, y+80, "Font Blur Outline", NULL);
	fontDilate(vg,0);
	nvg::fontBlur(vg,0);
	nvg::fillColor(vg, nvg::rgb(255,255,255));
	nvg::text(vg, x, y+80, "Font Blur Outline", NULL);
}

void drawEyes(nvg::Context& vg, float x, float y, float w, float h, float mx, float my, float t)
{
	nvg::Paint gloss, bg;
	float ex = w *0.23f;
	float ey = h * 0.5f;
	float lx = x + ex;
	float ly = y + ey;
	float rx = x + w - ex;
	float ry = y + ey;
	float dx,dy,d;
	float br = (ex < ey ? ex : ey) * 0.5f;
	float blink = 1.0f - powf(sinf(t*0.5f), 200.0f)*0.8f;

	bg = nvg::linearGradient(vg, x,y+h*0.5f,x+w*0.1f,y+h, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,16));
	nvg::beginPath(vg);
	nvg::ellipse(vg, lx+3.0f,ly+16.0f, ex,ey);
	nvg::ellipse(vg, rx+3.0f,ry+16.0f, ex,ey);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	bg = nvg::linearGradient(vg, x,y+h*0.25f,x+w*0.1f,y+h, nvg::rgba(220,220,220,255), nvg::rgba(128,128,128,255));
	nvg::beginPath(vg);
	nvg::ellipse(vg, lx,ly, ex,ey);
	nvg::ellipse(vg, rx,ry, ex,ey);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	nvg::beginPath(vg);
	nvg::ellipse(vg, lx+dx,ly+dy+ey*0.25f*(1-blink), br,br*blink);
	nvg::fillColor(vg, nvg::rgba(32,32,32,255));
	nvg::fill(vg);

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	nvg::beginPath(vg);
	nvg::ellipse(vg, rx+dx,ry+dy+ey*0.25f*(1-blink), br,br*blink);
	nvg::fillColor(vg, nvg::rgba(32,32,32,255));
	nvg::fill(vg);

	gloss = nvg::radialGradient(vg, lx-ex*0.25f,ly-ey*0.5f, ex*0.1f,ex*0.75f, nvg::rgba(255,255,255,128), nvg::rgba(255,255,255,0));
	nvg::beginPath(vg);
	nvg::ellipse(vg, lx,ly, ex,ey);
	nvg::fillPaint(vg, gloss);
	nvg::fill(vg);

	gloss = nvg::radialGradient(vg, rx-ex*0.25f,ry-ey*0.5f, ex*0.1f,ex*0.75f, nvg::rgba(255,255,255,128), nvg::rgba(255,255,255,0));
	nvg::beginPath(vg);
	nvg::ellipse(vg, rx,ry, ex,ey);
	nvg::fillPaint(vg, gloss);
	nvg::fill(vg);
}

void drawGraph(nvg::Context& vg, float x, float y, float w, float h, float t)
{
	nvg::Paint bg;
	std::array<float, 6> samples{};
	std::array<float, 6> sx{}, sy{};
	float dx = w/5.0f;
	int i;

	samples[0] = (1+sinf(t*1.2345f+cosf(t*0.33457f)*0.44f))*0.5f;
	samples[1] = (1+sinf(t*0.68363f+cosf(t*1.3f)*1.55f))*0.5f;
	samples[2] = (1+sinf(t*1.1642f+cosf(t*0.33457f)*1.24f))*0.5f;
	samples[3] = (1+sinf(t*0.56345f+cosf(t*1.63f)*0.14f))*0.5f;
	samples[4] = (1+sinf(t*1.6245f+cosf(t*0.254f)*0.3f))*0.5f;
	samples[5] = (1+sinf(t*0.345f+cosf(t*0.03f)*0.6f))*0.5f;

	for (i = 0; i < 6; i++) {
		sx[i] = x+i*dx;
		sy[i] = y+h*samples[i]*0.8f;
	}

	// Graph background
	bg = nvg::linearGradient(vg, x,y,x,y+h, nvg::rgba(0,160,192,0), nvg::rgba(0,160,192,64));
	nvg::beginPath(vg);
	nvg::moveTo(vg, sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		nvg::bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	nvg::lineTo(vg, x+w, y+h);
	nvg::lineTo(vg, x, y+h);
	nvg::fillPaint(vg, bg);
	nvg::fill(vg);

	// Graph line
	nvg::beginPath(vg);
	nvg::moveTo(vg, sx[0], sy[0]+2);
	for (i = 1; i < 6; i++)
		nvg::bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1]+2, sx[i]-dx*0.5f,sy[i]+2, sx[i],sy[i]+2);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,32));
	nvg::strokeWidth(vg, 3.0f);
	nvg::stroke(vg);

	nvg::beginPath(vg);
	nvg::moveTo(vg, sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		nvg::bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	nvg::strokeColor(vg, nvg::rgba(0,160,192,255));
	nvg::strokeWidth(vg, 3.0f);
	nvg::stroke(vg);

	// Graph sample pos
	for (i = 0; i < 6; i++) {
		bg = nvg::radialGradient(vg, sx[i],sy[i]+2, 3.0f,8.0f, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,0));
		nvg::beginPath(vg);
		nvg::rect(vg, sx[i]-10, sy[i]-10+2, 20,20);
		nvg::fillPaint(vg, bg);
		nvg::fill(vg);
	}

	nvg::beginPath(vg);
	for (i = 0; i < 6; i++)
		nvg::circle(vg, sx[i], sy[i], 4.0f);
	nvg::fillColor(vg, nvg::rgba(0,160,192,255));
	nvg::fill(vg);
	nvg::beginPath(vg);
	for (i = 0; i < 6; i++)
		nvg::circle(vg, sx[i], sy[i], 2.0f);
	nvg::fillColor(vg, nvg::rgba(220,220,220,255));
	nvg::fill(vg);

	nvg::strokeWidth(vg, 1.0f);
}

void drawSpinner(nvg::Context& vg, float cx, float cy, float r, float t)
{
	float a0 = 0.0f + t*6;
	float a1 = (float)M_PI + t*6;
	float r0 = r;
	float r1 = r * 0.75f;
	float ax,ay, bx,by;
	nvg::Paint paint;

	nvg::save(vg);

	nvg::beginPath(vg);
	nvg::arc(vg, cx,cy, r0, a0, a1, static_cast<int>(nvg::Winding::CW));
	nvg::arc(vg, cx,cy, r1, a1, a0, static_cast<int>(nvg::Winding::CCW));
	nvg::closePath(vg);
	ax = cx + cosf(a0) * (r0+r1)*0.5f;
	ay = cy + sinf(a0) * (r0+r1)*0.5f;
	bx = cx + cosf(a1) * (r0+r1)*0.5f;
	by = cy + sinf(a1) * (r0+r1)*0.5f;
	paint = nvg::linearGradient(vg, ax,ay, bx,by, nvg::rgba(0,0,0,0), nvg::rgba(0,0,0,128));
	nvg::fillPaint(vg, paint);
	nvg::fill(vg);

	nvg::restore(vg);
}

void drawThumbnails(nvg::Context& vg, float x, float y, float w, float h, const int* images, int nimages, float t)
{
	float cornerRadius = 3.0f;
	nvg::Paint shadowPaint, imgPaint, fadePaint;
	float ix,iy,iw,ih;
	float thumb = 60.0f;
	float arry = 30.5f;
	int imgw, imgh;
	float stackh = (nimages/2) * (thumb+10) + 10;
	int i;
	float u = (1+cosf(t*0.5f))*0.5f;
	float u2 = (1-cosf(t*0.2f))*0.5f;
	float scrollh, dv;

	nvg::save(vg);
//	clearState(vg);

	// Drop shadow
	shadowPaint = nvg::boxGradient(vg, x,y+4, w,h, cornerRadius*2, 20, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	nvg::beginPath(vg);
	nvg::rect(vg, x-10,y-10, w+20,h+30);
	nvg::roundedRect(vg, x,y, w,h, cornerRadius);
	nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
	nvg::fillPaint(vg, shadowPaint);
	nvg::fill(vg);

	// Window
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x,y, w,h, cornerRadius);
	nvg::moveTo(vg, x-10,y+arry);
	nvg::lineTo(vg, x+1,y+arry-11);
	nvg::lineTo(vg, x+1,y+arry+11);
	nvg::fillColor(vg, nvg::rgba(200,200,200,255));
	nvg::fill(vg);

	nvg::save(vg);
	nvg::scissor(vg, x,y,w,h);
	nvg::translate(vg, 0, -(stackh - h)*u);

	dv = 1.0f / (float)(nimages-1);

	for (i = 0; i < nimages; i++) {
		float tx, ty, v, a;
		tx = x+10;
		ty = y+10;
		tx += (i%2) * (thumb+10);
		ty += (i/2) * (thumb+10);
		nvg::imageSize(vg, images[i], imgw, imgh);
		if (imgw < imgh) {
			iw = thumb;
			ih = iw * (float)imgh/(float)imgw;
			ix = 0;
			iy = -(ih-thumb)*0.5f;
		} else {
			ih = thumb;
			iw = ih * (float)imgw/(float)imgh;
			ix = -(iw-thumb)*0.5f;
			iy = 0;
		}

		v = i * dv;
		a = clampf((u2-v) / dv, 0, 1);

		if (a < 1.0f)
			drawSpinner(vg, tx+thumb/2,ty+thumb/2, thumb*0.25f, t);

		imgPaint = nvg::imagePattern(vg, tx+ix, ty+iy, iw,ih, 0.0f/180.0f*M_PI, images[i], a);
		nvg::beginPath(vg);
		nvg::roundedRect(vg, tx,ty, thumb,thumb, 5);
		nvg::fillPaint(vg, imgPaint);
		nvg::fill(vg);

		shadowPaint = nvg::boxGradient(vg, tx-1,ty, thumb+2,thumb+2, 5, 3, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
		nvg::beginPath(vg);
		nvg::rect(vg, tx-5,ty-5, thumb+10,thumb+10);
		nvg::roundedRect(vg, tx,ty, thumb,thumb, 6);
		nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
		nvg::fillPaint(vg, shadowPaint);
		nvg::fill(vg);

		nvg::beginPath(vg);
		nvg::roundedRect(vg, tx+0.5f,ty+0.5f, thumb-1,thumb-1, 4-0.5f);
		nvg::strokeWidth(vg,1.0f);
		nvg::strokeColor(vg, nvg::rgba(255,255,255,192));
		nvg::stroke(vg);
	}
	nvg::restore(vg);

	// Hide fades
	fadePaint = nvg::linearGradient(vg, x,y,x,y+6, nvg::rgba(200,200,200,255), nvg::rgba(200,200,200,0));
	nvg::beginPath(vg);
	nvg::rect(vg, x+4,y,w-8,6);
	nvg::fillPaint(vg, fadePaint);
	nvg::fill(vg);

	fadePaint = nvg::linearGradient(vg, x,y+h,x,y+h-6, nvg::rgba(200,200,200,255), nvg::rgba(200,200,200,0));
	nvg::beginPath(vg);
	nvg::rect(vg, x+4,y+h-6,w-8,6);
	nvg::fillPaint(vg, fadePaint);
	nvg::fill(vg);

	// Scroll bar
	shadowPaint = nvg::boxGradient(vg, x+w-12+1,y+4+1, 8,h-8, 3,4, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,92));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+w-12,y+4, 8,h-8, 3);
	nvg::fillPaint(vg, shadowPaint);
//	nvg::fillColor(vg, nvg::rgba(255,0,0,128));
	nvg::fill(vg);

	scrollh = (h/stackh) * (h-8);
	shadowPaint = nvg::boxGradient(vg, x+w-12-1,y+4+(h-8-scrollh)*u-1, 8,scrollh, 3,4, nvg::rgba(220,220,220,255), nvg::rgba(128,128,128,255));
	nvg::beginPath(vg);
	nvg::roundedRect(vg, x+w-12+1,y+4+1 + (h-8-scrollh)*u, 8-2,scrollh-2, 2);
	nvg::fillPaint(vg, shadowPaint);
//	nvg::fillColor(vg, nvg::rgba(0,0,0,128));
	nvg::fill(vg);

	nvg::restore(vg);
}

void drawColorwheel(nvg::Context& vg, float x, float y, float w, float h, float t)
{
	int i;
	float r0, r1, ax,ay, bx,by, cx,cy, aeps, r;
	float hue = sinf(t * 0.12f);
	nvg::Paint paint;

	nvg::save(vg);

/*	nvg::beginPath(vg);
	nvg::rect(vg, x,y,w,h);
	nvg::fillColor(vg, nvg::rgba(255,0,0,128));
	nvg::fill(vg);*/

	cx = x + w*0.5f;
	cy = y + h*0.5f;
	r1 = (w < h ? w : h) * 0.5f - 5.0f;
	r0 = r1 - 20.0f;
	aeps = 0.5f / r1;	// half a pixel arc length in radians (2pi cancels out).
	const float pi = (float)M_PI;
	const float twoPi = pi * 2.0f;

	for (i = 0; i < 6; i++) {
		float a0 = (float)i / 6.0f * twoPi - aeps;
		float a1 = (float)(i+1.0f) / 6.0f * twoPi + aeps;
		nvg::beginPath(vg);
		nvg::arc(vg, cx,cy, r0, a0, a1, static_cast<int>(nvg::Winding::CW));
		nvg::arc(vg, cx,cy, r1, a1, a0, static_cast<int>(nvg::Winding::CCW));
		nvg::closePath(vg);
		ax = cx + cosf(a0) * (r0+r1)*0.5f;
		ay = cy + sinf(a0) * (r0+r1)*0.5f;
		bx = cx + cosf(a1) * (r0+r1)*0.5f;
		by = cy + sinf(a1) * (r0+r1)*0.5f;
		paint = nvg::linearGradient(vg, ax,ay, bx,by, nvg::hsla(a0/twoPi,1.0f,0.55f,255), nvg::hsla(a1/twoPi,1.0f,0.55f,255));
		nvg::fillPaint(vg, paint);
		nvg::fill(vg);
	}

	nvg::beginPath(vg);
	nvg::circle(vg, cx,cy, r0-0.5f);
	nvg::circle(vg, cx,cy, r1+0.5f);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,64));
	nvg::strokeWidth(vg, 1.0f);
	nvg::stroke(vg);

	// Selector
	nvg::save(vg);
	nvg::translate(vg, cx,cy);
	nvg::rotate(vg, hue*twoPi);

	// Marker on
	nvg::strokeWidth(vg, 2.0f);
	nvg::beginPath(vg);
	nvg::rect(vg, r0-1,-3,r1-r0+2,6);
	nvg::strokeColor(vg, nvg::rgba(255,255,255,192));
	nvg::stroke(vg);

	paint = nvg::boxGradient(vg, r0-3,-5,r1-r0+6,10, 2,4, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	nvg::beginPath(vg);
	nvg::rect(vg, r0-2-10,-4-10,r1-r0+4+20,8+20);
	nvg::rect(vg, r0-2,-4,r1-r0+4,8);
	nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
	nvg::fillPaint(vg, paint);
	nvg::fill(vg);

	// Center triangle
	r = r0 - 6;
	ax = cosf(120.0f/180.0f*pi) * r;
	ay = sinf(120.0f/180.0f*pi) * r;
	bx = cosf(-120.0f/180.0f*pi) * r;
	by = sinf(-120.0f/180.0f*pi) * r;
	nvg::beginPath(vg);
	nvg::moveTo(vg, r,0);
	nvg::lineTo(vg, ax,ay);
	nvg::lineTo(vg, bx,by);
	nvg::closePath(vg);
	paint = nvg::linearGradient(vg, r,0, ax,ay, nvg::hsla(hue,1.0f,0.5f,255), nvg::rgba(255,255,255,255));
	nvg::fillPaint(vg, paint);
	nvg::fill(vg);
	paint = nvg::linearGradient(vg, (r+ax)*0.5f,(0+ay)*0.5f, bx,by, nvg::rgba(0,0,0,0), nvg::rgba(0,0,0,255));
	nvg::fillPaint(vg, paint);
	nvg::fill(vg);
	nvg::strokeColor(vg, nvg::rgba(0,0,0,64));
	nvg::stroke(vg);

	// Select circle on triangle
	ax = cosf(120.0f/180.0f*pi) * r*0.3f;
	ay = sinf(120.0f/180.0f*pi) * r*0.4f;
	nvg::strokeWidth(vg, 2.0f);
	nvg::beginPath(vg);
	nvg::circle(vg, ax,ay,5);
	nvg::strokeColor(vg, nvg::rgba(255,255,255,192));
	nvg::stroke(vg);

	paint = nvg::radialGradient(vg, ax,ay, 7,9, nvg::rgba(0,0,0,64), nvg::rgba(0,0,0,0));
	nvg::beginPath(vg);
	nvg::rect(vg, ax-20,ay-20,40,40);
	nvg::circle(vg, ax,ay,7);
	nvg::pathWinding(vg, static_cast<int>(nvg::Solidity::Hole));
	nvg::fillPaint(vg, paint);
	nvg::fill(vg);

	nvg::restore(vg);

	// Render hue label
	const float tw = 50;
	const float th = 25;
	r1 += 0.5f*sqrt(tw*tw+th*th);
	nvg::beginPath(vg);
	nvg::fillColor(vg, nvg::rgb(32,32,32));
	ax = cx + r1*cosf(hue*twoPi);
	ay = cy + r1*sinf(hue*twoPi);
	nvg::roundedRect(vg, ax - tw*0.5f, ay -th*0.5f, tw, th,5.0f);
	nvg::fill(vg);

	nvg::save(vg);
	nvg::translate(vg, ax, ay);
	nvg::scale(vg, 2.0f, 2.0f); // Check that local transforms work with text
	nvg::fontSize(vg, 0.5f * th);
	nvg::textAlign(vg, static_cast<int>(nvg::Align::Center | nvg::Align::Top));
	nvg::fontFace(vg, "sans");
	nvg::fillColor(vg, nvg::rgb(255,255,255));
	std::array<char, 128> str{};
	sprintf(str.data(), "%d%%", (int)(100.0f * (hue + 1.0f)) % 100);
	nvg::beginPath(vg);
	nvg::text(vg, 0.0f, -0.2f * th, str.data(), 0);
	nvg::fill(vg);

	nvg::restore(vg);
	nvg::restore(vg);
}

void drawStylizedLines(nvg::Context& vg, float x, float y, float w, float h){
	nvg::lineJoin(vg, static_cast<int>(nvg::LineCap::Round));
	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Dashed));
	nvg::strokeColor(vg,nvg::rgbaf(0.6f,0.6f,1.0f,1.0f));
	nvg::strokeWidth(vg, 5.0f);
	nvg::beginPath(vg);
	nvg::rect(vg, x, y, w, h);
	nvg::stroke(vg);
	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Solid));
}

void drawLines(nvg::Context& vg, float x, float y, float w, float h, float lineWidth, nvg::Color color, float t)
{
	int i, j;
	float pad = 5.0f, s = w/9.0f - pad*2;
	std::array<float, 8> pts{};
	float fx, fy;
	const std::array<int, 3> joins = {static_cast<int>(nvg::LineCap::Miter), static_cast<int>(nvg::LineCap::Round), static_cast<int>(nvg::LineCap::Bevel)};
	const std::array<int, 3> caps = {static_cast<int>(nvg::LineCap::Butt), static_cast<int>(nvg::LineCap::Round), static_cast<int>(nvg::LineCap::Square)};
	UNUSED(h);

	nvg::save(vg);
	pts[0] = -s*0.25f + cosf(t*0.3f) * s*0.5f;
	pts[1] = sinf(t*0.3f) * s*0.5f;
	pts[2] = -s*0.25f;
	pts[3] = 0;
	pts[4] = s*0.25f;
	pts[5] = 0;
	pts[6] = s*0.25f + cosf(-t*0.3f) * s*0.5f;
	pts[7] = sinf(-t*0.3f) * s*0.5f;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			fx = x + s*0.5f + (i*3+j)/9.0f*w + pad;
			fy = y - s*0.5f + pad;

			nvg::lineCap(vg, caps[i]);
			nvg::lineJoin(vg, joins[j]);

			nvg::strokeWidth(vg, lineWidth);
			nvg::strokeColor(vg, color);
			nvg::beginPath(vg);
			nvg::moveTo(vg, fx+pts[0], fy+pts[1]);
			nvg::lineTo(vg, fx+pts[2], fy+pts[3]);
			nvg::lineTo(vg, fx+pts[4], fy+pts[5]);
			nvg::lineTo(vg, fx+pts[6], fy+pts[7]);
			nvg::stroke(vg);

			nvg::lineCap(vg, static_cast<int>(nvg::LineCap::Butt));
			nvg::lineJoin(vg, static_cast<int>(nvg::LineCap::Bevel));

			nvg::strokeWidth(vg, 1.0f);
			nvg::strokeColor(vg, nvg::rgba(0,192,255,255));
			nvg::beginPath(vg);
			nvg::moveTo(vg, fx+pts[0], fy+pts[1]);
			nvg::lineTo(vg, fx+pts[2], fy+pts[3]);
			nvg::lineTo(vg, fx+pts[4], fy+pts[5]);
			nvg::lineTo(vg, fx+pts[6], fy+pts[7]);
			nvg::stroke(vg);
		}
	}


	nvg::restore(vg);
}

int loadDemoData(nvg::Context& vg, DemoData* data)
{
	int i;

	for (i = 0; i < 12; i++) {
		std::array<char, 128> file{};
		snprintf(file.data(), (int)file.size(), "../example/images/image%d.jpg", i+1);
		data->images[i] = nvg::createImage(vg, file.data(), 0);
		if (data->images[i] == 0) {
			printf("Could not load %s.\n", file.data());
			return -1;
		}
	}

	data->fontIcons = nvg::createFont(vg, "icons", "../example/entypo.ttf");
	if (data->fontIcons == -1) {
		printf("Could not add font icons.\n");
		return -1;
	}
	data->fontNormal = nvg::createFont(vg, "sans", "../example/Roboto-Regular.ttf");
	if (data->fontNormal == -1) {
		printf("Could not add font italic.\n");
		return -1;
	}
	data->fontBold = nvg::createFont(vg, "sans-bold", "../example/Roboto-Bold.ttf");
	if (data->fontBold == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}
	data->fontEmoji = nvg::createFont(vg, "emoji", "../example/NotoEmoji-Regular.ttf");
	if (data->fontEmoji == -1) {
		printf("Could not add font emoji.\n");
		return -1;
	}
	nvg::addFallbackFontId(vg, data->fontNormal, data->fontEmoji);
	nvg::addFallbackFontId(vg, data->fontBold, data->fontEmoji);

	return 0;
}

void freeDemoData(nvg::Context& vg, DemoData* data)
{
	int i;

	for (i = 0; i < 12; i++)
		nvg::deleteImage(vg, data->images[i]);
}

void drawParagraph(nvg::Context& vg, float x, float y, float width, float height, float mx, float my)
{
	std::array<nvg::TextRow, 3> rows{};
	std::array<nvg::GlyphPosition, 100> glyphs{};
	const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.🎉";
	const char* start;
	const char* end;
	int nrows, i, nglyphs, j, lnum = 0;
	float lineh;
	float caretx, px;
	std::array<float, 4> bounds{};
	float a;
	const char* hoverText = "Hover your mouse over the text to see calculated caret position.";
	float gx,gy;
	int gutter = 0;
	UNUSED(height);

	nvg::save(vg);

	nvg::fontSize(vg, 15.0f);
	nvg::fontFace(vg, "sans");
	nvg::textAlign(vg, static_cast<int>(nvg::Align::Left | nvg::Align::Top));
	nvg::textMetrics(vg, NULL, NULL, &lineh);

	// The text break API can be used to fill a large buffer of rows,
	// or to iterate over the text just few lines (or just one) at a time.
	// The "next" variable of the last returned item tells where to continue.
	start = text;
	end = text + strlen(text);
	while ((nrows = nvg::textBreakLines(vg, start, end, width, rows.data(), (int)rows.size(), 0))) {
		for (i = 0; i < nrows; i++) {
			nvg::TextRow* row = &rows[(size_t)i];
			int hit = mx > x && mx < (x+width) && my >= y && my < (y+lineh);

			nvg::beginPath(vg);
			nvg::fillColor(vg, nvg::rgba(255,255,255,hit?64:16));
			nvg::rect(vg, x + row->minx, y, row->maxx - row->minx, lineh);
			nvg::fill(vg);

			nvg::fillColor(vg, nvg::rgba(255,255,255,255));
			nvg::text(vg, x, y, row->start, row->end);

			if (hit) {
				caretx = (mx < x+row->width/2) ? x : x+row->width;
				px = x;
				nglyphs = nvg::textGlyphPositions(vg, x, y, row->start, row->end, glyphs.data(), (int)glyphs.size());
				for (j = 0; j < nglyphs; j++) {
					float x0 = glyphs[(size_t)j].x;
					float x1 = (j+1 < nglyphs) ? glyphs[(size_t)j+1].x : x+row->width;
					float gx = x0 * 0.3f + x1 * 0.7f;
					if (mx >= px && mx < gx)
						caretx = glyphs[(size_t)j].x;
					px = gx;
				}
				nvg::beginPath(vg);
				nvg::fillColor(vg, nvg::rgba(255,192,0,255));
				nvg::rect(vg, caretx, y, 1, lineh);
				nvg::fill(vg);

				gutter = lnum+1;
				gx = x - 10;
				gy = y + lineh/2;
			}
			lnum++;
			y += lineh;
		}
		// Keep going...
		start = rows[(size_t)nrows-1].next;
	}

	if (gutter) {
		std::array<char, 16> txt{};
		snprintf(txt.data(), (int)txt.size(), "%d", gutter);
		nvg::fontSize(vg, 12.0f);
		nvg::textAlign(vg, static_cast<int>(nvg::Align::Right | nvg::Align::Middle));

		nvg::textBounds(vg, gx,gy, txt.data(), NULL, bounds.data());

		nvg::beginPath(vg);
		nvg::fillColor(vg, nvg::rgba(255,192,0,255));
		nvg::roundedRect(
			vg,
			(float)((int)bounds[0] - 4), (float)((int)bounds[1] - 2),
			(float)((int)(bounds[2]-bounds[0]) + 8), (float)((int)(bounds[3]-bounds[1]) + 4),
			(float)(((int)(bounds[3]-bounds[1]) + 4)/2 - 1)
		);
		nvg::fill(vg);

		nvg::fillColor(vg, nvg::rgba(32,32,32,255));
		nvg::text(vg, gx,gy, txt.data(), NULL);
	}

	y += 20.0f;

	nvg::fontSize(vg, 11.0f);
	nvg::textAlign(vg, static_cast<int>(nvg::Align::Left | nvg::Align::Top));
	nvg::textLineHeight(vg, 1.2f);

	nvg::textBoxBounds(vg, x,y, 150, hoverText, NULL, bounds.data());

	// Fade the tooltip out when close to it.
	gx = clampf(mx, bounds[0], bounds[2]) - mx;
	gy = clampf(my, bounds[1], bounds[3]) - my;
	a = sqrtf(gx*gx + gy*gy) / 30.0f;
	a = clampf(a, 0, 1);
	nvg::globalAlpha(vg, a);

	nvg::beginPath(vg);
	nvg::fillColor(vg, nvg::rgba(220,220,220,255));
	nvg::roundedRect(vg, bounds[0]-2.0f, bounds[1]-2.0f, (float)((int)(bounds[2]-bounds[0]) + 4), (float)((int)(bounds[3]-bounds[1]) + 4), 3.0f);
	px = (bounds[2] + bounds[0]) * 0.5f;
	nvg::moveTo(vg, px, bounds[1] - 10.0f);
	nvg::lineTo(vg, px + 7.0f, bounds[1] + 1.0f);
	nvg::lineTo(vg, px - 7.0f, bounds[1] + 1.0f);
	nvg::fill(vg);

	nvg::fillColor(vg, nvg::rgba(0,0,0,220));
	nvg::textBox(vg, x,y, 150, hoverText, NULL);

	nvg::restore(vg);
}

void drawWidths(nvg::Context& vg, float x, float y, float width)
{
	int i;

	nvg::save(vg);

	nvg::strokeColor(vg, nvg::rgba(0,0,0,255));

	for (i = 0; i < 20; i++) {
		float w = (i+0.5f)*0.1f;
		nvg::strokeWidth(vg, w);
		nvg::beginPath(vg);
		nvg::moveTo(vg, x,y);
		nvg::lineTo(vg, x+width,y+width*0.3f);
		nvg::stroke(vg);
		y += 10;
	}

	nvg::restore(vg);
}

void drawCaps(nvg::Context& vg, float x, float y, float width)
{
	int i;
	const std::array<int, 3> caps = {static_cast<int>(nvg::LineCap::Butt), static_cast<int>(nvg::LineCap::Round), static_cast<int>(nvg::LineCap::Square)};
	float lineWidth = 8.0f;

	nvg::save(vg);

	nvg::beginPath(vg);
	nvg::rect(vg, x-lineWidth/2, y, width+lineWidth, 40);
	nvg::fillColor(vg, nvg::rgba(255,255,255,32));
	nvg::fill(vg);

	nvg::beginPath(vg);
	nvg::rect(vg, x, y, width, 40);
	nvg::fillColor(vg, nvg::rgba(255,255,255,32));
	nvg::fill(vg);

	nvg::strokeWidth(vg, lineWidth);
	for (i = 0; i < 3; i++) {
		nvg::lineCap(vg, caps[i]);
		nvg::strokeColor(vg, nvg::rgba(0,0,0,255));
		nvg::beginPath(vg);
		nvg::moveTo(vg, x, y + i*10 + 5);
		nvg::lineTo(vg, x+width, y + i*10 + 5);
		nvg::stroke(vg);
	}

	nvg::restore(vg);
}

void drawScissor(nvg::Context& vg, float x, float y, float t)
{
	nvg::save(vg);

	// Draw first rect and set scissor to it's area.
	nvg::translate(vg, x, y);
	nvg::rotate(vg, nvg::degToRad(5));
	nvg::beginPath(vg);
	nvg::rect(vg, -20,-20,60,40);
	nvg::fillColor(vg, nvg::rgba(255,0,0,255));
	nvg::fill(vg);
	nvg::scissor(vg, -20,-20,60,40);

	// Draw second rectangle with offset and rotation.
	nvg::translate(vg, 40,0);
	nvg::rotate(vg, t);

	// Draw the intended second rectangle without any scissoring.
	nvg::save(vg);
	nvg::resetScissor(vg);
	nvg::beginPath(vg);
	nvg::rect(vg, -20,-10,60,30);
	nvg::fillColor(vg, nvg::rgba(255,128,0,64));
	nvg::fill(vg);
	nvg::restore(vg);

	// Draw second rectangle with combined scissoring.
	nvg::intersectScissor(vg, -20,-10,60,30);
	nvg::beginPath(vg);
	nvg::rect(vg, -20,-10,60,30);
	nvg::fillColor(vg, nvg::rgba(255,128,0,255));
	nvg::fill(vg);

	nvg::restore(vg);
}

void drawBezierCurve(nvg::Context& vg, float x0, float y0, float radius, float t){

	const float pi = (float)M_PI;
	float x1 = x0 + radius*cosf(2.0f*pi*t/5.0f);
	float y1 = y0 + radius*sinf(2.0f*pi*t/5.0f);

	float cx0 = x0;
	float cy0 = y0 + ((y1 - y0) * 0.75f);
	float cx1 = x1;
	float cy1 = y0 + ((y1 - y0) * 0.75f);

	nvg::beginPath(vg);
	nvg::moveTo(vg, x0, y0);
	nvg::lineTo(vg, cx0, cy0);
	nvg::lineTo(vg, cx1, cy1);
	nvg::lineTo(vg, x1, y1);
	nvg::strokeColor(vg,nvg::rgba(200,200,200,255));
	nvg::strokeWidth(vg,2.0f);
	nvg::stroke(vg);

	nvg::lineCap(vg, static_cast<int>(nvg::LineCap::Round));
	nvg::strokeWidth(vg,5);
	nvg::lineJoin(vg, static_cast<int>(nvg::LineCap::Round));

	nvg::beginPath(vg);
	nvg::moveTo(vg, x0, y0);
	nvg::bezierTo(vg, cx0, cy0, cx1, cy1, x1, y1);
	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Solid));
	nvg::strokeColor(vg, nvg::rgba(40, 53, 147,255));
	nvg::stroke(vg);

	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Dashed));
	nvg::strokeColor(vg, nvg::rgba(255, 195, 0,255));
	nvg::stroke(vg);
	
	nvg::beginPath(vg);
	nvg::circle(vg,x0,y0,5.0f);
	nvg::circle(vg,cx0,cy0,5.0f);
	nvg::circle(vg,cx1,cy1,5.0f);
	nvg::circle(vg,x1,y1,5.0f);
	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Solid));
	nvg::fillColor(vg,nvg::rgba(64,192,64,255));
	nvg::fill(vg);
}

void drawScaledText(nvg::Context& vg, float x0, float y0, float t){
	nvg::save(vg);
	const float textScale = (cosf(2.0f * (float)M_PI * t * 0.25f) + 1.0f) + 0.1f;
	nvg::translate(vg, x0, y0);
	nvg::scale(vg, textScale, textScale);
	nvg::fontSize(vg, 24.0f);
	nvg::fontFace(vg, "sans-bold");
	nvg::textAlign(vg,static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	nvg::fillColor(vg, nvg::rgba(255,255,255,255));
	nvg::beginPath(vg);
	nvg::text(vg, 0.0f, 0.0f, "NanoVG", NULL);
	nvg::fill(vg);
	nvg::restore(vg);
}

void renderDemo(nvg::Context& vg, float mx, float my, float width, float height,
				float t, int blowup, DemoData* data)
{
	float x,y,popy;

	drawEyes(vg, width - 230, 30, 150, 100, mx, my, t);
	drawStylizedLines(vg, width - 245, 15, 180 ,130);
	drawFancyText(vg, width - 160, 170);
	drawParagraph(vg, width - 450, 50, 150, 100, mx, my);
	drawGraph(vg, 0, height/2, width, height/2, t);
	drawColorwheel(vg, width - 280, height - 320, 250.0f, 250.0f, t);

	// Line joints

	switch((int)(t/5.0f)%3){
		case 0:
			nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Dashed));break;
		case 1:
			nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Dotted));break;
		case 2:
			nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Glow));break;
		default:
			nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Solid));
	}
	drawLines(vg, 100, height-5, 800, 100, 10.0f, nvg::rgba(255, 153, 0, 255), t*3);

	nvg::lineStyle(vg, static_cast<int>(nvg::LineStyle::Solid));
	drawLines(vg, 120, height-75, 600, 50, 17.0f, nvg::rgba(0,0,0,160), t);

	// Line caps
	drawWidths(vg, 10, 50, 30);

	// Line caps
	drawCaps(vg, 10, 300, 30);

	drawScissor(vg, 50, height-80, t);

	nvg::save(vg);
	if (blowup) {
		nvg::rotate(vg, sinf(t*0.3f)*5.0f/180.0f*(float)M_PI);
		nvg::scale(vg, 2.0f, 2.0f);
	}

	// Widgets
	drawWindow(vg, "Widgets `n Stuff", 50, 50, 300, 400);
	x = 60; y = 95;
	drawSearchBox(vg, "Search", x,y,280,25);
	y += 40;
	drawDropDown(vg, "Effects", x,y,280,28);
	popy = y + 14;
	y += 45;

	// Form
	drawLabel(vg, "Login", x,y, 280,20);
	y += 25;
	drawEditBox(vg, "Email",  x,y, 280,28);
	y += 35;
	drawEditBox(vg, "Password", x,y, 280,28);
	y += 38;
	drawCheckBox(vg, "Remember me", x,y, 140,28);
	drawButton(vg, ICON_LOGIN, "Sign in", x+138, y, 140, 28, nvg::rgba(0,96,128,255));
	y += 45;

	// Slider
	drawLabel(vg, "Diameter", x,y, 280,20);
	y += 25;
	drawEditBoxNum(vg, "123.00", "px", x+180,y, 100,28);
	drawSlider(vg, 0.4f, x,y, 170,28);
	y += 55;

	drawButton(vg, ICON_TRASH, "Delete", x, y, 160, 28, nvg::rgba(128,16,8,255));
	drawButton(vg, 0, "Cancel", x+170, y, 110, 28, nvg::rgba(0,0,0,0));

	// Thumbnails box
	drawThumbnails(vg, 365, popy-30, 160, 300, data->images.data(), (int)data->images.size(), t);

	nvg::restore(vg);

	drawBezierCurve(vg, width - 380, height - 220, 100, t);
	drawScaledText(vg, 450.0, 50, t);
}

static int mini(int a, int b) { return a < b ? a : b; }

static void unpremultiplyAlpha(unsigned char* image, int w, int h, int stride)
{
	int x,y;

	// Unpremultiply
	for (y = 0; y < h; y++) {
		unsigned char *row = &image[y*stride];
		for (x = 0; x < w; x++) {
			int r = row[0], g = row[1], b = row[2], a = row[3];
			if (a != 0) {
				row[0] = (int)mini(r*255/a, 255);
				row[1] = (int)mini(g*255/a, 255);
				row[2] = (int)mini(b*255/a, 255);
			}
			row += 4;
		}
	}

	// Defringe
	for (y = 0; y < h; y++) {
		unsigned char *row = &image[y*stride];
		for (x = 0; x < w; x++) {
			int r = 0, g = 0, b = 0, a = row[3], n = 0;
			if (a == 0) {
				if (x-1 > 0 && row[-1] != 0) {
					r += row[-4];
					g += row[-3];
					b += row[-2];
					n++;
				}
				if (x+1 < w && row[7] != 0) {
					r += row[4];
					g += row[5];
					b += row[6];
					n++;
				}
				if (y-1 > 0 && row[-stride+3] != 0) {
					r += row[-stride];
					g += row[-stride+1];
					b += row[-stride+2];
					n++;
				}
				if (y+1 < h && row[stride+3] != 0) {
					r += row[stride];
					g += row[stride+1];
					b += row[stride+2];
					n++;
				}
				if (n > 0) {
					row[0] = r/n;
					row[1] = g/n;
					row[2] = b/n;
				}
			}
			row += 4;
		}
	}
}

static void setAlpha(unsigned char* image, int w, int h, int stride, unsigned char a)
{
	int x, y;
	for (y = 0; y < h; y++) {
		unsigned char* row = &image[y*stride];
		for (x = 0; x < w; x++)
			row[x*4+3] = a;
	}
}

static void flipHorizontal(unsigned char* image, int w, int h, int stride)
{
	int i = 0, j = h-1, k;
	while (i < j) {
		unsigned char* ri = &image[i * stride];
		unsigned char* rj = &image[j * stride];
		for (k = 0; k < w*4; k++) {
			unsigned char t = ri[k];
			ri[k] = rj[k];
			rj[k] = t;
		}
		i++;
		j--;
	}
}

bool comparePreviousScreenShot(int nw, int nh, int premult, unsigned char* nimg, const char* name)
{
	int w, h, n;
	unsigned char* img;
	stbi_set_unpremultiply_on_load(premult);
	stbi_convert_iphone_png_to_rgb(1);
	img = stbi_load(name, &w, &h, &n, 4);
	if (img == NULL) {
		printf("Skipped screenshot comparison since there is no previous screenshot to compare to.\n");
		return true;
	}

	if (w!=nw||h!=nh||n!=4){
		printf("Screenshot does not match previous screenshot size %d x %d vs %d x %d.\n", w, h, nw, nh);
		return false;
	}

	size_t size= w*h*4;
	const int Threshold= 2;
	unsigned char* diff = (unsigned char*)malloc(size);
	bool ret=true;
	for(size_t idx=0;idx<size;idx+=4){
			int rd=abs(img[idx] - nimg[idx]);
			int gd=abs(img[idx+1] - nimg[idx+1]);
			int bd=abs(img[idx+2] - nimg[idx+2]);
			int ad=abs(img[idx+3] - nimg[idx+3]);
			diff[idx]=std::min(rd*4,255);
			diff[idx+1]=std::min(gd*4,255);
			diff[idx+2]=std::min(bd*4,255);
			diff[idx+3]=255;
			if(rd > Threshold || gd > Threshold || bd > Threshold || ad > Threshold){
				ret=false;
			}
	}
	stbi_image_free(img);	

	if(!ret){
		std::string fileName=name;
		auto pos= fileName.find_last_of(".");
		fileName=fileName.substr(0, pos)+"-diff"+fileName.substr(pos);
		printf("Saving screenshot diff to '%s'\n", fileName.c_str());
		stbi_write_png(fileName.c_str(), w, h, 4, diff, w*4);
	}
	return ret;
}

bool saveScreenShot(int w, int h, int premult, const char* name, bool compare)
{
	bool ret = true;
	unsigned char* image = (unsigned char*)malloc(w*h*4);
	if (image == NULL)
		return false;

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);
	if (premult)
		unpremultiplyAlpha(image, w, h, w*4);
	else
		setAlpha(image, w, h, w*4, 255);
	flipHorizontal(image, w, h, w*4);
	
	std::string fileName= name;	
	if(compare) {
		ret = comparePreviousScreenShot(w, h, premult, image, name);
		if(!ret){ 
					auto pos= fileName.find_last_of(".");
				fileName=fileName.substr(0, pos)+"-failed"+fileName.substr(pos);
		}
	}

	printf("Saving screenshot to '%s'\n", fileName.c_str());
	stbi_write_png(fileName.c_str(), w, h, 4, image, w*4);
 	free(image);
	return ret;
}
