#include "window.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

namespace gfx {
	void Window::create(int w, int h) {
		if (open)
			destroy();

		width = w;
		height = h;
		mouseX = 0.0f; mouseY = 0.0f;
		wheelX = 0.0f; wheelY = 0.0f;


		{
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

			int contextFlags;
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
			contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
		}

		window = SDL_CreateWindow("music_suite", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		SDL_SetWindowMinimumSize(window, 800, 600);
		SDL_ShowWindow(window);

		open = true;
	}

	void Window::destroy() {
		SDL_DestroyWindow(window);
		open = false;
	}

	void Window::eat_events() {
		dt = 1.0f / 60.0f; // TODO: actually calculate dt

		SDL_Event ev;
		while (0 != SDL_PollEvent(&ev)) {
			switch (ev.type) {

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
				open = false;
			} break;
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
				width = ev.window.data1;
				height = ev.window.data2;
			} break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN: {
				switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					mouseL = true;
					break;
				case SDL_BUTTON_MIDDLE:
					mouseM = true;
					break;
				case SDL_BUTTON_RIGHT:
					mouseR = true;
					break;
				}
			} break;
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					mouseL = false;
					break;
				case SDL_BUTTON_MIDDLE:
					mouseM = false;
					break;
				case SDL_BUTTON_RIGHT:
					mouseR = false;
					break;
				}
			} break;
			case SDL_EVENT_MOUSE_MOTION: {
				mouseX = ev.motion.x;
				mouseY = ev.motion.y;
			} break;
			case SDL_EVENT_MOUSE_WHEEL: {
				wheelX = ev.wheel.x;
				wheelY = ev.wheel.y;
			} break;

			}
		}

	}
}