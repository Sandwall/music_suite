#pragma once

#include <SDL3/SDL_video.h>
#include <GL/gl3w.h>

#include "common.h"

namespace gfx {
	// Wraps Shader
	struct GLShader {
		GLuint program = 0;

		bool compile(const char* vs, const char* fs);
		void destroy();

		operator GLuint& () { return program; }
	};

	struct GL_BatchRenderer : public Renderer {
		SDL_GLContext context = nullptr;

		DrawBatch batch = { };

		GLShader shader;
		GLuint vbo, vao;


		void init(SDL_Window* window) override;
		void cleanup() override;
		void render(bool flush = true) override;

		void clear() override;
		void swap_screen(SDL_Window* window) override;
	};
}