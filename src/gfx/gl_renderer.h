#pragma once

#include "common.h"
#include <SDL3/SDL_video.h>

namespace gfx {
	struct GLRenderer : public Renderer {
		SDL_GLContext context = nullptr;

		GLuint mainShader;
		GLuint vbo, vao;

		void init(SDL_Window* window) override;
		void cleanup() override;
		void render(struct DrawBatch& batch, bool flush = true) override;

		void clear() override;
		void swap_screen(SDL_Window* window) override;
	};
}