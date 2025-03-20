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
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"

#include "vmhal9x.h"

#include "nocrt.h"

static FBHDA_t *hda = NULL;

/**
 * VXD <-> DLL communication
 * Older version:
 * Application/DLL can communicate standard way by DeviceIoControl call.
 * You need HANDLE to driver, it can created by calling CreateFileA(“\\.\driver-file.vxd”, ...).
 * A good idea is open HANDLE at the beginning, and don’t waste time with CreateFileA
 * call every time. But this library is shared, so variables in memory are
 * persistent but HANDLEs are private for every process. Solution can be
 * using TLS (thread local storage), but it is relatively slow (you need more
 * system calls) and in Windows 95 it is buggy. I solve this with table (in
 * system memory) with process PID and VXD handle pairs.
 *
 * New version:
 * Communication to VXD is exported to another DLL which is loaded to program space.
 * We can check library presense by GetModuleHandle (NULL if not loaded), and
 * VXD handles from it are now in proccess private space.
 *
 * If someone, anyone, have better solution don’t be shy and rewrite this!
 *
 **/
typedef FBHDA_t *(__cdecl *FBHDA_setup_t)();
typedef void (__cdecl *FBHDA_access_begin_t)(DWORD flags);
typedef void (__cdecl *FBHDA_access_end_t)(DWORD flags);
typedef void (__cdecl *FBHDA_access_rect_t)(DWORD left, DWORD top, DWORD right, DWORD bottom);
typedef BOOL (__cdecl *FBHDA_swap_t)(DWORD offset);
typedef BOOL (__cdecl *FBHDA_page_modify_t)(DWORD flat_address, DWORD size, const BYTE *new_data);

#define VMDISP9X_LIB "vmdisp9x.dll"

static struct
{
	HMODULE lib;
	LONG lock;
	FBHDA_setup_t pFBHDA_setup;
	FBHDA_access_begin_t pFBHDA_access_begin;
	FBHDA_access_end_t pFBHDA_access_end;
	FBHDA_access_rect_t pFBHDA_access_rect;
	FBHDA_swap_t pFBHDA_swap;
	FBHDA_page_modify_t pFBHDA_page_modify;
} fbhda_lib = {NULL, 0};

static void FBHDA_call_lock()
{
	LONG tmp;
	do
	{
		tmp = InterlockedExchange(&fbhda_lib.lock, 1);
	} while(tmp == 1);
}

static void FBHDA_call_unlock()
{
	InterlockedExchange(&fbhda_lib.lock, 0);
}

#define LoadAddress(_n) fbhda_lib.p ## _n = (_n ## _t)GetProcAddress(fbhda_lib.lib, #_n)

static BOOL FBHDA_handle()
{
	//TRACE_ENTRY
	
	HMODULE mod = GetModuleHandle(VMDISP9X_LIB);
	BOOL need_load = TRUE;
	if(mod)
	{
		if(fbhda_lib.lib == mod)
		{
			need_load = FALSE;
		}
		else
		{
			WARN("Library on wrong address GetModuleHandle(...)=0x%X, fbhda_lib.lib=0x%X",
				mod, fbhda_lib.lib
			);
			fbhda_lib.lib = mod;
		}
	}
	else
	{
		fbhda_lib.lib = LoadLibraryA(VMDISP9X_LIB);
		TRACE("LoadLibraryA(%s)", VMDISP9X_LIB);
	}
	
	if(fbhda_lib.lib)
	{
		if(need_load)
		{
			LoadAddress(FBHDA_setup);
			LoadAddress(FBHDA_access_begin);
			LoadAddress(FBHDA_access_end);
			LoadAddress(FBHDA_access_rect);
			LoadAddress(FBHDA_swap);
			LoadAddress(FBHDA_page_modify);
		}
		
		//TRACE("FBHDA_handle() = TRUE");

		return TRUE;
	}
	
	return FALSE;
}

BOOL FBHDA_load_ex(VMDAHAL_t *pHal)
{
	TRACE_ENTRY
	BOOL rc = FALSE;
	
	FBHDA_call_lock();
	rc = FBHDA_handle();
	if(rc)
	{
		hda = fbhda_lib.pFBHDA_setup();
	}
	
	FBHDA_call_unlock();
	
	return rc;
}

void FBHDA_free()
{
	TRACE_ENTRY
	
	// ...
}

FBHDA_t *FBHDA_setup()
{
	//TRACE_ENTRY
	
	FBHDA_t *fbhda = NULL;
	
	FBHDA_call_lock();
	if(FBHDA_handle())
	{
		fbhda = fbhda_lib.pFBHDA_setup();
	}
	FBHDA_call_unlock();
	
	return fbhda;
}


void FBHDA_access_begin(DWORD flags)
{
	TRACE_ENTRY
	
	FBHDA_call_lock();
	if(FBHDA_handle())
	{
		fbhda_lib.pFBHDA_access_begin(flags);
	}
	FBHDA_call_unlock();
}

void FBHDA_access_end(DWORD flags)
{
	TRACE_ENTRY
	
	FBHDA_call_lock();
	if(FBHDA_handle())
	{
		fbhda_lib.pFBHDA_access_end(flags);
	}
	FBHDA_call_unlock();
}

BOOL FBHDA_swap(DWORD offset)
{
	TRACE_ENTRY
	BOOL rc = FALSE;
	
	FBHDA_call_lock();
	if(FBHDA_handle())
	{
		rc = fbhda_lib.pFBHDA_swap(offset);
	}
	FBHDA_call_unlock();
	
	return rc;
}

BOOL FBHDA_page_modify(DWORD flat_address, DWORD size, const BYTE *new_data)
{
	TRACE_ENTRY
	BOOL rc = FALSE;
	
	FBHDA_call_lock();
	if(FBHDA_handle())
	{
		rc = fbhda_lib.pFBHDA_page_modify(flat_address, size, new_data);
	}
	FBHDA_call_unlock();
	
	return rc;
}
