#include "common.h"

namespace gfx {
	Color::Color(f32 red, f32 green, f32 blue, f32 alpha) {
		r = static_cast<float>(red) / 255.0f;
		g = static_cast<float>(green) / 255.0f;
		b = static_cast<float>(blue) / 255.0f;
		a = static_cast<float>(alpha) / 255.0f;
	}

	Color::Color(u8 red, u8 green, u8 blue, u8 alpha) {
		r = red;
		g = green;
		b = blue;
		a = alpha;
	}
}