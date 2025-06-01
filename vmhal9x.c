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
#include "wine.h"

#include "vmsetup.h"

#include "nocrt.h"

#ifndef DDHALINFO_ISPRIMARYDISPLAY
#define DDHALINFO_ISPRIMARYDISPLAY 0x00000001l
// indicates driver is primary display driver
#endif

#ifndef DDHALINFO_MODEXILLEGAL
#define DDHALINFO_MODEXILLEGAL 0x00000002l
// indicates this hardware does not support modex modes
#endif

#ifndef DDHALINFO_GETDRIVERINFOSET
#define DDHALINFO_GETDRIVERINFOSET 0x00000004l
// indicates that GetDriverInfo is set
#endif

#ifndef DDHALINFO_GETDRIVERINFO2
#define DDHALINFO_GETDRIVERINFO2 0x00000008l
// indicates driver support GetDriverInfo2 variant
// of GetDriverInfo. New for DX 8.0
#endif

static HINSTANCE dllHinst = NULL;

VMDAHAL_t *globalHal;

BOOL halVSync = FALSE;

static VMHAL_enviroment_t VMHALenv = {
	FALSE, /* scanned */
	FALSE, /* runtime dx5 */
	FALSE, /* runtime dx6 */
	FALSE, /* runtime dx7 */
	FALSE, /* runtime dx8 */
	FALSE, /* runtime dx9 */
	8, // DDI (maximum)
#ifndef DEBUG
	7, // HW T&L
#else
	8, // HW T&L
#endif
	TRUE, // readback
	TRUE, // touchdepth
	16384, // tex w  (can be query by GL_MAX_TEXTURE_SIZE)
	16384, // tex h
	8, // tex units
	8, // lights (GL min. is 8)
	6, // clip planes (GL min. is 6), GL_MAX_CLIP_PLANES
	TRUE, // use float32 in Z buffer (eg 64-bit F32_S8_X24 depth plane), on FALSE 32-bit S24_S8 depth plane
	16, // max anisotropy
	FALSE, // vertexblend
	FALSE, // use palette
	TRUE,  // filter bug
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

BOOL GetVMHALenv(VMHAL_enviroment_t *dst)
{
	if(dst == NULL) return FALSE;

	memcpy(dst, &VMHALenv, sizeof(VMHAL_enviroment_t));

	return TRUE;
}

static void ReadEnv(VMHAL_enviroment_t *dst)
{
	memcpy(dst, &VMHALenv, sizeof(VMHAL_enviroment_t));

	if(vmhal_setup_str("hal", "ddi", FALSE) != NULL)
	{
		dst->ddi = vmhal_setup_dw("hal", "ddi"); 
	}
	
	if(vmhal_setup_str("hal", "hwtl", FALSE) != NULL)
	{
		dst->hwtl_ddi = vmhal_setup_dw("hal", "hwtl") ;
	}
	
	if(vmhal_setup_str("hal", "readback", FALSE) != NULL)
	{
		dst->readback = vmhal_setup_dw("hal", "readback") ? TRUE : FALSE;
	}
	
	if(vmhal_setup_str("hal", "touchdepth", FALSE) != NULL)
	{
		dst->touchdepth = vmhal_setup_dw("hal", "touchdepth") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "vertexblend", FALSE) != NULL)
	{
		dst->vertexblend = vmhal_setup_dw("hal", "vertexblend") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "palette", FALSE) != NULL)
	{
		dst->allow_palette = vmhal_setup_dw("hal", "palette") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "filter_bug", FALSE) != NULL)
	{
		dst->filter_bug = vmhal_setup_dw("hal", "filter_bug") ? TRUE : FALSE;
	}
}

void VMHALenv_RuntimeVer(int ver)
{
	if(ver >= 9) VMHALenv.dx9 = TRUE;
	if(ver >= 8) VMHALenv.dx8 = TRUE;
	if(ver >= 7) VMHALenv.dx7 = TRUE;
	if(ver >= 6) VMHALenv.dx6 = TRUE;
	if(ver >= 5) VMHALenv.dx5 = TRUE;
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

	ReadEnv(&VMHALenv);

	globalHal = ptr;
	
	globalHal->cb32.CreateSurface = CreateSurface32;
	globalHal->cb32.DestroySurface = DestroySurface32;
	globalHal->cb32.CanCreateSurface = CanCreateSurface32;
	
	globalHal->cb32.Flip = Flip32;
	globalHal->cb32.GetFlipStatus = GetFlipStatus32;
	globalHal->cb32.Lock = Lock32;
	globalHal->cb32.Unlock = Unlock32;
	globalHal->cb32.WaitForVerticalBlank = WaitForVerticalBlank32;
	globalHal->cb32.Blt = Blt32;
	globalHal->cb32.GetBltStatus = GetBltStatus32;
//	globalHal->cb32.SetExclusiveMode = SetExclusiveMode32;
#ifdef DEBUG
	globalHal->cb32.SetMode = SetMode32;
#endif
	globalHal->cb32.SetColorKey = SetColorKey32;
	globalHal->cb32.AddAttachedSurface = AddAttachedSurface32;

#ifdef D3DHAL
	if(VMHALenv.ddi >= 5)
	{
		globalHal->cb32.GetDriverInfo = GetDriverInfo32;
	}
	globalHal->cb32.flags = DDHALINFO_ISPRIMARYDISPLAY; // TODO: set for drivers DDHALINFO_MODEXILLEGAL (vmware workstation)
	if(VMHALenv.ddi >= 8)
	{
		globalHal->cb32.flags |= DDHALINFO_GETDRIVERINFO2;
	}
#endif
	//globalHal->cb32.DestroyDriver = DestroyDriver32;
	// JH: ^ one important thing, when driver is destroyed, pm16 SetInfo callback mus be set to NULL
	//       and do it from pm16 driver is much more comfortable
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

	/* do cleanup */
	Mesa3DCleanProc();
	SurfaceDeleteAll();

#ifdef D3DHAL
	D3DHALCreateDriver(
		&globalHal->d3dhal_global,
		&globalHal->d3dhal_callbacks,
		&globalHal->d3dhal_exebuffcallbacks,
		&globalHal->d3dhal_flags);
#else
	globalHal->d3dhal_global = 0;
	globalHal->d3dhal_callbacks = 0;
	memset(&globalHal->d3dhal_exebuffcallbacks, 0, sizeof(DDHAL_DDEXEBUFCALLBACKS));
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

BOOL __stdcall UninstallWineHook();

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
				TRACE("--- vmhal9x created ---");
				hal_memory_init();
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
			/* Update: some utilities like 'KillHelp.exe' allows unload
			 *         ddraw.dll (ddraw16.dll inc.) and all drivers 32-bit RING-3
			 *         DLLs.
			 */
#ifdef D3DHAL
			Mesa3DCleanProc();
			hal_dump_allocs();
#endif
			FBHDA_free();
			do
			{
				tmp = InterlockedExchange(&lProcessCount, -1);
			} while (tmp == -1);
			tmp -= 1;

			if(tmp == 0)         // Last process?
			{
				UninstallWineHook();
				hal_memory_destroy();
				TRACE("--- vmhal9x destroyed ---");
			}
			InterlockedExchange(&lProcessCount, tmp);
		break;
	}

	return TRUE;
}
