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
#ifndef __STRETCHBLT_H__INCLUDED__
#define __STRETCHBLT_H__INCLUDED__

typedef struct _stretchBltRect
{
	/* source rectage */
	uint32_t srx;
	uint32_t sry;
	uint32_t srw;
	uint32_t srh;
	
	/* source surface */
	uint32_t sw;
	uint32_t sh;
	uint32_t spitch;
	
	/* destination rectagle */
	uint32_t drx;
	uint32_t dry;
	uint32_t drw;
	uint32_t drh;
	
	/* destination source */
	uint32_t dw;
	uint32_t dh;
	uint32_t dpitch;
	
	/* mirroring */
	uint32_t mirrorx;
	uint32_t mirrory;
} stretchBltRect;

#endif /* __STRETCHBLT_H__INCLUDED__ */
