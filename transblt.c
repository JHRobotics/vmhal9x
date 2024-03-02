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
#include <stdint.h>

#include "transblt.h"
#include "stretchblttf.h"

static void transblt32(void *src, void *dst, uint32_t transparent,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
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
			if(*ps != transparent)
			{
				*ds = *ps;
			}
			ps++;
			ds++;
		}
	}
}

static void transblt16(void *src, void *dst, uint16_t transparent,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
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
			if(*ps != transparent)
			{
				*ds = *ps;
			}
			ps++;
			ds++;
		}
	}
}

static void transblt8(void *src, void *dst, uint8_t transparent,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*sizeof(uint8_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*sizeof(uint8_t);
	
	for(y = 0; y < h; y++)
	{
		uint8_t *ps = psrc + spitch*y;
		uint8_t *ds = pdst + dpitch*y;
		for(x = 0; x < w; x++)
		{
			if(*ps != transparent)
			{
				*ds = *ps;
			}
			ps++;
			ds++;
		}
	}
}

static void transblt24(void *src, void *dst, uint32_t transparent,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	uint32_t x, y;
	uint8_t *psrc = (uint8_t *)src + spitch*sy + sx*3*sizeof(uint8_t);
	uint8_t *pdst = (uint8_t *)dst + dpitch*dy + dx*3*sizeof(uint8_t);
	
	for(y = 0; y < h; y++)
	{
		uint8_t *ps = psrc + spitch*y;
		uint8_t *ds = pdst + dpitch*y;
		for(x = 0; x < w; x++)
		{
			uint32_t test = (*ps) | ((*ps) << 8) | ((*ps) << 16);
			if(test != transparent)
			{
				*ds = *ps; ps++; ds++;
				*ds = *ps; ps++; ds++;
				*ds = *ps; ps++; ds++;
			}
			else
			{
				ps += 3;
				ds += 3;
			}
		}
	}
}

static void transstretchblt32(void *src, void *dst, uint32_t transparent, stretchBltRect *rect)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint32_t);
	
	for(y = 0; y < rect->drh; y++)
	{
		uint32_t *ds = (uint32_t*)(pdst + rect->dpitch*y);
		for(x = 0; x < rect->drw; x++)
		{
			const uint32_t px = stretch_32(src, x, y, rect);
			if(px != transparent)
			{
				*ds = px;
			}
			ds++;
		}
	}
}

static void transstretchblt16(void *src, void *dst, uint16_t transparent, stretchBltRect *rect)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint16_t);
	
	for(y = 0; y < rect->drh; y++)
	{
		uint16_t *ds = (uint16_t*)(pdst + rect->dpitch*y);
		for(x = 0; x < rect->drw; x++)
		{
			const uint16_t px = stretch_16(src, x, y, rect);
			if(px != transparent)
			{
				*ds = px;
			}
			ds++;
		}
	}
}

static void transstretchblt8(void *src, void *dst, uint8_t transparent, stretchBltRect *rect)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*sizeof(uint8_t);
	
	for(y = 0; y < rect->drh; y++)
	{
		uint8_t *ds = pdst + rect->dpitch*y;
		for(x = 0; x < rect->drw; x++)
		{
			const uint8_t px = stretch_8(src, x, y, rect);
			if(px != transparent)
			{
				*ds = px;
			}
			ds++;
		}
	}
}

static void transstretchblt24(void *src, void *dst, uint32_t transparent, stretchBltRect *rect)
{
	uint32_t x, y;
	uint8_t *pdst = (uint8_t *)dst + rect->dpitch*rect->dry + rect->drx*3*sizeof(uint8_t);
	
	for(y = 0; y < rect->drh; y++)
	{
		uint8_t *ds = pdst + rect->dpitch*y;
		for(x = 0; x < rect->drw; x++)
		{
			uint32_t px = stretch_24(src, x, y, rect);
			if(px != transparent)
			{
				*ds = (px      ) & 0xFF; ds++;
				*ds = (px >>  8) & 0xFF; ds++;
				*ds = (px >> 16) & 0xFF; ds++;
			}
			else
			{
				ds += 3;
			}
		}
	}
}

void transblt(uint8_t bpp, void *src, void *dst, uint32_t transparent,
	uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h, uint32_t spitch, uint32_t dpitch)
{
	switch(bpp)
	{
		case 8:
			transblt8(src, dst, transparent, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 16:
			transblt16(src, dst, transparent, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 24:
			transblt24(src, dst, transparent, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
		case 32:
			transblt32(src, dst, transparent, sx, sy, dx, dy, w, h, spitch, dpitch);
			break;
	}
}

void transstretchblt(uint8_t bpp, void *src, void *dst, uint32_t transparent, stretchBltRect *rect)
{
	switch(bpp)
	{
		case 8:
			transstretchblt8(src, dst, transparent, rect);
			break;
		case 16:
			transstretchblt16(src, dst, transparent, rect);
			break;
		case 24:
			transstretchblt24(src, dst, transparent, rect);
			break;
		case 32:
			transstretchblt32(src, dst, transparent, rect);
			break;
	}
}
