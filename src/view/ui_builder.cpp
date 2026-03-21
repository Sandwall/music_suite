#include "ui_builder.h"
#include "gfx/window.h"
#include "theme.hpp"

#include <clay.h>

#include <tinydef.hpp>

#include <stdio.h>

namespace ctrl {
	bool button(const char* contents) {
		return true;
	}

	void playbar(const Theme& theme) {
		CLAY({
			.id = CLAY_ID("PlayBarContainer"),
			.layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(64) } },
			.backgroundColor = {0, 0, 0, 200}, 
			}) {
		}
	}
}

namespace view {
	static void handle_clay_errors(Clay_ErrorData errorData) {
		fprintf(stderr, "Clay Error! %s\n\t", errorData.errorText.chars);
		switch (errorData.errorType) {
		case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_FUNCTION_NOT_PROVIDED:
			fprintf(stderr, "Measurement function not provided.\n");
			break;
		case CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED:
			fprintf(stderr, "Memory arena capacity exceeded.\n");
			break;
		case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED:
			fprintf(stderr, "UI element memory capacity exceeded.\n");
			break;
		case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED:
			fprintf(stderr, "Text measurement capacity exceeded.\n");
			break;
		case CLAY_ERROR_TYPE_DUPLICATE_ID:
			fprintf(stderr, "Duplicate UI element ID detected.\n");
			break;
		case CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND:
			fprintf(stderr, "Could not find parent of floating container.\n");
			break;
		case CLAY_ERROR_TYPE_PERCENTAGE_OVER_1:
			fprintf(stderr, "Percentage value over 1.0f was used.\n");
			break;
		case CLAY_ERROR_TYPE_INTERNAL_ERROR:
			fprintf(stderr, "Internal error encountered.\n");
			break;
		}
	}

	static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
		Clay_Dimensions dimensions = {
			.width = static_cast<float>(text.length * config->fontSize),
			.height = static_cast<float>(config->fontSize)
		};

		if (!userData)
			return dimensions;
		
		gfx::FontAtlas& fontAtlas = *reinterpret_cast<gfx::FontAtlas*>(userData);

		if (config->fontId >= fontAtlas.numFonts)
			return dimensions;

		f32 cursorX = 0, cursorY = config->fontSize + config->lineHeight;
		dimensions.width = cursorX;
		dimensions.height = cursorY;

		// TODO: I think this strategy might need a bit more nuance... it's 5am code...
		const f32 scale = 1.0f / static_cast<f32>(fontAtlas.oversampling);
		f32 maxGlyphHeight = 0.0f;
		for (i32 i = 0; i < text.length; i++) {
			char current = text.chars[i];
			if (current > gfx::FontAtlas::CHARS_PER_FONT)
				continue;

			if (current == '\n') {
				cursorX = 0.0f;
				cursorY += maxGlyphHeight + config->lineHeight;
				maxGlyphHeight = 0.0f;
			}
			else {
				const stbtt_packedchar& packedChar = fontAtlas.packedChars.get(current, config->fontId);
				f32 width = static_cast<f32>(packedChar.x1 - packedChar.x0) * scale;
				f32 height = static_cast<f32>(packedChar.y1 - packedChar.y0) * scale;
				maxGlyphHeight = tim::max(maxGlyphHeight, height);
				cursorX += config->letterSpacing + (packedChar.xadvance);
			}

			dimensions.width = tim::max(dimensions.width, cursorY);
			dimensions.height = tim::max(dimensions.height, cursorX);
		}

		return dimensions;
	}

	void UiBuilder::init(const gfx::Window& window) {
		u32 totalMemorySize = Clay_MinMemorySize();
		Clay_Arena clayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
		Clay_Initialize(clayArena, Clay_Dimensions { (f32)window.width, (f32)window.height }, Clay_ErrorHandler {handle_clay_errors});

	}

	void UiBuilder::load_textures(gfx::BakedAtlas::Packer& packer) {
		// TODO: can get virtual texture IDs in this
	}

	void UiBuilder::set_fonts(gfx::FontAtlas& fontAtlas) {
		Clay_SetMeasureTextFunction(measure_text, &fontAtlas);
	}

	void UiBuilder::cleanup() { }

	Clay_RenderCommandArray UiBuilder::layout(const gfx::Window& window, const Theme& theme, void* prg) {
		f32 winWidth = static_cast<f32>(window.width), winHeight = static_cast<f32>(window.height);
		Clay_SetCullingEnabled(true);
		Clay_SetLayoutDimensions(Clay_Dimensions{ winWidth, winHeight });
		Clay_SetPointerState(Clay_Vector2{ window.mouseX, window.mouseY }, window.mouseL);
		Clay_UpdateScrollContainers(true, Clay_Vector2{ window.wheelX, window.wheelY }, window.dt);

		Clay_BeginLayout();

		CLAY_TEXT(CLAY_STRING("Hello! This is some test text!\n I am now testing some newlines!!!"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 12 }));
		//CLAY({
		//	.id = CLAY_ID("OuterContainer"),
		//	.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16},
		//	.backgroundColor = {100, 100, 100, 255}
		//	}) {
		//}

		return Clay_EndLayout();
	}
}