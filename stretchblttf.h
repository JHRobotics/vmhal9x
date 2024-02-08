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
#ifndef __STRETCH_BLTTF_H__INCLUDED__
#define __STRETCH_BLTTF_H__INCLUDED__

#include "stretchblt.h"

#ifndef INLINE
# ifdef __GNUC__
#  define INLINE static inline
# else
#  define INLINE static
# endif
#endif


INLINE uint32_t stretch_32(const void *src, const uint32_t x, const uint32_t y, const stretchBltRect *rect)
{
	// calculate stretch offset
	uint32_t px = (x*rect->srw) / rect->drw;
	uint32_t py = (y*rect->srh) / rect->drh;
	
	// mirror offset if needed
	if(rect->mirrorx)	px = rect->srw - px - 1;	
	if(rect->mirrory) py = rect->srh - py - 1;
	
	// absolute coordinates in source
	px += rect->srx;
	px += rect->sry;
	
	// check bounds
	if(px >= rect->sw) px = rect->sw - 1;
	if(py >= rect->sh) py = rect->sh - 1;
	
	// return pixel
	return ((uint32_t*)((uint8_t*)src + rect->spitch*py))[px];
}

INLINE uint16_t stretch_16(const void *src, const uint32_t x, const uint32_t y, const stretchBltRect *rect)
{
	// calculate stretch offset
	uint32_t px = (x*rect->srw) / rect->drw;
	uint32_t py = (y*rect->srh) / rect->drh;
	
	// mirror offset if needed
	if(rect->mirrorx)	px = rect->srw - px - 1;	
	if(rect->mirrory) py = rect->srh - py - 1;
	
	// absolute coordinates in source
	px += rect->srx;
	px += rect->sry;
	
	// check bounds
	if(px >= rect->sw) px = rect->sw - 1;
	if(py >= rect->sh) py = rect->sh - 1;
	
	// return pixel
	return ((uint16_t*)((uint8_t*)src + rect->spitch*py))[px];
}

INLINE uint8_t stretch_8(const void *src, const uint32_t x, const uint32_t y, const stretchBltRect *rect)
{
	// calculate stretch offset
	uint32_t px = (x*rect->srw) / rect->drw;
	uint32_t py = (y*rect->srh) / rect->drh;
	
	// mirror offset if needed
	if(rect->mirrorx)	px = rect->srw - px - 1;	
	if(rect->mirrory) py = rect->srh - py - 1;
	
	// absolute coordinates in source
	px += rect->srx;
	px += rect->sry;
	
	// check bounds
	if(px >= rect->sw) px = rect->sw - 1;
	if(py >= rect->sh) py = rect->sh - 1;
	
	// return pixel
	return ((uint8_t*)src + rect->spitch*py)[px];
}

INLINE uint32_t stretch_24(const void *src, const uint32_t x, const uint32_t y, const stretchBltRect *rect)
{
	// calculate stretch offset
	uint32_t px = (x*rect->srw) / rect->drw;
	uint32_t py = (y*rect->srh) / rect->drh;
	
	// mirror offset if needed
	if(rect->mirrorx)	px = rect->srw - px - 1;	
	if(rect->mirrory) py = rect->srh - py - 1;
	
	// absolute coordinates in source
	px += rect->srx;
	px += rect->sry;
	
	// check bounds
	if(px >= rect->sw) px = rect->sw - 1;
	if(py >= rect->sh) py = rect->sh - 1;
	
	// return pixel
	return *((uint32_t*)((uint8_t*)src + rect->spitch*py + px*3)) & 0x00FFFFFF; // low endian only!
}

#endif /* __STRETCH_BLTTF_H__INCLUDED__ */
