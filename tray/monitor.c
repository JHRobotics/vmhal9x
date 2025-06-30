#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define VERSION_ONLY
#include "../vmhal9x.h"
#include "tray3d.h"

#include "../3d_accel.h"

#include "nocrt.h"

#define IDT_TIMER1    9

#define WVAL_RAM_TOTAL 10
#define WVAL_RAM_VGPU 11
#define WVAL_RAM_FREE 12

#define WVAL_VRAM_TOTAL 14
#define WVAL_VRAM_VFB 15
#define WVAL_VRAM_SFB 16
#define WVAL_VRAM_DX 17
#define WVAL_VRAM_OVR 18
#define WVAL_VRAM_FREE 19

#define WVAL_VGPU_TOTAL  20
#define WVAL_VGPU_USED   21
#define WVAL_VGPU_FREE   22

#define WVAL_MAX 23

static HWND vals[WVAL_MAX] = {NULL};
static BOOL close_win = FALSE;

static HMODULE vmdisp9x = NULL;
static HMODULE vmhal9x = NULL;

static void draw(HWND win, HINSTANCE inst)
{
	int y = 10;
	int x = 10;
	CreateWindowA("STATIC", "RAM, physical total: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "RAM, reserved by vGPU: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "RAM, physical free: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	y += 20;
	CreateWindowA("STATIC", "VRAM, total: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "VRAM, used by (virtual) FB: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "VRAM, used by shadow FB: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "VRAM, used by DD/DX: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "VRAM, used by overlays: ",WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "VRAM, free: ", WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	y += 20;
	CreateWindowA("STATIC", "vGPU, total: ", WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "vGPU, used: ", WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;
	CreateWindowA("STATIC", "vGPU, free: ", WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, 200, 20, win, 0, inst, NULL); y += 20;

	y = 10;
	x += 200;

	vals[WVAL_RAM_TOTAL] = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_RAM_TOTAL, inst, NULL);   y += 20;
	vals[WVAL_RAM_VGPU]  = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_RAM_VGPU, inst, NULL);    y += 20;
	vals[WVAL_RAM_FREE]  = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_RAM_FREE, inst, NULL);    y += 20;
	y += 20;
	vals[WVAL_VRAM_TOTAL] = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_TOTAL, inst, NULL); y += 20;
	vals[WVAL_VRAM_VFB]   = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_VFB, inst, NULL);   y += 20;
	vals[WVAL_VRAM_SFB]   = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_SFB, inst, NULL);   y += 20;
	vals[WVAL_VRAM_DX]    = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_DX, inst, NULL);    y += 20;
	vals[WVAL_VRAM_OVR]   = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_OVR, inst, NULL);   y += 20;
	vals[WVAL_VRAM_FREE]  = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VRAM_FREE, inst, NULL);  y += 20;
	y += 20;
	vals[WVAL_VGPU_TOTAL] = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VGPU_TOTAL, inst, NULL); y += 20;
	vals[WVAL_VGPU_USED]  = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VGPU_USED, inst, NULL);  y += 20;
	vals[WVAL_VGPU_FREE]  = CreateWindowA("STATIC", "- kB", WS_VISIBLE | WS_CHILD | SS_RIGHT, x, y, 100, 20, win, (HMENU)WVAL_VGPU_FREE, inst, NULL);  y += 20;
}

static void setKB(int id, DWORD v)
{
	static char buf[32];
	sprintf(buf, "%lu kB", v/1024);
	
	HWND win = vals[id];
	if(win)
	{
		SetWindowTextA(win, buf);
	}
}

typedef BOOL (__stdcall *VidMemInfo_t)(DWORD *pused, DWORD *pfree);

static void update(HWND win)
{
	MEMORYSTATUS minfo;
	GlobalMemoryStatus(&minfo);
	
	setKB(WVAL_RAM_TOTAL, minfo.dwTotalPhys);
	setKB(WVAL_RAM_FREE, minfo.dwAvailPhys);
	
	if(vmdisp9x)
	{
		FBHDA_setup_t pFBHDA_setup = (FBHDA_setup_t)GetProcAddress(vmdisp9x, "FBHDA_setup");
		if(pFBHDA_setup)
		{
			FBHDA_t *hda = pFBHDA_setup();
			if(hda)
			{
				setKB(WVAL_VRAM_TOTAL, hda->vram_size);
				
				if(hda->system_surface != 0)
				{
					setKB(WVAL_VRAM_VFB, hda->system_surface);
					setKB(WVAL_VRAM_SFB, hda->stride);
				}
				else
				{
					setKB(WVAL_VRAM_VFB, hda->stride);
					setKB(WVAL_VRAM_SFB, 0);
				}
				setKB(WVAL_VRAM_OVR, hda->overlays_size);

				if(hda->flags & FB_ACCEL_GPUMEM)
				{
					setKB(WVAL_VGPU_TOTAL, hda->gpu_mem_total);
					setKB(WVAL_VGPU_USED, hda->gpu_mem_used);
					setKB(WVAL_VGPU_FREE, hda->gpu_mem_total-hda->gpu_mem_used);

					setKB(WVAL_RAM_VGPU, hda->gpu_mem_total);
				}
				else
				{
					setKB(WVAL_VGPU_TOTAL, 0);
					setKB(WVAL_VGPU_USED, 0);
					setKB(WVAL_VGPU_FREE, 0);
					setKB(WVAL_RAM_VGPU, 0);
				}
			}
		}
	} // vmdisp9x

	if(vmhal9x)
	{
		VidMemInfo_t pVidMemInfo = GetProcAddress(vmhal9x, "VidMemInfo");
		if(pVidMemInfo)
		{
			DWORD dx_used;
			DWORD dx_free;
			if(pVidMemInfo(&dx_used, &dx_free))
			{
				setKB(WVAL_VRAM_DX, dx_used);
				setKB(WVAL_VRAM_FREE, dx_free);
			}
			else
			{
				//setKB(WVAL_VRAM_DX, 0);
				setKB(WVAL_VRAM_FREE, 0);
			}
		}
	}
}

static LRESULT CALLBACK monproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
		{
			draw(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);
			update(hwnd);
			SetTimer(hwnd, IDT_TIMER1, 1000, (TIMERPROC) NULL);
			break;
		}
		case WM_TIMER:
		{ 
			switch(wParam) 
			{ 
        case IDT_TIMER1:
        	update(hwnd);
        	break;
			}
			break;
		}
		case WM_DESTROY:
			KillTimer(hwnd, IDT_TIMER1);
			close_win = TRUE;
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#define WND_MON_CLASS_NAME "tray3dmon"

void monitor()
{
	WNDCLASS moncs;
	HWND win;
	MSG msg;
	HINSTANCE hInst = GetModuleHandle(NULL);

	vmdisp9x = LoadLibraryA("vmdisp9x.dll");
	vmhal9x  = LoadLibraryA("vmhal9x.dll");

	memset(&moncs, 0, sizeof(moncs));

	moncs.style         = CS_HREDRAW | CS_VREDRAW;
	moncs.lpfnWndProc   = monproc;
	moncs.lpszClassName = WND_MON_CLASS_NAME;
	moncs.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	moncs.hCursor       = LoadCursor(0, IDC_ARROW);
	moncs.hIcon         = LoadIconA(hInst, MAKEINTRESOURCE(ICON_MAIN));
	moncs.hInstance     = hInst;
	RegisterClass(&moncs);

	win = CreateWindowA(WND_MON_CLASS_NAME, "vGPU monitor", WS_OVERLAPPED|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, 0, 0, hInst, 0);
	
  while(GetMessage(&msg, NULL, 0, 0))
  {
		if(!IsDialogMessageA(win, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(close_win)
		{
			DestroyWindow(win);
			break;
		}
	}

	if(vmdisp9x)
	{
		FreeLibrary(vmdisp9x);
	}
	
	if(vmhal9x)
	{
		FreeLibrary(vmhal9x);
	}
}
