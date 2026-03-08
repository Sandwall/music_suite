#pragma once

#include <tinydef.hpp>

struct SDL_Window;

namespace gfx {
	struct Color {
		u8 r, g, b, a;
	};

	struct Vertex {
		f32 x, y;
		f32 u, v;
		Color color;
	};

	// NOTE(sand): Sure, vtable lookups might be slow because of cache misses,
	// but when there's only a few of these per frame, it's negligible
	struct Renderer {
		virtual void init(SDL_Window* window) = 0;
		virtual void cleanup() = 0;
		virtual void render(bool flush = true) = 0;

		virtual void clear() = 0;
		virtual void swap_screen(SDL_Window* window) = 0;
	};

	// stbrp and stbtt contexts will probably be initialized here
	// not gonna cleanup these things because program
	void startup();

	struct TextureAtlas {
		tds::Slice2<u8> cpuAtlas;  // cpu side texture atlas
	};

	/* draw_batch - This is probably a good way to do this?
	* ====================================================
	*
	* So essentially, if you want to draw anything to the screen, you go to this guy first.
	* The renderer then passes the cpu vertex buffer to the gpu, which draws it with a shader
	*
	* Note that this batch is also responsible 
	* 
	*/


	struct DrawBatch {
		// number of vertices added to the buffer
		u32 numVertices;

		tds::Slice<Vertex> buffer; // cpu side vertex buffer gets allocated on init

		void init(u32 maxVertexSize);
		void cleanup();

		void start_frame(f32 width, f32 height);
		void flush(); // sets numVertices to 0

		void add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color);

	private:
		f32 targetWidth, targetHeight;
	};
}
