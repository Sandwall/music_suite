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

	struct GLRenderer : public Renderer {
		SDL_GLContext context = nullptr;

		GLShader shader;
		GLuint vbo = 0, vao = 0;

		void init(SDL_Window* window) override;
		void cleanup() override;
		void render_quads(bool flush = true) override;

		void clear(const Color& clearColor) override;
		void swap_screen(SDL_Window* window) override;
	};
}