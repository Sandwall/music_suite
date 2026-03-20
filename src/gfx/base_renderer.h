#pragma once

#include <tinydef.hpp>
#include "common.hpp"

#include "texture_atlas.h"

struct SDL_Window;

namespace gfx {
	struct Renderer {
		virtual void init(SDL_Window* window) = 0;
		virtual void create_textures(const BakedAtlas& bakedAtlas, const FontAtlas& fontAtlas) = 0;
		virtual void cleanup() = 0;
		virtual void render(bool flush = true) = 0;
		virtual void scissor(i32 x, i32 y, i32 w, i32 h) = 0;

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

		BakedAtlas* textureAtlasRef;
		FontAtlas* fontAtlasRef;

		// flushes batch and sets numDrawnObjects to 0
		void start_frame(f32 width, f32 height, BakedAtlas* tAtlas, FontAtlas* fAtlas);

		// sets numVertices to 0
		void flush_batch();

		// draws a colored rect with specified source uv coordinates
		void add_rect(f32 x, f32 y, f32 w, f32 h, i32 texIdx, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		// draws a plain colored rect
		void add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		// draws a quad from the font atlas
		void add_char(f32 x, f32 y, f32 w, f32 h, i32 fontId, i32 codepoint, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });
	protected:
		f32 targetWidth, targetHeight;
	};
}
