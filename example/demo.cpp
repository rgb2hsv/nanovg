#include "demo.hpp"
#include <stdio.h>
#include <string.h>
#include <string>
#include <array>
#include <filesystem>
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
	case 1: str[0] = static_cast<char>(cp);
	}
	return str;
}


void drawWindow(nvg::Context& vg, const std::string& title, float x, float y, float w, float h)
{
	float cornerRadius = 3.0f;
	nvg::Paint shadowPaint;
	nvg::Paint headerPaint;

	vg.save();
//	clearState(vg);

	// Window
	vg.beginPath();
	vg.roundedRect( x,y, w,h, cornerRadius);
	vg.fillColor( nvg::rgba(28,30,34,192));
//	vg.fillColor( nvg::rgba(0,0,0,128));
	vg.fill();

	// Drop shadow
	shadowPaint = vg.boxGradient( x,y+2, w,h, cornerRadius*2, 10, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	vg.beginPath();
	vg.rect( x-10,y-10, w+20,h+30);
	vg.roundedRect( x,y, w,h, cornerRadius);
	vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
	vg.fillPaint( shadowPaint);
	vg.fill();

	// Header
	headerPaint = vg.linearGradient( x,y,x,y+15, nvg::rgba(255,255,255,8), nvg::rgba(0,0,0,16));
	vg.beginPath();
	vg.roundedRect( x+1,y+1, w-2,30, cornerRadius-1);
	vg.fillPaint( headerPaint);
	vg.fill();
	vg.beginPath();
	vg.moveTo( x+0.5f, y+0.5f+30);
	vg.lineTo( x+0.5f+w-1, y+0.5f+30);
	vg.strokeColor( nvg::rgba(0,0,0,32));
	vg.stroke();

	vg.fontSize( 15.0f);
	vg.fontFace( "sans-bold");
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));

	vg.fontBlur(2);
	vg.fillColor( nvg::rgba(0,0,0,128));
	vg.text( x+w/2,y+16+1, title.c_str(), NULL);

	vg.fontBlur(0);
	vg.fillColor( nvg::rgba(220,220,220,160));
	vg.text( x+w/2,y+16, title.c_str(), NULL);

	vg.restore();
}

void drawSearchBox(nvg::Context& vg, const std::string& text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = h/2-1;

	// Edit
	bg = vg.boxGradient( x,y+1.5f, w,h, h/2,5, nvg::rgba(0,0,0,16), nvg::rgba(0,0,0,92));
	vg.beginPath();
	vg.roundedRect( x,y, w,h, cornerRadius);
	vg.fillPaint( bg);
	vg.fill();

/*	vg.beginPath();
	vg.roundedRect( x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,48));
	vg.stroke();*/

	vg.fontSize( h*1.3f);
	vg.fontFace( "icons");
	vg.fillColor( nvg::rgba(255,255,255,64));
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	vg.text( x+h*0.55f, y+h*0.55f, cpToUTF8(ICON_SEARCH, icon.data()), NULL);

	vg.fontSize( 17.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,32));

	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.text( x+h*1.05f,y+h*0.5f,text.c_str(), NULL);

	vg.fontSize( h*1.3f);
	vg.fontFace( "icons");
	vg.fillColor( nvg::rgba(255,255,255,32));
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	vg.text( x+w-h*0.55f, y+h*0.55f, cpToUTF8(ICON_CIRCLED_CROSS, icon.data()), NULL);
}

void drawDropDown(nvg::Context& vg, const std::string& text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = 4.0f;

	bg = vg.linearGradient( x,y,x,y+h, nvg::rgba(255,255,255,16), nvg::rgba(0,0,0,16));
	vg.beginPath();
	vg.roundedRect( x+1,y+1, w-2,h-2, cornerRadius-1);
	vg.fillPaint( bg);
	vg.fill();

	vg.beginPath();
	vg.roundedRect( x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,48));
	vg.stroke();

	vg.fontSize( 17.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,160));
	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.text( x+h*0.3f,y+h*0.5f,text.c_str(), NULL);

	vg.fontSize( h*1.3f);
	vg.fontFace( "icons");
	vg.fillColor( nvg::rgba(255,255,255,64));
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	vg.text( x+w-h*0.5f, y+h*0.5f, cpToUTF8(ICON_CHEVRON_RIGHT, icon.data()), NULL);
}

void drawLabel(nvg::Context& vg, const std::string& text, float x, float y, float w, float h)
{
	UNUSED(w);

	vg.fontSize( 15.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,128));

	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.text( x,y+h*0.5f,text.c_str(), NULL);
}

void drawEditBoxBase(nvg::Context& vg, float x, float y, float w, float h)
{
	nvg::Paint bg;
	// Edit
	bg = vg.boxGradient( x+1,y+1+1.5f, w-2,h-2, 3,4, nvg::rgba(255,255,255,32), nvg::rgba(32,32,32,32));
	vg.beginPath();
	vg.roundedRect( x+1,y+1, w-2,h-2, 4-1);
	vg.fillPaint( bg);
	vg.fill();

	vg.beginPath();
	vg.roundedRect( x+0.5f,y+0.5f, w-1,h-1, 4-0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,48));
	vg.stroke();
}

void drawEditBox(nvg::Context& vg, const std::string& text, float x, float y, float w, float h)
{

	drawEditBoxBase(vg, x,y, w,h);

	vg.fontSize( 17.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,64));
	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.text( x+h*0.3f,y+h*0.5f,text.c_str(), NULL);
}

void drawEditBoxNum(nvg::Context& vg,
					const std::string& text, const std::string& units, float x, float y, float w, float h)
{
	float uw;

	drawEditBoxBase(vg, x,y, w,h);

	uw = vg.textBounds( 0,0, units.c_str(), NULL, NULL);

	vg.fontSize( 15.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,64));
	vg.textAlign(static_cast<int>(nvg::Align::Right | nvg::Align::Middle));
	vg.text( x+w-h*0.3f,y+h*0.5f,units.c_str(), NULL);

	vg.fontSize( 17.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,128));
	vg.textAlign(static_cast<int>(nvg::Align::Right | nvg::Align::Middle));
	vg.text( x+w-uw-h*0.5f,y+h*0.5f,text.c_str(), NULL);
}

void drawCheckBox(nvg::Context& vg, const std::string& text, float x, float y, float w, float h)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	UNUSED(w);

	vg.fontSize( 15.0f);
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgba(255,255,255,160));

	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.text( x+28,y+h*0.5f,text.c_str(), NULL);

	bg = vg.boxGradient( x+1,y+(int)(h*0.5f)-9+1, 18,18, 3,3, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,92));
	vg.beginPath();
	vg.roundedRect( x+1,y+(int)(h*0.5f)-9, 18,18, 3);
	vg.fillPaint( bg);
	vg.fill();

	vg.fontSize( 33);
	vg.fontFace( "icons");
	vg.fillColor( nvg::rgba(255,255,255,128));
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	vg.text( x+9+2, y+h*0.5f, cpToUTF8(ICON_CHECK, icon.data()), NULL);
}

void drawButton(nvg::Context& vg, int preicon, const std::string& text, float x, float y, float w, float h, nvg::Color col)
{
	nvg::Paint bg;
	std::array<char, 8> icon{};
	float cornerRadius = 4.0f;
	float tw = 0, iw = 0;

	bg = vg.linearGradient( x,y,x,y+h, nvg::rgba(255,255,255,isBlack(col)?16:32), nvg::rgba(0,0,0,isBlack(col)?16:32));
	vg.beginPath();
	vg.roundedRect( x+1,y+1, w-2,h-2, cornerRadius-1);
	if (!isBlack(col)) {
		vg.fillColor( col);
		vg.fill();
	}
	vg.fillPaint( bg);
	vg.fill();

	vg.beginPath();
	vg.roundedRect( x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,48));
	vg.stroke();

	vg.fontSize( 17.0f);
	vg.fontFace( "sans-bold");
	tw = vg.textBounds( 0,0, text.c_str(), NULL, NULL);
	if (preicon != 0) {
		vg.fontSize( h*1.3f);
		vg.fontFace( "icons");
		iw = vg.textBounds( 0,0, cpToUTF8(preicon, icon.data()), NULL, NULL);
		iw += h*0.15f;
	}

	if (preicon != 0) {
		vg.fontSize( h*1.3f);
		vg.fontFace( "icons");
		vg.fillColor( nvg::rgba(255,255,255,96));
		vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
		vg.text( x+w*0.5f-tw*0.5f-iw*0.75f, y+h*0.5f, cpToUTF8(preicon, icon.data()), NULL);
	}

	vg.fontSize( 17.0f);
	vg.fontFace( "sans-bold");
	vg.textAlign(static_cast<int>(nvg::Align::Left | nvg::Align::Middle));
	vg.fillColor( nvg::rgba(0,0,0,160));
	vg.text( x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f-1,text.c_str(), NULL);
	vg.fillColor( nvg::rgba(255,255,255,160));
	vg.text( x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f,text.c_str(), NULL);
}

void drawSlider(nvg::Context& vg, float pos, float x, float y, float w, float h)
{
	nvg::Paint bg, knob;
	float cy = y + h*0.5f;
	float kr = h*0.25f;

	vg.save();
//	clearState(vg);

	// Slot
	bg = vg.boxGradient( x,cy-2+1, w,4, 2,2, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,128));
	vg.beginPath();
	vg.roundedRect( x,cy-2, w,4, 2);
	vg.fillPaint( bg);
	vg.fill();

	// Knob Shadow
	bg = vg.radialGradient( x+(int)(pos*w),cy+1, kr-3,kr+3, nvg::rgba(0,0,0,64), nvg::rgba(0,0,0,0));
	vg.beginPath();
	vg.rect( x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
	vg.circle( x+(int)(pos*w),cy, kr);
	vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
	vg.fillPaint( bg);
	vg.fill();

	// Knob
	knob = vg.linearGradient( x,cy-kr,x,cy+kr, nvg::rgba(255,255,255,16), nvg::rgba(0,0,0,16));
	vg.beginPath();
	vg.circle( x+(int)(pos*w),cy, kr-1);
	vg.fillColor( nvg::rgba(40,43,48,255));
	vg.fill();
	vg.fillPaint( knob);
	vg.fill();

	vg.beginPath();
	vg.circle( x+(int)(pos*w),cy, kr-0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,92));
	vg.stroke();

	vg.restore();
}

void drawFancyText(nvg::Context& vg, float x, float y){
	vg.fontSize( 30.0f);
	vg.fontFace( "sans-bold");
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));

	vg.fontBlur( 10);
	vg.fillColor( nvg::rgb(255, 0, 102));
	vg.text( x, y, "Font Blur", NULL);
	vg.fontBlur(0);
	vg.fillColor( nvg::rgb(255,255,255));
	vg.text( x, y, "Font Blur", NULL);

	vg.fontDilate( 2);
	vg.fillColor( nvg::rgb(255, 0, 102));
	vg.text( x, y+40, "Font Outline", NULL);
	vg.fontDilate(0);
	vg.fillColor( nvg::rgb(255,255,255));
	vg.text( x, y+40, "Font Outline", NULL);

	vg.fontDilate( 3); // Dilate will always be applied before blur
	vg.fontBlur( 2);
	vg.fillColor( nvg::rgb(255, 0, 102));
	vg.text( x, y+80, "Font Blur Outline", NULL);
	vg.fontDilate(0);
	vg.fontBlur(0);
	vg.fillColor( nvg::rgb(255,255,255));
	vg.text( x, y+80, "Font Blur Outline", NULL);
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

	bg = vg.linearGradient( x,y+h*0.5f,x+w*0.1f,y+h, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,16));
	vg.beginPath();
	vg.ellipse( lx+3.0f,ly+16.0f, ex,ey);
	vg.ellipse( rx+3.0f,ry+16.0f, ex,ey);
	vg.fillPaint( bg);
	vg.fill();

	bg = vg.linearGradient( x,y+h*0.25f,x+w*0.1f,y+h, nvg::rgba(220,220,220,255), nvg::rgba(128,128,128,255));
	vg.beginPath();
	vg.ellipse( lx,ly, ex,ey);
	vg.ellipse( rx,ry, ex,ey);
	vg.fillPaint( bg);
	vg.fill();

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	vg.beginPath();
	vg.ellipse( lx+dx,ly+dy+ey*0.25f*(1-blink), br,br*blink);
	vg.fillColor( nvg::rgba(32,32,32,255));
	vg.fill();

	dx = (mx - rx) / (ex * 10);
	dy = (my - ry) / (ey * 10);
	d = sqrtf(dx*dx+dy*dy);
	if (d > 1.0f) {
		dx /= d; dy /= d;
	}
	dx *= ex*0.4f;
	dy *= ey*0.5f;
	vg.beginPath();
	vg.ellipse( rx+dx,ry+dy+ey*0.25f*(1-blink), br,br*blink);
	vg.fillColor( nvg::rgba(32,32,32,255));
	vg.fill();

	gloss = vg.radialGradient( lx-ex*0.25f,ly-ey*0.5f, ex*0.1f,ex*0.75f, nvg::rgba(255,255,255,128), nvg::rgba(255,255,255,0));
	vg.beginPath();
	vg.ellipse( lx,ly, ex,ey);
	vg.fillPaint( gloss);
	vg.fill();

	gloss = vg.radialGradient( rx-ex*0.25f,ry-ey*0.5f, ex*0.1f,ex*0.75f, nvg::rgba(255,255,255,128), nvg::rgba(255,255,255,0));
	vg.beginPath();
	vg.ellipse( rx,ry, ex,ey);
	vg.fillPaint( gloss);
	vg.fill();
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
	bg = vg.linearGradient( x,y,x,y+h, nvg::rgba(0,160,192,0), nvg::rgba(0,160,192,64));
	vg.beginPath();
	vg.moveTo( sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		vg.bezierTo( sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	vg.lineTo( x+w, y+h);
	vg.lineTo( x, y+h);
	vg.fillPaint( bg);
	vg.fill();

	// Graph line
	vg.beginPath();
	vg.moveTo( sx[0], sy[0]+2);
	for (i = 1; i < 6; i++)
		vg.bezierTo( sx[i-1]+dx*0.5f,sy[i-1]+2, sx[i]-dx*0.5f,sy[i]+2, sx[i],sy[i]+2);
	vg.strokeColor( nvg::rgba(0,0,0,32));
	vg.strokeWidth( 3.0f);
	vg.stroke();

	vg.beginPath();
	vg.moveTo( sx[0], sy[0]);
	for (i = 1; i < 6; i++)
		vg.bezierTo( sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
	vg.strokeColor( nvg::rgba(0,160,192,255));
	vg.strokeWidth( 3.0f);
	vg.stroke();

	// Graph sample pos
	for (i = 0; i < 6; i++) {
		bg = vg.radialGradient( sx[i],sy[i]+2, 3.0f,8.0f, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,0));
		vg.beginPath();
		vg.rect( sx[i]-10, sy[i]-10+2, 20,20);
		vg.fillPaint( bg);
		vg.fill();
	}

	vg.beginPath();
	for (i = 0; i < 6; i++)
		vg.circle( sx[i], sy[i], 4.0f);
	vg.fillColor( nvg::rgba(0,160,192,255));
	vg.fill();
	vg.beginPath();
	for (i = 0; i < 6; i++)
		vg.circle( sx[i], sy[i], 2.0f);
	vg.fillColor( nvg::rgba(220,220,220,255));
	vg.fill();

	vg.strokeWidth( 1.0f);
}

void drawSpinner(nvg::Context& vg, float cx, float cy, float r, float t)
{
	float a0 = 0.0f + t*6;
	float a1 = (float)M_PI + t*6;
	float r0 = r;
	float r1 = r * 0.75f;
	float ax,ay, bx,by;
	nvg::Paint paint;

	vg.save();

	vg.beginPath();
	vg.arc( cx,cy, r0, a0, a1, static_cast<int>(nvg::Winding::CW));
	vg.arc( cx,cy, r1, a1, a0, static_cast<int>(nvg::Winding::CCW));
	vg.closePath();
	ax = cx + cosf(a0) * (r0+r1)*0.5f;
	ay = cy + sinf(a0) * (r0+r1)*0.5f;
	bx = cx + cosf(a1) * (r0+r1)*0.5f;
	by = cy + sinf(a1) * (r0+r1)*0.5f;
	paint = vg.linearGradient( ax,ay, bx,by, nvg::rgba(0,0,0,0), nvg::rgba(0,0,0,128));
	vg.fillPaint( paint);
	vg.fill();

	vg.restore();
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

	vg.save();
//	clearState(vg);

	// Drop shadow
	shadowPaint = vg.boxGradient( x,y+4, w,h, cornerRadius*2, 20, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	vg.beginPath();
	vg.rect( x-10,y-10, w+20,h+30);
	vg.roundedRect( x,y, w,h, cornerRadius);
	vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
	vg.fillPaint( shadowPaint);
	vg.fill();

	// Window
	vg.beginPath();
	vg.roundedRect( x,y, w,h, cornerRadius);
	vg.moveTo( x-10,y+arry);
	vg.lineTo( x+1,y+arry-11);
	vg.lineTo( x+1,y+arry+11);
	vg.fillColor( nvg::rgba(200,200,200,255));
	vg.fill();

	vg.save();
	vg.scissor( x,y,w,h);
	vg.translate( 0, -(stackh - h)*u);

	dv = 1.0f / (float)(nimages-1);

	for (i = 0; i < nimages; i++) {
		float tx, ty, v, a;
		tx = x+10;
		ty = y+10;
		tx += (i%2) * (thumb+10);
		ty += (i/2) * (thumb+10);
		vg.imageSize( images[i], imgw, imgh);
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

		imgPaint = vg.imagePattern( tx+ix, ty+iy, iw,ih, 0.0f/180.0f*M_PI, images[i], a);
		vg.beginPath();
		vg.roundedRect( tx,ty, thumb,thumb, 5);
		vg.fillPaint( imgPaint);
		vg.fill();

		shadowPaint = vg.boxGradient( tx-1,ty, thumb+2,thumb+2, 5, 3, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
		vg.beginPath();
		vg.rect( tx-5,ty-5, thumb+10,thumb+10);
		vg.roundedRect( tx,ty, thumb,thumb, 6);
		vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
		vg.fillPaint( shadowPaint);
		vg.fill();

		vg.beginPath();
		vg.roundedRect( tx+0.5f,ty+0.5f, thumb-1,thumb-1, 4-0.5f);
		vg.strokeWidth(1.0f);
		vg.strokeColor( nvg::rgba(255,255,255,192));
		vg.stroke();
	}
	vg.restore();

	// Hide fades
	fadePaint = vg.linearGradient( x,y,x,y+6, nvg::rgba(200,200,200,255), nvg::rgba(200,200,200,0));
	vg.beginPath();
	vg.rect( x+4,y,w-8,6);
	vg.fillPaint( fadePaint);
	vg.fill();

	fadePaint = vg.linearGradient( x,y+h,x,y+h-6, nvg::rgba(200,200,200,255), nvg::rgba(200,200,200,0));
	vg.beginPath();
	vg.rect( x+4,y+h-6,w-8,6);
	vg.fillPaint( fadePaint);
	vg.fill();

	// Scroll bar
	shadowPaint = vg.boxGradient( x+w-12+1,y+4+1, 8,h-8, 3,4, nvg::rgba(0,0,0,32), nvg::rgba(0,0,0,92));
	vg.beginPath();
	vg.roundedRect( x+w-12,y+4, 8,h-8, 3);
	vg.fillPaint( shadowPaint);
//	vg.fillColor( nvg::rgba(255,0,0,128));
	vg.fill();

	scrollh = (h/stackh) * (h-8);
	shadowPaint = vg.boxGradient( x+w-12-1,y+4+(h-8-scrollh)*u-1, 8,scrollh, 3,4, nvg::rgba(220,220,220,255), nvg::rgba(128,128,128,255));
	vg.beginPath();
	vg.roundedRect( x+w-12+1,y+4+1 + (h-8-scrollh)*u, 8-2,scrollh-2, 2);
	vg.fillPaint( shadowPaint);
//	vg.fillColor( nvg::rgba(0,0,0,128));
	vg.fill();

	vg.restore();
}

void drawColorwheel(nvg::Context& vg, float x, float y, float w, float h, float t)
{
	int i;
	float r0, r1, ax,ay, bx,by, cx,cy, aeps, r;
	float hue = sinf(t * 0.12f);
	nvg::Paint paint;

	vg.save();

/*	vg.beginPath();
	vg.rect( x,y,w,h);
	vg.fillColor( nvg::rgba(255,0,0,128));
	vg.fill();*/

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
		vg.beginPath();
		vg.arc( cx,cy, r0, a0, a1, static_cast<int>(nvg::Winding::CW));
		vg.arc( cx,cy, r1, a1, a0, static_cast<int>(nvg::Winding::CCW));
		vg.closePath();
		ax = cx + cosf(a0) * (r0+r1)*0.5f;
		ay = cy + sinf(a0) * (r0+r1)*0.5f;
		bx = cx + cosf(a1) * (r0+r1)*0.5f;
		by = cy + sinf(a1) * (r0+r1)*0.5f;
		paint = vg.linearGradient( ax,ay, bx,by, nvg::hsla(a0/twoPi,1.0f,0.55f,255), nvg::hsla(a1/twoPi,1.0f,0.55f,255));
		vg.fillPaint( paint);
		vg.fill();
	}

	vg.beginPath();
	vg.circle( cx,cy, r0-0.5f);
	vg.circle( cx,cy, r1+0.5f);
	vg.strokeColor( nvg::rgba(0,0,0,64));
	vg.strokeWidth( 1.0f);
	vg.stroke();

	// Selector
	vg.save();
	vg.translate( cx,cy);
	vg.rotate( hue*twoPi);

	// Marker on
	vg.strokeWidth( 2.0f);
	vg.beginPath();
	vg.rect( r0-1,-3,r1-r0+2,6);
	vg.strokeColor( nvg::rgba(255,255,255,192));
	vg.stroke();

	paint = vg.boxGradient( r0-3,-5,r1-r0+6,10, 2,4, nvg::rgba(0,0,0,128), nvg::rgba(0,0,0,0));
	vg.beginPath();
	vg.rect( r0-2-10,-4-10,r1-r0+4+20,8+20);
	vg.rect( r0-2,-4,r1-r0+4,8);
	vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
	vg.fillPaint( paint);
	vg.fill();

	// Center triangle
	r = r0 - 6;
	ax = cosf(120.0f/180.0f*pi) * r;
	ay = sinf(120.0f/180.0f*pi) * r;
	bx = cosf(-120.0f/180.0f*pi) * r;
	by = sinf(-120.0f/180.0f*pi) * r;
	vg.beginPath();
	vg.moveTo( r,0);
	vg.lineTo( ax,ay);
	vg.lineTo( bx,by);
	vg.closePath();
	paint = vg.linearGradient( r,0, ax,ay, nvg::hsla(hue,1.0f,0.5f,255), nvg::rgba(255,255,255,255));
	vg.fillPaint( paint);
	vg.fill();
	paint = vg.linearGradient( (r+ax)*0.5f,(0+ay)*0.5f, bx,by, nvg::rgba(0,0,0,0), nvg::rgba(0,0,0,255));
	vg.fillPaint( paint);
	vg.fill();
	vg.strokeColor( nvg::rgba(0,0,0,64));
	vg.stroke();

	// Select circle on triangle
	ax = cosf(120.0f/180.0f*pi) * r*0.3f;
	ay = sinf(120.0f/180.0f*pi) * r*0.4f;
	vg.strokeWidth( 2.0f);
	vg.beginPath();
	vg.circle( ax,ay,5);
	vg.strokeColor( nvg::rgba(255,255,255,192));
	vg.stroke();

	paint = vg.radialGradient( ax,ay, 7,9, nvg::rgba(0,0,0,64), nvg::rgba(0,0,0,0));
	vg.beginPath();
	vg.rect( ax-20,ay-20,40,40);
	vg.circle( ax,ay,7);
	vg.pathWinding( static_cast<int>(nvg::Solidity::Hole));
	vg.fillPaint( paint);
	vg.fill();

	vg.restore();

	// Render hue label
	const float tw = 50;
	const float th = 25;
	r1 += 0.5f*sqrt(tw*tw+th*th);
	vg.beginPath();
	vg.fillColor( nvg::rgb(32,32,32));
	ax = cx + r1*cosf(hue*twoPi);
	ay = cy + r1*sinf(hue*twoPi);
	vg.roundedRect( ax - tw*0.5f, ay -th*0.5f, tw, th,5.0f);
	vg.fill();

	vg.save();
	vg.translate( ax, ay);
	vg.scale( 2.0f, 2.0f); // Check that local transforms work with text
	vg.fontSize( 0.5f * th);
	vg.textAlign( static_cast<int>(nvg::Align::Center | nvg::Align::Top));
	vg.fontFace( "sans");
	vg.fillColor( nvg::rgb(255,255,255));
	std::array<char, 128> str{};
	sprintf(str.data(), "%d%%", (int)(100.0f * (hue + 1.0f)) % 100);
	vg.beginPath();
	vg.text( 0.0f, -0.2f * th, str.data(), 0);
	vg.fill();

	vg.restore();
	vg.restore();
}

void drawStylizedLines(nvg::Context& vg, float x, float y, float w, float h){
	vg.lineJoin( static_cast<int>(nvg::LineCap::Round));
	vg.lineStyle( static_cast<int>(nvg::LineStyle::Dashed));
	vg.strokeColor(nvg::rgbaf(0.6f,0.6f,1.0f,1.0f));
	vg.strokeWidth( 5.0f);
	vg.beginPath();
	vg.rect( x, y, w, h);
	vg.stroke();
	vg.lineStyle( static_cast<int>(nvg::LineStyle::Solid));
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

	vg.save();
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

			vg.lineCap( caps[i]);
			vg.lineJoin( joins[j]);

			vg.strokeWidth( lineWidth);
			vg.strokeColor( color);
			vg.beginPath();
			vg.moveTo( fx+pts[0], fy+pts[1]);
			vg.lineTo( fx+pts[2], fy+pts[3]);
			vg.lineTo( fx+pts[4], fy+pts[5]);
			vg.lineTo( fx+pts[6], fy+pts[7]);
			vg.stroke();

			vg.lineCap( static_cast<int>(nvg::LineCap::Butt));
			vg.lineJoin( static_cast<int>(nvg::LineCap::Bevel));

			vg.strokeWidth( 1.0f);
			vg.strokeColor( nvg::rgba(0,192,255,255));
			vg.beginPath();
			vg.moveTo( fx+pts[0], fy+pts[1]);
			vg.lineTo( fx+pts[2], fy+pts[3]);
			vg.lineTo( fx+pts[4], fy+pts[5]);
			vg.lineTo( fx+pts[6], fy+pts[7]);
			vg.stroke();
		}
	}


	vg.restore();
}

int loadDemoData(nvg::Context& vg, DemoData* data)
{
	int i;
			namespace fs = std::filesystem;
		auto findFile=[](const fs::path& name){
			fs::path current_dir = fs::current_path();
			fs::path path =current_dir/"example"/ name;
			if (fs::exists(path)) {
				return path.string();
			}

			path = current_dir.parent_path()/"example"/ name;
			if (fs::exists(path)) {
				return path.string();
			}
			
			path = current_dir.parent_path().parent_path()/"example"/name;
			return path.string();
		};

	for (i = 0; i < 12; i++) {
		const std::string file = findFile(fs::path("images")/("image" + std::to_string(i+1) + ".jpg"));
		data->images[i] = vg.createImage(file.c_str(), 0);
		if (data->images[i] == 0) {
			printf("Could not load %s.\n", file.c_str());
			return -1;
		}
	}
	const std::string fontIcons = findFile("entypo.ttf");
	data->fontIcons = vg.createFont("icons", fontIcons.c_str());
	if (data->fontIcons == -1) {
		printf("Could not add font icons.\n");
		return -1;
	}
	const std::string fontNormal = findFile("Roboto-Regular.ttf");
	data->fontNormal = vg.createFont( "sans", fontNormal.c_str());
	if (data->fontNormal == -1) {
		printf("Could not add font normal %s\n",fontNormal.c_str());
		return -1;
	}
	const std::string fontBold = findFile("Roboto-Bold.ttf");
	data->fontBold = vg.createFont(  "sans-bold", fontBold.c_str());
	if (data->fontBold == -1) {
		printf("Could not add font bold %s\n",fontBold.c_str());
		return -1;
	}
	const std::string fontEmoji = findFile("NotoEmoji-Regular.ttf");
	data->fontEmoji =  vg.createFont("emoji",fontEmoji.c_str());
	if (data->fontEmoji == -1) {
		printf("Could not add font emoji %s\n",fontEmoji.c_str());
		return -1;
	}
	vg.addFallbackFontId(data->fontNormal, data->fontEmoji);
	vg.addFallbackFont(fontNormal.c_str(), fontEmoji.c_str());

	return 0;
}

void freeDemoData(nvg::Context& vg, DemoData* data)
{
	int i;

	for (i = 0; i < 12; i++)
		vg.deleteImage( data->images[i]);
}

void drawParagraph(nvg::Context& vg, float x, float y, float width, float height, float mx, float my)
{
	static const std::string kParagraphText =
		"This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.🎉";
	static const std::string kHoverTooltip =
		"Hover your mouse over the text to see calculated caret position.";
	std::array<nvg::TextRow, 3> rows{};
	std::array<nvg::GlyphPosition, 100> glyphs{};
	const char* start;
	const char* end;
	int nrows, i, nglyphs, j, lnum = 0;
	float lineh;
	float caretx, px;
	std::array<float, 4> bounds{};
	float a;
	float gx = 0.0f;
	float gy = 0.0f;
	int gutter = 0;
	UNUSED(height);

	vg.save();

	vg.fontSize( 15.0f);
	vg.fontFace( "sans");
	vg.textAlign( static_cast<int>(nvg::Align::Left | nvg::Align::Top));
	vg.textMetrics( NULL, NULL, &lineh);

	// The text break API can be used to fill a large buffer of rows,
	// or to iterate over the text just few lines (or just one) at a time.
	// The "next" variable of the last returned item tells where to continue.
	{
		const char* text = kParagraphText.c_str();
		start = text;
		end = text + kParagraphText.size();
	}
	while ((nrows = vg.textBreakLines( start, end, width, rows.data(), (int)rows.size(), 0))) {
		for (i = 0; i < nrows; i++) {
			nvg::TextRow* row = &rows[(size_t)i];
			int hit = mx > x && mx < (x+width) && my >= y && my < (y+lineh);

			vg.beginPath();
			vg.fillColor( nvg::rgba(255,255,255,hit?64:16));
			vg.rect( x + row->minx, y, row->maxx - row->minx, lineh);
			vg.fill();

			vg.fillColor( nvg::rgba(255,255,255,255));
			vg.text( x, y, row->start, row->end);

			if (hit) {
				caretx = (mx < x+row->width/2) ? x : x+row->width;
				px = x;
				nglyphs = vg.textGlyphPositions( x, y, row->start, row->end, glyphs.data(), (int)glyphs.size());
				for (j = 0; j < nglyphs; j++) {
					float x0 = glyphs[(size_t)j].x;
					float x1 = (j+1 < nglyphs) ? glyphs[(size_t)j+1].x : x+row->width;
					float splitX = x0 * 0.3f + x1 * 0.7f;
					if (mx >= px && mx < splitX)
						caretx = glyphs[(size_t)j].x;
					px = splitX;
				}
				vg.beginPath();
				vg.fillColor( nvg::rgba(255,192,0,255));
				vg.rect( caretx, y, 1, lineh);
				vg.fill();

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
		vg.fontSize( 12.0f);
		vg.textAlign( static_cast<int>(nvg::Align::Right | nvg::Align::Middle));

		vg.textBounds( gx,gy, txt.data(), NULL, bounds.data());

		vg.beginPath();
		vg.fillColor( nvg::rgba(255,192,0,255));
		vg.roundedRect(
			(float)((int)bounds[0] - 4), (float)((int)bounds[1] - 2),
			(float)((int)(bounds[2]-bounds[0]) + 8), (float)((int)(bounds[3]-bounds[1]) + 4),
			(float)(((int)(bounds[3]-bounds[1]) + 4)/2 - 1)
		);
		vg.fill();

		vg.fillColor( nvg::rgba(32,32,32,255));
		vg.text( gx,gy, txt.data(), NULL);
	}

	y += 20.0f;

	vg.fontSize( 11.0f);
	vg.textAlign( static_cast<int>(nvg::Align::Left | nvg::Align::Top));
	vg.textLineHeight( 1.2f);

	vg.textBoxBounds( x,y, 150, kHoverTooltip.c_str(), NULL, bounds.data());

	// Fade the tooltip out when close to it.
	gx = clampf(mx, bounds[0], bounds[2]) - mx;
	gy = clampf(my, bounds[1], bounds[3]) - my;
	a = sqrtf(gx*gx + gy*gy) / 30.0f;
	a = clampf(a, 0, 1);
	vg.globalAlpha( a);

	vg.beginPath();
	vg.fillColor( nvg::rgba(220,220,220,255));
	vg.roundedRect( bounds[0]-2.0f, bounds[1]-2.0f, (float)((int)(bounds[2]-bounds[0]) + 4), (float)((int)(bounds[3]-bounds[1]) + 4), 3.0f);
	px = (bounds[2] + bounds[0]) * 0.5f;
	vg.moveTo( px, bounds[1] - 10.0f);
	vg.lineTo( px + 7.0f, bounds[1] + 1.0f);
	vg.lineTo( px - 7.0f, bounds[1] + 1.0f);
	vg.fill();

	vg.fillColor( nvg::rgba(0,0,0,220));
	vg.textBox( x,y, 150, kHoverTooltip.c_str(), NULL);

	vg.restore();
}

void drawWidths(nvg::Context& vg, float x, float y, float width)
{
	int i;

	vg.save();

	vg.strokeColor( nvg::rgba(0,0,0,255));

	for (i = 0; i < 20; i++) {
		float w = (i+0.5f)*0.1f;
		vg.strokeWidth( w);
		vg.beginPath();
		vg.moveTo( x,y);
		vg.lineTo( x+width,y+width*0.3f);
		vg.stroke();
		y += 10;
	}

	vg.restore();
}

void drawCaps(nvg::Context& vg, float x, float y, float width)
{
	int i;
	const std::array<int, 3> caps = {static_cast<int>(nvg::LineCap::Butt), static_cast<int>(nvg::LineCap::Round), static_cast<int>(nvg::LineCap::Square)};
	float lineWidth = 8.0f;

	vg.save();

	vg.beginPath();
	vg.rect( x-lineWidth/2, y, width+lineWidth, 40);
	vg.fillColor( nvg::rgba(255,255,255,32));
	vg.fill();

	vg.beginPath();
	vg.rect( x, y, width, 40);
	vg.fillColor( nvg::rgba(255,255,255,32));
	vg.fill();

	vg.strokeWidth( lineWidth);
	for (i = 0; i < 3; i++) {
		vg.lineCap( caps[i]);
		vg.strokeColor( nvg::rgba(0,0,0,255));
		vg.beginPath();
		vg.moveTo( x, y + i*10 + 5);
		vg.lineTo( x+width, y + i*10 + 5);
		vg.stroke();
	}

	vg.restore();
}

void drawScissor(nvg::Context& vg, float x, float y, float t)
{
	vg.save();

	// Draw first rect and set scissor to it's area.
	vg.translate( x, y);
	vg.rotate( nvg::degToRad(5));
	vg.beginPath();
	vg.rect( -20,-20,60,40);
	vg.fillColor( nvg::rgba(255,0,0,255));
	vg.fill();
	vg.scissor( -20,-20,60,40);

	// Draw second rectangle with offset and rotation.
	vg.translate( 40,0);
	vg.rotate( t);

	// Draw the intended second rectangle without any scissoring.
	vg.save();
	vg.resetScissor();
	vg.beginPath();
	vg.rect( -20,-10,60,30);
	vg.fillColor( nvg::rgba(255,128,0,64));
	vg.fill();
	vg.restore();

	// Draw second rectangle with combined scissoring.
	vg.intersectScissor( -20,-10,60,30);
	vg.beginPath();
	vg.rect( -20,-10,60,30);
	vg.fillColor( nvg::rgba(255,128,0,255));
	vg.fill();

	vg.restore();
}

void drawBezierCurve(nvg::Context& vg, float x0, float y0, float radius, float t){

	const float pi = (float)M_PI;
	float x1 = x0 + radius*cosf(2.0f*pi*t/5.0f);
	float y1 = y0 + radius*sinf(2.0f*pi*t/5.0f);

	float cx0 = x0;
	float cy0 = y0 + ((y1 - y0) * 0.75f);
	float cx1 = x1;
	float cy1 = y0 + ((y1 - y0) * 0.75f);

	vg.beginPath();
	vg.moveTo( x0, y0);
	vg.lineTo( cx0, cy0);
	vg.lineTo( cx1, cy1);
	vg.lineTo( x1, y1);
	vg.strokeColor(nvg::rgba(200,200,200,255));
	vg.strokeWidth(2.0f);
	vg.stroke();

	vg.lineCap( static_cast<int>(nvg::LineCap::Round));
	vg.strokeWidth(5);
	vg.lineJoin( static_cast<int>(nvg::LineCap::Round));

	vg.beginPath();
	vg.moveTo( x0, y0);
	vg.bezierTo( cx0, cy0, cx1, cy1, x1, y1);
	vg.lineStyle( static_cast<int>(nvg::LineStyle::Solid));
	vg.strokeColor( nvg::rgba(40, 53, 147,255));
	vg.stroke();

	vg.lineStyle( static_cast<int>(nvg::LineStyle::Dashed));
	vg.strokeColor( nvg::rgba(255, 195, 0,255));
	vg.stroke();
	
	vg.beginPath();
	vg.circle(x0,y0,5.0f);
	vg.circle(cx0,cy0,5.0f);
	vg.circle(cx1,cy1,5.0f);
	vg.circle(x1,y1,5.0f);
	vg.lineStyle( static_cast<int>(nvg::LineStyle::Solid));
	vg.fillColor(nvg::rgba(64,192,64,255));
	vg.fill();
}

void drawScaledText(nvg::Context& vg, float x0, float y0, float t){
	vg.save();
	const float textScale = (cosf(2.0f * (float)M_PI * t * 0.25f) + 1.0f) + 0.1f;
	vg.translate( x0, y0);
	vg.scale( textScale, textScale);
	vg.fontSize( 24.0f);
	vg.fontFace( "sans-bold");
	vg.textAlign(static_cast<int>(nvg::Align::Center | nvg::Align::Middle));
	vg.fillColor( nvg::rgba(255,255,255,255));
	vg.beginPath();
	vg.text( 0.0f, 0.0f, "NanoVG", NULL);
	vg.fill();
	vg.restore();
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
			vg.lineStyle( static_cast<int>(nvg::LineStyle::Dashed));break;
		case 1:
			vg.lineStyle( static_cast<int>(nvg::LineStyle::Dotted));break;
		case 2:
			vg.lineStyle( static_cast<int>(nvg::LineStyle::Glow));break;
		default:
			vg.lineStyle( static_cast<int>(nvg::LineStyle::Solid));
	}
	drawLines(vg, 100, height-5, 800, 100, 10.0f, nvg::rgba(255, 153, 0, 255), t*3);

	vg.lineStyle( static_cast<int>(nvg::LineStyle::Solid));
	drawLines(vg, 120, height-75, 600, 50, 17.0f, nvg::rgba(0,0,0,160), t);

	// Line caps
	drawWidths(vg, 10, 50, 30);

	// Line caps
	drawCaps(vg, 10, 300, 30);

	drawScissor(vg, 50, height-80, t);

	vg.save();
	if (blowup) {
		vg.rotate( sinf(t*0.3f)*5.0f/180.0f*(float)M_PI);
		vg.scale( 2.0f, 2.0f);
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

	vg.restore();

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
				row[0] = static_cast<unsigned char>(mini(r*255/a, 255));
				row[1] = static_cast<unsigned char>(mini(g*255/a, 255));
				row[2] = static_cast<unsigned char>(mini(b*255/a, 255));
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
					row[0] = static_cast<unsigned char>(r/n);
					row[1] = static_cast<unsigned char>(g/n);
					row[2] = static_cast<unsigned char>(b/n);
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

bool comparePreviousScreenShot(int nw, int nh, int premult, unsigned char* nimg, const std::string& name)
{
	int w, h, n;
	unsigned char* img;
	stbi_set_unpremultiply_on_load(premult);
	stbi_convert_iphone_png_to_rgb(1);
	img = stbi_load(name.c_str(), &w, &h, &n, 4);
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
			diff[idx]=static_cast<unsigned char>(std::min(rd*4,255));
			diff[idx+1]=static_cast<unsigned char>(std::min(gd*4,255));
			diff[idx+2]=static_cast<unsigned char>(std::min(bd*4,255));
			diff[idx+3]=255;
			if(rd > Threshold || gd > Threshold || bd > Threshold || ad > Threshold){
				ret=false;
			}
	}
	stbi_image_free(img);	

	if(!ret){
		std::string fileName = name;
		auto pos= fileName.find_last_of(".");
		fileName=fileName.substr(0, pos)+"-diff"+fileName.substr(pos);
		printf("Saving screenshot diff to '%s'\n", fileName.c_str());
		stbi_write_png(fileName.c_str(), w, h, 4, diff, w*4);
	}
	return ret;
}

bool saveScreenShot(int w, int h, int premult, const std::string& name, bool compare)
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
	
	std::string fileName = name;
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
