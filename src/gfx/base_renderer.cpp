#include "base_renderer.h"

#include <assert.h>

namespace gfx {
	void Renderer::start_frame(f32 width, f32 height, BakedAtlas* tAtlas, FontAtlas* fAtlas) {
		targetWidth = width;
		targetHeight = height;

		textureAtlasRef = tAtlas;
		fontAtlasRef = fAtlas;

		numDrawnObjects = 0;
		flush_batch();
	}

	void Renderer::flush_batch() {
		numQuadsAdded = 0;
		numCharsAdded = 0;
	}

	// assumes value is in [0, range]
	f32 to_ndc(f32 value, f32 range) {
		return ((value / range) - 0.5f) * 2.0f;
	}

	void Renderer::add_tex(f32 x, f32 y, f32 w, f32 h, i32 texIdx, const Color& color) {
		assert((numQuadsAdded * 4) < (quadVertices.len - 4));

		Vertex* current = &(quadVertices[(numQuadsAdded++) * 4]);
		f32 z = -to_ndc(static_cast<f32>(numDrawnObjects), static_cast<f32>(quadVertices.len + textVertices.len));	// -z forward

		Rect uvRect = { 0.0f, 0.0f, 1.0f, 1.0f };
		if (textureAtlasRef) {
			uvRect = textureAtlasRef->regions[texIdx];
		}

		// TL
		current->x = to_ndc(x, targetWidth);
		current->y = -to_ndc(y, targetHeight);
		current->z = z;
		current->u = uvRect.x;
		current->v = uvRect.y;
		current->color = color;

		current++;

		// BL
		current->x = to_ndc(x, targetWidth);
		current->y = -to_ndc(y + h, targetHeight);
		current->z = z;
		current->u = uvRect.x;
		current->v = uvRect.y + uvRect.h;
		current->color = color;

		current++;

		// BR
		current->x = to_ndc(x + w, targetWidth);
		current->y = -to_ndc(y + h, targetHeight);
		current->z = z;
		current->u = uvRect.x + uvRect.w;
		current->v = uvRect.y + uvRect.h;
		current->color = color;

		current++;

		// TR
		current->x = to_ndc(x + w, targetWidth);
		current->y = -to_ndc(y, targetHeight);
		current->z = z;
		current->u = uvRect.x + uvRect.w;
		current->v = uvRect.y;
		current->color = color;

		numDrawnObjects++;
	}

	void Renderer::add_rect(f32 x, f32 y, f32 w, f32 h, const Color& color) {
		// NOTE!!! We require that a white pixel texture be loaded in slot 0
		add_tex(x, y, w, h, 0, color);
	}

	void Renderer::add_char(f32 x, f32 y, f32 w, f32 h, const stbtt_packedchar& packedChar, const Color& color) {
		assert((numCharsAdded * 4) < (textVertices.len - 4));

		Vertex* current = &(textVertices[(numCharsAdded++) * 4]);
		f32 z = -to_ndc(static_cast<f32>(numDrawnObjects), static_cast<f32>(quadVertices.len + textVertices.len));	// -z forward

		if (!fontAtlasRef) return;

		const Rect uvs = {
			.x0 = static_cast<f32>(packedChar.x0) / static_cast<f32>(fontAtlasRef->bitmap.width),
			.y0 = static_cast<f32>(packedChar.y0) / static_cast<f32>(fontAtlasRef->bitmap.height),
			.x1 = static_cast<f32>(packedChar.x1) / static_cast<f32>(fontAtlasRef->bitmap.width),
			.y1 = static_cast<f32>(packedChar.y1) / static_cast<f32>(fontAtlasRef->bitmap.height),
		};

		// TL
		current->x = to_ndc(x, targetWidth);
		current->y = -to_ndc(y, targetHeight);
		current->z = z;
		current->u = uvs.x0;
		current->v = uvs.y0;
		current->color = color;

		current++;

		// BL
		current->x = to_ndc(x, targetWidth);
		current->y = -to_ndc(y + h, targetHeight);
		current->z = z;
		current->u = uvs.x0;
		current->v = uvs.y1;
		current->color = color;

		current++;

		// BR
		current->x = to_ndc(x + w, targetWidth);
		current->y = -to_ndc(y + h, targetHeight);
		current->z = z;
		current->u = uvs.x1;
		current->v = uvs.y1;
		current->color = color;

		current++;

		// TR
		current->x = to_ndc(x + w, targetWidth);
		current->y = -to_ndc(y, targetHeight);
		current->z = z;
		current->u = uvs.x1;
		current->v = uvs.y0;
		current->color = color;

		numDrawnObjects++;
	}
}

