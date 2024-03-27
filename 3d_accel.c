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
#include "vmdahal32.h"

#include "vmhal9x.h"

#include "nocrt.h"

static FBHDA_t *hda = NULL;
static VMDAHAL_t *hda_hal = NULL;

/**
 * VXD <-> DLL communication
 * Application/DLL can communicate standard way by DeviceIoControl call.
 * You need HANDLE to driver, it can created by calling CreateFileA(“\\.\driver-file.vxd”, ...).
 * A good idea is open HANDLE at the beginning, and don’t waste time with CreateFileA
 * call every time. But this library is shared, so variables in memory are
 * persistent but HANDLEs are private for every process. Solution can be
 * using TLS (thread local storage), but it is relatively slow (you need more
 * system calls) and in Windows 95 it is buggy. I solve this with table (in
 * system memory) with process PID and VXD handle pairs.
 *
 * If someone, anyone, have better solution don’t be shy and rewrite this!
 *
 **/
static HANDLE FBHDA_get_vxd(VMDAHAL_t *pHal)
{
	if(pHal == NULL) return FALSE;
	
	DWORD pid = GetCurrentProcessId();
	HANDLE h = INVALID_HANDLE_VALUE;
	int i = 0;
	int i_zero = -1;
	
	for(i = 0; i < VXD_PAIRS_CNT; i++)
	{
		if(pHal->vxd_table[i].pid == pid)
		{
			h = (HANDLE)pHal->vxd_table[i].vxd;
			break;
		}
		else if(pHal->vxd_table[i].pid == 0)
		{
			if(i_zero < 0)
			{
				i_zero = i;
			}
		}
	}
	
	if(h == INVALID_HANDLE_VALUE)
	{
		char strbuf[MAX_PATH];
		strcpy(strbuf, "\\\\.\\");
		strcat(strbuf, pHal->pFBHDA32->vxdname);
		h = CreateFileA(strbuf, 0, 0, NULL, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
	}
	else
	{
		TRACE("Reuse VXD for PID = %d", pid);
		return h;
	}
	
	if(i < VXD_PAIRS_CNT)
	{
		pHal->vxd_table[i].vxd = (DWORD)h;
	}
	else if(i_zero >= 0)
	{
		pHal->vxd_table[i_zero].vxd = (DWORD)h;
		pHal->vxd_table[i_zero].pid = pid;
	}
	
	TRACE("New VXD for PID = %d", pid);
	
	return h;
}

BOOL FBHDA_load_ex(VMDAHAL_t *pHal)
{
	TRACE_ENTRY
	
	if(pHal->FBHDA_version != 2024)
	{
		ERR("Wrong version: %d != 2024", pHal->FBHDA_version);
		return FALSE;
	}
	
	if(pHal->pFBHDA32->cb != sizeof(FBHDA_t))
	{
		ERR("Wrong FBHDA size: %d (expected %d)", pHal->pFBHDA32->cb, sizeof(FBHDA_t));
		return FALSE;
	}
	
	if(pHal->pFBHDA32->version != API_3DACCEL_VER)
	{
		ERR("Wrong FBHDA version: %d (header version is %d)", pHal->pFBHDA32->version, API_3DACCEL_VER);
		return FALSE;
	}
	
	hda = pHal->pFBHDA32;
	hda_hal = pHal;
	
	return TRUE;
}

void FBHDA_free()
{
	TRACE_ENTRY
	
	if(hda_hal == NULL) return;
	
	DWORD pid = GetCurrentProcessId();
	int i;
			
	for(i = 0; i < VXD_PAIRS_CNT; i++)
	{
		if(hda_hal->vxd_table[i].pid == pid)
		{
			if(hda_hal->vxd_table[i].vxd != (DWORD)INVALID_HANDLE_VALUE)
			{
				CloseHandle((HANDLE)hda_hal->vxd_table[i].vxd);
			}
			
			hda_hal->vxd_table[i].vxd = (DWORD)INVALID_HANDLE_VALUE;
			hda_hal->vxd_table[i].pid = 0;
			break;
		}
	}
}

FBHDA_t *FBHDA_setup()
{
	return hda;
}

/* 
 * multiple locking is fine, but we can save some time
 * if we avoid it.
 */
static volatile int fbhda_locks_cnt = 0;

void FBHDA_access_begin(DWORD flags)
{
	TRACE_ENTRY
	
	if(fbhda_locks_cnt++ == 0)
	{
		HANDLE h = FBHDA_get_vxd(hda_hal);
		
		if(h != INVALID_HANDLE_VALUE)
		{
			DeviceIoControl(h, OP_FBHDA_ACCESS_BEGIN, &flags, sizeof(DWORD), NULL, 0, NULL, NULL);
		}
	}
}

void FBHDA_access_end(DWORD flags)
{
	TRACE_ENTRY
	
	if(--fbhda_locks_cnt <= 0)
	{
		HANDLE h = FBHDA_get_vxd(hda_hal);
		
		if(h != INVALID_HANDLE_VALUE)
		{
			DeviceIoControl(h, OP_FBHDA_ACCESS_END, &flags, sizeof(DWORD), NULL, 0, NULL, NULL);
		}
		
		if(fbhda_locks_cnt < 0)
		{
			fbhda_locks_cnt = 0;
		}
	}
}

BOOL FBHDA_swap(DWORD offset)
{
	TRACE_ENTRY
	
	DWORD result = 0;
	HANDLE h = FBHDA_get_vxd(hda_hal);
	
	if(h != INVALID_HANDLE_VALUE)
	{	
		if(DeviceIoControl(h, OP_FBHDA_SWAP,
			&offset, sizeof(DWORD), &result, sizeof(DWORD),
			NULL, NULL))
		{
			return result == 0 ? FALSE : TRUE;
		}
	}
	
	return FALSE;
}


