/******************************************************************************
 * Copyright (c) 2025 Jaroslav Hensl                                          *
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
#include <math.h>

#include <tlhelp32.h>

#include "vmdahal32.h"

#include "vmhal9x.h"
#include "wine.h"

#include "hotpatch.h"

#include "nocrt.h"

static const char *blacklist_dll[] = {
	"opengl32.dll",
	"mesa3d.dll",
	"vmwsgl32.dll",
	"vmhal9x.dll",
	NULL
};

static char winelib_dd[] = "winedd.dll";
static char winelib_d8[] = "wined8.dll";
static char winelib_d9[] = "wined9.dll";

static char ninelib_d8[] = "mesa89.dll";
static char ninelib_d9[] = "mesa99.dll";

static char *GetExeName(char *buffer)
{
	char pathbuf[MAX_PATH];
	if(GetModuleFileNameA(NULL, pathbuf, MAX_PATH))
	{
		const char *exe = strrchr(pathbuf, '\\');
		if(exe != NULL && strlen(exe) > 1)
		{
			strcpy(buffer, exe+1);
			return buffer;
		}
	}
	
	return NULL;
}

static const char reg_baseDD[]  = "Software\\DDSwitcher";
static const char reg_baseDX8[] = "Software\\D8Switcher";
static const char reg_baseDX9[] = "Software\\D9Switcher";

static BOOL CheckNine()
{
	FBHDA_t *hda = FBHDA_setup();
	if(!hda)
	{
		if((hda->flags & (FB_ACCEL_VMSVGA10 | FB_ACCEL_VMSVGA | FB_ACCEL_VMSVGA3D | FB_FORCE_SOFTWARE))
			 == (FB_ACCEL_VMSVGA | FB_ACCEL_VMSVGA3D))
		{
			/* vGPU9 not support NINE */
			return FALSE;
		}
		return TRUE;
	}
	return TRUE; /* not VMDISP */
}

static char *GetLibName(const char *reg_base, char *wine_libname, char *nine_libname)
{
	LSTATUS lResult;
	HKEY hKey;
	static char buffer[PATH_MAX];
	static char exe[PATH_MAX];
	char *result = NULL;
	
	if(GetExeName(exe) == NULL)
	{
		return NULL;
	}

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_base, 0, KEY_READ, &hKey);
	if(lResult == ERROR_SUCCESS)
	{
		DWORD size = PATH_MAX;
		DWORD type;
		lResult = RegQueryValueExA(hKey, exe, NULL, &type, (LPBYTE)buffer, &size);
		if(lResult == ERROR_SUCCESS)
		{
			switch(type)
			{
				case REG_SZ:
				case REG_MULTI_SZ:
				case REG_EXPAND_SZ:
				{
					if(stricmp(buffer, "system") == 0)
					{
						result = NULL;
					}
					else if(stricmp(buffer, "wine") == 0)
					{
						result = wine_libname;
					}
					else if(
						nine_libname != NULL &&
						(stricmp(buffer, "nine") == 0 || stricmp(buffer, "ninemore") == 0)
					){
						if(CheckNine())
						{
							result = nine_libname;
						}
					}
					else if(nine_libname != NULL && stricmp(buffer, "nineforce") == 0)
					{
						result = nine_libname;
					}
					else
					{
						result = buffer;
					}
					break;
				}
			}
		}
		else
		{
			lResult = RegQueryValueExA(hKey, "global", NULL, &type, (LPBYTE)buffer, &size);
	    if(lResult == ERROR_SUCCESS)
	    {
				switch(type)
				{
					case REG_SZ:
					case REG_MULTI_SZ:
					case REG_EXPAND_SZ:
					{
						if(stricmp(buffer, "system") == 0)
						{
							result = NULL;
						}
						else if(stricmp(buffer, "wine") == 0)
						{
							result = wine_libname;
						}
						else if(
							nine_libname != NULL &&
							(stricmp(buffer, "nine") == 0 || stricmp(buffer, "ninemore") == 0)
						){
							if(CheckNine())
							{
								result = nine_libname;
							}
						}
						else if(nine_libname != NULL && stricmp(buffer, "nineforce") == 0)
						{
							result = nine_libname;
						}
						else
						{
							result = buffer;
						}
						break;
					}
				}
			}
		}
		RegCloseKey(hKey);
	}
	
	return result;
}

static char *GetLibNameDD()
{
	return GetLibName(reg_baseDD, winelib_dd, NULL);
}

static char *GetLibNameD8()
{
	return GetLibName(reg_baseDX8, winelib_d8, ninelib_d8);
}

static char *GetLibNameD9()
{
	return GetLibName(reg_baseDX9, winelib_d9, ninelib_d9);
}

BOOL CheckTrace(BYTE *returnaddress)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	MODULEENTRY32 modentry = { 0 };

	if(hSnapshot != NULL)
	{
		modentry.dwSize = sizeof(MODULEENTRY32);
		Module32First(hSnapshot, &modentry);
		do
		{
			if ((modentry.modBaseAddr <= returnaddress) &&
				(modentry.modBaseAddr + modentry.modBaseSize > returnaddress))
			{
				int i = 0;
				const char *blentry;
				for(i = 0; (blentry = blacklist_dll[i]) != NULL; i++)
				{
					/* WARNING: check for module name case insensitive! */
					if(stricmp(modentry.szModule, blentry) == 0)
					{
						CloseHandle(hSnapshot);
						return FALSE;
					}
					
				}
			}
		} while (Module32Next(hSnapshot, &modentry));
			
		CloseHandle(hSnapshot);
	}
	
	return TRUE;
}

static struct {
	BOOL init;
	void *TPDirectDrawCreate;
	void *TPDirectDrawCreateClipper;
	void *TPDirectDrawCreateEx;
	void *TPDirectDrawEnumerateA;
	void *TPDirectDrawEnumerateExA;
	void *TPDirectDrawEnumerateExW;
	void *TPDirectDrawEnumerateW;
	void *TPDirectDrawDllMain;
	void *TPDirect3DCreate8;
	void *TPDirect3DCreate9;
	void *TPDirect3DCreate9Ex;
} wine_trampolines = {FALSE};

#define CREATE_TP(_l, _f, _p) \
	wine_trampolines.TP ## _f = patch_make_trampoline(_l, #_f, _p)

#define INSTALL_TP(_l, _f) patch_install(_l, #_f, wine_trampolines.TP ## _f)
#define INSTALL_TP_DllMain(_l, _f) patch_install(_l, NULL, wine_trampolines.TP ## _f)

#define UNINSTALL_TP(_l, _f) patch_uninstall(_l, #_f, wine_trampolines.TP ## _f)
#define UNINSTALL_TP_DllMain(_l, _f) patch_uninstall(_l, NULL, wine_trampolines.TP ## _f)

/* DirectDraw/DX7 functions */
void *__stdcall ddraw_entry(const char *libname, const char *procname, DWORD *stack)
{
	TOPIC("PATCH", "ddraw_entry(0x%X, 0x%X, 0x%X)", libname, procname, stack[0]);
	TOPIC("PATCH", "entrypoint %s -> %s", libname, procname);

	if(CheckTrace((BYTE*)stack[0]))
	{
		const char *lib = GetLibNameDD();
		if(lib != NULL)
		{
			HMODULE dll = GetModuleHandle(lib);
			if(!dll)
			{
				dll = LoadLibraryA(lib);
			}
			
			if(dll)
			{
				void *ptr = GetProcAddress(dll, procname);
				TOPIC("PATCH", "All OK, new proc=0x%X", ptr);
				return ptr;
				//return NULL;
			}
		}
	}
	
	return NULL;
}

/* DX8 functions */
void *__stdcall d3d8_entry(const char *libname, const char *procname, DWORD *stack)
{
	TOPIC("PATCH", "d3d8_entry(0x%X, 0x%X, 0x%X)", libname, procname, stack[0]);
	TOPIC("PATCH", "entrypoint %s -> %s", libname, procname);

	if(CheckTrace((BYTE*)stack[0]))
	{
		const char *lib = GetLibNameD8();
		if(lib != NULL)
		{
			HMODULE dll = GetModuleHandle(lib);
			if(!dll)
			{
				dll = LoadLibraryA(lib);
			}

			if(dll)
			{
				return GetProcAddress(dll, procname);
			}
		}
	}
	
	return NULL;
}

/* DX9 functions */
void *__stdcall d3d9_entry(const char *libname, const char *procname, DWORD *stack)
{
	TOPIC("PATCH", "d3d9_entry(0x%X, 0x%X, 0x%X)", libname, procname, stack[0]);
	TOPIC("PATCH", "entrypoint %s -> %s", libname, procname);

	if(CheckTrace((BYTE*)stack[0]))
	{
		const char *lib = GetLibNameD9();
		if(lib != NULL)
		{
			HMODULE dll = GetModuleHandle(lib);
			if(!dll)
			{
				dll = LoadLibraryA(lib);
			}

			if(dll)
			{
				return GetProcAddress(dll, procname);
			}
		}
	}
	
	return NULL;
}

/* DirectDraw dllmain function (hook DX8/DX9 libs) */
void *__stdcall ddraw_load(const char *libname, const char *procname, DWORD *stack)
{
	/* 0 - return address */
	/* 1 - arg 0 (HINSTANCE inst) */
	/* 2 - arg 1 (DWORD reason) */
	/* 3 - arg 2 (void *reserved) */
	
	TOPIC("PATCH", "DDRAW dllmain hook");
	
	if(stack[2] == DLL_PROCESS_ATTACH)
	{
		HMODULE hD3D9 = GetModuleHandle("d3d9.dll");
		if(hD3D9 == NULL)
		{
			if(GetLibNameD9() != NULL)
			{
				/*
					When is D3D9.dll is statically or dynamically loaded, is automatically
					loaded ddraw.dll and executed DllMain, but when is loaded ddraw.dll
					first and then d3d9.dll ddraw's DllMain isn't executed twice, so
					if there is DX9 hook, we preload the lib and patch it.
				 */
				hD3D9 = LoadLibraryA("d3d9.dll");
			}
		}
		if(hD3D9)
		{
			INSTALL_TP("d3d9.dll", Direct3DCreate9);
			INSTALL_TP("d3d9.dll", Direct3DCreate9Ex);
			TOPIC("PATCH", "D3D9 can be hooked!");
		}

		HMODULE hD3D8 = GetModuleHandle("d3d8.dll");
		if(hD3D8 == NULL)
		{
			if(GetLibNameD8() != NULL)
			{
				/* D3D8.dll ditto */
				hD3D8 = LoadLibraryA("d3d8.dll");
			}
		}
		if(hD3D8)
		{
			INSTALL_TP("d3d8.dll", Direct3DCreate8);
			TOPIC("PATCH", "D3D8 hooked!");
		}
	}

	return NULL;
}

BOOL __stdcall InstallWineHook()
{
	if(wine_trampolines.init == FALSE)
	{
		if(patch_init())
		{
			CREATE_TP("ddraw.dll", DirectDrawCreate,        ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawCreateClipper, ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawCreateEx,      ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawEnumerateA,    ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawEnumerateExA,  ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawEnumerateExW,  ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawEnumerateW,    ddraw_entry);
			CREATE_TP("ddraw.dll", DirectDrawDllMain,       ddraw_load);
			CREATE_TP("d3d8.dll",  Direct3DCreate8,         d3d8_entry);
			CREATE_TP("d3d9.dll",  Direct3DCreate9,         d3d9_entry);
			CREATE_TP("d3d9.dll",  Direct3DCreate9Ex,       d3d9_entry);
			wine_trampolines.init = TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	INSTALL_TP("ddraw.dll", DirectDrawCreate);
	INSTALL_TP("ddraw.dll", DirectDrawCreateClipper);
	INSTALL_TP("ddraw.dll", DirectDrawCreateEx);
	INSTALL_TP("ddraw.dll", DirectDrawEnumerateA);
	INSTALL_TP("ddraw.dll", DirectDrawEnumerateExA);
	INSTALL_TP("ddraw.dll", DirectDrawEnumerateExW);
	INSTALL_TP("ddraw.dll", DirectDrawEnumerateW);
	INSTALL_TP_DllMain("ddraw.dll", DirectDrawDllMain);
	
	return FALSE;
}

BOOL __stdcall UninstallWineHook()
{
	if(wine_trampolines.init != FALSE)
	{
		UNINSTALL_TP("ddraw.dll", DirectDrawCreate);
		UNINSTALL_TP("ddraw.dll", DirectDrawCreateClipper);
		UNINSTALL_TP("ddraw.dll", DirectDrawCreateEx);
		UNINSTALL_TP("ddraw.dll", DirectDrawEnumerateA);
		UNINSTALL_TP("ddraw.dll", DirectDrawEnumerateExA);
		UNINSTALL_TP("ddraw.dll", DirectDrawEnumerateExW);
		UNINSTALL_TP("ddraw.dll", DirectDrawEnumerateW);
		UNINSTALL_TP_DllMain("ddraw.dll", DirectDrawDllMain);

		return TRUE;
	}
	
	return TRUE;
}

int __stdcall CheckWineHook()
{
	if(wine_trampolines.init != FALSE)
	{
		int rc = patch_validate("ddraw.dll", NULL, wine_trampolines.TPDirectDrawDllMain);
		
		switch(rc)
		{
			case PATCH_PATCHABLE: return 0;
			case PATCH_PATCHED: return 1;
			case PATCH_NOTAPPLICABLE: return -1;
		}
	}
	
	return 0;
}
