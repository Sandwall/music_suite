#include "common.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>

#include <clay.h>

#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL_filesystem.h>

namespace gfx {
	void Renderer::start_frame(f32 width, f32 height) {
		targetWidth = width;
		targetHeight = height;
		flush_batch();
	}

	void Renderer::flush_batch() { numVertices = 0; }

	// assumes value is in [0, range]
	f32 to_ndc(f32 value, f32 range) {
		return ((value / range) - 0.5f) * 2.0f;
	}


	void Renderer::add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color) {
		Vertex* current = &(quadVertices[numVertices++]);

		// TL
		current->x = to_ndc(x, targetWidth);
		current->y = to_ndc(y, targetHeight);
		current->z = -to_ndc(numVertices, quadVertices.len);	// -z forward

		current->u = -1.0;
		current->v = -1.0;

		current->color = color;

	}

	void Renderer::add_tex_rect(f32 x, f32 y, f32 u, f32 v, f32 w, f32 h, const Color& color) {
		assert((numVertices + 6) < quadVertices.len);

	}

	static constexpr size_t ATLAS_ALLOC_SIZE = 1024;
	void BakedTextureAtlas::init() {
		cleanup();

		// allocate cpu atlas
		cpuAtlas.len = ATLAS_ALLOC_SIZE * ATLAS_ALLOC_SIZE;
		cpuAtlas.data = (u32*)malloc(sizeof(u32) * cpuAtlas.len);
		cpuAtlas.width = ATLAS_ALLOC_SIZE;
		cpuAtlas.height = ATLAS_ALLOC_SIZE;

		// allocate memory for stb libs
		rpNodes = (stbrp_node*)malloc(sizeof(stbrp_node) * cpuAtlas.width);
	}

	void BakedTextureAtlas::cleanup() {
		if (cpuAtlas.data) free(cpuAtlas.data);
		TINY_ZERO(cpuAtlas);

		if (rpNodes) free(rpNodes);
		rpNodes = nullptr;
	}

	void BakedTextureAtlas::start_packing() {
		stbrp_init_target(&rpContext, cpuAtlas.width, cpuAtlas.height, rpNodes, cpuAtlas.width);
	}

	void BakedTextureAtlas::add_font(const char* path) {
		if (!SDL_GetPathInfo(path, nullptr)) return;

		mem::Arena& scratch = mem::get_scratch();
		mem::ArenaScope scope(scratch);

		FILE* fp = fopen(path, "rb");
		fseek(fp, 0, SEEK_END);
		size_t fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		void* fileData = scratch.push(fileSize + 1);
		fread(fileData, fileSize, 1, fp);
		fclose(fp);

		stbtt_InitFont(fonts, (u8*)fileData, 0);
	}

	void BakedTextureAtlas::add_tex(const char* path) {

	}

	void BakedTextureAtlas::bake() {

	}

}

