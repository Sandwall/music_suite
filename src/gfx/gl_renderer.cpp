#include <GL/gl3w.h>

#include "gl_renderer.h"

#include <SDL3/SDL_opengl.h>

const char* main_vertexSource = /* vertex shader */ R"(
#version 460 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in u8vec4 inCol;

out vec2 uv;
out u8vec4 vertexCol;

uniform sampler2D

void main() {
	gl_Position = vec4(inPos, 1.0);
}
)";

const char* main_fragSource = /* fragment shader */ R"(
#version 460 core
out vec4 FragColor;

in vec2 uv;
in u8vec4 vertexCol;

void main() {
	FragColor = 
}
)";

namespace gfx {
	bool GLShader::compile(const char* vs, const char* fs) {
		destroy();

		GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
		GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(vShader, 1, &vs, nullptr);
		glShaderSource(fShader, 1, &fs, nullptr);
		glCompileShader(vShader);
		glCompileShader(fShader);

		program = glCreateProgram();
		glAttachShader(program, vShader);
		glAttachShader(program, fShader);
		glLinkProgram(program);

		glDeleteShader(vShader);
		glDeleteShader(fShader);

		return true;
	}

	void GLShader::destroy() {
		if (program) {
			glDeleteProgram(program);
			program = 0;
		}
	}

	void GL_BatchRenderer::init(SDL_Window* window) {
		cleanup();

		context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);

		{	// init buffers and vertex attributes
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			constexpr GLsizei STRIDE = (sizeof(f32) * 4) + (sizeof(u8) * 4);
			glEnableVertexAttribArray(0); // vec2 pos;
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, STRIDE, 0);

			glEnableVertexAttribArray(1); // vec2 uv;
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 2));

			glEnableVertexAttribArray(2); // u8vec4 color;
			glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 4));
		}

		shader.compile(main_vertexSource, main_fragSource);
	}

	void GL_BatchRenderer::cleanup() {
		if (context != nullptr) {
			shader.destroy();

			glDeleteBuffers(1, &vbo);
			vbo = 0;

			glDeleteVertexArrays(1, &vao);
			vao = 0;

			SDL_GL_DestroyContext(context);
			context = nullptr;
		}
	}

	void GL_BatchRenderer::render(bool flush) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindVertexArray(vao);
		glUseProgram(shader);



		glDrawArrays(GL_TRIANGLES, 0, batch.numVertices);

		if (flush)
			batch.flush();
	}

	void GL_BatchRenderer::clear() {
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void GL_BatchRenderer::swap_screen(SDL_Window* window) {
		SDL_GL_SwapWindow(window);
	}
}