#include <SDL3/SDL.h>

#include "gfx/draw_batch.h"
#include "gfx/gl_renderer.h"
#include "gfx/window.h"
#include <clay.h>

#include <stdio.h>

const Clay_Color COLOR_LIGHT = Clay_Color{ 224, 215, 210, 255 };
const Clay_Color COLOR_RED = Clay_Color{ 168, 66, 28, 255 };
const Clay_Color COLOR_ORANGE = Clay_Color{ 225, 138, 50, 255 };

gfx::Color from_clay_color(const Clay_Color& color) {
	return gfx::Color{
		.r = static_cast<u8>(color.r * 255.0f),
		.g = static_cast<u8>(color.g * 255.0f),
		.b = static_cast<u8>(color.b * 255.0f),
		.a = static_cast<u8>(color.a * 255.0f),
	};
}

void HandleClayErrors(Clay_ErrorData errorData) {
	// See the Clay_ErrorData struct for more information
	printf("%s", errorData.errorText.chars);
	switch (errorData.errorType) {
		// etc
	}
}

// Example measure text function
static inline Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, uintptr_t userData) {
	// Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing etc
	// Note: Clay_String->chars is not guaranteed to be null terminated
	return Clay_Dimensions {
		.width = static_cast<float>(text.length * config->fontSize), // <- this will only work for monospace fonts, see the renderers/ directory for more advanced text measurement
		.height = static_cast<float>(config->fontSize)
	};
}

// Layout config is just a struct that can be declared statically, or inline
Clay_ElementDeclaration sidebarItemConfig = {
	.layout = {
		.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) }
	},
	.backgroundColor = COLOR_ORANGE
};

int main(int argc, char** argv) {
	gfx::startup();

	//
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	gfx::Window window;
	window.create(1280, 720);

	// renderer init
	gfx::GL_BatchRenderer renderer;
	renderer.init(window);


	// clay init
	u32 totalMemorySize = Clay_MinMemorySize();
	Clay_Arena clayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

	// main loop
	while (window.open) {
		window.eat_events();
		if (!window.open) break;

		// main render
		f32 winWidth = static_cast<f32>(window.width), winHeight = static_cast<f32>(window.height);
		Clay_SetLayoutDimensions(Clay_Dimensions{ winWidth, winHeight });
		Clay_SetPointerState(Clay_Vector2 { window.mouseX, window.mouseY }, window.mouseL);
		Clay_UpdateScrollContainers(true, Clay_Vector2 { window.wheelX, window.wheelY }, window.dt);

		Clay_BeginLayout();

		// An example of laying out a UI with a fixed width sidebar and flexible width main content
		CLAY(CLAY_ID("OuterContainer"), { .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16 }, .backgroundColor = {250,250,255,255} }) {
			CLAY(CLAY_ID("SideBar"), {
				.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16 },
				.backgroundColor = COLOR_LIGHT
				}) {
				CLAY(CLAY_ID("ProfilePictureOuter"), { .layout = {.sizing = {.width = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16, .childAlignment = {.y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = COLOR_RED }) {
					CLAY(CLAY_ID("ProfilePicture"), { .layout = {.sizing = {.width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }}, .image = {.imageData = &profilePicture } }) {}
					CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
				}

				CLAY(CLAY_ID("MainContent"), { .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = COLOR_LIGHT }) {}
			}
		}

		Clay_RenderCommandArray renderCommands = Clay_EndLayout();
		renderer.batch.start_frame(winWidth, winHeight);

		// More comprehensive rendering examples can be found in the renderers/ directory
		for (int i = 0; i < renderCommands.length; i++) {
			Clay_RenderCommand* renderCommand = &renderCommands.internalArray[i];

			switch (renderCommand->commandType) {
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
				const Clay_BoundingBox& box = renderCommand->boundingBox;
				const Clay_Color& color = renderCommand->renderData.rectangle.backgroundColor;

				renderer.batch.add_rect(box.x, box.y, box.width, box.height, from_clay_color(color));
			} break;
			}
		}


		renderer.clear();
		renderer.render();
		renderer.swap_screen(window);

	}

	renderer.cleanup();
	window.destroy();
	SDL_Quit();

	return 0;
}