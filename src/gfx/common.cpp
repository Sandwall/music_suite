#include "common.h"

#include <stdlib.h>

#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>

#include <clay.h>

namespace gfx {
	stbrp_context rpContext;
	stbtt_pack_context packContext;

	void startup() {

	}

	void DrawBatch::init(u32 maxVertexSize) {
		cleanup();

		buffer.len = maxVertexSize;
		buffer.data = static_cast<Vertex*>(malloc(maxVertexSize * sizeof(Vertex)));
	}

	void DrawBatch::cleanup() {
		if (buffer.data) {
			free(buffer.data);
			buffer.data = nullptr;
		}

		buffer.len = 0;
		numVertices = 0;
	}

	void DrawBatch::start_frame(f32 width, f32 height) {
		targetWidth = width;
		targetHeight = height;
	}

	void DrawBatch::flush() { numVertices = 0; }

	void DrawBatch::add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color) {

	}
}

