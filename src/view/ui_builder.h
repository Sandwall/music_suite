#pragma once

#include <clay.h>

namespace gfx {
	struct Window;
}

namespace view {

/* UI Builder
 * A nice way of 
*/

	struct UiBuilder {
		void init(const gfx::Window& window);
		void cleanup();

		// takes in program state, uses clay to layout the ui elements
		Clay_RenderCommandArray layout(const gfx::Window& window, void* prg);


	};
}