/******************************************************************************
 * Copyright (c) 2025 Jaroslav Hensl                                          *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person                *
 * obtaining a copy of this software and associated documentation             *
 * files (the "Software"), to deal in the Software without                    *
 * restriction, including without limitation the rights to use,               *
 * copy, modify, merge, publish, distribute, sublicense, and/or sell          *
 * copies of the Software, and to permit persons to whom the                  *
 * Software is furnished to do so, subject to the following                   *
 * conditions:                                                                *
 *                                                                            *
 * The above copyright notice and this permission notice shall be             *
 * included in all copies or substantial portions of the Software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,            *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES            *
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                   *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT                *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,               *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING               *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR              *
 * OTHER DEALINGS IN THE SOFTWARE.                                            *
 *                                                                            *
 ******************************************************************************/
#ifndef __MESA3D_FLIP_H__INCLUDED__
#define __MESA3D_FLIP_H__INCLUDED__

static void *flip_y(mesa3d_ctx_t *ctx, const void *in, int w, int h, int bpp)
{
	int pitch = SurfacePitch(w, bpp);
	BYTE *plane = MesaTempAlloc(ctx, w, pitch*h);
	const BYTE *src = in;
	int h1 = h-1;
	int y;

	for(y = 0; y <= h1; y++)
	{
		memcpy(plane + (pitch*y), src + (pitch*(h1-y)), pitch);
	}

	return plane;
}

static void *flip_x(mesa3d_ctx_t *ctx, const void *in, int w, int h, int bpp)
{
	int pitch = SurfacePitch(w, bpp);
	BYTE *plane = MesaTempAlloc(ctx, w, pitch*h);
	const BYTE *src = in;
	int w1 = w-1;
	int x, y;

	switch(bpp)
	{
		case 16:
			for(y = 0; y < h; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const WORD *s_line = (const WORD*)(src + pitch*y);
					WORD *d_line = (WORD*)(plane + pitch*y);
					d_line[x] = s_line[w1-x];
				}
			}
			break;
		case 32:
			for(y = 0; y < h; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const DWORD *s_line = (const DWORD*)(src + pitch*y);
					DWORD *d_line = (DWORD*)(plane + pitch*y);
					d_line[x] = s_line[w1-x];
				}
			}
			break;
		case 24:
			for(y = 0; y < h; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const BYTE *s_line = src + pitch*y;
					BYTE *d_line = plane + pitch*y;
					d_line[x*3 + 0] = s_line[(w1-x)*3 + 0];
					d_line[x*3 + 1] = s_line[(w1-x)*3 + 1];
					d_line[x*3 + 2] = s_line[(w1-x)*3 + 2];
				}
			}
	}

	return plane;
}

static void *flip_xy(mesa3d_ctx_t *ctx, const void *in, int w, int h, int bpp)
{
	int pitch = SurfacePitch(w, bpp);
	BYTE *plane = MesaTempAlloc(ctx, w, pitch*h);
	const BYTE *src = in;
	int w1 = w-1;
	int h1 = h-1;
	int x, y;

	switch(bpp)
	{
		case 16:
			for(y = 0; y <= h1; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const WORD *s_line = (const WORD*)(src + pitch*y);
					WORD *d_line = (WORD*)(plane + pitch*(h1-y));
					d_line[x] = s_line[w1-x];
				}
			}
			break;
		case 32:
			for(y = 0; y < h; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const DWORD *s_line = (const DWORD*)(src + pitch*y);
					DWORD *d_line = (DWORD*)(plane + pitch*(h1-y));
					d_line[x] = s_line[w1-x];
				}
			}
			break;
		case 24:
			for(y = 0; y < h; y++)
			{
				for(x = 0; x <= w1; x++)
				{
					const BYTE *s_line = src + pitch*y;
					BYTE *d_line = plane + pitch*(h1-y);
					d_line[x*3 + 0] = s_line[(w1-x)*3 + 0];
					d_line[x*3 + 1] = s_line[(w1-x)*3 + 1];
					d_line[x*3 + 2] = s_line[(w1-x)*3 + 2];
				}
			}
			break;
	}

	return plane;
}


#define XY(_mem, _x, _y) _mem[(_y)*bpitch + (_x)]
#define XY24(_mem, _x, _y, _b) _mem[(_y)*pitch + (_x)*3 + (_b)]

#define ROT_cw90_X(_x, _y)  (_y)
#define ROT_cw90_Y(_x, _y)  (s1-_x)
#define ROT_cw270_X(_x, _y) (s1-_y)
#define ROT_cw270_Y(_x, _y) (_x)
#define ROT_cw180_X(_x, _y) (s1-_y)
#define ROT_cw180_Y(_x, _y) (s1-_x)

#define DEF_ROT(_deg) \
	static void *rot_ ## _deg(mesa3d_ctx_t *ctx, const void *in, int w, int h, int bpp){ \
	int pitch = SurfacePitch(w, bpp); \
	if(w != h){return NULL;} \
	void *plane = MesaTempAlloc(ctx, w, pitch*h); \
	int s1 = w-1; \
	int x, y; \
	switch(bpp){ \
		case 16: { \
			int bpitch = pitch/2; \
			WORD *dst = plane; const WORD *src = in; \
			for(y = 0; y <= s1; y++){ \
				for(x = 0; x <= s1; x++){ \
					XY(dst, ROT_ ## _deg ## _X(x, y), ROT_ ## _deg ## _Y(x, y)) = XY(src, x, y);\
			} } break; } \
		case 32: { \
			int bpitch = pitch/4; \
			DWORD *dst = plane; const DWORD *src = in; \
			for(y = 0; y <= s1; y++){ \
				for(x = 0; x <= s1; x++){ \
					XY(dst, ROT_ ## _deg ## _X(x, y), ROT_ ## _deg ## _Y(x, y)) = XY(src, x, y);\
			} } break; } \
		case 24: { \
			BYTE *dst = plane; const BYTE *src = in; \
			for(y = 0; y <= s1; y++){ \
				for(x = 0; x <= s1; x++){ \
					XY24(dst, ROT_ ## _deg ## _X(x, y), ROT_ ## _deg ## _Y(x, y), 0) = XY24(src, x, y, 0);\
					XY24(dst, ROT_ ## _deg ## _X(x, y), ROT_ ## _deg ## _Y(x, y), 1) = XY24(src, x, y, 1);\
					XY24(dst, ROT_ ## _deg ## _X(x, y), ROT_ ## _deg ## _Y(x, y), 2) = XY24(src, x, y, 2);\
			} } break; } \
	} \
	return plane; \
}

DEF_ROT(cw90)
DEF_ROT(cw180)
DEF_ROT(cw270)

#undef XY
#undef XY24

#endif /* __MESA3D_FLIP_H__INCLUDED__ */
