#include <SDL3/SDL.h>

#include "gfx/draw_batch.h"

int main(int argc, char** argv) {
	gfx::startup();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	SDL_Window* window = SDL_CreateWindow("music_suite", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_ShowWindow(window);

	bool keepOpen = true;
	while (keepOpen) {
		SDL_Event ev;
		while (0 != SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
				keepOpen = false;
			} break;
			}
		}

		if (!keepOpen) break;


	}

	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}