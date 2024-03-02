/******************************************************************************
 * Copyright (c) 2024 Jaroslav Hensl                                          *
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

#include "fill.h"

#include "nocrt.h"

static uint32_t cfillcolor(uint32_t bpp, uint32_t color)
{
	uint32_t fillcolor = color;
	switch(bpp)
	{
		case 8:
			color &= 0xFF;
			fillcolor = (color << 24) | (color << 16) | (color << 8) | color;
			break;
		case 16:
			color &= 0xFFFF;
			fillcolor = (color << 16) | color;
			break;
	}
	
	return fillcolor;
}

void fill4(void *dst, size_t size, uint32_t bpp, uint32_t color)
{
	uint32_t fillcolor = cfillcolor(bpp, color);
	uint32_t cnt = size/4;
	uint32_t *vdst = dst;
	
	while(cnt--)
	{
		*vdst = fillcolor;
		vdst++;
	}
}

#ifdef __GNUC__

#ifdef __SSE__
typedef unsigned int v4x4 __attribute__ ((vector_size(16), aligned(16)));

void memcpy16(void *dst, const void *src, size_t size)
{
	if(((uint32_t)dst) % 16 != 0 || 
		((uint32_t)src)  % 16 != 0 ||
		((uint32_t)size) % 16 != 0
	)
	{
		/* not aligned, use regual memcpy */
		memcpy(dst, src, size);
		return;
	}
	
	uint32_t cnt = size/16;
	v4x4 *vsrc = (v4x4*)src;
	v4x4 *vdst = (v4x4*)dst;
	
	while(cnt--)
	{
		*vdst = *vsrc;
		vdst++;
		vsrc++;
	}
}

void fill16(void *dst, size_t size, uint32_t bpp, uint32_t color)
{
	if(((uint32_t)dst) % 16 != 0 || 
		((uint32_t)size) % 16 != 0
	)
	{
		/* not alignedy */
		fill4(dst, size, bpp, color);
		return;
	}
	
	uint32_t cnt = size/16;
	uint32_t fillcolor = cfillcolor(bpp, color);
	v4x4 c = {fillcolor, fillcolor, fillcolor, fillcolor};
	
	v4x4 *vdst = (v4x4*)dst;
	
	while(cnt--)
	{
		*vdst = c;
		vdst++;
	}
}

#endif /* __SSE__ */

#ifdef __MMX__
typedef unsigned int v2x4 __attribute__ ((vector_size(8), aligned(8)));

void memcpy8(void *dst, const void *src, size_t size)
{
	if(((uint32_t)dst) % 8 != 0 || 
		((uint32_t)src)  % 8 != 0 ||
		((uint32_t)size) % 8 != 0
	)
	{
		/* not aligned, use regual memcpy */
		memcpy(dst, src, size);
		return;
	}
	
	uint32_t cnt = size/8;
	v2x4 *vsrc = (v2x4*)src;
	v2x4 *vdst = (v2x4*)dst;
	
	while(cnt--)
	{
		*vdst = *vsrc;
		vdst++;
		vsrc++;
	}
}

void fill8(void *dst, size_t size, uint32_t bpp, uint32_t color)
{
	if(((uint32_t)dst) % 8 != 0 || 
		((uint32_t)size) % 8 != 0
	)
	{
		/* not alignedy */
		fill4(dst, size, bpp, color);
		return;
	}
	
	uint32_t cnt = size/8;
	uint32_t fillcolor = cfillcolor(bpp, color);
	v2x4 c = {fillcolor, fillcolor};
	
	v2x4 *vdst = (v2x4*)dst;
	
	while(cnt--)
	{
		*vdst = c;
		vdst++;
	}
}

#endif /* __MMX__ */

#endif /* __GNUC__ */
