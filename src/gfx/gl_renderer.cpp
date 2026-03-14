#include <GL/gl3w.h>

#include "gl_renderer.h"

#include <SDL3/SDL_opengl.h>
#include <stdio.h>

const char* main_vertexSource = /* vertex shader */ R"(
#version 460 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec4 inCol;

out vec2 uv;
out vec4 vertexCol;

void main() {
	vertexCol = inCol;
	gl_Position = vec4(inPos, 1.0);
}
)";

const char* main_fragSource = /* fragment shader */ R"(
#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec4 vertexCol;

void main() {
	FragColor = vec4(1.0);
}
)";

namespace gfx {
	void print_shader_compile_status(GLuint shader) {
		GLint success;
		char logBuffer[512];

		glGetProgramiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader, 512, nullptr, logBuffer);
			fprintf(stderr, "Shader Compile Error: %s\n", logBuffer);
		}
	}

	void print_shader_link_status(GLuint program) {
		GLint success;
		char logBuffer[512];

		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(program, 512, nullptr, logBuffer);
			fprintf(stderr, "Shader Link Error: %s\n", logBuffer);
		}
	}

	void APIENTRY print_debug_output(
		GLenum source, GLenum type, unsigned int id, GLenum severity,
		GLsizei length, const char* message, const void* userParam) {
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

		fprintf(stderr, "---------------\n");
		fprintf(stderr, "Debug message (%u): %s\n", id, message);

		switch (source) {
			case GL_DEBUG_SOURCE_API:             fprintf(stderr, "Source: API"); break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   fprintf(stderr, "Source: Window System"); break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER: fprintf(stderr, "Source: Shader Compiler"); break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:     fprintf(stderr, "Source: Third Party"); break;
			case GL_DEBUG_SOURCE_APPLICATION:     fprintf(stderr, "Source: Application"); break;
			case GL_DEBUG_SOURCE_OTHER:           fprintf(stderr, "Source: Other"); break;
		} fprintf(stderr, "\n");

		switch (type) {
			case GL_DEBUG_TYPE_ERROR:               fprintf(stderr, "Type: Error"); break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: fprintf(stderr, "Type: Deprecated Behaviour"); break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  fprintf(stderr, "Type: Undefined Behaviour"); break;
			case GL_DEBUG_TYPE_PORTABILITY:         fprintf(stderr, "Type: Portability"); break;
			case GL_DEBUG_TYPE_PERFORMANCE:         fprintf(stderr, "Type: Performance"); break;
			case GL_DEBUG_TYPE_MARKER:              fprintf(stderr, "Type: Marker"); break;
			case GL_DEBUG_TYPE_PUSH_GROUP:          fprintf(stderr, "Type: Push Group"); break;
			case GL_DEBUG_TYPE_POP_GROUP:           fprintf(stderr, "Type: Pop Group"); break;
			case GL_DEBUG_TYPE_OTHER:               fprintf(stderr, "Type: Other"); break;
		} fprintf(stderr, "\n");

		switch (severity) {
			case GL_DEBUG_SEVERITY_HIGH:         fprintf(stderr, "Severity: high"); break;
			case GL_DEBUG_SEVERITY_MEDIUM:       fprintf(stderr, "Severity: medium"); break;
			case GL_DEBUG_SEVERITY_LOW:          fprintf(stderr, "Severity: low"); break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: fprintf(stderr, "Severity: notification"); break;
		} fprintf(stderr, "\n\n");
	}

	bool GLShader::compile(const char* vs, const char* fs) {
		destroy();

		GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vs, nullptr);
		glCompileShader(vShader);
		print_shader_compile_status(vShader);

		GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fs, nullptr);
		glCompileShader(fShader);
		print_shader_compile_status(fShader);

		program = glCreateProgram();
		glAttachShader(program, vShader);
		glAttachShader(program, fShader);
		glLinkProgram(program);
		print_shader_link_status(program);

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

	constexpr size_t MAX_VERTEX_SIZE = 8192;

	void GLRenderer::init(SDL_Window* window) {
		cleanup();

		context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);
		if (GL3W_OK != gl3wInit()) {
			fprintf(stderr, "Couldn't load OpenGL extensions!\n");
			return;
		}
		{
			int flags;
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &flags);
			if (flags & SDL_GL_CONTEXT_DEBUG_FLAG)
			{
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(print_debug_output, nullptr);
				glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			}
		}

		quadVertices.len = MAX_VERTEX_SIZE;
		quadVertices.data = static_cast<Vertex*>(malloc(MAX_VERTEX_SIZE * sizeof(Vertex)));

		{	// init buffers and vertex attributes
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			constexpr GLsizei STRIDE = sizeof(Vertex);
			glEnableVertexAttribArray(0); // vec3 pos;
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, 0);

			glEnableVertexAttribArray(1); // vec2 uv;
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 3));

			glEnableVertexAttribArray(2); // vec4 color;
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, STRIDE, (const void*)(sizeof(f32) * 5));
		}

		shader.compile(main_vertexSource, main_fragSource);
	}

	void GLRenderer::cleanup() {
		if (context) {
			shader.destroy();

			glDeleteBuffers(1, &vbo);
			vbo = 0;

			glDeleteVertexArrays(1, &vao);
			vao = 0;

			SDL_GL_DestroyContext(context);
			context = nullptr;
		}

		if (quadVertices.data) {
			free(quadVertices.data);
			quadVertices.data = nullptr;
		}

		quadVertices.len = 0;
		numVertices = 0;
	}

	void GLRenderer::render_quads(bool flush) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindVertexArray(vao);
		glUseProgram(shader);

		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		if (flush)
			flush_batch();
	}

	void GLRenderer::clear(const Color& clearColor) {
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClearDepthf(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLRenderer::swap_screen(SDL_Window* window) {
		SDL_GL_SwapWindow(window);
	}

}