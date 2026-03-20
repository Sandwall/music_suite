#pragma once

#include "gfx/common.hpp"

struct Theme {
	// font ids
	static constexpr u32 FONT_MAIN = 0;

	// base colors
	gfx::Color background = gfx::Color::from_hsv(0.0f, 25.0f / 100.0f, 25.0f / 100.0f);
	gfx::Color text = { 0.9f, 0.9f, 0.9f, 1.0f };

	// derivative colors

	void recompute() {

	}
};