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
	gl_Position = vec4(inPos, 1.0);
	uv = inUv;
	vertexCol = inCol;
}
)";

const char* main_fragSource = /* fragment shader */ R"(
#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec4 vertexCol;

void main() {
	vec2 user = uv;
	user.x = 1.0;
	user.y *= uv.x * user.x;
	FragColor = vertexCol;
}
)";

namespace gfx {
	
	//
	// HELPERS
	//

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
	
	//
	// GL SHADER
	//

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

	//
	// GL RENDERER
	//


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
			if (flags & SDL_GL_CONTEXT_DEBUG_FLAG) {
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(print_debug_output, nullptr);
				glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			}
		}

		// allocate cpu size buffers
		quadVertices = tds::Slice<Vertex>::alloc(VERTICES_CAPACITY);
		textVertices = tds::Slice<Vertex>::alloc(VERTICES_CAPACITY);

		{	// init buffers and vertex attributes
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, VERTICES_ALLOC_SIZE, nullptr, GL_DYNAMIC_DRAW);

			glGenBuffers(1, &ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			tds::Slice<u32> quadIndices = tds::Slice<u32>::alloc(INDICES_CAPACITY);
			for (u32 i = 0; i < quadIndices.len / 6; i++) {
				// Assuming vertices are laid out in TL BL BR TR order
				const u32 firstIndexIndex = i * 6;
				const u32 firstVertexIndex = i * 4;

				quadIndices[firstIndexIndex + 0] = firstVertexIndex + 0;
				quadIndices[firstIndexIndex + 1] = firstVertexIndex + 1;
				quadIndices[firstIndexIndex + 2] = firstVertexIndex + 2;
				quadIndices[firstIndexIndex + 3] = firstVertexIndex + 0;
				quadIndices[firstIndexIndex + 4] = firstVertexIndex + 2;
				quadIndices[firstIndexIndex + 5] = firstVertexIndex + 3;
			}
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDICES_ALLOC_SIZE, quadIndices.data, GL_STATIC_DRAW);
			tds::Slice<u32>::free(quadIndices);

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

			glDeleteBuffers(1, &ibo);
			ibo = 0;

			glDeleteVertexArrays(1, &vao);
			vao = 0;

			SDL_GL_DestroyContext(context);
			context = nullptr;
		}

		tds::Slice<Vertex>::free(quadVertices);
		tds::Slice<Vertex>::free(textVertices);
		
		numQuadsAdded = 0;
		numCharsAdded = 0;
	}

	void GLRenderer::render_quads(bool flush) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBindVertexArray(vao);
		glUseProgram(shader);

		glBufferSubData(GL_ARRAY_BUFFER, 0, numQuadsAdded * 4 * sizeof(Vertex), quadVertices.data);

		glDrawElements(GL_TRIANGLES, numQuadsAdded * 6, GL_UNSIGNED_INT, nullptr);

		if (flush)
			flush_batch();
	}

	void GLRenderer::clear(const Color& clearColor) {
		glViewport(0, 0, targetWidth, targetHeight);
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClearDepthf(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLRenderer::swap_screen(SDL_Window* window) {
		SDL_GL_SwapWindow(window);
	}

}