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
	//globalHal->cb32.SetMode = SetMode32; (not in use)
	
	globalHal->hInstance = (DWORD)dllHinst;
	
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
