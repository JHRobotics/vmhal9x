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
#include <stdint.h>
#include "vmdahal32.h"

#include "vmhal9x.h"

#include "nocrt.h"

#ifndef HEAP_SHARED
/* undocumented heap behaviour for shared DLL (from D3_DD32.C) */
#define HEAP_SHARED      0x04000000
#endif

HANDLE hSharedHeap;
HANDLE hSharedLargeHeap;
static HINSTANCE dllHinst = NULL;

VMDAHAL_t *globalHal;

BOOL halVSync = FALSE;

VMHAL_enviroment_t VMHALenv = {
	FALSE,
	FALSE,
	FALSE,
	7, // DDI (maximum)
	TRUE, // HW T&L
	FALSE, // readback
	2048, // tex w  (can be query by GL_MAX_TEXTURE_SIZE)
	2048, // tex h
	9, // tex units
	8, // lights (GL min. is 8)
	6, // clip planes (GL min. is 6), GL_MAX_CLIP_PLANES
	TRUE, // use float32 in Z buffer (eg 64-bit F32_S8_X24 depth plane), on FALSE 32-bit S24_S8 depth plane
	16, // max anisotropy
	FALSE // vertexblend
};

static DWORD CalcPitch(DWORD w, DWORD bpp)
{
	DWORD bp = (bpp+7) / 8;
	
	return ((w * bp) + 15) & (~((DWORD)15));
}

static BOOL InsertMode(VMDAHAL_t *hal, DWORD idx, DWORD w, DWORD h, DWORD bpp)
{
	if(idx >= DISP_MODES_MAX)
	{
		return FALSE;
	}
	
	TRACE("new mode %u = %ux%ux%u", idx, w, h, bpp);
	
	hal->modes[idx].dwWidth  = w;
	hal->modes[idx].dwHeight = h;
	hal->modes[idx].lPitch   = CalcPitch(w, bpp);
	hal->modes[idx].wRefreshRate = 0;

	switch(bpp)
	{
		case 8:
			hal->modes[idx].dwBPP  = 8;
			hal->modes[idx].wFlags = DDMODEINFO_PALETTIZED;
			hal->modes[idx].dwRBitMask = 0x00000000;
			hal->modes[idx].dwGBitMask = 0x00000000;
			hal->modes[idx].dwBBitMask = 0x00000000;
			hal->modes[idx].dwAlphaBitMask = 0x00000000;
			return TRUE;
		case 16:
			hal->modes[idx].dwBPP  = 16;
			hal->modes[idx].wFlags = 0;
			hal->modes[idx].dwRBitMask = 0x0000F800;
			hal->modes[idx].dwGBitMask = 0x000007E0;
			hal->modes[idx].dwBBitMask = 0x0000001F;
			hal->modes[idx].dwAlphaBitMask = 0x00000000;
			return TRUE;
		case 24:
			hal->modes[idx].dwBPP  = 24;
			hal->modes[idx].wFlags = 0;
			hal->modes[idx].dwRBitMask = 0x00FF0000;
			hal->modes[idx].dwGBitMask = 0x0000FF00;
			hal->modes[idx].dwBBitMask = 0x000000FF;
			hal->modes[idx].dwAlphaBitMask = 0x00000000;
			return TRUE;
		case 32:
			hal->modes[idx].dwBPP  = 32;
			hal->modes[idx].wFlags = 0;
			hal->modes[idx].dwRBitMask = 0x00FF0000;
			hal->modes[idx].dwGBitMask = 0x0000FF00;
			hal->modes[idx].dwBBitMask = 0x000000FF;
			hal->modes[idx].dwAlphaBitMask = 0x00000000;
			return TRUE;
	}
	
	return FALSE;
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
				if(InsertMode(hal, idx, mode.dmPelsWidth, mode.dmPelsHeight, mode.dmBitsPerPel))
				{
					hal->modes_count++;
				}
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

static DWORD FindMode(VMDAHAL_t *hal, DWORD w, DWORD h, DWORD bpp)
{
	DWORD idx;
	for(idx = 0; idx < hal->modes_count; idx++)
	{
		if(hal->modes[idx].dwWidth == w &&
			hal->modes[idx].dwHeight == h &&
			hal->modes[idx].dwBPP == bpp)
		{
			return idx;
		}
	}
	
	return -1;
}

void UpdateCustomMode(VMDAHAL_t *hal)
{
	TRACE_ENTRY
	
	DWORD current = 0;
	current = FindMode(hal, hal->pFBHDA32->width, hal->pFBHDA32->height, hal->pFBHDA32->bpp);
	
	if(current == -1)
	{
		if(InsertMode(hal, hal->custom_mode_id, hal->pFBHDA32->width, hal->pFBHDA32->height, hal->pFBHDA32->bpp))
		{
			TRACE("custom mode: %u %u %u", hal->pFBHDA32->width, hal->pFBHDA32->height, hal->pFBHDA32->bpp);
			if(hal->custom_mode_id == hal->modes_count)
			{
				hal->modes_count++;
			}
		}
	}
}

BOOL ProcessExists(DWORD pid)
{
	BOOL rc = FALSE;
	HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if(proc != NULL)
	{
		DWORD code;
		
		rc = TRUE;
		if(GetExitCodeProcess(proc, &code))
		{
			if(code != STILL_ACTIVE)
			{
				rc = FALSE;
			}
		}

		CloseHandle(proc);
	}
	
	return rc;
}

//VMDAHAL_t __stdcall *DriverInit(LPVOID ptr)
DWORD __stdcall DriverInit(LPVOID ptr)
{
	if(ptr == NULL)
	{
		return 1;
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
//	globalHal->cb32.SetMode = SetMode32;
	globalHal->cb32.SetColorKey = SetColorKey32;


#ifdef D3DHAL
	globalHal->cb32.GetDriverInfo = GetDriverInfo32;
#endif
	globalHal->cb32.DestroyDriver = DestroyDriver32;
	
	globalHal->hInstance = (DWORD)dllHinst;
	
	if(globalHal->modes_count == 0)
	{
		if(!FillModes(globalHal))
		{
			return 0;
		}
	}
	
	UpdateCustomMode(globalHal);
	
#ifdef DEBUG
	SetExceptionHandler();
#endif

	{
		/*
		 TODO: same key in in S3V
				HKEY_CURRENT_CONFIG
				"Display\\Settings"
				"WaitForVSync"
				"OFF"
		*/
		
		HKEY reg;
		if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\vmdisp9x", 0, KEY_READ, &reg) == ERROR_SUCCESS)
		{
			DWORD type;
			BYTE buf[128];
			DWORD size = sizeof(buf);
			if(RegQueryValueExA(reg, "HAL_VSYNC", NULL, &type, (LPBYTE)&buf[0], &size) == ERROR_SUCCESS)
			{
		  	switch(type)
		   	{
					case REG_SZ:
					case REG_MULTI_SZ:
					case REG_EXPAND_SZ:
					{
						int n = atoi((char*)buf);
						if(n != 0)
						{
							halVSync = TRUE;
						}
						break;
					}
					case REG_DWORD:
					{
						DWORD dw = *((LPDWORD)buf);
						if(dw != 0)
						{
							halVSync = TRUE;
						}
						break;
					}
				}
			}
			RegCloseKey(reg);
		}
	}
	
#ifdef D3DHAL
	D3DHALCreateDriver(
		&globalHal->d3dhal_global,
		&globalHal->d3dhal_callbacks,
		&globalHal->d3dhal_flags);
#else
	globalHal->d3dhal_global = 0;
	globalHal->d3dhal_callbacks = 0;
	memset(&globalHal->d3dhal_flags, 0, sizeof(VMDAHAL_D3DCAPS_t));
#endif

	globalHal->invalid = FALSE;
	
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
				hSharedHeap = HeapCreate(HEAP_SHARED, 0x2000, 0);
				hSharedLargeHeap = HeapCreate(HEAP_SHARED, 0x10000, 0);
				TRACE("--- vmhal9x created ---");
			}
			tmp += 1;
			InterlockedExchange(&lProcessCount, tmp);
			
			dllHinst = hModule;
#ifdef D3DHAL
			Mesa3DCleanProc();
#endif
			
			break;

		case DLL_PROCESS_DETACH:
			/* usually never calls */
#ifdef D3DHAL
			Mesa3DCleanProc();
#endif
			FBHDA_free();
			do
			{
				tmp = InterlockedExchange(&lProcessCount, -1);
			} while (tmp == -1);
			tmp -= 1;

			if(tmp == 0)         // Last process?
			{
				HeapDestroy(hSharedHeap);
				HeapDestroy(hSharedLargeHeap);
				hSharedHeap = NULL;
				hSharedLargeHeap = NULL;
				TRACE("--- vmhal9x destroyed ---");
			}
			InterlockedExchange(&lProcessCount, tmp);
		break;
	}

	return TRUE;
}
