//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#pragma once

#include "nanovg.hpp"

#include <array>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ranges>
#include <vector>

struct FONScontext;

namespace nvg {

enum class PathCommand : int {
	MoveTo = 0,
	LineTo = 1,
	BezierTo = 2,
	Close = 3,
	Winding = 4,
};

enum class PointFlags : unsigned char {
	Corner = 0x01,
	Left = 0x02,
	Bevel = 0x04,
	InnerBevel = 0x08,
};

inline constexpr unsigned char operator&(unsigned char a, PointFlags b) noexcept {
	return a & static_cast<unsigned char>(b);
}

inline constexpr unsigned char operator|(unsigned char a, PointFlags b) noexcept {
	return a | static_cast<unsigned char>(b);
}

inline constexpr unsigned char& operator|=(unsigned char& a, PointFlags b) noexcept {
	a |= static_cast<unsigned char>(b);
	return a;
}

inline constexpr PointFlags operator|(PointFlags a, PointFlags b) noexcept {
	return static_cast<PointFlags>(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

struct State {
	CompositeOperationState compositeOperation;
	int shapeAntiAlias;
	Paint fill;
	Paint stroke;
	float strokeWidth;
	float miterLimit;
	int lineJoin;
	int lineCap;
	int lineStyle;
	float alpha;
	std::array<float, 6> xform;
	Scissor scissor;
	float fontSize;
	float letterSpacing;
	float lineHeight;
	float fontBlur;
	float fontDilate;
	int textAlign;
	int fontId;
};

struct Point {
	float x,y;
	float dx, dy;
	float len;
	float dmx, dmy;
	unsigned char flags;
};

struct PathCache {
	std::vector<Point> points;
	std::vector<Path> paths;
	std::vector<Vertex> verts;
	std::array<float, 4> bounds;
};

struct ContextImpl {
	explicit ContextImpl(const Params& params);
	~ContextImpl();
	ContextImpl(const ContextImpl&) = delete;
	ContextImpl& operator=(const ContextImpl&) = delete;
	ContextImpl(ContextImpl&&) = delete;
	ContextImpl& operator=(ContextImpl&&) = delete;

	Params params;
	std::vector<float> commands;
	float commandx, commandy;
	float tessTol;
	float distTol;
	float fringeWidth;
	float devicePxRatio;
	std::shared_ptr<FONScontext> fs;
	std::vector<int> fontImages;
	size_t fontImageIdx;
	size_t drawCallCount;
	size_t fillTriCount;
	size_t strokeTriCount;
	size_t textTriCount;
	ScissorBounds scissor;
	std::vector<State> states;
	std::shared_ptr<PathCache> cache;

	State& topState();
	const State& topState() const;
	void setDevicePixelRatio(float ratio);

	template <class C>
		requires std::ranges::range<C> && std::same_as<std::ranges::range_value_t<C>, float>
	void appendCommands(C& vals)
	{
		State& st = topState();
		auto nvals = std::ranges::size(vals);
		if (nvals == 0) {
			std::terminate();
		}
		if (auto front = static_cast<int>(vals.front());
			front != static_cast<int>(PathCommand::Close) && front != static_cast<int>(PathCommand::Winding)) {
			auto iter = std::rbegin(vals);
			commandy = *iter++;
			commandx = *iter;
		}

		size_t i = 0;
		while (i < nvals) {
			int cmd = static_cast<int>(vals[i]);
			switch (cmd) {
			case static_cast<int>(PathCommand::MoveTo):
				transformPoint(vals[i+1], vals[i+2], st.xform.data(), vals[i+1], vals[i+2]);
				i += 3;
				break;
			case static_cast<int>(PathCommand::LineTo):
				transformPoint(vals[i+1], vals[i+2], st.xform.data(), vals[i+1], vals[i+2]);
				i += 3;
				break;
			case static_cast<int>(PathCommand::BezierTo):
				transformPoint(vals[i+1], vals[i+2], st.xform.data(), vals[i+1], vals[i+2]);
				transformPoint(vals[i+3], vals[i+4], st.xform.data(), vals[i+3], vals[i+4]);
				transformPoint(vals[i+5], vals[i+6], st.xform.data(), vals[i+5], vals[i+6]);
				i += 7;
				break;
			case static_cast<int>(PathCommand::Close):
				i++;
				break;
			case static_cast<int>(PathCommand::Winding):
				i += 2;
				break;
			default:
				i++;
			}
		}
		commands.insert(commands.end(), vals.begin(), vals.end());
	}

	void clearPathCache();
	Path* lastPath();
	void appendPath();
	Point* lastPoint();
	void addPoint(float x, float y, int flags);
	void markCachedPathClosed();
	void setCachedPathWinding(int winding);
	void prepareTempVerts(int nverts);
	void tesselateBezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int level, int type);
	void flattenPaths();
	void calculateJoins(float w, int lineJoin, float miterLimit);
	int expandStroke(float w, float fringe, int lineCap, int lineJoin, int lineStyle, float miterLimit);
	int expandFill(float w, int lineJoin, float miterLimit);
	void flushTextTexture();
	int allocTextAtlas();
	void renderText(Vertex* verts, int nverts);
};

} // namespace nvg
