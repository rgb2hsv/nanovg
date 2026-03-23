#include "demo.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef NANOVG_GLEW
#  include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#include "nanovg.hpp"
using namespace nvg;
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


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
int isBlack(Color col)
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


void drawWindow(Context* vg, const char* title, float x, float y, float w, float h)
{
	float cornerRadius = 3.0f;
	Paint shadowPaint;
	Paint headerPaint;

	save(vg);
//	clearState(vg);

	// Window
	beginPath(vg);
	roundedRect(vg, x,y, w,h, cornerRadius);
	fillColor(vg, rgba(28,30,34,192));
//	fillColor(vg, rgba(0,0,0,128));
	fill(vg);

	// Drop shadow
	shadowPaint = boxGradient(vg, x,y+2, w,h, cornerRadius*2, 10, rgba(0,0,0,128), rgba(0,0,0,0));
	beginPath(vg);
	rect(vg, x-10,y-10, w+20,h+30);
	roundedRect(vg, x,y, w,h, cornerRadius);
	pathWinding(vg, static_cast<int>(Solidity::Hole));
	fillPaint(vg, shadowPaint);
	fill(vg);

	// Header
	headerPaint = linearGradient(vg, x,y,x,y+15, rgba(255,255,255,8), rgba(0,0,0,16));
	beginPath(vg);
	roundedRect(vg, x+1,y+1, w-2,30, cornerRadius-1);
	fillPaint(vg, headerPaint);
	fill(vg);
	beginPath(vg);
	moveTo(vg, x+0.5f, y+0.5f+30);
	lineTo(vg, x+0.5f+w-1, y+0.5f+30);
	strokeColor(vg, rgba(0,0,0,32));
	stroke(vg);

	fontSize(vg, 15.0f);
	fontFace(vg, "sans-bold");
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));

	fontBlur(vg,2);
	fillColor(vg, rgba(0,0,0,128));
	::text(vg, x+w/2,y+16+1, title, NULL);

	fontBlur(vg,0);
	fillColor(vg, rgba(220,220,220,160));
	::text(vg, x+w/2,y+16, title, NULL);

	restore(vg);
}

void drawSearchBox(Context* vg, const char* text, float x, float y, float w, float h)
{
	Paint bg;
	char icon[8];
	float cornerRadius = h/2-1;

	// Edit
	bg = boxGradient(vg, x,y+1.5f, w,h, h/2,5, rgba(0,0,0,16), rgba(0,0,0,92));
	beginPath(vg);
	roundedRect(vg, x,y, w,h, cornerRadius);
	fillPaint(vg, bg);
	fill(vg);

/*	beginPath(vg);
	roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	strokeColor(vg, rgba(0,0,0,48));
	stroke(vg);*/

	fontSize(vg, h*1.3f);
	fontFace(vg, "icons");
	fillColor(vg, rgba(255,255,255,64));
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));
	::text(vg, x+h*0.55f, y+h*0.55f, cpToUTF8(ICON_SEARCH,icon), NULL);

	fontSize(vg, 17.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,32));

	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	::text(vg, x+h*1.05f,y+h*0.5f,text, NULL);

	fontSize(vg, h*1.3f);
	fontFace(vg, "icons");
	fillColor(vg, rgba(255,255,255,32));
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));
	::text(vg, x+w-h*0.55f, y+h*0.55f, cpToUTF8(ICON_CIRCLED_CROSS,icon), NULL);
}

void drawDropDown(Context* vg, const char* text, float x, float y, float w, float h)
{
	Paint bg;
	char icon[8];
	float cornerRadius = 4.0f;

	bg = linearGradient(vg, x,y,x,y+h, rgba(255,255,255,16), rgba(0,0,0,16));
	beginPath(vg);
	roundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
	fillPaint(vg, bg);
	fill(vg);

	beginPath(vg);
	roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	strokeColor(vg, rgba(0,0,0,48));
	stroke(vg);

	fontSize(vg, 17.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,160));
	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	::text(vg, x+h*0.3f,y+h*0.5f,text, NULL);

	fontSize(vg, h*1.3f);
	fontFace(vg, "icons");
	fillColor(vg, rgba(255,255,255,64));
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));
	::text(vg, x+w-h*0.5f, y+h*0.5f, cpToUTF8(ICON_CHEVRON_RIGHT,icon), NULL);
}

void drawLabel(Context* vg, const char* text, float x, float y, float w, float h)
{
	UNUSED(w);

	fontSize(vg, 15.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,128));

	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	::text(vg, x,y+h*0.5f,text, NULL);
}

void drawEditBoxBase(Context* vg, float x, float y, float w, float h)
{
	Paint bg;
	// Edit
	bg = boxGradient(vg, x+1,y+1+1.5f, w-2,h-2, 3,4, rgba(255,255,255,32), rgba(32,32,32,32));
	beginPath(vg);
	roundedRect(vg, x+1,y+1, w-2,h-2, 4-1);
	fillPaint(vg, bg);
	fill(vg);

	beginPath(vg);
	roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, 4-0.5f);
	strokeColor(vg, rgba(0,0,0,48));
	stroke(vg);
}

void drawEditBox(Context* vg, const char* text, float x, float y, float w, float h)
{

	drawEditBoxBase(vg, x,y, w,h);

	fontSize(vg, 17.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,64));
	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	::text(vg, x+h*0.3f,y+h*0.5f,text, NULL);
}

void drawEditBoxNum(Context* vg,
					const char* text, const char* units, float x, float y, float w, float h)
{
	float uw;

	drawEditBoxBase(vg, x,y, w,h);

	uw = textBounds(vg, 0,0, units, NULL, NULL);

	fontSize(vg, 15.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,64));
	textAlign(vg,static_cast<int>(Align::Right | Align::Middle));
	::text(vg, x+w-h*0.3f,y+h*0.5f,units, NULL);

	fontSize(vg, 17.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,128));
	textAlign(vg,static_cast<int>(Align::Right | Align::Middle));
	::text(vg, x+w-uw-h*0.5f,y+h*0.5f,text, NULL);
}

void drawCheckBox(Context* vg, const char* text, float x, float y, float w, float h)
{
	Paint bg;
	char icon[8];
	UNUSED(w);

	fontSize(vg, 15.0f);
	fontFace(vg, "sans");
	fillColor(vg, rgba(255,255,255,160));

	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	::text(vg, x+28,y+h*0.5f,text, NULL);

	bg = boxGradient(vg, x+1,y+(int)(h*0.5f)-9+1, 18,18, 3,3, rgba(0,0,0,32), rgba(0,0,0,92));
	beginPath(vg);
	roundedRect(vg, x+1,y+(int)(h*0.5f)-9, 18,18, 3);
	fillPaint(vg, bg);
	fill(vg);

	fontSize(vg, 33);
	fontFace(vg, "icons");
	fillColor(vg, rgba(255,255,255,128));
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));
	::text(vg, x+9+2, y+h*0.5f, cpToUTF8(ICON_CHECK,icon), NULL);
}

void drawButton(Context* vg, int preicon, const char* text, float x, float y, float w, float h, Color col)
{
	Paint bg;
	char icon[8];
	float cornerRadius = 4.0f;
	float tw = 0, iw = 0;

	bg = linearGradient(vg, x,y,x,y+h, rgba(255,255,255,isBlack(col)?16:32), rgba(0,0,0,isBlack(col)?16:32));
	beginPath(vg);
	roundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
	if (!isBlack(col)) {
		fillColor(vg, col);
		fill(vg);
	}
	fillPaint(vg, bg);
	fill(vg);

	beginPath(vg);
	roundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	strokeColor(vg, rgba(0,0,0,48));
	stroke(vg);

	fontSize(vg, 17.0f);
	fontFace(vg, "sans-bold");
	tw = textBounds(vg, 0,0, text, NULL, NULL);
	if (preicon != 0) {
		fontSize(vg, h*1.3f);
		fontFace(vg, "icons");
		iw = textBounds(vg, 0,0, cpToUTF8(preicon,icon), NULL, NULL);
		iw += h*0.15f;
	}

	if (preicon != 0) {
		fontSize(vg, h*1.3f);
		fontFace(vg, "icons");
		fillColor(vg, rgba(255,255,255,96));
		textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
		::text(vg, x+w*0.5f-tw*0.5f-iw*0.75f, y+h*0.5f, cpToUTF8(preicon,icon), NULL);
	}

	fontSize(vg, 17.0f);
	fontFace(vg, "sans-bold");
	textAlign(vg,static_cast<int>(Align::Left | Align::Middle));
	fillColor(vg, rgba(0,0,0,160));
	::text(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f-1,text, NULL);
	fillColor(vg, rgba(255,255,255,160));
	::text(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f,text, NULL);
}

void drawSlider(Context* vg, float pos, float x, float y, float w, float h)
{
	Paint bg, knob;
	float cy = y+(int)(h*0.5f);
	float kr = (int)(h*0.25f);

	save(vg);
//	clearState(vg);

	// Slot
	bg = boxGradient(vg, x,cy-2+1, w,4, 2,2, rgba(0,0,0,32), rgba(0,0,0,128));
	beginPath(vg);
	roundedRect(vg, x,cy-2, w,4, 2);
	fillPaint(vg, bg);
	fill(vg);

	// Knob Shadow
	bg = radialGradient(vg, x+(int)(pos*w),cy+1, kr-3,kr+3, rgba(0,0,0,64), rgba(0,0,0,0));
	beginPath(vg);
	rect(vg, x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
	circle(vg, x+(int)(pos*w),cy, kr);
	pathWinding(vg, static_cast<int>(Solidity::Hole));
	fillPaint(vg, bg);
	fill(vg);

	// Knob
	knob = linearGradient(vg, x,cy-kr,x,cy+kr, rgba(255,255,255,16), rgba(0,0,0,16));
	beginPath(vg);
	circle(vg, x+(int)(pos*w),cy, kr-1);
	fillColor(vg, rgba(40,43,48,255));
	fill(vg);
	fillPaint(vg, knob);
	fill(vg);

	beginPath(vg);
	circle(vg, x+(int)(pos*w),cy, kr-0.5f);
	strokeColor(vg, rgba(0,0,0,92));
	stroke(vg);

	restore(vg);
}

void drawFancyText(Context* vg, float x, float y){
	fontSize(vg, 30.0f);
	fontFace(vg, "sans-bold");
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));

	fontBlur(vg, 10);
	fillColor(vg, rgb(255, 0, 102));
	::text(vg, x, y, "Font Blur", NULL);
	fontBlur(vg,0);
	fillColor(vg, rgb(255,255,255));
	::text(vg, x, y, "Font Blur", NULL);

	fontDilate(vg, 2);
	fillColor(vg, rgb(255, 0, 102));
	::text(vg, x, y+40, "Font Outline", NULL);
	fontDilate(vg,0);
	fillColor(vg, rgb(255,255,255));
	::text(vg, x, y+40, "Font Outline", NULL);

	fontDilate(vg, 3); // Dilate will always be applied before blur
	fontBlur(vg, 2);
	fillColor(vg, rgb(255, 0, 102));
	::text(vg, x, y+80, "Font Blur Outline", NULL);
	fontDilate(vg,0);
	fontBlur(vg,0);
	fillColor(vg, rgb(255,255,255));
	::text(vg, x, y+80, "Font Blur Outline", NULL);
}

void drawEyes(Context* vg, float x, float y, float w, float h, float mx, float my, float t)
{
	Paint gloss, bg;
	float ex = w *0.23f;
	float ey = h * 0.5f;
	float lx = x + ex;
	float ly = y + ey;
	float rx = x + w - ex;
	float ry = y + ey;
	float dx,dy,d;
	float br = (ex < ey ? ex : ey) * 0.5f;
	float blink = 1 - pow(sinf(t*0.5f),200)*0.8f;

	bg = linearGradient(vg, x,y+h*0.5f,x+w*0.1f,y+h, rgba(0,0,0,32), rgba(0,0,0,16));
	beginPath(vg);
	ellipse(vg, lx+3.0f,ly+16.0f, ex,ey);
	ellipse(vg, rx+3.0f,ry+16.0f, ex,ey);
	fillPaint(vg, bg);
	fill(vg);

	bg = linearGradient(vg, x,y+h*0.25f,x+w*0.1f,y+h, rgba(220,220,220,255), rgba(128,128,128,255));
	beginPath(vg);
	ellipse(vg, lx,ly, ex,ey);
	ellipse(vg, rx,ry, ex,ey);
	fillPaint(vg, bg);
	fill(vg);

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	beginPath(vg);
	ellipse(vg, lx+dx,ly+dy+ey*0.25f*(1-blink), br,br*blink);
	fillColor(vg, rgba(32,32,32,255));
	fill(vg);

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	beginPath(vg);
	ellipse(vg, rx+dx,ry+dy+ey*0.25f*(1-blink), br,br*blink);
	fillColor(vg, rgba(32,32,32,255));
	fill(vg);

	gloss = radialGradient(vg, lx-ex*0.25f,ly-ey*0.5f, ex*0.1f,ex*0.75f, rgba(255,255,255,128), rgba(255,255,255,0));
	beginPath(vg);
	ellipse(vg, lx,ly, ex,ey);
	fillPaint(vg, gloss);
	fill(vg);

	gloss = radialGradient(vg, rx-ex*0.25f,ry-ey*0.5f, ex*0.1f,ex*0.75f, rgba(255,255,255,128), rgba(255,255,255,0));
	beginPath(vg);
	ellipse(vg, rx,ry, ex,ey);
	fillPaint(vg, gloss);
	fill(vg);
}

void drawGraph(Context* vg, float x, float y, float w, float h, float t)
{
	Paint bg;
	float samples[6];
	float sx[6], sy[6];
	float dx = w/5.0f;
	int i;

	samples[0] = (1+sinf(t*1.2345f+cosf(t*0.33457f)*0.44f))*0.5f;
	samples[1] = (1+sinf(t*0.68363f+cosf(t*1.3f)*1.55f))*0.5f;
	samples[2] = (1+sinf(t*1.1642f+cosf(t*0.33457)*1.24f))*0.5f;
	samples[3] = (1+sinf(t*0.56345f+cosf(t*1.63f)*0.14f))*0.5f;
	samples[4] = (1+sinf(t*1.6245f+cosf(t*0.254f)*0.3f))*0.5f;
	samples[5] = (1+sinf(t*0.345f+cosf(t*0.03f)*0.6f))*0.5f;

	for (i = 0; i < 6; i++) {
		sx[i] = x+i*dx;
		sy[i] = y+h*samples[i]*0.8f;
	}

	// Graph background
	bg = linearGradient(vg, x,y,x,y+h, rgba(0,160,192,0), rgba(0,160,192,64));
	beginPath(vg);
	moveTo(vg, sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	lineTo(vg, x+w, y+h);
	lineTo(vg, x, y+h);
	fillPaint(vg, bg);
	fill(vg);

	// Graph line
	beginPath(vg);
	moveTo(vg, sx[0], sy[0]+2);
	for (i = 1; i < 6; i++)
		bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1]+2, sx[i]-dx*0.5f,sy[i]+2, sx[i],sy[i]+2);
	strokeColor(vg, rgba(0,0,0,32));
	strokeWidth(vg, 3.0f);
	stroke(vg);

	beginPath(vg);
	moveTo(vg, sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		bezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	strokeColor(vg, rgba(0,160,192,255));
	strokeWidth(vg, 3.0f);
	stroke(vg);

	// Graph sample pos
	for (i = 0; i < 6; i++) {
		bg = radialGradient(vg, sx[i],sy[i]+2, 3.0f,8.0f, rgba(0,0,0,32), rgba(0,0,0,0));
		beginPath(vg);
		rect(vg, sx[i]-10, sy[i]-10+2, 20,20);
		fillPaint(vg, bg);
		fill(vg);
	}

	beginPath(vg);
	for (i = 0; i < 6; i++)
		circle(vg, sx[i], sy[i], 4.0f);
	fillColor(vg, rgba(0,160,192,255));
	fill(vg);
	beginPath(vg);
	for (i = 0; i < 6; i++)
		circle(vg, sx[i], sy[i], 2.0f);
	fillColor(vg, rgba(220,220,220,255));
	fill(vg);

	strokeWidth(vg, 1.0f);
}

void drawSpinner(Context* vg, float cx, float cy, float r, float t)
{
	float a0 = 0.0f + t*6;
	float a1 = Pi + t*6;
	float r0 = r;
	float r1 = r * 0.75f;
	float ax,ay, bx,by;
	Paint paint;

	save(vg);

	beginPath(vg);
	arc(vg, cx,cy, r0, a0, a1, static_cast<int>(Winding::CW));
	arc(vg, cx,cy, r1, a1, a0, static_cast<int>(Winding::CCW));
	closePath(vg);
	ax = cx + cosf(a0) * (r0+r1)*0.5f;
	ay = cy + sinf(a0) * (r0+r1)*0.5f;
	bx = cx + cosf(a1) * (r0+r1)*0.5f;
	by = cy + sinf(a1) * (r0+r1)*0.5f;
	paint = linearGradient(vg, ax,ay, bx,by, rgba(0,0,0,0), rgba(0,0,0,128));
	fillPaint(vg, paint);
	fill(vg);

	restore(vg);
}

void drawThumbnails(Context* vg, float x, float y, float w, float h, const int* images, int nimages, float t)
{
	float cornerRadius = 3.0f;
	Paint shadowPaint, imgPaint, fadePaint;
	float ix,iy,iw,ih;
	float thumb = 60.0f;
	float arry = 30.5f;
	int imgw, imgh;
	float stackh = (nimages/2) * (thumb+10) + 10;
	int i;
	float u = (1+cosf(t*0.5f))*0.5f;
	float u2 = (1-cosf(t*0.2f))*0.5f;
	float scrollh, dv;

	save(vg);
//	clearState(vg);

	// Drop shadow
	shadowPaint = boxGradient(vg, x,y+4, w,h, cornerRadius*2, 20, rgba(0,0,0,128), rgba(0,0,0,0));
	beginPath(vg);
	rect(vg, x-10,y-10, w+20,h+30);
	roundedRect(vg, x,y, w,h, cornerRadius);
	pathWinding(vg, static_cast<int>(Solidity::Hole));
	fillPaint(vg, shadowPaint);
	fill(vg);

	// Window
	beginPath(vg);
	roundedRect(vg, x,y, w,h, cornerRadius);
	moveTo(vg, x-10,y+arry);
	lineTo(vg, x+1,y+arry-11);
	lineTo(vg, x+1,y+arry+11);
	fillColor(vg, rgba(200,200,200,255));
	fill(vg);

	save(vg);
	scissor(vg, x,y,w,h);
	translate(vg, 0, -(stackh - h)*u);

	dv = 1.0f / (float)(nimages-1);

	for (i = 0; i < nimages; i++) {
		float tx, ty, v, a;
		tx = x+10;
		ty = y+10;
		tx += (i%2) * (thumb+10);
		ty += (i/2) * (thumb+10);
		imageSize(vg, images[i], &imgw, &imgh);
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

		imgPaint = imagePattern(vg, tx+ix, ty+iy, iw,ih, 0.0f/180.0f*Pi, images[i], a);
		beginPath(vg);
		roundedRect(vg, tx,ty, thumb,thumb, 5);
		fillPaint(vg, imgPaint);
		fill(vg);

		shadowPaint = boxGradient(vg, tx-1,ty, thumb+2,thumb+2, 5, 3, rgba(0,0,0,128), rgba(0,0,0,0));
		beginPath(vg);
		rect(vg, tx-5,ty-5, thumb+10,thumb+10);
		roundedRect(vg, tx,ty, thumb,thumb, 6);
		pathWinding(vg, static_cast<int>(Solidity::Hole));
		fillPaint(vg, shadowPaint);
		fill(vg);

		beginPath(vg);
		roundedRect(vg, tx+0.5f,ty+0.5f, thumb-1,thumb-1, 4-0.5f);
		strokeWidth(vg,1.0f);
		strokeColor(vg, rgba(255,255,255,192));
		stroke(vg);
	}
	restore(vg);

	// Hide fades
	fadePaint = linearGradient(vg, x,y,x,y+6, rgba(200,200,200,255), rgba(200,200,200,0));
	beginPath(vg);
	rect(vg, x+4,y,w-8,6);
	fillPaint(vg, fadePaint);
	fill(vg);

	fadePaint = linearGradient(vg, x,y+h,x,y+h-6, rgba(200,200,200,255), rgba(200,200,200,0));
	beginPath(vg);
	rect(vg, x+4,y+h-6,w-8,6);
	fillPaint(vg, fadePaint);
	fill(vg);

	// Scroll bar
	shadowPaint = boxGradient(vg, x+w-12+1,y+4+1, 8,h-8, 3,4, rgba(0,0,0,32), rgba(0,0,0,92));
	beginPath(vg);
	roundedRect(vg, x+w-12,y+4, 8,h-8, 3);
	fillPaint(vg, shadowPaint);
//	fillColor(vg, rgba(255,0,0,128));
	fill(vg);

	scrollh = (h/stackh) * (h-8);
	shadowPaint = boxGradient(vg, x+w-12-1,y+4+(h-8-scrollh)*u-1, 8,scrollh, 3,4, rgba(220,220,220,255), rgba(128,128,128,255));
	beginPath(vg);
	roundedRect(vg, x+w-12+1,y+4+1 + (h-8-scrollh)*u, 8-2,scrollh-2, 2);
	fillPaint(vg, shadowPaint);
//	fillColor(vg, rgba(0,0,0,128));
	fill(vg);

	restore(vg);
}

void drawColorwheel(Context* vg, float x, float y, float w, float h, float t)
{
	int i;
	float r0, r1, ax,ay, bx,by, cx,cy, aeps, r;
	float hue = sinf(t * 0.12f);
	Paint paint;

	save(vg);

/*	beginPath(vg);
	rect(vg, x,y,w,h);
	fillColor(vg, rgba(255,0,0,128));
	fill(vg);*/

	cx = x + w*0.5f;
	cy = y + h*0.5f;
	r1 = (w < h ? w : h) * 0.5f - 5.0f;
	r0 = r1 - 20.0f;
	aeps = 0.5f / r1;	// half a pixel arc length in radians (2pi cancels out).

	for (i = 0; i < 6; i++) {
		float a0 = (float)i / 6.0f * Pi * 2.0f - aeps;
		float a1 = (float)(i+1.0f) / 6.0f * Pi * 2.0f + aeps;
		beginPath(vg);
		arc(vg, cx,cy, r0, a0, a1, static_cast<int>(Winding::CW));
		arc(vg, cx,cy, r1, a1, a0, static_cast<int>(Winding::CCW));
		closePath(vg);
		ax = cx + cosf(a0) * (r0+r1)*0.5f;
		ay = cy + sinf(a0) * (r0+r1)*0.5f;
		bx = cx + cosf(a1) * (r0+r1)*0.5f;
		by = cy + sinf(a1) * (r0+r1)*0.5f;
		paint = linearGradient(vg, ax,ay, bx,by, hsla(a0/(Pi*2),1.0f,0.55f,255), hsla(a1/(Pi*2),1.0f,0.55f,255));
		fillPaint(vg, paint);
		fill(vg);
	}

	beginPath(vg);
	circle(vg, cx,cy, r0-0.5f);
	circle(vg, cx,cy, r1+0.5f);
	strokeColor(vg, rgba(0,0,0,64));
	strokeWidth(vg, 1.0f);
	stroke(vg);

	// Selector
	save(vg);
	translate(vg, cx,cy);
	rotate(vg, hue*Pi*2);

	// Marker on
	strokeWidth(vg, 2.0f);
	beginPath(vg);
	rect(vg, r0-1,-3,r1-r0+2,6);
	strokeColor(vg, rgba(255,255,255,192));
	stroke(vg);

	paint = boxGradient(vg, r0-3,-5,r1-r0+6,10, 2,4, rgba(0,0,0,128), rgba(0,0,0,0));
	beginPath(vg);
	rect(vg, r0-2-10,-4-10,r1-r0+4+20,8+20);
	rect(vg, r0-2,-4,r1-r0+4,8);
	pathWinding(vg, static_cast<int>(Solidity::Hole));
	fillPaint(vg, paint);
	fill(vg);

	// Center triangle
	r = r0 - 6;
	ax = cosf(120.0f/180.0f*Pi) * r;
	ay = sinf(120.0f/180.0f*Pi) * r;
	bx = cosf(-120.0f/180.0f*Pi) * r;
	by = sinf(-120.0f/180.0f*Pi) * r;
	beginPath(vg);
	moveTo(vg, r,0);
	lineTo(vg, ax,ay);
	lineTo(vg, bx,by);
	closePath(vg);
	paint = linearGradient(vg, r,0, ax,ay, hsla(hue,1.0f,0.5f,255), rgba(255,255,255,255));
	fillPaint(vg, paint);
	fill(vg);
	paint = linearGradient(vg, (r+ax)*0.5f,(0+ay)*0.5f, bx,by, rgba(0,0,0,0), rgba(0,0,0,255));
	fillPaint(vg, paint);
	fill(vg);
	strokeColor(vg, rgba(0,0,0,64));
	stroke(vg);

	// Select circle on triangle
	ax = cosf(120.0f/180.0f*Pi) * r*0.3f;
	ay = sinf(120.0f/180.0f*Pi) * r*0.4f;
	strokeWidth(vg, 2.0f);
	beginPath(vg);
	circle(vg, ax,ay,5);
	strokeColor(vg, rgba(255,255,255,192));
	stroke(vg);

	paint = radialGradient(vg, ax,ay, 7,9, rgba(0,0,0,64), rgba(0,0,0,0));
	beginPath(vg);
	rect(vg, ax-20,ay-20,40,40);
	circle(vg, ax,ay,7);
	pathWinding(vg, static_cast<int>(Solidity::Hole));
	fillPaint(vg, paint);
	fill(vg);

	restore(vg);

	// Render hue label
	const float tw = 50;
	const float th = 25;
	r1 += 0.5f*sqrt(tw*tw+th*th);
	beginPath(vg);
	fillColor(vg, rgb(32,32,32));
	ax = cx + r1*cosf(hue*Pi*2);
	ay = cy + r1*sinf(hue*Pi*2);
	roundedRect(vg, ax - tw*0.5f, ay -th*0.5f, tw, th,5.0f);
	fill(vg);

	save(vg);
	translate(vg, ax, ay);
	scale(vg, 2.0f, 2.0f); // Check that local transforms work with text
	fontSize(vg, 0.5f * th);
	textAlign(vg, static_cast<int>(Align::Center | Align::Top));
	fontFace(vg, "sans");
	fillColor(vg, rgb(255,255,255));
	char str[128];
	sprintf(str, "%d%%", (int)(100.0f * (hue + 1.0f)) % 100);
	beginPath(vg);
	::text(vg, 0.0f, -0.2f * th, str, 0);
	fill(vg);

	restore(vg);
	restore(vg);
}

void drawStylizedLines(Context* vg, float x, float y, float w, float h){
	lineJoin(vg, static_cast<int>(LineCap::Round));
	lineStyle(vg, static_cast<int>(LineStyle::Dashed));
	strokeColor(vg,rgbaf(0.6f,0.6f,1.0f,1.0f));
	strokeWidth(vg, 5.0f);
	beginPath(vg);
	rect(vg, x, y, w, h);
	stroke(vg);
	lineStyle(vg, static_cast<int>(LineStyle::Solid));
}

void drawLines(Context* vg, float x, float y, float w, float h, float lineWidth, Color color, float t)
{
	int i, j;
	float pad = 5.0f, s = w/9.0f - pad*2;
	float pts[4*2], fx, fy;
	int joins[3] = {static_cast<int>(LineCap::Miter), static_cast<int>(LineCap::Round), static_cast<int>(LineCap::Bevel)};
	int caps[3] = {static_cast<int>(LineCap::Butt), static_cast<int>(LineCap::Round), static_cast<int>(LineCap::Square)};
	UNUSED(h);

	save(vg);
	pts[0] = -s*0.25f + cosf(t*0.3f) * s*0.5f;
	pts[1] = sinf(t*0.3f) * s*0.5f;
	pts[2] = -s*0.25;
	pts[3] = 0;
	pts[4] = s*0.25f;
	pts[5] = 0;
	pts[6] = s*0.25f + cosf(-t*0.3f) * s*0.5f;
	pts[7] = sinf(-t*0.3f) * s*0.5f;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			fx = x + s*0.5f + (i*3+j)/9.0f*w + pad;
			fy = y - s*0.5f + pad;

			lineCap(vg, caps[i]);
			lineJoin(vg, joins[j]);

			strokeWidth(vg, lineWidth);
			strokeColor(vg, color);
			beginPath(vg);
			moveTo(vg, fx+pts[0], fy+pts[1]);
			lineTo(vg, fx+pts[2], fy+pts[3]);
			lineTo(vg, fx+pts[4], fy+pts[5]);
			lineTo(vg, fx+pts[6], fy+pts[7]);
			stroke(vg);

			lineCap(vg, static_cast<int>(LineCap::Butt));
			lineJoin(vg, static_cast<int>(LineCap::Bevel));

			strokeWidth(vg, 1.0f);
			strokeColor(vg, rgba(0,192,255,255));
			beginPath(vg);
			moveTo(vg, fx+pts[0], fy+pts[1]);
			lineTo(vg, fx+pts[2], fy+pts[3]);
			lineTo(vg, fx+pts[4], fy+pts[5]);
			lineTo(vg, fx+pts[6], fy+pts[7]);
			stroke(vg);
		}
	}


	restore(vg);
}

int loadDemoData(Context* vg, DemoData* data)
{
	int i;

	if (vg == NULL)
		return -1;

	for (i = 0; i < 12; i++) {
		char file[128];
		snprintf(file, 128, "../example/images/image%d.jpg", i+1);
		data->images[i] = createImage(vg, file, 0);
		if (data->images[i] == 0) {
			printf("Could not load %s.\n", file);
			return -1;
		}
	}

	data->fontIcons = createFont(vg, "icons", "../example/entypo.ttf");
	if (data->fontIcons == -1) {
		printf("Could not add font icons.\n");
		return -1;
	}
	data->fontNormal = createFont(vg, "sans", "../example/Roboto-Regular.ttf");
	if (data->fontNormal == -1) {
		printf("Could not add font italic.\n");
		return -1;
	}
	data->fontBold = createFont(vg, "sans-bold", "../example/Roboto-Bold.ttf");
	if (data->fontBold == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}
	data->fontEmoji = createFont(vg, "emoji", "../example/NotoEmoji-Regular.ttf");
	if (data->fontEmoji == -1) {
		printf("Could not add font emoji.\n");
		return -1;
	}
	addFallbackFontId(vg, data->fontNormal, data->fontEmoji);
	addFallbackFontId(vg, data->fontBold, data->fontEmoji);

	return 0;
}

void freeDemoData(Context* vg, DemoData* data)
{
	int i;

	if (vg == NULL)
		return;

	for (i = 0; i < 12; i++)
		deleteImage(vg, data->images[i]);
}

void drawParagraph(Context* vg, float x, float y, float width, float height, float mx, float my)
{
	TextRow rows[3];
	GlyphPosition glyphs[100];
	const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.🎉";
	const char* start;
	const char* end;
	int nrows, i, nglyphs, j, lnum = 0;
	float lineh;
	float caretx, px;
	float bounds[4];
	float a;
	const char* hoverText = "Hover your mouse over the text to see calculated caret position.";
	float gx,gy;
	int gutter = 0;
	UNUSED(height);

	save(vg);

	fontSize(vg, 15.0f);
	fontFace(vg, "sans");
	textAlign(vg, static_cast<int>(Align::Left | Align::Top));
	textMetrics(vg, NULL, NULL, &lineh);

	// The text break API can be used to fill a large buffer of rows,
	// or to iterate over the text just few lines (or just one) at a time.
	// The "next" variable of the last returned item tells where to continue.
	start = text;
	end = text + strlen(text);
	while ((nrows = textBreakLines(vg, start, end, width, rows, 3, 0))) {
		for (i = 0; i < nrows; i++) {
			TextRow* row = &rows[i];
			int hit = mx > x && mx < (x+width) && my >= y && my < (y+lineh);

			beginPath(vg);
			fillColor(vg, rgba(255,255,255,hit?64:16));
			rect(vg, x + row->minx, y, row->maxx - row->minx, lineh);
			fill(vg);

			fillColor(vg, rgba(255,255,255,255));
			::text(vg, x, y, row->start, row->end);

			if (hit) {
				caretx = (mx < x+row->width/2) ? x : x+row->width;
				px = x;
				nglyphs = textGlyphPositions(vg, x, y, row->start, row->end, glyphs, 100);
				for (j = 0; j < nglyphs; j++) {
					float x0 = glyphs[j].x;
					float x1 = (j+1 < nglyphs) ? glyphs[j+1].x : x+row->width;
					float gx = x0 * 0.3f + x1 * 0.7f;
					if (mx >= px && mx < gx)
						caretx = glyphs[j].x;
					px = gx;
				}
				beginPath(vg);
				fillColor(vg, rgba(255,192,0,255));
				rect(vg, caretx, y, 1, lineh);
				fill(vg);

				gutter = lnum+1;
				gx = x - 10;
				gy = y + lineh/2;
			}
			lnum++;
			y += lineh;
		}
		// Keep going...
		start = rows[nrows-1].next;
	}

	if (gutter) {
		char txt[16];
		snprintf(txt, sizeof(txt), "%d", gutter);
		fontSize(vg, 12.0f);
		textAlign(vg, static_cast<int>(Align::Right | Align::Middle));

		textBounds(vg, gx,gy, txt, NULL, bounds);

		beginPath(vg);
		fillColor(vg, rgba(255,192,0,255));
		roundedRect(vg, (int)bounds[0]-4,(int)bounds[1]-2, (int)(bounds[2]-bounds[0])+8, (int)(bounds[3]-bounds[1])+4, ((int)(bounds[3]-bounds[1])+4)/2-1);
		fill(vg);

		fillColor(vg, rgba(32,32,32,255));
		::text(vg, gx,gy, txt, NULL);
	}

	y += 20.0f;

	fontSize(vg, 11.0f);
	textAlign(vg, static_cast<int>(Align::Left | Align::Top));
	textLineHeight(vg, 1.2f);

	textBoxBounds(vg, x,y, 150, hoverText, NULL, bounds);

	// Fade the tooltip out when close to it.
	gx = clampf(mx, bounds[0], bounds[2]) - mx;
	gy = clampf(my, bounds[1], bounds[3]) - my;
	a = sqrtf(gx*gx + gy*gy) / 30.0f;
	a = clampf(a, 0, 1);
	globalAlpha(vg, a);

	beginPath(vg);
	fillColor(vg, rgba(220,220,220,255));
	roundedRect(vg, bounds[0]-2,bounds[1]-2, (int)(bounds[2]-bounds[0])+4, (int)(bounds[3]-bounds[1])+4, 3);
	px = (int)((bounds[2]+bounds[0])/2);
	moveTo(vg, px,bounds[1] - 10);
	lineTo(vg, px+7,bounds[1]+1);
	lineTo(vg, px-7,bounds[1]+1);
	fill(vg);

	fillColor(vg, rgba(0,0,0,220));
	textBox(vg, x,y, 150, hoverText, NULL);

	restore(vg);
}

void drawWidths(Context* vg, float x, float y, float width)
{
	int i;

	save(vg);

	strokeColor(vg, rgba(0,0,0,255));

	for (i = 0; i < 20; i++) {
		float w = (i+0.5f)*0.1f;
		strokeWidth(vg, w);
		beginPath(vg);
		moveTo(vg, x,y);
		lineTo(vg, x+width,y+width*0.3f);
		stroke(vg);
		y += 10;
	}

	restore(vg);
}

void drawCaps(Context* vg, float x, float y, float width)
{
	int i;
	int caps[3] = {static_cast<int>(LineCap::Butt), static_cast<int>(LineCap::Round), static_cast<int>(LineCap::Square)};
	float lineWidth = 8.0f;

	save(vg);

	beginPath(vg);
	rect(vg, x-lineWidth/2, y, width+lineWidth, 40);
	fillColor(vg, rgba(255,255,255,32));
	fill(vg);

	beginPath(vg);
	rect(vg, x, y, width, 40);
	fillColor(vg, rgba(255,255,255,32));
	fill(vg);

	strokeWidth(vg, lineWidth);
	for (i = 0; i < 3; i++) {
		lineCap(vg, caps[i]);
		strokeColor(vg, rgba(0,0,0,255));
		beginPath(vg);
		moveTo(vg, x, y + i*10 + 5);
		lineTo(vg, x+width, y + i*10 + 5);
		stroke(vg);
	}

	restore(vg);
}

void drawScissor(Context* vg, float x, float y, float t)
{
	save(vg);

	// Draw first rect and set scissor to it's area.
	translate(vg, x, y);
	rotate(vg, degToRad(5));
	beginPath(vg);
	rect(vg, -20,-20,60,40);
	fillColor(vg, rgba(255,0,0,255));
	fill(vg);
	scissor(vg, -20,-20,60,40);

	// Draw second rectangle with offset and rotation.
	translate(vg, 40,0);
	rotate(vg, t);

	// Draw the intended second rectangle without any scissoring.
	save(vg);
	resetScissor(vg);
	beginPath(vg);
	rect(vg, -20,-10,60,30);
	fillColor(vg, rgba(255,128,0,64));
	fill(vg);
	restore(vg);

	// Draw second rectangle with combined scissoring.
	intersectScissor(vg, -20,-10,60,30);
	beginPath(vg);
	rect(vg, -20,-10,60,30);
	fillColor(vg, rgba(255,128,0,255));
	fill(vg);

	restore(vg);
}

void drawBezierCurve(Context* vg, float x0, float y0, float radius, float t){

	float x1 = x0 + radius*cos(2*Pi*t/5);
	float y1 = y0 + radius*sin(2*Pi*t/5);

	float cx0 = x0;
	float cy0 = y0 + ((y1 - y0) * 0.75f);
	float cx1 = x1;
	float cy1 = y0 + ((y1 - y0) * 0.75f);

	beginPath(vg);
	moveTo(vg, x0, y0);
	lineTo(vg, cx0, cy0);
	lineTo(vg, cx1, cy1);
	lineTo(vg, x1, y1);
	strokeColor(vg,rgba(200,200,200,255));
	strokeWidth(vg,2.0f);
	stroke(vg);

	lineCap(vg, static_cast<int>(LineCap::Round));
	strokeWidth(vg,5);
	lineJoin(vg, static_cast<int>(LineCap::Round));

	beginPath(vg);
	moveTo(vg, x0, y0);
	bezierTo(vg, cx0, cy0, cx1, cy1, x1, y1);
	lineStyle(vg, static_cast<int>(LineStyle::Solid));
	strokeColor(vg, rgba(40, 53, 147,255));
	stroke(vg);

	lineStyle(vg, static_cast<int>(LineStyle::Dashed));
	strokeColor(vg, rgba(255, 195, 0,255));
	stroke(vg);
	
	beginPath(vg);
	circle(vg,x0,y0,5.0f);
	circle(vg,cx0,cy0,5.0f);
	circle(vg,cx1,cy1,5.0f);
	circle(vg,x1,y1,5.0f);
	lineStyle(vg, static_cast<int>(LineStyle::Solid));
	fillColor(vg,rgba(64,192,64,255));
	fill(vg);
}

void drawScaledText(Context* vg, float x0, float y0, float t){
	save(vg);
	const float textScale = (cos(2 * Pi * t * 0.25)+1.0) + 0.1;
	translate(vg, x0, y0);
	scale(vg, textScale, textScale);
	fontSize(vg, 24.0f);
	fontFace(vg, "sans-bold");
	textAlign(vg,static_cast<int>(Align::Center | Align::Middle));
	fillColor(vg, rgba(255,255,255,255));
	beginPath(vg);
	::text(vg, 0.0f, 0.0f, "NanoVG", NULL);
	fill(vg);
	restore(vg);
}

void renderDemo(Context* vg, float mx, float my, float width, float height,
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

	switch((int)(t/5.0)%3){
		case 0:
			lineStyle(vg, static_cast<int>(LineStyle::Dashed));break;
		case 1:
			lineStyle(vg, static_cast<int>(LineStyle::Dotted));break;
		case 2:
			lineStyle(vg, static_cast<int>(LineStyle::Glow));break;
		default:
			lineStyle(vg, static_cast<int>(LineStyle::Solid));
	}
	drawLines(vg, 100, height-5, 800, 100, 10.0f, rgba(255, 153, 0, 255), t*3);

	lineStyle(vg, static_cast<int>(LineStyle::Solid));
	drawLines(vg, 120, height-75, 600, 50, 17.0f, rgba(0,0,0,160), t);

	// Line caps
	drawWidths(vg, 10, 50, 30);

	// Line caps
	drawCaps(vg, 10, 300, 30);

	drawScissor(vg, 50, height-80, t);

	save(vg);
	if (blowup) {
		rotate(vg, sinf(t*0.3f)*5.0f/180.0f*Pi);
		scale(vg, 2.0f, 2.0f);
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
	drawButton(vg, ICON_LOGIN, "Sign in", x+138, y, 140, 28, rgba(0,96,128,255));
	y += 45;

	// Slider
	drawLabel(vg, "Diameter", x,y, 280,20);
	y += 25;
	drawEditBoxNum(vg, "123.00", "px", x+180,y, 100,28);
	drawSlider(vg, 0.4f, x,y, 170,28);
	y += 55;

	drawButton(vg, ICON_TRASH, "Delete", x, y, 160, 28, rgba(128,16,8,255));
	drawButton(vg, 0, "Cancel", x+170, y, 110, 28, rgba(0,0,0,0));

	// Thumbnails box
	drawThumbnails(vg, 365, popy-30, 160, 300, data->images, 12, t);

	restore(vg);

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

void saveScreenShot(int w, int h, int premult, const char* name)
{
	unsigned char* image = (unsigned char*)malloc(w*h*4);
	if (image == NULL)
		return;
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);
	if (premult)
		unpremultiplyAlpha(image, w, h, w*4);
	else
		setAlpha(image, w, h, w*4, 255);
	flipHorizontal(image, w, h, w*4);
 	stbi_write_png(name, w, h, 4, image, w*4);
 	free(image);
}
