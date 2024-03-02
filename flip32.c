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

static void DoFlipping(VMDAHAL_t *ddhal, void *from, void *to)
{
	TRACE_ENTRY
	
	if(ddhal->pFBHDA32->flags & FB_SUPPORT_FLIPING) /* HW flip support */
	{
		DWORD offFrom = GetOffset(ddhal, from);
		if(offFrom == 0)
		{
			uint32_t offTo = GetOffset(ddhal, to);
			if(offTo != INVALID_OFFSET)
			{
				FBHDA_swap(offTo);
			}
		}
	}
	else /* nope, copy it to fixed frame buffer surface */
	{
		DWORD offFrom = GetOffset(ddhal, from);
		if(offFrom == 0)
		{
			uint32_t offTo = GetOffset(ddhal, to);
			if(offTo != INVALID_OFFSET)
			{
				CopyFront(ddhal, to);
			}
		}
		
	}
}

DWORD __stdcall Flip32(LPDDHAL_FLIPDATA pfd)
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
	 
	TRACE("Flip: %08lx -> %08lx",
		(uint32_t)pfd->lpSurfCurr->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32,
		(uint32_t)pfd->lpSurfTarg->lpGbl->fpVidMem - (uint32_t)ddhal->pFBHDA32->vram_pm32
	);

	DoFlipping(ddhal, (void*)pfd->lpSurfCurr->lpGbl->fpVidMem, (void*)pfd->lpSurfTarg->lpGbl->fpVidMem);
	
	pfd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
} /* Flip32 */

DWORD __stdcall GetFlipStatus32(LPDDHAL_GETFLIPSTATUSDATA pfd)
{
	TRACE_ENTRY
	
	/* we do nothing async yet, so return always success */	
	pfd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
} /* GetFlipStatus32 */
