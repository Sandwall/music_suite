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

		fontAtlas = gfx::FontAtlas::load(512, 512, &fontLoadInfo, 1);
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
			} break;
			case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
				const gfx::Color color = from_clay_color(renderCommand.renderData.image.backgroundColor);
				const u32 texId = reinterpret_cast<u32>(renderCommand.renderData.image.imageData);

				if (texId > textureAtlas.regions.len)
					break;

				// want to draw transparent objects on top of everything currently opaque
				if (color.a < 1.0)
					renderer.render();

				if (color.a > 0.0)
					renderer.add_rect(box.x, box.y, box.width, box.height, texId, color);
			} break;
			case CLAY_RENDER_COMMAND_TYPE_TEXT: {
				// TODO: this is mega broken... need to figure out how to correctly size chars
				const gfx::Color color = from_clay_color(renderCommand.renderData.text.textColor);
				if (color.a == 0.0) break;

				const Clay_StringSlice& text = renderCommand.renderData.text.stringContents;
				const u16 id = renderCommand.renderData.text.fontId;
				const u16 fontSize = renderCommand.renderData.text.fontSize;
				const u16 spacing = renderCommand.renderData.text.letterSpacing;
				const u16 lineHeight = renderCommand.renderData.text.lineHeight;

				if (id >= fontAtlas.numFonts)
					break;

				f32 cursorX = box.x;
				f32 cursorY = box.y;

				// want to draw transparent objects on top of everything currently opaque
				if (color.a < 1.0)
					renderer.render();

				const f32 scale = 1.0f / static_cast<f32>(fontAtlas.oversampling);
				//gfx::FontAtlas::FontMetrics& metrics = fontAtlas.metadata[id];
				

				// TODO: We still need to find a way to guarantee the height of the font is overall fontSize
				// The behaviour doesn't seem too uniform...
				// at least I'll have to fix the scale factor calculation above
				// and also multiply xadvance by something like that 
				f32 maxGlyphHeight = 0.0f;
				for (i32 i = 0; i < text.length; i++) {
					char current = text.chars[i];
					if (current >= gfx::FontAtlas::CHARS_PER_FONT)
						continue;

					if (current == '\n') {
						cursorX = box.x;
						cursorY += maxGlyphHeight + lineHeight;
						maxGlyphHeight = 0.0f;
					} else {
						const stbtt_packedchar& packedChar = fontAtlas.packedChars.get(current, id);
						f32 width = static_cast<f32>(packedChar.x1 - packedChar.x0) * scale;
						f32 height = static_cast<f32>(packedChar.y1 - packedChar.y0) * scale;
						maxGlyphHeight = tim::max(maxGlyphHeight, height);

						renderer.add_char(
							cursorX + (static_cast<f32>(packedChar.xoff) * scale),
							cursorY + (static_cast<f32>(packedChar.yoff) * scale),
							width, height, id, current, color);
						
						cursorX += spacing + (packedChar.xadvance);
					}
				}

			} break;
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
				// No need to render when starting a scissor, since rendering is done on scissor end
				//renderer.render();

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