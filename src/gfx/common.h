#pragma once

#include <tinydef.hpp>

#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>

struct SDL_Window;

namespace gfx {
	struct Rect {
		f32 x, y;
		f32 w, h;
	};

	struct Color {
		f32 r, g, b, a;
	};

	struct Vertex {
		f32 x, y, z;
		f32 u, v;
		Color color;
	};

	struct Renderer {
		virtual void init(SDL_Window* window) = 0;
		virtual void cleanup() = 0;
		virtual void render_quads(bool flush = true) = 0;

		virtual void clear(const Color& clearColor) = 0;
		virtual void swap_screen(SDL_Window* window) = 0;

		// API-Agnostic Batch System
		// =========================

		static constexpr u32 MAX_QUADS_IN_ONE_DRAW = 1 << 14;
		static constexpr u32 INDICES_CAPACITY = MAX_QUADS_IN_ONE_DRAW * 6;
		static constexpr u32 INDICES_ALLOC_SIZE = INDICES_CAPACITY * sizeof(u32);
		static constexpr u32 VERTICES_CAPACITY = MAX_QUADS_IN_ONE_DRAW * 4;
		static constexpr u32 VERTICES_ALLOC_SIZE = VERTICES_CAPACITY * sizeof(Vertex);

		u32 numDrawnObjects;                  // used to count depth of every drawn object

		u32 numQuadsAdded;                    // used to index quad buffers
		tds::Slice<Vertex> quadVertices;      // cpu side quad vertex buffer
		u32 numCharsAdded;                    // same as above
		tds::Slice<Vertex> textVertices;      // ...

		// flushes batch and sets numDrawnObjects to 0
		void start_frame(f32 width, f32 height);

		// sets numVertices to 0
		void flush_batch();

		// draws a plain colored rect
		void add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		// TODO: draws a colored rect with specified source uv coordinates
		void add_rect(Rect destination, Rect source, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });
	protected:
		f32 targetWidth, targetHeight;
	};

	struct BakedTextureAtlas {
		stbtt_fontinfo fonts[4];
		struct stbtt_pack_context packContext;

		struct stbrp_context rpContext;
		struct stbrp_node* rpNodes;

		tds::Slice2<u32> cpuAtlas;  // cpu side texture atlas

		void init();
		void cleanup();

		void start_packing();
		void add_font(const char* path);
		void add_tex(const char* path);
		void bake();
	};
}
