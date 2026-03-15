#include <SDL3/SDL.h>

#include "gfx/gl_renderer.h"
#include "gfx/window.h"
#include "view/ui_builder.h"

#include <clay.h>

#include <stdio.h>

struct Theme {
	gfx::Color background = { 0.0, 0.0, 0.0, 1.0 };
} currentTheme;

gfx::Color from_clay_color(const Clay_Color& color) {
	return gfx::Color{
		.r = color.r / 255.0f,
		.g = color.g / 255.0f,
		.b = color.b / 255.0f,
		.a = color.a / 255.0f,
	};
}

void handle_clay_errors(Clay_ErrorData errorData) {
	// See the Clay_ErrorData struct for more information
	printf("%s", errorData.errorText.chars);
	switch (errorData.errorType) {
		// etc
		break;
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

int main(int argc, char** argv) {
	mem::init();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	gfx::Window window = { 0 };
	window.create(1280, 720);

	// renderer init
	gfx::GLRenderer renderer = {};
	renderer.init(window.window);

	// clay init
	view::UiBuilder uiBuilder = {};
	uiBuilder.init(window);

	while (window.open) {
		window.eat_events();
		if (!window.open) break;

		// main render
		Clay_RenderCommandArray renderCommands = uiBuilder.layout(window, nullptr);

		renderer.start_frame(static_cast<f32>(window.width), static_cast<f32>(window.height));
		renderer.clear(currentTheme.background);

		for (int i = 0; i < renderCommands.length; i++) {
			Clay_RenderCommand* renderCommand = &renderCommands.internalArray[i];
			const Clay_BoundingBox& box = renderCommand->boundingBox;

			switch (renderCommand->commandType) {
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
				const Clay_Color& color = renderCommand->renderData.rectangle.backgroundColor;

				// TODO: the clay example doesn't render properly, it's a bit weird, ned to fix
				renderer.add_rect(box.x, box.y, box.width, box.height, from_clay_color(color));
			} break;
			case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
				const Clay_Color& color = renderCommand->renderData.image.backgroundColor;
				const void* data = renderCommand->renderData.image.imageData;

				// render all available stuff so that we can switch textures
				renderer.render_quads();

				// TODO:
			} break;
			case CLAY_RENDER_COMMAND_TYPE_TEXT: {
				const Clay_Color& color = renderCommand->renderData.text.textColor;
				const Clay_StringSlice& str = renderCommand->renderData.text.stringContents;
				const u16 id = renderCommand->renderData.text.fontId;
				const u16 size = renderCommand->renderData.text.fontSize;
				const u16 spacing = renderCommand->renderData.text.letterSpacing;
				const u16 height = renderCommand->renderData.text.lineHeight;

				// TODO:

			} break;
			}
		}

		renderer.render_quads();
		renderer.swap_screen(window.window);

	}

	uiBuilder.cleanup();
	renderer.cleanup();
	window.destroy();
	SDL_Quit();

	return 0;
}