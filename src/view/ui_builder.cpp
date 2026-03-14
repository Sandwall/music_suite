#include "ui_builder.h"
#include "gfx/window.h"

#include <clay.h>

#include <tinydef.hpp>

#include <stdio.h>

namespace view {
	void handle_clay_errors(Clay_ErrorData errorData) {
		// See the Clay_ErrorData struct for more information
		fprintf(stderr, "%s", errorData.errorText.chars);
		switch (errorData.errorType) {
			// etc
		}
	}

	void UiBuilder::init(const gfx::Window& window) {
		u32 totalMemorySize = Clay_MinMemorySize();
		Clay_Arena clayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
		Clay_Initialize(clayArena, Clay_Dimensions { (f32)window.width, (f32)window.height }, Clay_ErrorHandler {handle_clay_errors});
	}

	void UiBuilder::cleanup() {

	}

	Clay_RenderCommandArray UiBuilder::layout(const gfx::Window& window, void* prg) {
		f32 winWidth = static_cast<f32>(window.width), winHeight = static_cast<f32>(window.height);
		Clay_SetCullingEnabled(true);
		Clay_SetLayoutDimensions(Clay_Dimensions{ winWidth, winHeight });
		Clay_SetPointerState(Clay_Vector2{ window.mouseX, window.mouseY }, window.mouseL);
		Clay_UpdateScrollContainers(true, Clay_Vector2{ window.wheelX, window.wheelY }, window.dt);

		Clay_BeginLayout();

		CLAY({
			.id = CLAY_ID("OuterContainer"),
			.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16},
			.backgroundColor = {250,250,255,255}
			}) {
		}

		return Clay_EndLayout();
	}
}