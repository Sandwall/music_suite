#pragma once

#include <tinydef.hpp>

struct SDL_Window;

namespace gfx {
	//
	struct Color {
		u8 r, g, b, a;

		Color(u8 red, u8 green, u8 blue, u8 alpha);
		Color(f32 red = 1.0f, f32 green = 1.0f, f32 blue = 1.0f, f32 alpha = 1.0f);
	};

	struct Vertex {
		f32 x, y;
		Color color;
	};

	// NOTE(sand): Sure, vtable lookups might be slow because of cache misses,
	// but when there's only a few of these per frame, it's negligible
	struct Renderer {
		virtual void init(SDL_Window* window) = 0;
		virtual void cleanup() = 0;
		virtual void render(struct DrawBatch& batch, bool flush = true) = 0;

		virtual void clear() = 0;
		virtual void swap_screen() = 0;
	};
}
