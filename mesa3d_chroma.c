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
#ifndef NUKED_SKIP
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

NUKED_FAST void *MesaChroma32(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const DWORD *ptr = buf;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD px = ptr[x] & 0x00FFFFFF;
				if(!(px >= lwkey && px <= hikey))
				{
					px |= 0xFF000000;
				}
				out[x] = px;
			}
			
			ptr += pitch4;
			out += pitch4;
		}
	}
	
	return mem;
}

NUKED_FAST void *MesaChroma24(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const BYTE *ptr = buf;
	DWORD pitch_src = SurfacePitch(w, 24);
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD px = (*((DWORD*)(ptr+3*x))) & 0x00FFFFFF;
				if(!(px >= lwkey && px <= hikey))
				{
					px |= 0xFF000000;
				}
				out[x] = px;
			}
			
			ptr += pitch_src;
			out += pitch4;
		}
	}
	
	return mem;
}

NUKED_FAST void *MesaChroma16(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD spx = ptr[x];
				DWORD px = 0;

				px |= (spx & 0x001F) << 3;
				px |= (spx & 0x07E0) << 5;
				px |= (spx & 0xF800) << 8;
				
				if(!(px >= lwkey && px <= hikey))
				{
					px |= 0xFF000000;
				}
				
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

NUKED_FAST void *MesaChroma15(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD spx = ptr[x] & 0x7FFF;
				DWORD px = 0x0;
				px |= (spx & 0x001F) << 3;
				px |= (spx & 0x03E0) << 6;
				px |= (spx & 0x7C00) << 9;
				
				if(!(px >= lwkey && px <= hikey))
				{
					px |= 0xFF000000;
				}
				
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

NUKED_FAST void *MesaChroma12(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD spx = ptr[x] & 0x0FFF;
				DWORD px = 0;
				px |= (spx & 0x000F) << 4;
				px |= (spx & 0x00F0) << 8;
				px |= (spx & 0x0F00) << 12;
				
				if(px >= lwkey && px <= hikey)
				{
					px |= 0xFF000000;
				}
				
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

NUKED_FAST void MesaChromaFree(mesa3d_ctx_t *ctx, void *ptr)
{
	MesaTempFree(ctx, ptr);
}

/* ported from Wine9x/surface.c */
NUKED_FAST void *MesaChromaDXT1(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const BYTE *src = buf;
	DWORD src_pitch = w*2; // w * 8
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	
	unsigned int x, y, i;
	DWORD *dst_line[4];

	for(y = 0; y < h; y += 4)
	{
		const BYTE *src_line = src + (y / 4) * src_pitch;
		dst_line[0] = mem + y  * pitch4;
		dst_line[1] = mem + (y + 1) * pitch4;
		dst_line[2] = mem + (y + 2) * pitch4;
		dst_line[3] = mem + (y + 3) * pitch4;

		for(x = 0; x < w; x += 4)
		{
			const DWORD c0_16 = *((WORD*)(src_line + 2*x));
			const DWORD c1_16 = *((WORD*)(src_line + 2*x + 2));

			DWORD cmap[4];
			
			// separate the colors (and move to bit position)
			const DWORD r0 = (c0_16 & 0x0F00) << 12;
			const DWORD g0 = (c0_16 & 0x00F0) << 8;
			const DWORD b0 = (c0_16 & 0x000F) << 4;

			const DWORD r1 = (c1_16 & 0x0F00) << 12;
			const DWORD g1 = (c1_16 & 0x00F0) << 8;
			const DWORD b1 = (c1_16 & 0x000F) << 4;
			
			// RGB565 to RGBA8888
			cmap[0]  = r0 | g0 | b0;
			cmap[1]  = r1 | g1 | b1;

			/* Interpolate intermediate colours */
			if(c0_16 > c1_16)
			{
				// (2*color0 + color1) / 3 
				cmap[2]  = 0xFF000000;
				cmap[2] |= ((2*r0 + r1)/3) & 0x00FF0000;
				cmap[2] |= ((2*g0 + g1)/3) & 0x0000FF00;
				cmap[2] |= ((2*b0 + b1)/3) & 0x000000FF;
				
				// (color0 + 2*color1) / 3
				cmap[3]  = 0xFF000000;
				cmap[3] |= ((r0 + 2*r1)/3) & 0x00FF0000;
				cmap[3] |= ((g0 + 2*g1)/3) & 0x0000FF00;
				cmap[3] |= ((r0 + 2*b1)/3) & 0x000000FF;
			}
			else
			{
				// (color0 + color1) / 2 
				cmap[2]  = 0xFF000000;
				cmap[2] = ((r0 + r1)/2) & 0x00FF0000;
				cmap[2] = ((g0 + g1)/2) & 0x0000FF00;
				cmap[2] = ((b0 + b1)/2) & 0x000000FF;
				
				// Black
				cmap[3] = 0xFF000000;
			}

			if(!(cmap[0] >= lwkey && cmap[0] <= hikey))
				cmap[0] |= 0xFF000000;

			if(!(cmap[1] >= lwkey && cmap[1] <= hikey))
				cmap[1] |= 0xFF000000;

			if(!(cmap[2] >= lwkey && cmap[2] <= hikey))
				cmap[2] |= 0xFF000000;

			if(!(cmap[3] >= lwkey && cmap[3] <= hikey))
				cmap[3] |= 0xFF000000;

			DWORD bitmap = *((DWORD*)(src_line + 2 * x + 4));
      
      for(i = 0; i < 16; i++)
      {
      	dst_line[i/4][i%4] = cmap[(bitmap >> (i*2)) & 0x3];
      }
		}
	}
	
	return mem;
}

NUKED_FAST void *MesaChromaDXT3(mesa3d_ctx_t *ctx, const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const BYTE *src = buf;
	DWORD src_pitch = w*4; // w * 8
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = MesaTempAlloc(ctx, w, pitch4*4*h);
	
	unsigned int x, y, i;
	DWORD *dst_line[4];

	for(y = 0; y < h; y += 4)
	{
		const BYTE *src_line = src + (y / 4) * src_pitch;
		dst_line[0] = mem + y  * pitch4;
		dst_line[1] = mem + (y + 1) * pitch4;
		dst_line[2] = mem + (y + 2) * pitch4;
		dst_line[3] = mem + (y + 3) * pitch4;

		for(x = 0; x < w; x += 4)
		{
			const uint64_t alpha = *((uint64_t*)(src_line + 4*x));
			const DWORD c0_16 = *((WORD*)(src_line + 4*x + 4));
			const DWORD c1_16 = *((WORD*)(src_line + 4*x + 6));

			DWORD cmap[4];
			
			// separate the colors (and move to bit position)
			const DWORD r0 = (c0_16 & 0x0F00) << 12;
			const DWORD g0 = (c0_16 & 0x00F0) << 8;
			const DWORD b0 = (c0_16 & 0x000F) << 4;

			const DWORD r1 = (c1_16 & 0x0F00) << 12;
			const DWORD g1 = (c1_16 & 0x00F0) << 8;
			const DWORD b1 = (c1_16 & 0x000F) << 4;
			
			// RGB565 to RGBA8888
			cmap[0]  = r0 | g0 | b0;
			cmap[1]  = r1 | g1 | b1;

			/* Interpolate intermediate colours */
			// (color0 + color1) / 2 
			cmap[2]  = 0xFF000000;
			cmap[2] = ((r0 + r1)/2) & 0x00FF0000;
			cmap[2] = ((g0 + g1)/2) & 0x0000FF00;
			cmap[2] = ((b0 + b1)/2) & 0x000000FF;

			// Black
			cmap[3] = 0xFF000000;

			DWORD bitmap = *((DWORD*)(src_line + 2 * x + 4));
      
      for(i = 0; i < 16; i++)
      {
      	DWORD px = cmap[(bitmap >> (i*2)) & 0x3];

	    	if(!(px >= lwkey && px <= hikey))
	    	{
	    		px |= (((alpha >> (i*4)) & 0xF) * (17*16777216));
	    	}

	    	dst_line[i/4][i%4] = px;
	    }
		}
	}
	
	return mem;
}
