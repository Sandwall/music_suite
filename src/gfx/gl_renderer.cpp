#include "gl_renderer.h"

#include "draw_batch.h"

#include <GL/gl3w.h>
#include <SDL3/SDL_opengl.h>

const char* main_vertexSource = /* vertex shader */ R"(
#version 460

)";

const char* main_fragSource = /* fragment shader */ R"(
#version 460

)";

namespace gfx {

	void GLRenderer::init(SDL_Window* window) {
		cleanup();


		context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);

		{	// init buffers and vertex attributes
			glGenBuffers(1, &vbo);
			glGenVertexArrays(1, &vao);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBindVertexArray(vao);

			constexpr GLsizei STRIDE = (sizeof(f32) * 4) + (sizeof(u8) * 4);
			glEnableVertexAttribArray(0); // vec2 pos;
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, STRIDE, 0);

			glEnableVertexAttribArray(1); // vec2 uv;
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 2));

			glEnableVertexAttribArray(2); // u8vec4 color;
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 4));
		}

		{	// init shaders
			GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
			GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(vShader, 1, &main_vertexSource, nullptr);
			glShaderSource(fShader, 1, &main_fragSource, nullptr);
			glCompileShader(vShader);
			glCompileShader(fShader);

			gl
		}
	}

	void GLRenderer::cleanup() {
		if (context != nullptr) {
			glDeleteProgram()

			glDeleteBuffers(1, &vbo);
			vbo = 0;

			glDeleteVertexArrays(1, &vao);
			vao = 0;

			SDL_GL_DestroyContext(context);
			context = nullptr;
		}
	}

	void GLRenderer::render(DrawBatch& batch, bool flush) {




		if (flush)
			batch.flush();
	}

	void GLRenderer::clear() {
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void GLRenderer::swap_screen(SDL_Window* window) {
		SDL_GL_SwapWindow(window);
	}
}