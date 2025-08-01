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
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "3d_accel.h"

#define VMHAL9X_LIB
#include "vmsetup.h"

#include "nocrt.h"

static FBHDA_t *hda = NULL;
static HANDLE hda_vxd = INVALID_HANDLE_VALUE;

void FBHDA_load()
{
	HWND hDesktop = GetDesktopWindow();
	HDC hdc = GetDC(hDesktop);
	char strbuf[PATH_MAX];
	
	if(ExtEscape(hdc, OP_FBHDA_SETUP, 0, NULL, sizeof(FBHDA_t *), (LPVOID)&hda))
	{
		if(hda != NULL)
		{
			if(hda->cb == sizeof(FBHDA_t) && hda->version == API_3DACCEL_VER)
			{
				strcpy(strbuf, "\\\\.\\");
				strcat(strbuf, hda->vxdname);
				
				hda_vxd = CreateFileA(strbuf, 0, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
				if(hda_vxd != INVALID_HANDLE_VALUE)
				{
					return; /* SUCCESS */
				}
			}
		}
	}

	hda = NULL;
}

static BOOL FBHDA_valid()
{
	return hda_vxd != INVALID_HANDLE_VALUE;
}

void FBHDA_free()
{
	if(hda_vxd != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hda_vxd);
	}
}

FBHDA_t *FBHDA_setup()
{
	return hda;
}

void FBHDA_access_begin(DWORD flags)
{
	if(!FBHDA_valid()) return;

	DeviceIoControl(hda_vxd, OP_FBHDA_ACCESS_BEGIN, &flags, sizeof(DWORD), NULL, 0, NULL, NULL);
}

void FBHDA_access_end(DWORD flags)
{
	if(!FBHDA_valid()) return;

	DeviceIoControl(hda_vxd, OP_FBHDA_ACCESS_END, &flags, sizeof(DWORD), NULL, 0, NULL, NULL);
}

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	if(!FBHDA_valid()) return;

	DWORD rect[4] = {left, top, right, bottom};

	DeviceIoControl(hda_vxd, OP_FBHDA_ACCESS_RECT, &rect, sizeof(rect), NULL, 0, NULL, NULL);
}

BOOL FBHDA_swap(DWORD offset)
{
	DWORD result = 0;
	
	if(!FBHDA_valid()) return FALSE;
	
	if(DeviceIoControl(hda_vxd, OP_FBHDA_SWAP,
		&offset, sizeof(DWORD), &result, sizeof(DWORD),
		NULL, NULL))
	{
		return result == 0 ? FALSE : TRUE;
	}
	
	return FALSE;
}

BOOL FBHDA_page_modify(DWORD flat_address, DWORD size, const BYTE *new_data)
{
	DWORD buffer[1024 + 2];
	
	if(size > 4096) return FALSE;
	
	if(!FBHDA_valid()) return FALSE;

	buffer[0] = flat_address;
	buffer[1] = size;
	memcpy(&buffer[2], new_data, size);

	if(DeviceIoControl(hda_vxd, OP_FBHDA_PAGE_MOD,
		&buffer[0], sizeof(DWORD)*2+size, NULL, 0,
		NULL, NULL))
	{
		return TRUE;
	}
	
	return FALSE;
}

void FBHDA_clean()
{
	if(!FBHDA_valid()) return;

	DeviceIoControl(hda_vxd, OP_FBHDA_CLEAN, NULL, 0, NULL, 0, NULL, NULL);
}

BOOL FBHDA_mode_query(DWORD index, FBHDA_mode_t *mode)
{
	if(!FBHDA_valid()) return FALSE;

	struct
	{
		DWORD result;
		FBHDA_mode_t mode;
	} output;
	output.result = 0;
	output.mode.cb = sizeof(FBHDA_mode_t);

	if(DeviceIoControl(hda_vxd, OP_FBHDA_MODE_QUERY,
		&index, sizeof(DWORD), &output, sizeof(output),
		NULL, NULL))
	{
		if(output.result)
		{
			if(mode)
			{
				if(mode->cb == output.mode.cb)
				{
					memcpy(mode, &output.mode, sizeof(FBHDA_mode_t));
				}
			}
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	switch( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);
			FBHDA_load();
			vmhal_setup_load(TRUE);
			break;
		case DLL_PROCESS_DETACH:
			if(FBHDA_valid())
			{
				FBHDA_free();
			}
			break;
	}

	return TRUE;
}
