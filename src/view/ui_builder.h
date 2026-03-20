#pragma once

#include <clay.h>

#include "theme.hpp"
#include "gfx/texture_atlas.h"

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
		Theme* currentTheme;
		// these are functions 
	};
}