#pragma once

#include <tinydef.hpp>

#include "./common.h"

/* draw_batch - This is probably a good way to do this?
 * ====================================================
 * 
 * So essentially, if you want to draw anything to the screen, instead of calling the renderer directly
 * You would populate the DrawBatch, and that gets passed to the renderer.
 * 
 * So the renderer
 * 
 * This works 
 */

namespace gfx {
	// stbrp and stbtt contexts will probably be initialized here
	// not gonna cleanup these things because program
	void startup();
	
	struct DrawBatch {
		// number of vertices added to the buffer
		u32 numVertices;

		// these get allocated on init
		tds::Slice<Vertex> buffer; // cpu side vertex buffer
		tds::Slice2<u8> cpuAtlas;  // cpu side texture atlas

		void init(u32 maxVertexSize);
		void cleanup();

		void start_frame(f32 width, f32 height);
		void flush(); // sets numVertices to 0

		void add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color);

	private:
		f32 targetWidth, targetHeight;
	};
}
