#pragma once

#include "base_renderer.h"

#include <GL/gl3w.h>
#include <SDL3/SDL_video.h>

namespace gfx {
	// Wraps Shader
	struct GLShader {
		GLuint program = 0;

		bool compile(const char* vs, const char* fs);
		void destroy();

		operator GLuint& () { return program; }
	};

	// Wraps Texture
	struct GLTexture {
		GLuint texture = 0;
		bool loaded = false;
		i32 width = 0, height = 0;

		void create(i32 w, i32 h, i32 format, const void* data);
		void destroy();

		operator GLuint& () { return texture; }
	};

	struct GLRenderer : public Renderer {
		SDL_GLContext context = nullptr;

		GLShader shader;
		GLint textureUniformLoc;

		GLTexture mainTexture, fontTexture;

		GLuint vbo = 0, vao = 0, ibo = 0;

		void init(SDL_Window* window) override;
		void create_textures(const BakedAtlas& bakedAtlas, const FontAtlas& fontAtlas) override;
		void cleanup() override;
		void render(bool flush = true) override;
		void scissor(i32 x, i32 y, i32 w, i32 h) override;

		void clear(const Color& clearColor) override;
		void swap_screen(SDL_Window* window) override;
	};
}