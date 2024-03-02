/******************************************************************************
 * Copyright (c) 2023 Jaroslav Hensl                                          *
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
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "nocrt.h"

#include "rop3.h"
#include "rop3op.h"
#include "stretchblttf.h"

typedef void (*rop32_h)(void *src, void *dst, uint32_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch);
typedef void (*rop16_h)(void *src, void *dst, uint16_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch);
typedef void (*rop8_h) (void *src, void *dst,  uint8_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch);

typedef void (*stretchrop32_h)(void *src, void *dst, uint32_t pattern, stretchBltRect *rect);
typedef void (*stretchrop16_h)(void *src, void *dst, uint16_t pattern, stretchBltRect *rect);
typedef void (*stretchrop8_h )(void *src, void *dst, uint8_t  pattern, stretchBltRect *rect);

/*** ROP32 ***/
#define ROP3ITEM(_n) \
static void blt_ ## _n ## _32 (void *src, void *dst, uint32_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch){ \
	uint32_t x, y; \
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint32_t); \
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint32_t); \
	for(y = 0; y < h; y++){ \
		uint32_t *ps = (uint32_t*)(psrc + spitch*y); \
		uint32_t *ds = (uint32_t*)(pdst + dpitch*y); \
		for(x = 0; x < w; x++){ \
			*ds = _n (*ds, pattern, *ps); \
			ps++; \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** ROP16 ***/
#define ROP3ITEM(_n) \
static void blt_ ## _n ## _16 (void *src, void *dst, uint16_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch){ \
	uint32_t x, y; \
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint16_t); \
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint16_t); \
	for(y = 0; y < h; y++){ \
		uint16_t *ps = (uint16_t*)(psrc + spitch*y); \
		uint16_t *ds = (uint16_t*)(pdst + dpitch*y); \
		for(x = 0; x < w; x++){ \
			*ds = _n (*ds, pattern, *ps); \
			ps++; \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** ROP8 ***/
#define ROP3ITEM(_n) \
static void blt_ ## _n ## _8 (void *src, void *dst, uint8_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch){ \
	uint32_t x, y; \
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint8_t); \
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint8_t); \
	for(y = 0; y < h; y++){ \
		uint8_t *ps = (psrc + spitch*y); \
		uint8_t *ds = (pdst + dpitch*y); \
		for(x = 0; x < w; x++){ \
			*ds = _n (*ds, pattern, *ps); \
			ps++; \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** ROP24 ***/
// FIXME: slow, do more opts
#define ROP3ITEM(_n) \
static void blt_ ## _n ## _24 (void *src, void *dst, uint32_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch){ \
	uint32_t x, y; \
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*3*sizeof(uint8_t); \
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*3*sizeof(uint8_t); \
	for(y = 0; y < h; y++){ \
		uint8_t *ps = (psrc + spitch*y); \
		uint8_t *ds = (pdst + dpitch*y); \
		for(x = 0; x < w; x++){ \
			*ds = _n (*ds, ( pattern        & 0xFF), *ps); ps++; ds++; \
			*ds = _n (*ds, ((pattern >>  8) & 0xFF), *ps); ps++; ds++; \
			*ds = _n (*ds, ((pattern >> 16) & 0xFF), *ps); ps++; ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** stretch ROP3 - 32 ***/
#define ROP3ITEM(_n) \
static void stretchblt_ ## _n ## _32 (void *src, void *dst, uint32_t pattern, \
	stretchBltRect *rect){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint32_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint32_t *ds = (uint32_t*)(pdst + rect->dpitch*y); \
		for(x = 0; x < rect->drw; x++){ \
			/*uint32_t spx = stretch_32(src, x, y, rect);*/ \
			*ds = _n (*ds, pattern, stretch_32(src, x, y, rect)); \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** stretch ROP3 - 16 ***/
#define ROP3ITEM(_n) \
static void stretchblt_ ## _n ## _16 (void *src, void *dst, uint16_t pattern, \
	stretchBltRect *rect){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint16_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint16_t *ds = (uint16_t*)(pdst + rect->dpitch*y); \
		for(x = 0; x < rect->drw; x++){ \
			*ds = _n (*ds, pattern, stretch_16(src, x, y, rect)); \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** stretch ROP3 - 8 ***/
#define ROP3ITEM(_n) \
static void stretchblt_ ## _n ## _8 (void *src, void *dst, uint8_t pattern, \
	stretchBltRect *rect){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint8_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint8_t *ds = pdst + rect->dpitch*y; \
		for(x = 0; x < rect->drw; x++){ \
			*ds = _n (*ds, pattern, stretch_8(src, x, y, rect)); \
			ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** stretch ROP24 ***/
#define ROP3ITEM(_n) \
static void stretchblt_ ## _n ## _24 (void *src, void *dst, uint32_t pattern, \
	stretchBltRect *rect){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*3*sizeof(uint8_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint8_t *ds = pdst + rect->dpitch*y; \
		for(x = 0; x < rect->drw; x++){ \
			uint32_t px = _n ((*(uint32_t*)ds), pattern, stretch_24(src, x, y, rect)); \
			*ds = ( px        & 0xFF); ds++; \
			*ds = ((px >>  8) & 0xFF); ds++; \
			*ds = ((px >> 16) & 0xFF); ds++; \
		} } }

#include "rop3list.h"
#undef ROP3ITEM

/*** tables ***/
#define ROP3ITEM(_n) blt_ ## _n ## _32,
static const rop32_h rop32_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) blt_ ## _n ## _16,
static const rop16_h rop16_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) blt_ ## _n ## _8,
static const rop8_h rop8_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) blt_ ## _n ## _24,
static const rop32_h rop24_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) stretchblt_ ## _n ## _32,
static const stretchrop32_h stretchrop32_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) stretchblt_ ## _n ## _16,
static const stretchrop16_h stretchrop16_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) stretchblt_ ## _n ## _8,
static const stretchrop8_h stretchrop8_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

#define ROP3ITEM(_n) stretchblt_ ## _n ## _24,
static const stretchrop32_h stretchrop24_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM

// bitblt

static void rop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop8_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
}

static void rop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop16_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
}

static void rop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop24_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
}

static void rop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop32_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
}

// stretchblt
static void stretchrop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	(stretchrop32_table[code])(src, dst, pattern, rect);
}

static void stretchrop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	(stretchrop24_table[code])(src, dst, pattern, rect);
}

static void stretchrop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern, stretchBltRect *rect)
{
	(stretchrop16_table[code])(src, dst, pattern, rect);
}

static void stretchrop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern, stretchBltRect *rect)
{
	(stretchrop8_table[code])(src, dst, pattern, rect);
}

void rop3(uint8_t bpp, uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	switch(bpp)
	{
		case 8:
			rop3_8(code, src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 16:
			rop3_16(code, src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 24:
			rop3_24(code, src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 32:
			rop3_32(code, src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
	}
}

void stretchrop3(uint8_t bpp, uint8_t code, void *src, void *dst, uint32_t pattern,
	stretchBltRect *rect)
{
	switch(bpp)
	{
		case 8:
			stretchrop3_8(code, src, dst, pattern, rect);
			break;
		case 16:
			stretchrop3_16(code, src, dst, pattern, rect);
			break;
		case 24:
			stretchrop3_24(code, src, dst, pattern, rect);
			break;
		case 32:
			stretchrop3_32(code, src, dst, pattern, rect);
			break;
	}
}

