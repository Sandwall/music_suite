#pragma once

#include <tinydef.hpp>

#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>

struct SDL_Window;

namespace gfx {
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

		static constexpr u32 QUAD_CAPACITY_BYTES = 1 << 16;
		static constexpr u32 TEXTCHAR_CAPACITY_BYTES = 1 << 20;
		static constexpr u32 INDICES_CAPACITY_BYTES = tim::max(QUAD_CAPACITY_BYTES, TEXTCHAR_CAPACITY_BYTES);
		u32 numDrawnObjects;             // used to count depth of every drawn object

		tds::Slice<u32> quadIndices;     // cpu side quad buffer
		                                 // this will stay largely the same through each frame
		                                 // since we are exclusively drawing quads

		u32 numQuads;                    // used to index quad buffers
		tds::Slice<Vertex> quadVertices; // cpu side quad vertex buffer

		u32 numChars;
		tds::Slice<Vertex> textVertices;

		// flushes batch and sets numDrawnObjects to 0
		void start_frame(f32 width, f32 height);

		// sets numVertices to 0
		void flush_batch();

		// 
		void add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color);
		void add_tex_rect(f32 x, f32 y, f32 u, f32 v, f32 w, f32 h, const Color& color);

	private:
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
