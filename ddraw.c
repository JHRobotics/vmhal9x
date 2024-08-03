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

#include "nocrt.h"

#define IS_GLOBAL_ADDR(_ptr) (((DWORD)_ptr) >= 0x80000000UL)

VMDAHAL_t *GetHAL(LPDDRAWI_DIRECTDRAW_GBL lpDD)
{
	if(IS_GLOBAL_ADDR(lpDD->dwReserved3)) // running on DX5 or greater?
		return (VMDAHAL_t*)lpDD->dwReserved3;
	else
		return globalHal;         // our global value for this
}

/* surfaces */

DWORD __stdcall CanCreateSurface(LPDDHAL_CANCREATESURFACEDATA pccsd)
{
	TRACE_ENTRY
	
	VMDAHAL_t *hal = GetHAL(pccsd->lpDD);
	if(!hal) return DDHAL_DRIVER_HANDLED;

	if(!pccsd->bIsDifferentPixelFormat)
	{
//		UpdateCustomMode(hal);
		
		pccsd->ddRVal = DD_OK;
		return DDHAL_DRIVER_HANDLED;
	}

	pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
	return DDHAL_DRIVER_HANDLED;
} /* CanCreateSurface */

DWORD __stdcall CreateSurface(LPDDHAL_CREATESURFACEDATA pcsd)
{
	TRACE_ENTRY
	
 	VMDAHAL_t *hal = GetHAL(pcsd->lpDD);
  if(!hal) return DDHAL_DRIVER_NOTHANDLED;

	pcsd->ddRVal = DD_OK;
	return DDHAL_DRIVER_NOTHANDLED;

} /* CreateSurface32 */

DWORD __stdcall DestroySurface( LPDDHAL_DESTROYSURFACEDATA lpd)
{
	TRACE_ENTRY
	
 	VMDAHAL_t *hal = GetHAL(lpd->lpDD);
  if(!hal) return DDHAL_DRIVER_NOTHANDLED;
	
	lpd->ddRVal = DD_OK;
	return DDHAL_DRIVER_NOTHANDLED;
}

/* return tenths of millisecionds */
uint64_t GetTimeTMS()
{
	LARGE_INTEGER freq, stamp;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&stamp);
	
	freq.QuadPart /= 10*1000;
	
	return stamp.QuadPart / freq.QuadPart;
}

DWORD __stdcall WaitForVerticalBlank32(LPDDHAL_WAITFORVERTICALBLANKDATA pwd)
{
	TRACE_ENTRY
	
	switch(pwd->dwFlags)
	{
		case DDWAITVB_I_TESTVB:
			/* just request for current blank status */
			/* JH: we haven't any display timing information in VM, but some application,
			       may waited in loop to vertical blank. Thats why I emulating VGA timing here.
			 */
			pwd->ddRVal = DD_OK;
			DWORD period = 10000/600;
			DWORD frame_pos = GetTimeTMS() % period;
			DWORD visible_time = (period*4800)/5250;
			if(frame_pos <= visible_time)
			{
				pwd->bIsInVB = FALSE;
			}
			else
			{
				pwd->bIsInVB = TRUE;
			}
			return DDHAL_DRIVER_HANDLED;
			break;
		case DDWAITVB_BLOCKBEGIN:
			/* wait until vertical blank is over and wait display period to the end */
			return DDHAL_DRIVER_HANDLED;
			break;
		case DDWAITVB_BLOCKEND:
			/* wait for blank end */
			return DDHAL_DRIVER_HANDLED;
			break;
	}
	
	pwd->ddRVal = DD_OK;
	return DDHAL_DRIVER_NOTHANDLED;
}

DWORD __stdcall Lock32(LPDDHAL_LOCKDATA pld)
{
	TRACE_ENTRY
	
	VMDAHAL_t *ddhal = GetHAL(pld->lpDD);
	if(IsInFront(ddhal, (void*)pld->lpDDSurface->lpGbl->fpVidMem))
	{
		FBHDA_access_begin(0);
	}
	
	return DDHAL_DRIVER_NOTHANDLED; /* let the lock processed */
}

DWORD __stdcall Unlock32(LPDDHAL_UNLOCKDATA pld)
{	
	TRACE_ENTRY
	VMDAHAL_t *ddhal = GetHAL(pld->lpDD);
	if(IsInFront(ddhal, (void*)pld->lpDDSurface->lpGbl->fpVidMem))
	{
		FBHDA_access_end(0);
	}
	
	return DDHAL_DRIVER_NOTHANDLED; /* let the unlock processed */
}

DWORD __stdcall SetExclusiveMode32(LPDDHAL_SETEXCLUSIVEMODEDATA psem)
{
	TRACE_ENTRY
	
#ifdef DEBUG
	SetExceptionHandler();
#endif

	//VMDAHAL_t *ddhal = GetHAL(psem->lpDD);
	
	if(psem->dwEnterExcl)
	{
		TRACE("exclusive mode: ON");
	}
	else
	{
		TRACE("exclusive mode: OFF");
	}
	
	psem->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall SetMode32(LPDDHAL_SETMODEDATA psmod)
{
	VMDAHAL_t *ddhal = GetHAL(psmod->lpDD);
	DWORD index = psmod->dwModeIndex;
	
//	UpdateCustomMode(ddhal);
	
	if(index < ddhal->modes_count)
	{
		DEVMODEA newmode;
		memset(&newmode, 0, sizeof(DEVMODEA));
		newmode.dmSize = sizeof(DEVMODEA);
		
		TRACE("SetMode32: %u", index);
	
		newmode.dmBitsPerPel  = ddhal->modes[index].dwBPP;
		newmode.dmPelsWidth   = ddhal->modes[index].dwWidth;
		newmode.dmPelsHeight  = ddhal->modes[index].dwHeight;
		newmode.dmFields      = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	
		if(ChangeDisplaySettingsA(&newmode, CDS_RESET|CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{
			psmod->ddRVal = DD_OK;
			return DDHAL_DRIVER_HANDLED;
		}
	}
	
	return DDHAL_DRIVER_NOTHANDLED;
}
