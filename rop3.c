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

/*
 level 0: function for every bpp switch for ROP
 level 1: function for every bpp, call tables for every ROP,
          15 chosen function expaned for indivisual ROP,
          others in switch
 level 2: function for every bpp, call tables for every ROP,
          all ROP expanded to individual function

*/
#ifndef ROP_EXPAND_LEVEL
#define ROP_EXPAND_LEVEL 1
#endif

typedef void (*rop32_h)(void *src, void *dst, uint32_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code);
typedef void (*rop16_h)(void *src, void *dst, uint16_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code);
typedef void (*rop8_h) (void *src, void *dst,  uint8_t pattern, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code);

typedef void (*stretchrop32_h)(void *src, void *dst, uint32_t pattern, stretchBltRect *rect, uint_fast8_t code);
typedef void (*stretchrop16_h)(void *src, void *dst, uint16_t pattern, stretchBltRect *rect, uint_fast8_t code);
typedef void (*stretchrop8_h )(void *src, void *dst, uint8_t  pattern, stretchBltRect *rect, uint_fast8_t code);

/******************************************************************************
 * individual functions for every bpp x every rop                             *
 ******************************************************************************/

#if ROP_EXPAND_LEVEL != 0

/*** ROP32 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void blt_ ## _n ## _32 (void *src, void *dst, uint32_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code){ \
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

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

/*** ROP16 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void blt_ ## _n ## _16 (void *src, void *dst, uint16_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code){ \
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

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

/*** ROP8 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void blt_ ## _n ## _8 (void *src, void *dst, uint8_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code){ \
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

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL != 0 */

#if ROP_EXPAND_LEVEL < 2

#define ROP3ITEM(_n, _code) case _code: *ds = _n (*ds, pattern, *ps); break;
#define ROP3ITEM_PREF ROP3ITEM

static void blt_small_32 (void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy,
	uint32_t w, uint32_t h,
	uint32_t spitch, uint32_t dpitch, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint32_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint32_t);
	for(y = 0; y < h; y++)
	{
		uint32_t *ps = (uint32_t*)(psrc + spitch*y);
		uint32_t *ds = (uint32_t*)(pdst + dpitch*y);
		for(x = 0; x < w; x++)
		{
			switch(code)
			{
#include "rop3list.h"
			}
			ps++;
			ds++;
		}
	}
}

static void blt_small_16 (void *src, void *dst, uint16_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy,
	uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint16_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint16_t);
	for(y = 0; y < h; y++)
	{
		uint16_t *ps = (uint16_t*)(psrc + spitch*y);
		uint16_t *ds = (uint16_t*)(pdst + dpitch*y);
		for(x = 0; x < w; x++)
		{
			switch(code)
			{
#include "rop3list.h"
			}
			ps++;
			ds++;
		}
	}
}

static void blt_small_8 (void *src, void *dst, uint8_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy,
	uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint8_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint8_t);
	for(y = 0; y < h; y++)
	{
		uint8_t *ps = (psrc + spitch*y);
		uint8_t *ds = (pdst + dpitch*y);
		for(x = 0; x < w; x++)
		{
			switch(code)
			{
#include "rop3list.h"
			}
			ps++;
			ds++;
		}
	}
}

#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL < 2 */

#if ROP_EXPAND_LEVEL != 0

/*** ROP24 ***/
// FIXME: slow, do more opts
#define ROP3ITEM_PREF(_n, _code) \
static void blt_ ## _n ## _24 (void *src, void *dst, uint32_t pattern, \
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code){ \
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

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL != 0 */

#if ROP_EXPAND_LEVEL < 2

#define ROP3ITEM(_n, _code) case _code: \
	*ds = _n (*ds, ( pattern        & 0xFF), *ps); ps++; ds++; \
	*ds = _n (*ds, ((pattern >>  8) & 0xFF), *ps); ps++; ds++; \
	*ds = _n (*ds, ((pattern >> 16) & 0xFF), *ps); ps++; ds++; \
	break;

#define ROP3ITEM_PREF ROP3ITEM

static void blt_small_24 (void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy,
	uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*3*sizeof(uint8_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*3*sizeof(uint8_t);
	for(y = 0; y < h; y++)
	{
		uint8_t *ps = (psrc + spitch*y);
		uint8_t *ds = (pdst + dpitch*y);
		for(x = 0; x < w; x++)
		{
			switch(code)
			{
#include "rop3list.h"
			}
		}
	}
}
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL < 2 */

#if ROP_EXPAND_LEVEL != 0

/*** stretch ROP3 - 32 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void stretchblt_ ## _n ## _32 (void *src, void *dst, uint32_t pattern, \
	stretchBltRect *rect, uint_fast8_t code){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint32_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint32_t *ds = (uint32_t*)(pdst + rect->dpitch*y); \
		for(x = 0; x < rect->drw; x++){ \
			/*uint32_t spx = stretch_32(src, x, y, rect);*/ \
			*ds = _n (*ds, pattern, stretch_32(src, x, y, rect)); \
			ds++; \
		} } }

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

/*** stretch ROP3 - 16 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void stretchblt_ ## _n ## _16 (void *src, void *dst, uint16_t pattern, \
	stretchBltRect *rect, uint_fast8_t code){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint16_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint16_t *ds = (uint16_t*)(pdst + rect->dpitch*y); \
		for(x = 0; x < rect->drw; x++){ \
			*ds = _n (*ds, pattern, stretch_16(src, x, y, rect)); \
			ds++; \
		} } }

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

/*** stretch ROP3 - 8 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void stretchblt_ ## _n ## _8 (void *src, void *dst, uint8_t pattern, \
	stretchBltRect *rect, uint_fast8_t code){ \
	uint32_t x, y; \
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint8_t); \
	for(y = 0; y < rect->drh; y++){ \
		uint8_t *ds = pdst + rect->dpitch*y; \
		for(x = 0; x < rect->drw; x++){ \
			*ds = _n (*ds, pattern, stretch_8(src, x, y, rect)); \
			ds++; \
		} } }

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL != 0 */

#if ROP_EXPAND_LEVEL < 2

#define ROP3ITEM(_n, _code) case _code: *ds = _n (*ds, pattern, ss); break;
#define ROP3ITEM_PREF ROP3ITEM

static void stretchblt_small_32 (void *src, void *dst, uint32_t pattern,
	stretchBltRect *rect, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint32_t);
	for(y = 0; y < rect->drh; y++)
	{
		uint32_t *ds = (uint32_t*)(pdst + rect->dpitch*y);
		for(x = 0; x < rect->drw; x++)
		{
			uint32_t ss = stretch_32(src, x, y, rect);
			switch(code)
			{
#include "rop3list.h"
			}
			ds++;
		}
	}
}

static void stretchblt_small_16 (void *src, void *dst, uint16_t pattern,
	stretchBltRect *rect, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint16_t);
	for(y = 0; y < rect->drh; y++)
	{
		uint16_t *ds = (uint16_t*)(pdst + rect->dpitch*y);
		for(x = 0; x < rect->drw; x++)
		{
			uint16_t ss = stretch_16(src, x, y, rect);
			switch(code)
			{
#include "rop3list.h"
			}
			ds++;
		}
	}
}

static void stretchblt_small_8 (void *src, void *dst, uint8_t pattern,
	stretchBltRect *rect, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint8_t);
	for(y = 0; y < rect->drh; y++)
	{
		uint8_t *ds = pdst + rect->dpitch*y;
		for(x = 0; x < rect->drw; x++)
		{
			uint8_t ss = stretch_8(src, x, y, rect);
			switch(code)
			{
#include "rop3list.h"
			}
			ds++;
		}
	}
}

#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL < 2 */

#if ROP_EXPAND_LEVEL != 0

/*** stretch ROP24 ***/
#define ROP3ITEM_PREF(_n, _code) \
static void stretchblt_ ## _n ## _24 (void *src, void *dst, uint32_t pattern, \
	stretchBltRect *rect, uint_fast8_t code){ \
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

#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code)
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

#include "rop3list.h"
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL != 0 */

#if ROP_EXPAND_LEVEL < 2

#define ROP3ITEM(_n, _code) case _code: px = _n ((*(uint32_t*)ds), pattern, ss); break;
#define ROP3ITEM_PREF ROP3ITEM

static void stretchblt_small_24 (void *src, void *dst, uint32_t pattern,
	stretchBltRect *rect, uint_fast8_t code)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*3*sizeof(uint8_t);
	for(y = 0; y < rect->drh; y++)
	{
		uint8_t *ds = pdst + rect->dpitch*y;
		for(x = 0; x < rect->drw; x++)
		{
			uint32_t ss = stretch_24(src, x, y, rect);
			uint32_t px;
			switch(code)
			{
#include "rop3list.h"
			}
			*ds = ( px        & 0xFF); ds++;
			*ds = ((px >>  8) & 0xFF); ds++;
			*ds = ((px >> 16) & 0xFF); ds++;
		}
	}
}

#undef ROP3ITEM
#undef ROP3ITEM_PREF

#endif /* ROP_EXPAND_LEVEL < 2 */

/******************************************************************************
 * tables                                                                     *
 ******************************************************************************/
#if ROP_EXPAND_LEVEL > 0

#define ROP3ITEM_PREF(_n, _code) blt_ ## _n ## _32,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) blt_small_32,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const rop32_h rop32_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) blt_ ## _n ## _16,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) blt_small_16,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const rop16_h rop16_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) blt_ ## _n ## _8,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) blt_small_8,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const rop8_h rop8_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) blt_ ## _n ## _24,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) blt_small_24,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const rop32_h rop24_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) stretchblt_ ## _n ## _32,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) stretchblt_small_32,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const stretchrop32_h stretchrop32_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) stretchblt_ ## _n ## _16,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) stretchblt_small_16,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const stretchrop16_h stretchrop16_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) stretchblt_ ## _n ## _8,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) stretchblt_small_8,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const stretchrop8_h stretchrop8_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

#define ROP3ITEM_PREF(_n, _code) stretchblt_ ## _n ## _24,
#if ROP_EXPAND_LEVEL == 1
# define ROP3ITEM(_n, _code) stretchblt_small_24,
#else
# define ROP3ITEM ROP3ITEM_PREF
#endif

static const stretchrop32_h stretchrop24_table[] =
{
	#include "rop3list.h"
};
#undef ROP3ITEM
#undef ROP3ITEM_PREF

// bitblt

static void rop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop8_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop16_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop24_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	(rop32_table[code])(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

// stretchblt
static void stretchrop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	(stretchrop32_table[code])(src, dst, pattern, rect, code);
}

static void stretchrop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	(stretchrop24_table[code])(src, dst, pattern, rect, code);
}

static void stretchrop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern, stretchBltRect *rect)
{
	(stretchrop16_table[code])(src, dst, pattern, rect, code);
}

static void stretchrop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern, stretchBltRect *rect)
{
	(stretchrop8_table[code])(src, dst, pattern, rect, code);
}

#else /* ROP_EXPAND_LEVEL > 0 */

static void rop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	blt_small_8(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	blt_small_16(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	blt_small_24(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

static void rop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	blt_small_32(src, dst, pattern, sx, sy, dx, dy, w, h, spitch, dpitch, code);
}

// stretchblt
static void stretchrop3_32(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	stretchblt_small_32(src, dst, pattern, rect, code);
}

static void stretchrop3_24(const uint8_t code, void *src, void *dst, uint32_t pattern, stretchBltRect *rect)
{
	stretchblt_small_24(src, dst, pattern, rect, code);
}

static void stretchrop3_16(const uint8_t code, void *src, void *dst, uint16_t pattern, stretchBltRect *rect)
{
	stretchblt_small_16(src, dst, pattern, rect, code);
}

static void stretchrop3_8(const uint8_t code, void *src, void *dst, uint8_t pattern, stretchBltRect *rect)
{
	stretchblt_small_8(src, dst, pattern, rect, code);
}

#endif /* ROP_EXPAND_LEVEL == 0 */

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

