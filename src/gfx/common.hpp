#pragma once

#include <math.h> // for fmodf and fminf
#include <tinydef.hpp>

namespace gfx {
	struct Rect {
		f32 x, y, w, h;
	};

	// NOTE: Color values are strictly in the range [0.0, 1.0]
	union Color {
		// Array Format
		f32 f[4];

		// RGBA Format
		struct {
			f32 r, g, b, a;
		};

		// HS(V/L) Format
		struct {
			struct {
				f32 h, s;
			};

			union {
				f32 v, l;
			};
		};
		
		// h in [0, 360], s in [0, 1], v in [0, 1]
		// formula from https://en.wikipedia.org/wiki/HSL_and_HSV
		static Color from_hsv(f32 h, f32 s, f32 v) {
			Color color = { .a = 1.0f };

			f32 k = fmodf(5.0f + (h / 60.0f), 6.0f);
			color.r = v - (v * s * tim::clamp(fminf(k, 4.0f - k), 0.0f, 1.0f));
			
			k = fmodf(3.0f + (h / 60.0f), 6.0f);
			color.g = v - (v * s * tim::clamp(fminf(k, 4.0f - k), 0.0f, 1.0f));
			
			k = fmodf(1.0f + (h / 60.0f), 6.0f);
			color.b = v - (v * s * tim::clamp(fminf(k, 4.0f - k), 0.0f, 1.0f));

			return color;
		}

		// h in [0, 360], s in [0, 1], v in [0, 1]
		// formula from https://en.wikipedia.org/wiki/HSL_and_HSV
		static Color from_hsl(f32 h, f32 s, f32 l) {
			Color color = { .a = 1.0f };
			f32 a = s * fminf(l, 1.0f - l);

			f32 k = fmodf(0.0f + (h / 30.0f), 12.0f);
			color.r = l - (a * tim::clamp(fminf(k - 3.0f, 9.0f - k), -1.0f, 1.0f));

			k = fmodf(8.0f + (h / 30.0f), 12.0f);
			color.g = l - (a * tim::clamp(fminf(k - 3.0f, 9.0f - k), -1.0f, 1.0f));

			k = fmodf(4.0f + (h / 30.0f), 12.0f);
			color.b = l - (a * tim::clamp(fminf(k - 3.0f, 9.0f - k), -1.0f, 1.0f));

			return color;
		}

		static Color lerp(const Color& col1, const Color& col2, f32 x) {
			const float oneMinus = 1.0f - x;
			
			Color c; // compiler should do loop unrolling...
			for (int i = 0; i < 4; i++) {
				c.f[i] = (oneMinus * col1.f[i]) + (x * col2.f[i]);
			}

			return c;
		}
	};

	struct Vertex {
		f32 x, y, z;
		f32 u, v;
		Color color;
	};
}