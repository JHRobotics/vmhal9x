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
#include "ddrawi_ddk.h"

#include "vmdahal32.h"

#include "vmhal9x.h"
#include "rop3.h"
#include "transblt.h"

#include "nocrt.h"
/*
 * Blt32
 *
 * 32-bit blit routine.
 */
DWORD __stdcall Blt32(LPDDHAL_BLTDATA pbd)
{
	TRACE_ENTRY
	
	VMDAHAL_t *ddhal = GetHAL(pbd->lpDD);	
	
	DWORD	   dwFlags;	// For specifying the type of blit

	DWORD dwFillColor;	  // Used to specify the RGB color to fill with
	DWORD dwColorKey;	   // Holds our color key for a transparent blit

	DWORD dwDstOffset;	  // offset to beginning of destination surface
	long  dwDstWidth;	   // width of destination surface (in pixels)
	long  dwDstHeight;	  // height of destination surface (in rows)

	DWORD dwSrcOffset;	  // offset to the beginning of source surface
	long  dwSrcWidth;	   // width of source surface (in pixels)
	long  dwSrcHeight;	  // height of source surface (in rows)

	LPDDRAWI_DDRAWSURFACE_LCL  srcx; // local pointer to source surface
	LPDDRAWI_DDRAWSURFACE_LCL  dstx; // local pointer to destination surface
	LPDDRAWI_DDRAWSURFACE_GBL  src;  // global pointer to source surface
	LPDDRAWI_DDRAWSURFACE_GBL  dst;  // global pointer to destination surface
	
	BOOL isInFront = FALSE; // destination is front surface
	
	dstx = pbd->lpDDDestSurface;	   // destination surface
	dst = dstx->lpGbl;			         // destination data
	
	dwFlags = pbd->dwFlags;

	isInFront = IsInFront(ddhal, (void*)dst->fpVidMem);

	/* remove cursor before blit */
	if(isInFront)
	{
		FBHDA_access_begin(0);
	}

	/*
	 * NOTES:
	 *
	 * Everything you need is in pdb->bltFX .
	 * Look at pdb->dwFlags to determine what kind of blit you are doing,
	 * DDBLT_xxxx are the flags.
	 *
	 * COLORKEY NOTES:
	 *
	 * ColorKey ALWAY comes in BLTFX. You don't have to look it up in
	 * the surface.
	 */

	/*
	 * is a flip in pending on our destination surface?  If so, pass
	 * ddrval return value and exit
	 */
	// JH: flip is always synchronous
	
	/*
	 * If async, then only work if blitter isn't busy
	 * This should probably be a little more specific to each call, but
	 * this is pretty close
	 */
	// JH: blit is always synchronous

	/*
	 * get offset, width, and height for destination
	 */
	dwDstOffset = GetOffset(ddhal, (void*)dst->fpVidMem);
	dwDstWidth  = pbd->rDest.right  - pbd->rDest.left;
	dwDstHeight = pbd->rDest.bottom - pbd->rDest.top;

	/* 
	 * Handle ROPs such as copy, transparent blits and colorfills
	 */
	if(dwFlags & DDBLT_ROP)
	{
		uint8_t rop3_code = (pbd->bltFX.dwROP >> 16) & 0xFF;
		
		srcx = pbd->lpDDSrcSurface;   
		src = srcx->lpGbl;

		dwSrcOffset = GetOffset(ddhal, (void*)src->fpVidMem);
		dwSrcWidth  = pbd->rSrc.right  - pbd->rSrc.left;
		dwSrcHeight = pbd->rSrc.bottom - pbd->rSrc.top;
		
		dwColorKey  = pbd->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
		
		/* check if need stretch */
		if(dwDstWidth != dwSrcWidth ||
			dwDstHeight != dwSrcHeight ||
			dwSrcWidth < 0 ||
			dwDstHeight < 0)
		{
			stretchBltRect srect;
			srect.mirrorx = 0;
			srect.mirrory = 0;
			srect.srx = pbd->rSrc.left;
			srect.sry = pbd->rSrc.top;

			if(dwSrcWidth < 0)
			{
				srect.srw = -dwSrcWidth;
				srect.mirrorx ^= 1;
			}
			else
			{
				srect.srw = dwSrcWidth;
			}

			if(dwSrcHeight < 0)
			{
				srect.srh = -dwSrcHeight;
				srect.mirrory = srect.mirrory ^ 1;
			}
			else
			{
				srect.srh = dwSrcHeight;
			}
			
			srect.drx = pbd->rDest.left;
			srect.dry = pbd->rDest.top;
			
			if(dwDstWidth < 0)
			{
				srect.drw = -dwDstWidth;
				srect.mirrorx ^= 1;
			}
			else
			{
				srect.drw = dwDstWidth;
			}
			
			if(dwDstHeight < 0)
			{
				srect.drh = -dwDstHeight;
				srect.mirrory ^= 1;
			}
			else
			{
				srect.drh = dwDstHeight;
			}
			
			srect.spitch = srect.dpitch = ddhal->pFBHDA32->pitch;
			srect.sw = srect.dw = ddhal->pFBHDA32->width;
			srect.sh = srect.dh = ddhal->pFBHDA32->height;
			
			if (rop3_code == (SRCCOPY >> 16) && (dwFlags & DDBLT_KEYSRCOVERRIDE))
			{
				TRACE("Blt: transstretchblt: w = %l -> %l, h = %l -> %l",
					dwSrcWidth, dwDstWidth, dwSrcHeight, dwDstHeight);
				
				transstretchblt(ddhal->pFBHDA32->bpp,
					(void*)src->fpVidMem, (void*)dst->fpVidMem, dwColorKey, &srect);
			}
			else
			{
				TRACE("Blt: stretchrop3: w = %l -> %l, h = %l -> %l",
					dwSrcWidth, dwDstWidth, dwSrcHeight, dwDstHeight);
				
				stretchrop3(ddhal->pFBHDA32->bpp, rop3_code,
					(void*)src->fpVidMem, (void*)dst->fpVidMem, dwColorKey, &srect);
			}
			
		}
		else /* ROP 1:1 pixels */
		{
			if (rop3_code == (SRCCOPY >> 16) && (dwFlags & DDBLT_KEYSRCOVERRIDE))
			{
				TRACE("Blt: transblt from %08X %dx%d -> %08X %dx%dx%d key=%08X",
					dwSrcOffset, dwSrcWidth, dwSrcHeight, dwDstOffset, dwDstWidth, dwDstHeight, ddhal->pFBHDA32->bpp, dwColorKey);
				
				transblt(ddhal->pFBHDA32->bpp, (void*)src->fpVidMem, (void*)dst->fpVidMem, dwColorKey,
							pbd->rSrc.left, pbd->rSrc.top, pbd->rDest.left, pbd->rDest.top, dwSrcWidth, dwSrcHeight, ddhal->pFBHDA32->pitch);
			}
			else
			{
				TRACE("Blt: rop3 (%02X) from %08X %dx%d -> %08X %dx%dx%d", 
						rop3_code, dwSrcOffset, dwSrcWidth, dwSrcHeight, dwDstOffset, dwDstWidth, dwDstHeight, ddhal->pFBHDA32->bpp);
				
				TRACE("Rect: %dx%d (%dx%d) -> %dx%d|pitch %d|flags %X",
						pbd->rSrc.left, pbd->rSrc.top, dwSrcWidth, dwSrcHeight, pbd->rDest.left, pbd->rDest.top, ddhal->pFBHDA32->pitch, dwFlags);
				
				rop3(ddhal->pFBHDA32->bpp, rop3_code, (void*)src->fpVidMem, (void*)dst->fpVidMem, dwColorKey,
							pbd->rSrc.left, pbd->rSrc.top, pbd->rDest.left, pbd->rDest.top, dwSrcWidth, dwSrcHeight, ddhal->pFBHDA32->pitch);
			}
		}
	}
	else if (dwFlags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL))
	{
		dwFillColor = pbd->bltFX.dwFillColor;
		if(dwFlags & DDBLT_DEPTHFILL)
		{
			dwFillColor = pbd->bltFX.dwFillDepth;
		}

		TRACE("Blt: rop3 (F0) %08X %dx%d", dwFillColor, dwDstWidth, dwDstHeight);

		rop3(ddhal->pFBHDA32->bpp, 0xF0, (void*)dst->fpVidMem, (void*)dst->fpVidMem, dwFillColor,
				pbd->rDest.left, pbd->rDest.top, pbd->rDest.left, pbd->rDest.top, dwDstWidth, dwDstHeight, ddhal->pFBHDA32->pitch);
	}
	else
	{
		if(isInFront)
		{
			FBHDA_access_end(0);
		}
		
		// This is where you would handle extra ROPs
		WARN("invalid BLR %X", dwFlags);
		return DDHAL_DRIVER_NOTHANDLED;
		// we couldn't handle the blt, pass it on to HEL
	}
	
	if(isInFront)
	{
		FBHDA_access_end(0);
	}
	
	TRACE("BLT32 done");
	
	pbd->ddRVal = DD_OK;		 // pass the return value 
	return DDHAL_DRIVER_HANDLED; // Blt successful, exit function
} /* Blt32 */

DWORD __stdcall GetBltStatus32(LPDDHAL_GETBLTSTATUSDATA pbd)
{
	TRACE_ENTRY
	
	/* we do nothing async yet, so return always success */	
	pbd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
} /* GetFlipStatus32 */
