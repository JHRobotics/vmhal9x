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

extern HANDLE hSharedHeap;

void *MesaChroma32(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const DWORD *ptr = buf;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = HeapAlloc(hSharedHeap, 0, pitch4*4*h);
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

void *MesaChroma24(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const BYTE *ptr = buf;
	DWORD pitch_src = SurfacePitch(w, 24);
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = HeapAlloc(hSharedHeap, 0, pitch4*4*h);
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

void *MesaChroma16(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = HeapAlloc(hSharedHeap, 0, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD px = 0xFF000000;
				DWORD spx = ptr[x];
				if(spx >= lwkey && spx <= hikey)
				{
					px = 0;
				}
				px |= (spx & 0x001F) << 3;
				px |= (spx & 0x07E0) << 5;
				px |= (spx & 0xF800) << 8;
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

void *MesaChroma15(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = HeapAlloc(hSharedHeap, 0, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD px = 0xFF000000;
				DWORD spx = ptr[x] & 0x7FFF;
				if(spx >= lwkey && spx <= hikey)
				{
					px = 0;
				}
				px |= (spx & 0x001F) << 3;
				px |= (spx & 0x03E0) << 6;
				px |= (spx & 0x7C00) << 9;
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

void *MesaChroma12(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey)
{
	const WORD *ptr = buf;
	DWORD pitch2 = SurfacePitch(w, 16)/2;
	DWORD pitch4 = SurfacePitch(w, 32)/4;
	DWORD *mem = HeapAlloc(hSharedHeap, 0, pitch4*4*h);
	DWORD *out = mem;

	if(out)
	{
		DWORD x, y;

		for(y = 0; y < h; y++)
		{
			for(x = 0; x < w; x++)
			{
				DWORD px = 0xFF000000;
				DWORD spx = ptr[x] & 0x0FFF;
				if(spx >= lwkey && spx <= hikey)
				{
					px = 0;
				}
				px |= (spx & 0x000F) << 4;
				px |= (spx & 0x00F0) << 8;
				px |= (spx & 0x0F00) << 12;
				out[x] = px;
			}
			
			ptr += pitch2;
			out += pitch4;
		}
	}
	
	return mem;
}

void MesaChromaFree(void *ptr)
{
	HeapFree(hSharedHeap, 0, ptr);
}

