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
#ifndef __SURFACE_H__INCLUDED__
#define __SURFACE_H__INCLUDED__

typedef struct surface_info surface_info_t;

typedef struct _DDSURF_cache
{
	BOOL color_key;
	DWORD dwColorKeyLow;
	DWORD dwColorKeyHigh;
	DWORD dwColorKeyLowPal;
	DWORD dwColorKeyHighPal;
	DWORD pal_stamp;
	DWORD *data;
} DDSURF_cache_t;

#define DDSURF_ATTACH_MAX (6*16)

typedef struct _DDSURF
{
	FLATPTR fpVidMem;
	LPDDRAWI_DDRAWSURFACE_GBL lpGbl;
	LPDDRAWI_DDRAWSURFACE_LCL lpLclDX7; // DX7 and older only!
	DWORD dwFlags;
	DWORD dwCaps;
	DWORD dwCaps2;
	DWORD dwSurfaceHandle;
	DWORD dwColorKeyLow;
	DWORD dwColorKeyHigh;
	DWORD dwPaletteHandle;
	DWORD dwPaletteFlags;
	DWORD dwColorKeyLowPal;
	DWORD dwColorKeyHighPal;
	DWORD attachments_cnt;
	DDSURF_cache_t *cache;
	surface_id attachments[DDSURF_ATTACH_MAX];
} DDSURF;

LPDDRAWI_DDRAWSURFACE_LCL SurfaceDuplicate(LPDDRAWI_DDRAWSURFACE_LCL original);

NUKED_LOCAL DDSURF *SurfaceGetSURF(surface_id sid);
NUKED_LOCAL void *SurfaceGetVidMem(surface_id sid);
NUKED_LOCAL void SurfaceAttachTexture(surface_id sid, void *mesa_tex, int level, int side);
NUKED_LOCAL void SurfaceDeattachTexture(surface_id sid, void *mesa_tex, int level, int side);
NUKED_LOCAL void SurfaceAttachCtx(void *mesa_ctx);
NUKED_LOCAL void SurfaceDeattachCtx(void *mesa_ctx);
NUKED_LOCAL LPDDRAWI_DDRAWSURFACE_LCL SurfaceGetLCL_DX7(surface_id sid);

/* private, but for debuging sometimes needed in other files */
struct surface_attachment;

typedef struct surface_info
{
	DWORD magic;
	struct surface_attachment *first;
	DDSURF surf;
	DWORD flags;
	DWORD lock;
#ifdef DEBUG_MEMORY
	DWORD id;
#endif
} surface_info_t;

#endif /* __SURFACE_H__INCLUDED__ */
