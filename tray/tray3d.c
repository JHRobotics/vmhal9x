#include <initguid.h>
#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ddraw.h>

#include "../3d_accel.h"

#define VERSION_ONLY
#include "../vmhal9x.h"
#include "tray3d.h"

#include "nocrt.h"

#define CLSMAX 256
#define WND_TRAY_CLASS_NAME "WINETRAYCLS"

#define WM_NOTIFYMSG (WM_USER + 1)

typedef HRESULT (WINAPI *HDirectDrawCreate)(GUID*,LPDIRECTDRAW*,IUnknown*);
typedef HRESULT (WINAPI *HDirectDrawCreateEx)(GUID *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown *pUnkOuter);

typedef BOOL (WINAPI *HInstallWineHook)(void);
typedef BOOL (WINAPI *HUninstallWineHook)(void);
typedef int  (WINAPI *HCheckWineHook)(void);

static char errormsg[256] = {'\0'};

HMODULE hDDraw = NULL;

BOOL install_hook(BOOL uninstall)
{
	BOOL rc = FALSE;
	HDirectDrawCreate hDirectDrawCreate;

	if(hDDraw)
	{
		hDirectDrawCreate = (HDirectDrawCreate)GetProcAddress(hDDraw, "DirectDrawCreate");
		if(hDirectDrawCreate)
		{
			LPDIRECTDRAW dd = NULL;
			if(hDirectDrawCreate((LPGUID)DDCREATE_HARDWAREONLY, &dd, NULL) == DD_OK)
			{
				HMODULE hVMHAL = LoadLibraryA("vmhal9x.dll");
				if(hVMHAL)
				{
					if(!uninstall)
					{
						HInstallWineHook hInstallWineHook = (HInstallWineHook)GetProcAddress(hVMHAL, "InstallWineHook");
						if(hInstallWineHook)
						{
							if(hInstallWineHook())
							{
								rc = TRUE;
							} else sprintf(errormsg, "Hooking failure: InstallWineHook() = FALSE");
						} else sprintf(errormsg, "Hooking failure: GetProcAddress(\"InstallWineHook\")");
					}
					else
					{
						HUninstallWineHook hUninstallWineHook = (HUninstallWineHook)GetProcAddress(hVMHAL, "UninstallWineHook");
						if(hUninstallWineHook)
						{
							if(hUninstallWineHook())
							{
								rc = TRUE;
							} else sprintf(errormsg, "Hooking failure: UninstallWineHook() = FALSE");
						} else sprintf(errormsg, "Hooking failure: GetProcAddress(\"UninstallWineHook\")");
					}
					FreeLibrary(hVMHAL);
				} else sprintf(errormsg, "Hooking failure: LoadLibraryA(\"vmhal9x.dll\") = 0x%X", GetLastError());
				IDirectDraw_Release(dd);
			} else sprintf(errormsg, "Hooking failure: DirectDrawCreate(...)");
		} else sprintf(errormsg, "Hooking failure: GetProcAddress(\"DirectDrawCreate\")");
	} else sprintf(errormsg, "Hooking failure: LoadLibraryA(\"ddraw.dll\")");

	return rc;
}

int query_hook()
{
	int rc = -2;
	
	HMODULE hVMHAL = LoadLibraryA("vmhal9x.dll");
	if(hVMHAL)
	{
		HCheckWineHook hCheckHook = (HCheckWineHook)GetProcAddress(hVMHAL, "CheckWineHook");
		if(hCheckHook)
		{
			rc = hCheckHook();
		}
		FreeLibrary(hVMHAL);
	}
	
	return rc;
}

static void notify_set(HWND win, HANDLE icon, const char *tooltip)
{
	static BOOL installed = FALSE;
	HINSTANCE hInst = GetModuleHandle(NULL);

	NOTIFYICONDATAA nid;
	nid.cbSize = sizeof(nid);
	nid.hWnd = win;
	nid.uID = 100;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_NOTIFYMSG;
	nid.hIcon = LoadIcon(hInst, icon);

	strcpy(nid.szTip, tooltip);
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	if(installed)
	{
		Shell_NotifyIconA(NIM_MODIFY, &nid);
	}
	else
	{
		installed = Shell_NotifyIconA(NIM_ADD, &nid);
	}
}

static BOOL patchSuccess = FALSE;

static void show_error(HWND hwnd)
{
	MessageBoxA(hwnd, errormsg, "DD/DX patch error", MB_OK | MB_ICONERROR);
}

static void notify_status(HWND hwnd)
{
	if(patchSuccess)
	{
		int q = query_hook();
		if(q > 0)
		{
			notify_set(hwnd, MAKEINTRESOURCE(TRAY_ICON_GREEN), "D3D wrappers enabled");
		}
		else if(q == 0)
		{
			notify_set(hwnd, MAKEINTRESOURCE(TRAY_ICON_RED), "D3D wrappers disabled");
		}
		else
		{
			notify_set(hwnd, MAKEINTRESOURCE(TRAY_ICON_GRAY), "Interface cannot be patched");
		}
	}
	else
	{
		notify_set(hwnd, MAKEINTRESOURCE(TRAY_ICON_GRAY), "VMHAL9X driver not installed");
	}
}

static void run_monitor()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char exename[MAX_PATH];

	GetModuleFileNameA(NULL, exename, MAX_PATH);

	memset(&si, 0,  sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	// Start the child process.
	if(!CreateProcess(exename, "tray3d.exe /mon", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		return;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

static void run_vesamode(BOOL bpp15, BOOL bpp24)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char exename[MAX_PATH];
	char cmd[64];

	strcpy(cmd, "vesamode.exe /insert");

	if(bpp15)
	{
		strcat(cmd, " /15");
	}

	if(bpp24)
	{
		strcat(cmd, " /24");
	}

	GetModuleFileNameA(NULL, exename, MAX_PATH);
	char *ptr = strrchr(exename, '\\');
	if(ptr != NULL)
	{
		strcpy(ptr+1, "vesamode.exe");
	}

	memset(&si, 0,  sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	// Start the child process.
	if(!CreateProcess(exename, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		return;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

BOOL check_vesa_modes()
{
	BOOL rc = FALSE;
	HMODULE lib = LoadLibraryA("vmdisp9x.dll");
	if(lib)
	{
		FBHDA_setup_t fn_setup = (FBHDA_setup_t)GetProcAddress(lib, "FBHDA_setup");
		if(fn_setup != NULL)
		{
			FBHDA_t *hda = fn_setup();
			rc = ((hda->flags & FB_VESA_MODES) == 0) ? FALSE : TRUE;
		}
		FreeLibrary(lib);
	}

	return rc;
}

static BOOL is_vesa = FALSE;

LRESULT CALLBACK winproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
		{
			notify_status(hwnd);
			break;
		}
		case WM_NOTIFYMSG:
		{
			switch(lParam)
			{
				case WM_CONTEXTMENU:
				case WM_RBUTTONDOWN:
				case WM_LBUTTONDOWN:
				{
					HMENU menu = CreatePopupMenu();
					POINT point;
					if(menu)
					{
						int q = query_hook();

						InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_CLOSE,   "Close");
						InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_ABOUT,   "About");
						InsertMenuA(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0,       NULL);
						if(!patchSuccess)
						{
							InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_INSTALL, "Install");
						}
						else
						{
							if(q > 0)
							{
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_DISABLE, "Disable");
							}
							else if(q == 0)
							{
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_ENABLE, "Enable");
							}
							else
							{
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, "Enable");
							}
						}

						InsertMenuA(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0,       NULL);
						if(patchSuccess)
						{
							if(q > 0)
							{
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, "D3D wrappers enabled");
							}
							else if(q == 0)
							{
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, "D3D wrappers disabled");
							}
							else
							{
								char buf[128];
								sprintf(buf, "Interface cannot be patched, rc=%d", q);
								InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, /*"Interface cannot be patched"*/ buf);
							}
						}
						else
						{
							InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_SHOW_ERROR, "Interface patch error!");
						}

						if(is_vesa)
						{
							InsertMenuA(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
							InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_VESAMODE, "Update display modes");
						}

						InsertMenuA(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
						InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_MONITOR, "Open GPU monitor");

						SetForegroundWindow(hwnd);
						GetCursorPos(&point);
						TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd, NULL);
					}
					break;
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case ID_CLOSE:
					PostQuitMessage(0);
					break;
				case ID_SHOW_ERROR:
					show_error(hwnd);
					break;
				case ID_ABOUT:
					MessageBoxA(hwnd, "DirectX/DirectDraw interface patching utility.\nPart of vmhal9x (version " VMHAL9X_VERSION_STR ")", "About Tray3D", MB_OK | MB_ICONINFORMATION);
					break;
				case ID_INSTALL:
				case ID_ENABLE:
					patchSuccess = install_hook(FALSE);
					if(!patchSuccess)
					{
						show_error(hwnd);
					}
					notify_status(hwnd);
					break;
				case ID_DISABLE:
					patchSuccess = install_hook(TRUE);
					if(!patchSuccess)
					{
						show_error(hwnd);
					}
					notify_status(hwnd);
					break;
				case ID_MONITOR:
					run_monitor();
					break;
				case ID_VESAMODE:
					run_vesamode(FALSE, TRUE);
					break;
			}
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK kill_tray_windows(HWND hwnd, LPARAM lParam)
{
	static char cls_name[CLSMAX];

	if(GetClassName(hwnd, cls_name, CLSMAX) != 0)
	{
		if(stricmp(WND_TRAY_CLASS_NAME, cls_name) == 0)
		{
			SendMessage(hwnd, WM_DESTROY, 0, 0);
		}
	}

	return TRUE;
}

void monitor();

int main(int argc, char **argv)
{
	HINSTANCE hInst = GetModuleHandle(NULL);
	int i;
	for(i = 1; i < argc; i++)
	{
		if(stricmp(argv[i], "/kill") == 0)
		{
			EnumWindows(kill_tray_windows, 0);
			return EXIT_SUCCESS;
		}
		
		if(stricmp(argv[i], "/mon") == 0)
		{
			monitor();
			return EXIT_SUCCESS;
		}
	}

	if(CoInitialize(NULL) != S_OK)
	{
		fprintf(stderr, "FAILED: CoInitialize()\n");
		return EXIT_FAILURE;
	}

	hDDraw = LoadLibraryA("ddraw.dll");

	patchSuccess = install_hook(FALSE);

	is_vesa = check_vesa_modes();

	WNDCLASS wc_win;
	HWND win;
	MSG msg;
	memset(&wc_win, 0, sizeof(wc_win));

	wc_win.style         = CS_HREDRAW | CS_VREDRAW;
	wc_win.lpfnWndProc   = winproc;
	wc_win.lpszClassName = WND_TRAY_CLASS_NAME;
	wc_win.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc_win.hCursor       = LoadCursor(0, IDC_ARROW);
	wc_win.hIcon         = LoadIconA(hInst, MAKEINTRESOURCE(TRAY_ICON_GRAY));
	wc_win.hInstance     = hInst;
	RegisterClass(&wc_win);

	win = CreateWindowA(WND_TRAY_CLASS_NAME, "", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, 0, 0, hInst, 0);

  while(GetMessage(&msg, NULL, 0, 0))
  {
		if(!IsDialogMessageA(win, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
  }

  DestroyWindow(win);

  if(hDDraw)
  {
		FreeLibrary(hDDraw);
  }

	CoUninitialize();
	ExitProcess(EXIT_SUCCESS);
	
	return EXIT_SUCCESS;
}
