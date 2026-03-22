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
		//stbi_set_flip_vertically_on_load(true);

		assert(packerArena.capacity == 0);
		packerArena.alloc();
	}

	i32 BakedAtlas::Packer::add_tex(const char* path) {
		if (!SDL_GetPathInfo(path, nullptr)) return -1;

		ImageData imageData;
		imageData.data = stbi_load(path, &imageData.width, &imageData.height, &imageData.channels, 0);
		if (!imageData.data) return -1;

		ImageNode* node = packerArena.push_zero<ImageNode>();
		node->data = imageData;

		// append node to front of list (this requires we reverse it when baking to preserve load order)
		if (imagesList) {
			node->next = imagesList;
		} imagesList = node;

		return imagesLoaded++;
	}

	// returns the count of elements in the linked list
	// takes in a reference to a pointer to a linked list node (since we set list to the new front node)
	static i32 count_and_reverse_list(BakedAtlas::Packer::ImageNode*& list) {
		if (!list) return 0;
		using ImageNode = BakedAtlas::Packer::ImageNode;

		i32 count = 0;
		ImageNode* prev = nullptr;
		ImageNode* current = list;

		while (current != nullptr) {
			ImageNode* next = current->next;
			current->next = prev;

			prev = current;
			current = next;
			count++;
		}

		list = prev;
		return count;
	}

	// these functions sample an image loaded from stb_image with nChannels set to 0
	// that means that the data that stb_image loads will have usually either 1, 3, or 4 bytes per pixel
	static inline u32 sample_r(i32 x, i32 y, const BakedAtlas::Packer::ImageData& image) {
		return (static_cast<u32>(image.data[x + (y * image.width)]) << 24) | 0x00FFFFFF;
	}

	static inline u32 sample_rgb(i32 sourceX, i32 sourceY, const BakedAtlas::Packer::ImageData& image) {
		// NOTE: ensure that sourceX is the actual pixel x multiplied by 3!!!
		// also ensure that sourceY is the actual pixel y multiplied by the row stride in bytes!!!
		return 0xFF000000 |
			(static_cast<u32>(image.data[(sourceX + 0) + sourceY]) << 0) |
			(static_cast<u32>(image.data[(sourceX + 1) + sourceY]) << 8) |
			(static_cast<u32>(image.data[(sourceX + 2) + sourceY]) << 16);
	}

	static inline u32 sample_rgba(i32 x, i32 y, const BakedAtlas::Packer::ImageData& image) {
		return reinterpret_cast<u32*>(image.data)[x + (y * image.width)];
	}

	// releases most of the packing state and copies all of the images to a baked atlas
	BakedAtlas BakedAtlas::Packer::bake(i32 width, i32 height, i32 padding) {
		if(!imagesList) return { 0};

		i32 count = count_and_reverse_list(imagesList);

		// use the count to do some allocations
		stbrp_rect* rects = packerArena.push<stbrp_rect>(count);
		stbrp_node* rpNodes = packerArena.push<stbrp_node>(width);
		stbrp_context rpContext;
		stbrp_init_target(&rpContext, width, height, rpNodes, width);

		const i32 twoPadding = 2 * padding;
		
		// now fill rects array with image from ImageNodes
		count = 0;
		for (ImageNode* current = imagesList; current != nullptr; current = current->next) {
			const ImageData& data = current->data;
			
			rects[count] = {
				.id = count,
				.w = data.width + twoPadding, .h = data.height + twoPadding,
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
			stbrp_rect& newRect = rects[count];

			// offset the newRect xywh by padding values to stay within the bounds
			newRect.x += padding;
			newRect.y += padding;
			newRect.w -= twoPadding;
			newRect.h -= twoPadding;

			// set the region rect
			// NOTE that atlas.bitmap.width and height are the width and height of the original image, so we have to
			atlas.regions[count] = {
				.x = static_cast<f32>(newRect.x) / static_cast<f32>(atlas.bitmap.width),
				.y = static_cast<f32>(newRect.y) / static_cast<f32>(atlas.bitmap.height),
				.w = static_cast<f32>(newRect.w) / static_cast<f32>(atlas.bitmap.width),
				.h = static_cast<f32>(newRect.h) / static_cast<f32>(atlas.bitmap.height)
			};

			// now copy the image into its position in the atlas according to the newly placed stbrp_rect
			// - we'll also need to do some shenanigans to actually copy the image image depending on the number of 8bit channels
			// - we need to also consider padding values, they'll just take the values of their neighbors
			// 
			//   before you recoil in horror at the following code, consider that handling the padding for each case
			//   has a very similar structure (left padding, current row, right padding, top padding, bottom padding)
			//   and usually just varies in the way that the data is addressed (r, rgb, rgba) and the functions that are called

			switch (image.channels) {
			case 1: { // image.data is assumed to be R8 Grayscale format
                      // RGB components take 255, and A component takes value of the source bitmap
				for (i32 y = 0; y < image.height; y++) {
					const i32 newY = newRect.y + y;
					
					// left vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x - padding + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_r(0, y, image);
					}

					// current row
					for (i32 x = 0; x < image.width; x++) {
						const i32 newX = newRect.x + x;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_r(x, y, image);
					}

					// right vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x + newRect.w + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_r(image.width - 1, y, image);
					}
				}

				// top horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y - padding + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_r(srcX, 0, image);
					}
				}

				// bottom horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y + image.height + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_r(srcX, image.height - 1, image);
					}
				}
			} break;
			case 3: { // image.data is assumed to be RGB8 format
                      // RGB components take the values of the source bitmap, and A component takes 255
				const size_t sourceStride = 3 * image.width;
				
				for (i32 y = 0; y < image.height; y++) {
					size_t sourceY = y * sourceStride;
					const i32 newY = newRect.y + y;

					// left vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x - padding + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_rgb(0, sourceY, image);
					}

					// current row
					for (i32 x = 0; x < image.width; x++) {
						const i32 newX = newRect.x + x;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_rgb(x * 3, sourceY, image);
					}

					// right vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x + newRect.w + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_rgb((image.width - 1) * 3, sourceY, image);
					}
				}

				// top horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y - padding + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_rgb(srcX * 3, 0, image);
					}
				}

				// bottom horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y + image.height + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_rgb(srcX * 3, sourceStride * (image.height - 1), image);
					}
				}
			} break;
			case 4: { // image.data is assumed to be RGBA8 format
				const size_t sourceStride = 4 * image.width;

				for (i32 y = 0; y < image.height; y++) {
					const i32 newY = newRect.y + y;

					// left vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x - padding + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_rgba(0, y, image);
					}

					// current row
					memcpy(
						atlas.bitmap.data + newRect.x + ((newRect.y + y) * atlas.bitmap.width),
						reinterpret_cast<u32*>(image.data) + (y * image.width),
						sourceStride);

					// right vertical padding
					for (i32 p = 0; p < padding; p++) {
						const i32 newX = newRect.x + newRect.w + p;
						atlas.bitmap.data[newX + (newY * atlas.bitmap.width)] = sample_rgba(image.width - 1, y, image);
					}
				}

				// top horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y - padding + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_rgba(srcX, 0, image);
					}
				}

				// bottom horizontal padding
				for (i32 p = 0; p < padding; p++) {
					const i32 newY = newRect.y + image.height + p;
					for (i32 x = -padding; x < image.width + padding; x++) {
						i32 srcX = tim::clamp(x, 0, image.width - 1);
						atlas.bitmap.data[(newRect.x + x) + (newY * atlas.bitmap.width)] = sample_rgba(srcX, image.height - 1, image);
					}
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
		atlas.metadata = tds::Slice<FontMetrics>::alloc(numFonts);

		stbtt_PackBegin(&packContext, atlas.bitmap.data, width, height, width, 1, nullptr);
		atlas.oversamplingX = 4;
		stbtt_PackSetOversampling(&packContext, atlas.oversamplingX, atlas.oversamplingX * 2);

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

			//stbtt_fontinfo fontInfo;
			//stbtt_InitFont(&fontInfo, reinterpret_cast<u8*>(fileData), 0);

			// just loading the basic ASCII text for now
			stbtt_pack_range range = {
				.font_size = loadInfo.fontHeight,
				.first_unicode_codepoint_in_range = 0,
				.array_of_unicode_codepoints = nullptr,
				.num_chars = CHARS_PER_FONT,
				.chardata_for_range = atlas.packedChars.get_ptr(0, i)
			};

			FontMetrics& metrics = atlas.metadata[i];
			metrics.loadedFontSize = loadInfo.fontHeight;

			stbtt_PackFontRanges(&packContext, (unsigned char*)fileData, 0, &range, 1);
			atlas.numFonts++;
		}

		stbtt_PackEnd(&packContext);
		return atlas;
	}

	void FontAtlas::dealloc(FontAtlas& atlas) {
		tds::Slice2<u8>::free(atlas.bitmap);
		tds::Slice2<stbtt_packedchar>::free(atlas.packedChars);
		tds::Slice<FontMetrics>::free(atlas.metadata);
	}
}