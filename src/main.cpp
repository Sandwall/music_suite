#include <SDL3/SDL.h>

#include "gfx/texture_atlas.h"
#include "gfx/gl_renderer.h"
#include "gfx/window.h"
#include "view/theme.hpp"
#include "view/ui_builder.h"

#include <clay.h>

#include <stdio.h>

Theme currentTheme;

static gfx::Color from_clay_color(const Clay_Color& color) {
	return gfx::Color{
		.r = color.r / 255.0f,
		.g = color.g / 255.0f,
		.b = color.b / 255.0f,
		.a = color.a / 255.0f,
	};
}

int main(int argc, char** argv) {
	mem::init();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	gfx::Window window = { 0 };
	window.create(1280, 720);

	// renderer init
	gfx::GLRenderer renderer = { };
	renderer.init(window.window);

	// clay init
	view::UiBuilder uiBuilder = {};
	uiBuilder.init(window);

	// load fonts and image assets
	gfx::BakedAtlas textureAtlas;
	{
		gfx::BakedAtlas::Packer packer = { 0 };
		packer.start();
		packer.add_tex("./res/white_pix.png");
		uiBuilder.load_textures(packer);

		textureAtlas = packer.bake(512, 512, 1);
	}

	gfx::FontAtlas fontAtlas;
	{
		gfx::FontAtlas::LoadInfo fontLoadInfo = {
			.path = "./res/DMSans-Regular.ttf",
			.fontHeight = 12.0f,
		};

		fontAtlas = gfx::FontAtlas::load(1024, 1024, &fontLoadInfo, 1);
		uiBuilder.set_fonts(fontAtlas);
	}

	renderer.create_textures(textureAtlas, fontAtlas);

	while (window.open) {
		window.eat_events();
		if (!window.open) break;

		Clay_RenderCommandArray renderCommands = uiBuilder.layout(window, currentTheme, nullptr);
		renderer.start_frame(static_cast<f32>(window.width), static_cast<f32>(window.height), &textureAtlas, &fontAtlas);
		renderer.clear(currentTheme.background);

		for (int i = 0; i < renderCommands.length; i++) {
			const Clay_RenderCommand& renderCommand = renderCommands.internalArray[i];
			const Clay_BoundingBox& box = renderCommand.boundingBox;

			switch (renderCommand.commandType) {
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
				const gfx::Color color = from_clay_color(renderCommand.renderData.rectangle.backgroundColor);

				// want to draw transparent objects on top of everything currently opaque
				if (color.a < 1.0)
					renderer.render();

				if (color.a > 0.0)
					renderer.add_rect(box.x, box.y, box.width, box.height, color);
			} break;
			case CLAY_RENDER_COMMAND_TYPE_BORDER: {
				const gfx::Color color = from_clay_color(renderCommand.renderData.border.color);

				// as of right now if we have transparent borders, then the corner pixels will probably be 
				if (color.a < 1.0)
					renderer.render();

				if (color.a > 0.0) {
					const f32 left = static_cast<f32>(renderCommand.renderData.border.width.left);
					const f32 right = static_cast<f32>(renderCommand.renderData.border.width.left);
					const f32 top = static_cast<f32>(renderCommand.renderData.border.width.left);
					const f32 bottom = static_cast<f32>(renderCommand.renderData.border.width.left);

					renderer.add_rect(box.x, box.y, left, box.height, color);
					renderer.add_rect(box.x + box.width - right, box.y, right, box.height, color);
					renderer.add_rect(box.x, box.y, box.width, top, color);
					renderer.add_rect(box.x, box.y + box.height - bottom, box.width, bottom, color);
				}

			} break;
			case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
				const gfx::Color color = from_clay_color(renderCommand.renderData.image.backgroundColor);
				const u32 texId = reinterpret_cast<const u32>(renderCommand.renderData.image.imageData);

				if (texId > textureAtlas.regions.len)
					break;

				// want to draw transparent objects on top of everything currently opaque
				if (color.a < 1.0)
					renderer.render();

				if (color.a > 0.0)
					renderer.add_tex(box.x, box.y, box.width, box.height, texId, color);
			} break;
			case CLAY_RENDER_COMMAND_TYPE_TEXT: {
				const gfx::Color color = from_clay_color(renderCommand.renderData.text.textColor);
				if (color.a == 0.0) break;

				const Clay_StringSlice& text = renderCommand.renderData.text.stringContents;
				const u16 id = renderCommand.renderData.text.fontId;
				const u16 fontSize = renderCommand.renderData.text.fontSize;
				const u16 spacing = renderCommand.renderData.text.letterSpacing;
				const u16 lineHeight = renderCommand.renderData.text.lineHeight;

				if (id >= fontAtlas.numFonts)
					break;


				// want to draw transparent objects on top of everything currently opaque
				if (color.a < 1.0)
					renderer.render();

				gfx::FontAtlas::FontMetrics& metrics = fontAtlas.metadata[id];
				const f32 fontScale = static_cast<f32>(fontSize) / metrics.loadedFontSize;
				f32 cursorX = box.x;
				f32 cursorY = box.y + (static_cast<f32>(metrics.ascent) * fontScale * metrics.vMetricsScale);
				// adding this to account for the baseline

				for (i32 i = 0; i < text.length; i++) {
					char current = text.chars[i];
					if (current >= gfx::FontAtlas::CHARS_PER_FONT)
						continue;

					if (current == '\n') {
						cursorX = box.x;
						cursorY += fontSize + lineHeight;
					} else {
						const stbtt_packedchar& packedChar = fontAtlas.packedChars.get(current, id);
						gfx::Rect quad = {
							.x0 = cursorX + (fontScale * packedChar.xoff),
							.y0 = cursorY + (fontScale * packedChar.yoff),
							.x1 = cursorX + (fontScale * packedChar.xoff2),
							.y1 = cursorY + (fontScale * packedChar.yoff2)
						};

						cursorX += static_cast<f32>(spacing) + (packedChar.xadvance * fontScale);

						renderer.add_char(
							quad.x0, quad.y0,
							quad.x1 - quad.x0, quad.y1 - quad.y0,
							packedChar,
							color);
					}
				}

			} break;
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
				// No need to render when starting a scissor, since rendering is done on scissor end
				renderer.render();
				renderer.scissor((i32)box.x, (i32)box.y, (i32)box.width, (i32)box.height);
			} break;
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
				renderer.render();
				renderer.scissor(0, 0, window.width, window.height);
			} break;
			}
		}

		renderer.render();
		renderer.swap_screen(window.window);

	}

	uiBuilder.cleanup();
	renderer.cleanup();
	window.destroy();
	SDL_Quit();

	mem::close();

	return 0;
}