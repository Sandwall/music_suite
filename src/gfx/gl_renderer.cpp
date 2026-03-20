#include "gl_renderer.h"

#include <GL/gl3w.h>
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

uniform sampler2D uTexture;

void main() {
	FragColor = texture(uTexture, uv) * vertexCol;
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
	// GL TEXTURE
	//

	void get_format_info(int internalFormat, int& dataFormat, int& dataType) {
		switch (internalFormat) {
		case GL_R8:
		case GL_R16:
		case GL_R16F:
		case GL_R32F:
			dataFormat = GL_RED;
			break;
		case GL_RG8:
		case GL_RG16:
		case GL_RG16F:
		case GL_RG32F:
			dataFormat = GL_RG;
			break;
		case GL_RGB8:
		case GL_RGB16:
		case GL_RGB16F:
		case GL_RGB32F:
			dataFormat = GL_RGB;
			break;
		case GL_RGBA8:
		case GL_RGBA16:
		case GL_RGBA16F:
		case GL_RGBA32F:
			dataFormat = GL_RGBA;
			break;
		}

		switch (internalFormat) {
		case GL_R8:
		case GL_RG8:
		case GL_RGB8:
		case GL_RGBA8:
			dataType = GL_UNSIGNED_BYTE;
			break;
		case GL_R16:
		case GL_RG16:
		case GL_RGB16:
		case GL_RGBA16:
			dataType = GL_UNSIGNED_SHORT;
			break;
		case GL_R16F:
		case GL_RG16F:
		case GL_RGB16F:
		case GL_RGBA16F:
			dataType = GL_HALF_FLOAT;
			break;
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
			dataType = GL_FLOAT;
			break;
		}
	}

	void GLTexture::create(i32 w, i32 h, i32 format, const void* data) {
		destroy();

		width = w;
		height = h;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int dataFormat, dataType;
		get_format_info(format, dataFormat, dataType);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, dataFormat, dataType, data);
		glBindTexture(GL_TEXTURE_2D, 0);

		loaded = true;
	}

	void GLTexture::destroy() {
		width = 0;
		height = 0;
		loaded = false;

		if (texture) {
			glDeleteTextures(1, &texture);
			texture = 0;
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
		textureUniformLoc = glGetUniformLocation(shader.program, "uTexture");
	}

	void GLRenderer::create_textures(const BakedAtlas& bakedAtlas, const FontAtlas& fontAtlas) {
		mainTexture.create(bakedAtlas.bitmap.width, bakedAtlas.bitmap.height, GL_RGBA8, (void*)bakedAtlas.bitmap.data);
		fontTexture.create(fontAtlas.bitmap.width, fontAtlas.bitmap.height, GL_R8, (void*)bakedAtlas.bitmap.data);
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

	void GLRenderer::render(bool flush) {
		// set state
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBindVertexArray(vao);
		glUseProgram(shader);
		
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(textureUniformLoc, 0);

		// draw quads
		if (numQuadsAdded > 0) {
			glBindTexture(GL_TEXTURE_2D, mainTexture.texture);
			glBufferSubData(GL_ARRAY_BUFFER, 0, numQuadsAdded * 4 * sizeof(Vertex), quadVertices.data);
			glDrawElements(GL_TRIANGLES, numQuadsAdded * 6, GL_UNSIGNED_INT, nullptr);
		}

		// draw text
		if (numCharsAdded > 0) {
			glBindTexture(GL_TEXTURE_2D, fontTexture.texture);
			glBufferSubData(GL_ARRAY_BUFFER, 0, numCharsAdded * 4 * sizeof(Vertex), textVertices.data);
			glDrawElements(GL_TRIANGLES, numCharsAdded * 6, GL_UNSIGNED_INT, nullptr);
		}

		if (flush)
			flush_batch();
	}

	void GLRenderer::scissor(i32 x, i32 y, i32 w, i32 h) {
		glScissor(x, y, w, h);
	}

	void GLRenderer::clear(const Color& clearColor) {
		// TODO: decide if this function is the best place for this
		glViewport(0, 0, static_cast<GLsizei>(targetWidth), static_cast<GLsizei>(targetHeight));

		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClearDepthf(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLRenderer::swap_screen(SDL_Window* window) {
		SDL_GL_SwapWindow(window);
	}

}