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
#include <stddef.h>
#include "vmdahal32.h"

#include "vmhal9x.h"

#include "nocrt.h"

HANDLE hSharedHeap;
static HINSTANCE dllHinst = NULL;

VMDAHAL_t *globalHal;

static DWORD calc_pitch(DWORD w, DWORD bpp)
{
	DWORD bp = (bpp+7) / 8;
	
	return ((w * bp) + 15) & (~((DWORD)15));
}

static BOOL FillModes(VMDAHAL_t *hal)
{
	DEVMODEA mode;
	DWORD    id;
	
	memset(&mode, 0, sizeof(DEVMODEA));
	mode.dmSize = sizeof(DEVMODEA);
	
	for(id = 0; ; id++)
	{
		if(EnumDisplaySettingsA(NULL, id, &mode))
		{
			DWORD idx = hal->modes_count;
			if(idx >= DISP_MODES_MAX)
			{
				break;
			}
			
			if(mode.dmBitsPerPel == 8 ||
				mode.dmBitsPerPel == 16 ||
				mode.dmBitsPerPel == 24 ||
				mode.dmBitsPerPel == 32)
			{
				hal->modes[idx].dwWidth  = mode.dmPelsWidth;
				hal->modes[idx].dwHeight = mode.dmPelsHeight;
				hal->modes[idx].lPitch   = calc_pitch(mode.dmPelsWidth, mode.dmBitsPerPel);
				hal->modes[idx].wRefreshRate = 0;
				
				switch(mode.dmBitsPerPel)
				{
					case 8:
						hal->modes[idx].dwBPP  = 8;
						hal->modes[idx].wFlags = DDMODEINFO_PALETTIZED;
						hal->modes[idx].dwRBitMask = 0x00000000;
						hal->modes[idx].dwGBitMask = 0x00000000;
						hal->modes[idx].dwBBitMask = 0x00000000;
						hal->modes[idx].dwAlphaBitMask = 0x00000000;
						break;
					case 16:
						hal->modes[idx].dwBPP  = 16;
						hal->modes[idx].wFlags = 0;
						hal->modes[idx].dwRBitMask = 0x0000F800;
						hal->modes[idx].dwGBitMask = 0x000007E0;
						hal->modes[idx].dwBBitMask = 0x0000001F;
						hal->modes[idx].dwAlphaBitMask = 0x00000000;
						break;
					case 24:
						hal->modes[idx].dwBPP  = 24;
						hal->modes[idx].wFlags = 0;
						hal->modes[idx].dwRBitMask = 0x00FF0000;
						hal->modes[idx].dwGBitMask = 0x0000FF00;
						hal->modes[idx].dwBBitMask = 0x000000FF;
						hal->modes[idx].dwAlphaBitMask = 0x00000000;
						break;
					case 32:
						hal->modes[idx].dwBPP  = 32;
						hal->modes[idx].wFlags = 0;
						hal->modes[idx].dwRBitMask = 0x00FF0000;
						hal->modes[idx].dwGBitMask = 0x0000FF00;
						hal->modes[idx].dwBBitMask = 0x000000FF;
						hal->modes[idx].dwAlphaBitMask = 0x00000000;
						break;
				}
				
				hal->modes_count++;
			}
		}
		else
		{
			break;
		}
	}
	
	if(id == 0)
	{
		return FALSE;
	}
	
	hal->custom_mode_id = hal->modes_count;
	return TRUE;
}

//VMDAHAL_t __stdcall *DriverInit(LPVOID ptr)
DWORD __stdcall DriverInit(LPVOID ptr)
{
	if(ptr == NULL)
	{
		return 0;
	}
	TRACE_ENTRY
	TRACE("HAL size %d (%d %d %d %d)",
		sizeof(VMDAHAL_t),
		offsetof(VMDAHAL_t, ddpf),
		offsetof(VMDAHAL_t, ddHALInfo),
		offsetof(VMDAHAL_t, cb32),
		offsetof(VMDAHAL_t, FBHDA_version)
	);
	
	globalHal = ptr;
	
	globalHal->cb32.CreateSurface = CreateSurface;
	globalHal->cb32.DestroySurface = DestroySurface;
	globalHal->cb32.CanCreateSurface = CanCreateSurface;
	
	globalHal->cb32.Flip = Flip32;
	globalHal->cb32.GetFlipStatus = GetFlipStatus32;
	globalHal->cb32.Lock = Lock32;
	globalHal->cb32.Unlock = Unlock32;
	globalHal->cb32.WaitForVerticalBlank = WaitForVerticalBlank32;
	globalHal->cb32.Blt = Blt32;
	globalHal->cb32.GetBltStatus = GetBltStatus32;
	globalHal->cb32.SetExclusiveMode = SetExclusiveMode32;
	globalHal->cb32.SetMode = SetMode32;
	
	globalHal->hInstance = (DWORD)dllHinst;
	
	if(FillModes(globalHal))
	{
		return 1;
	}
	
	if(FBHDA_load_ex(globalHal))
	{
		return 1;
	}
	
	ERR("DriverInit FAILED!");
	return 0;
}


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	static long lProcessCount = 0;
	long tmp;

	switch( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);

			do
			{
				tmp = InterlockedExchange(&lProcessCount, -1);
			} while(tmp == -1);

			if(tmp == 0) // First process?
			{
				hSharedHeap = HeapCreate(/*HEAP_SHARED*/0, 2048, 0);
				TRACE("--- vmhal9x created ---");
			}
			tmp += 1;
			InterlockedExchange(&lProcessCount, tmp);
			
			dllHinst = hModule;
			
			break;

    case DLL_PROCESS_DETACH:
    	FBHDA_free();
			do
			{
				tmp = InterlockedExchange(&lProcessCount, -1);
			} while (tmp == -1);
			tmp -= 1;

			if( tmp == 0)         // Last process?
			{
				HeapDestroy( hSharedHeap);
				hSharedHeap = NULL;
				TRACE("--- vmhal9x destroyed ---");
			}
			InterlockedExchange(&lProcessCount, tmp);
			break;
    }

    return TRUE;

}
