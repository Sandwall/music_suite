#include "texture_atlas.h"

#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>
#include <SDL3/SDL_filesystem.h>

#include <stdio.h>
#include <stdlib.h>

#include <tinydef.hpp>

namespace gfx {
	//
	// BAKED ATLAS
	//

	BakedAtlas BakedAtlas::alloc(i32 width, i32 height, i32 numRegions) {
		BakedAtlas atlas = { 0 };
		atlas.bitmap = tds::Slice2<u32>::alloc0(width, height);
		atlas.regions = tds::Slice<Rect>::alloc0(numRegions);
		return atlas;
	}

	void BakedAtlas::dealloc(BakedAtlas& atlas) {
		tds::Slice2<u32>::free(atlas.bitmap);
		tds::Slice<Rect>::free(atlas.regions);
	}

	//
	// BAKED ATLAS PACKER
	//

	void BakedAtlas::Packer::start() {
		memset(this, 0, sizeof(BakedAtlas::Packer));

		stbi_set_flip_vertically_on_load(true);

		assert(packerArena.capacity == 0);
		packerArena.alloc();
	}

	i32 BakedAtlas::Packer::add_tex(const char* path) {
		if (!SDL_GetPathInfo(path, nullptr)) return -1;

		ImageData imageData;
		imageData.data = stbi_load(path, &imageData.width, &imageData.height, &imageData.channels, 0);
		if (!imageData.data) return -1;

		// push_zero ensures node->next == nullptr
		// this should mean to traverse the list, we can just follow next until we see next is nullptr
		// ...basic stuff...
		ImageNode* node = packerArena.push_zero<ImageNode>();
		node->data = imageData;

		// append node to front of imagesList
		if (imagesList) {
			node->next = imagesList;
		} imagesList = node;

		return imagesLoaded++;
	}

	// releases most of the packing state and copies all of the images to a baked atlas
	BakedAtlas BakedAtlas::Packer::bake(i32 width, i32 height) {
		mem::Arena& scratch = mem::get_scratch();
		mem::ArenaScope scope(scratch);

		// count rects in linked list & check if rects will fit in atlas
		i32 count = 0;
		{	// placing this in a block so that sumOfAllImageAreas is inaccessable when we don't need it
			i32 sumOfAllImageAreas = 0;
			for (ImageNode* current = imagesList; current != nullptr; current = current->next) {
				sumOfAllImageAreas += current->data.width * current->data.height;
				count++;
			}

			// this is a super simple heuristic and it definitely won't cover all cases
			if (sumOfAllImageAreas > (width * height)) {
				fprintf(stderr, "BakedAtlas::Packer Error! Too many images for the requested atlas size!\n");
				return { 0 };
			}
		}

		// use the count to do some allocations
		stbrp_rect* rects = scratch.push<stbrp_rect>(count);
		stbrp_node* rpNodes = scratch.push<stbrp_node>(width);
		stbrp_context rpContext;
		stbrp_init_target(&rpContext, width, height, rpNodes, width);
		
		// now fill rects array with image from ImageNodes
		count = 0;
		for (ImageNode* current = imagesList; current != nullptr; current = current->next) {
			const ImageData& data = current->data;
			
			rects[count] = {
				.id = count,
				.w = data.width, .h = data.height,
				.x = 0, .y = 0,
				.was_packed = 0
			};

			count++;
		}

		// pack rects
		if (0 == stbrp_pack_rects(&rpContext, rects, count)) {
			fprintf(stderr, "BakedAtlas::Packer Error! Could not pack all textures into a single atlas!\n");
			return { 0 };
		}

		BakedAtlas atlas = BakedAtlas::alloc(width, height, count);

		count = 0;
		for (ImageNode* current = imagesList; current != nullptr; current = current->next) {
			ImageData& image = current->data;
			const stbrp_rect& newRect = rects[count];

			// First, set the region rect
			atlas.regions[count] = {
				.x = static_cast<f32>(newRect.x) / static_cast<f32>(atlas.bitmap.width),
				.y = static_cast<f32>(newRect.y) / static_cast<f32>(atlas.bitmap.height),
				.w = static_cast<f32>(newRect.w) / static_cast<f32>(atlas.bitmap.width),
				.h = static_cast<f32>(newRect.h) / static_cast<f32>(atlas.bitmap.height)
			};

			// now copy the image into its position in the atlas according to the newly placed stbrp_rect
			// we'll also need to do some shenanigans to actually copy the image image depending on the number of 8bit channels

			switch (image.channels) {
			case 1: { // image.data is assumed to be R8 Grayscale format
				for (i32 y = 0; y < image.height; y++) {
					for (i32 x = 0; x < image.width; x++) {
						// RGB components take 255, and A component takes value of the source bitmap
						u32 currentValue = (static_cast<u32>(image.data[x + (y * image.width)]) << 24) | 0x00FFFFFF;

						const i32 newX = newRect.x + x;
						const i32 newY = newRect.y + y;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = currentValue;
					}
				}
			} break;
			case 3: { // image.data is assumed to be RGB8 format
				const size_t sourceStride = 3 * image.width;

				for (i32 y = 0; y < image.height; y++) {
					for (i32 x = 0; x < image.width; x++) {
						size_t sourceY = y * sourceStride;
						size_t sourceX = x * 3;

						// RGB components take the values of the source bitmap, and A component takes 255
						u32 currentValue = 0xFF000000 |
							(static_cast<u32>(image.data[(sourceX + 0) + sourceY]) << 0) |
							(static_cast<u32>(image.data[(sourceX + 1) + sourceY]) << 8) |
							(static_cast<u32>(image.data[(sourceX + 2) + sourceY]) << 16);

						const i32 newX = newRect.x + x;
						const i32 newY = newRect.y + y;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = currentValue;
					}
				}
			} break;
			case 4: { // image.data is assumed to be RGBA8 format
				const size_t sourceStride = 4 * image.width;

				for (i32 y = 0; y < image.height; y++) {
					const u32* startSource = reinterpret_cast<u32*>(image.data) + (y * image.width);
					u32* startTarget = atlas.bitmap.data + newRect.x + ((newRect.y + y) * atlas.bitmap.width);
					memcpy(startTarget, startSource, sourceStride);
				}
			} break;
			default: {
				fprintf(stderr, "BakedAtlas::Packer Error! One of the loaded images has an unsupported channel count.\n");
			}
			}

			// after we've done the copy, we can can free the image data since we got it from stb_image
			stbi_image_free(image.data);

			count++;
		}

		packerArena.dealloc(); // this frees all nodes in imagesList
		memset(this, 0, sizeof(BakedAtlas::Packer));

		return atlas;
	}

	//
	// FONT ATLAS
	//

	FontAtlas FontAtlas::load(i32 width, i32 height, LoadInfo* loadInfos, i32 numFonts) {
		stbtt_pack_context packContext;
		i32 actualNumFonts = 0;
		mem::Arena& scratch = mem::get_scratch();
		mem::ArenaScope scope(scratch);

		FontAtlas atlas = { 0 };
		atlas.bitmap = tds::Slice2<u8>::alloc(width, height);
		atlas.packedChars = tds::Slice2<stbtt_packedchar>::alloc(CHARS_PER_FONT, numFonts);

		stbtt_PackBegin(&packContext, atlas.bitmap.data, width, height, width, 1, nullptr);
		stbtt_PackSetOversampling(&packContext, 4, 4);

		for (i32 i = 0; i < numFonts; i++) {
			LoadInfo& loadInfo = loadInfos[i];

			if (!SDL_GetPathInfo(loadInfo.path, nullptr)) continue;

			FILE* fp = fopen(loadInfo.path, "rb");
			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			void* fileData = scratch.push(fileSize + 1);
			fread(fileData, fileSize, 1, fp);
			fclose(fp);

			// just loading the basic ASCII text for now
			stbtt_pack_range range = {
				.font_size = loadInfo.fontHeight,
				.first_unicode_codepoint_in_range = 0,
				.array_of_unicode_codepoints = nullptr,
				.num_chars = CHARS_PER_FONT,
				.chardata_for_range = atlas.packedChars.get_ptr(0, i)
			};

			stbtt_PackFontRanges(&packContext, (unsigned char*)fileData, 0, &range, 1);
		}

		stbtt_PackEnd(&packContext);
		return atlas;
	}

	void FontAtlas::dealloc(FontAtlas& atlas) {
		tds::Slice2<u8>::free(atlas.bitmap);
		tds::Slice2<stbtt_packedchar>::free(atlas.packedChars);
	}
}