#include "gl_renderer.h"

#include "draw_batch.h"

#include <GL/gl3w.h>
#include <SDL3/SDL_opengl.h>

namespace gfx {
	void GLRenderer::init(SDL_Window* window) {
		cleanup();

		context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, context);
	}

	void GLRenderer::cleanup() {
		if (context != nullptr) {
			SDL_GL_DestroyContext(context);
			context = nullptr;
		}
	}

	void GLRenderer::render(DrawBatch& batch, bool flush) {
		

		if (flush)
			batch.flush();
	}
}