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
#include <windows.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stdint.h>
#include "ddrawi_ddk.h"

#include "vmdahal32.h"

#include "vmhal9x.h"

#include "fill.h"

#include "nocrt.h"

DWORD GetOffset(VMDAHAL_t *ddhal, void *ptr)
{
	DWORD vram_begin = (DWORD)(ddhal->pFBHDA32->vram_pm32);
	DWORD vram_end   = vram_begin + ddhal->pFBHDA32->vram_size;
	DWORD dptr       = (DWORD)ptr;
	
	if(dptr >= vram_begin && dptr < vram_end)
	{
		return dptr - vram_begin;
	}
	
	return INVALID_OFFSET;
}

BOOL IsInFront(VMDAHAL_t *ddhal, void *ptr)
{
	DWORD offset = GetOffset(ddhal, ptr);
	
	if(offset >= ddhal->pFBHDA32->surface &&
		offset < (ddhal->pFBHDA32->surface + ddhal->pFBHDA32->stride))
	{
		return TRUE;
	}
	
	return FALSE;
}

static void CopyFront(VMDAHAL_t *ddhal, void *src)
{
	TRACE_ENTRY
	
	FBHDA_access_begin(0);
	
	fill_memcpy(ddhal->pFBHDA32->vram_pm32, src, ddhal->pFBHDA32->stride);
	
	FBHDA_access_end(0);
}

static void CopyFrontByLines(VMDAHAL_t *ddhal, void *src, DWORD src_pitch)
{
	TRACE_ENTRY
	
	FBHDA_access_begin(0);
	
	uint8_t *pdst = ddhal->pFBHDA32->vram_pm32;
	uint8_t *psrc = src;
	DWORD y;
	
	DWORD copy_size = src_pitch;
	if(src_pitch > ddhal->pFBHDA32->pitch)
	{
		copy_size = ddhal->pFBHDA32->pitch;
	}
	
	for(y = 0; y < ddhal->pFBHDA32->height; y++)
	{
		fill_memcpy(pdst, psrc, copy_size);
		
		pdst += ddhal->pFBHDA32->pitch;
		psrc += src_pitch;
	}
	
	FBHDA_access_end(0);
}

static void DoFlipping(VMDAHAL_t *ddhal, void *from, void *to, DWORD from_pitch, DWORD to_pitch)
{
	TRACE_ENTRY
	
	TOPIC("ENTRY", "DoFlipping(ddhal, %X, %X, %d, %d)", from, to, from_pitch, to_pitch);

	//DWORD offFrom = GetOffset(ddhal, from);
	//if(offFrom == ddhal->pFBHDA32->surface)
	if(1)
	{
		uint32_t offTo = GetOffset(ddhal, to);
		if(offTo != INVALID_OFFSET)
		{
			if(from_pitch != to_pitch) /* surface is not same as screen */
			{
				CopyFrontByLines(ddhal, to, to_pitch);
			}
			else if(ddhal->pFBHDA32->flags & FB_SUPPORT_FLIPING) /* HW flip support */
			{
				if(!FBHDA_swap(offTo, FBHDA_SWAP_NOWAIT))
				{
					ERR("FBHDA_swap failed (flip)");
				}
			}
			else /* nope, copy it to fixed frame buffer surface */
			{
				CopyFront(ddhal, to);
			}
		}
	}
}

BOOL FlipPrimary(VMDAHAL_t *ddhal, void *to)
{
	if(ddhal->pFBHDA32->flags & FB_SUPPORT_FLIPING) /* HW flip support */
	{
		uint32_t offTo = GetOffset(ddhal, to);
		if(!FBHDA_swap(offTo, 0))
		{
			ERR("SWAP failed, offset=%d", offTo);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

uint64_t last_flip_time = 0;

volatile BOOL is_flipping = FALSE;

DDENTRY_FPUSAVE(Flip32, LPDDHAL_FLIPDATA, pfd)
{
	TRACE_ENTRY
	/*
	 * NOTES:
	 *
	 * This callback is invoked whenever we are about to flip to from
	 * one surface to another. pfd->lpSurfCurr is the surface we were
	 * at, pfd->lpSurfTarg is the one we are flipping to.
	 *
	 * You should point the hardware registers at the new surface, and
	 * also keep track of the surface that was flipped away from, so
	 * that if the user tries to lock it, you can be sure that it is
	 * done being displayed
	 */
	VMDAHAL_t *ddhal = GetHAL(pfd->lpDD);	
	uint64_t flip_time = GetTimeTMS();

	if(is_flipping)
	{
		pfd->ddRVal = DDERR_WASSTILLDRAWING;
		return DDHAL_DRIVER_HANDLED;
	}

	if(halVSync && (pfd->dwFlags & DDFLIP_NOVSYNC) == 0)
	{
		if(last_flip_time <= flip_time) /* not in overflow */
		{
			if(last_flip_time + SCREEN_TIME > flip_time)
			{
				pfd->ddRVal = DDERR_WASSTILLDRAWING;
				return DDHAL_DRIVER_HANDLED;
			}
		}
	}

	if(pfd->lpSurfCurr->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		/* if currend surface is primary, do switching */
		TRACE("Flip: %08lx -> %08lx",
			(uint32_t)pfd->lpSurfCurr->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32,
			(uint32_t)pfd->lpSurfTarg->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32
		);
		
		TOPIC("FLIP", "Flip: 0x%08X -> 0x%08X [0x%08X]",
			(uint32_t)pfd->lpSurfCurr->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32,
			(uint32_t)pfd->lpSurfTarg->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32,
			16*1024*1024
		);

#ifdef D3DHAL
		SurfaceCtxLock();
#endif
		is_flipping = TRUE;

		TOPIC("READBACK", "Flip: %X -> %X",
			(uint32_t)pfd->lpSurfCurr->lpGbl->fpVidMem,
			(uint32_t)pfd->lpSurfTarg->lpGbl->fpVidMem);

		//SurfaceFlipMesa(pfd->lpSurfCurr, pfd->lpSurfTarg);

		DoFlipping(ddhal, (void*)pfd->lpSurfCurr->lpGbl->fpVidMem, (void*)pfd->lpSurfTarg->lpGbl->fpVidMem,
			pfd->lpSurfCurr->lpGbl->lPitch, pfd->lpSurfTarg->lpGbl->lPitch);
		
		TOPIC("TARGET", "Fliped (on screen: %X)", pfd->lpSurfTarg->lpGbl->fpVidMem);
		
		is_flipping = FALSE;
#ifdef D3DHAL
		SurfaceCtxUnlock();
#endif
	}
#if 1
	else
	{
		/* surface isn't primary, so only refresh primary surface */
#ifdef D3DHAL
		SurfaceCtxLock();
#endif
		is_flipping = TRUE;
		FBHDA_access_begin(0);
		
		// ...

		FBHDA_access_end(0);
		TOPIC("READBACK", "Flip, refresh: %X (!)", pfd->lpSurfTarg->lpGbl->fpVidMem);
		is_flipping = FALSE;
#ifdef D3DHAL
		SurfaceCtxUnlock();
#endif
	}
#endif

	last_flip_time = flip_time;
	pfd->ddRVal = DD_OK;

	return DDHAL_DRIVER_HANDLED;
} /* Flip32 */

DDENTRY_FPUSAVE(GetFlipStatus32, LPDDHAL_GETFLIPSTATUSDATA, pfd)
{
	TRACE_ENTRY
	VMDAHAL_t *ddhal = GetHAL(pfd->lpDD);	
	
	if(is_flipping)
	{
		pfd->ddRVal = DDERR_WASSTILLDRAWING;
		return DDHAL_DRIVER_HANDLED;
	}
	else if(ddhal->pFBHDA32->onflip)
	{
		FBHDA_swap(0, FBHDA_SWAP_QUERY);
		if(ddhal->pFBHDA32->onflip)
		{
			pfd->ddRVal = DDERR_WASSTILLDRAWING;
			return DDHAL_DRIVER_HANDLED;
		}
	}

	if(pfd->dwFlags == DDGFS_CANFLIP && halVSync)
	{
		uint64_t flip_time = GetTimeTMS();
		if(last_flip_time <= flip_time) /* not in overflow */
		{
			if(last_flip_time + SCREEN_TIME > flip_time)
			{
				pfd->ddRVal = DDERR_WASSTILLDRAWING;
				return DDHAL_DRIVER_HANDLED;
			}
		}
	}
	else if(pfd->dwFlags == DDGFS_ISFLIPDONE)
	{
		if(!IsInFront(ddhal, (void*)pfd->lpDDSurface->lpGbl->fpVidMem))
		{
			pfd->ddRVal = DDERR_WASSTILLDRAWING;
			return DDHAL_DRIVER_HANDLED;
		}
	}
	
	/* we do nothing async yet, so return always success */	
	pfd->ddRVal = DD_OK;
	
	return DDHAL_DRIVER_HANDLED;
} /* GetFlipStatus32 */
