#pragma once

#include <clay.h>

#include "theme.hpp"
#include "gfx/texture_atlas.h"
#include "gfx/base_renderer.h"

namespace gfx {
	struct Window;
}

namespace view {
	struct UiBuilder {
		void init(const gfx::Window& window);
		void cleanup();

		// takes in program state, uses clay to layout the ui elements
		Clay_RenderCommandArray layout(const gfx::Window& window, const Theme& theme, void* prg);
		void load_textures(gfx::BakedAtlas::Packer& packer);
		void set_fonts(gfx::FontAtlas& fontAtlas);

	private:
		// the UI elements should only ever be called from the layout function since this pointer is only valid there
		const Theme* currentTheme;
		
		// these are functions that create reusable UI elements
		bool button(const char* label);
		void playbar();
	};
}