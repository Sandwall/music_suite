#pragma once

#include <stb/stb_truetype.h>

#include <tinydef.hpp>
#include "common.hpp"

namespace gfx {
	struct BakedAtlas {
		tds::Slice2<u32> bitmap;  // cpu side bitmap, stored as 32-bits per pixel (rgba)
		tds::Slice<Rect> regions; // rectangles to inside cpuAtlas (uses uv coordinates)

		static BakedAtlas alloc(i32 width, i32 height, i32 numRegions);
		static void dealloc(BakedAtlas& atlas);

		struct Packer {
			mem::Arena packerArena; // will be used to allocate anything Packer-related

			// contains information about image size, channels, data
			struct ImageData {
				i32 width, height, channels;
				u8* data;
			};
			using ImageNode = tds::LinkedNode<ImageData>;
			ImageNode* imagesList; // build this up in packerArena, then free it when baking
			u32 imagesLoaded;

			void start();
			i32 add_tex(const char* path);
			BakedAtlas bake(i32 width, i32 height, i32 padding);
		};
	};

	struct FontAtlas {
		static constexpr u32 CHARS_PER_FONT = 128;
		i32 numFonts;
		i32 oversamplingX;
		i32 oversamplingY;

		struct FontMetrics {
			f32 loadedFontSize;

			f32 vMetricsScale;
			i32 ascent;
			i32 descent;
			i32 lineGap;
		};

		tds::Slice2<u8> bitmap;                    // grayscale bitmap with all fonts inside
		tds::Slice2<stbtt_packedchar> packedChars; // X is indexed by char, Y is indexed by font
		tds::Slice<FontMetrics> metadata;

		struct LoadInfo {
			const char* path;
			f32 fontHeight; // in pixels
		};

		static FontAtlas load(i32 width, i32 height, LoadInfo* loadInfo, i32 numFonts);
		static void dealloc(FontAtlas& atlas);
	};

}