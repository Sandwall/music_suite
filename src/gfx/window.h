#pragma once

/* Window
 * ======
 * 
 * Despite being called "Window", we'll be using this struct to aggregate all information the program needs from the Window
 * This includes event handling
 * 
*/

struct SDL_Window;

namespace gfx {
	struct Window {
		SDL_Window* window;

		bool open = false;

		// mouse button down
		bool mouseL, mouseM, mouseR;

		int width, height;
		float mouseX, mouseY;
		float wheelX, wheelY;
		float dt;

		void create(int w, int h);
		void destroy();

		// Make sure to call this each frame.
		// It populates the state of this struct + calculates deltaTime
		void eat_events();

		operator SDL_Window* () const {
			return window;
		}
	};
}