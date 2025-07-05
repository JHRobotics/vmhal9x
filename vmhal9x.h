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
#ifndef __VMHAL9X_H__INCLUDED__
#define __VMHAL9X_H__INCLUDED__

/* version */
#define VMHAL9X_STR_(x) #x
#define VMHAL9X_STR(x) VMHAL9X_STR_(x)

#define VMHAL9X_MAJOR 4 /* eq. Windows 9x */
#define VMHAL9X_MINOR 8
#define VMHAL9X_PATCH 2025
#ifndef VMHAL9X_BUILD
#define VMHAL9X_BUILD 0
#endif

#define VMHAL9X_VERSION_STR_BUILD(_ma, _mi, _pa, _bl) \
	_ma "." _mi "." _pa "." _bl

#define VMHAL9X_VERSION_STR VMHAL9X_VERSION_STR_BUILD( \
	VMHAL9X_STR(VMHAL9X_MAJOR), \
	VMHAL9X_STR(VMHAL9X_MINOR), \
	VMHAL9X_STR(VMHAL9X_PATCH), \
	VMHAL9X_STR(VMHAL9X_BUILD))

#define VMHAL9X_VERSION_NUM \
 	VMHAL9X_MAJOR,VMHAL9X_MINOR,VMHAL9X_PATCH,VMHAL9X_BUILD

#ifndef VERSION_ONLY

#include "x86.h"
#include "memory.h"


/* debuging macros */
#ifndef DDDEBUG
#define DDDEBUG 0
#endif

void dbg_prefix_printf(const char *topic, const char *prefix, const char *file, int line, const char *fmt, ...);

#if DDDEBUG >= 4
# define dbg_printf(_fmt, ...) dbg_prefix_printf(NULL, "D|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
#else
# define dbg_printf(_fmt, ...)
#endif

#if DDDEBUG >= 3
# define TRACE(_fmt, ...) dbg_prefix_printf(NULL, "T|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define TRACE_ENTRY TOPIC("ENTRY", "%s", __FUNCTION__);
# define TOPIC(_name, _fmt, ...) dbg_prefix_printf(_name, "T|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define TRACE_ON
#else
# define TRACE(_fmt, ...)
# define TRACE_ENTRY
# define TOPIC(_name, _fmt, ...)
#endif

#if DDDEBUG >= 2
# define WARN(_fmt, ...) dbg_prefix_printf(NULL, "W|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define WARN_ON
#else
# define WARN(_fmt, ...)
#endif

#if DDDEBUG >= 1
# define ERR(_fmt, ...) dbg_prefix_printf(NULL, "E|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define ERR_ON
#else
# define ERR(_fmt, ...)
#endif

/* calls */
VMDAHAL_t *GetHAL(LPDDRAWI_DIRECTDRAW_GBL lpDD);

DDENTRY(CanCreateSurface32, LPDDHAL_CANCREATESURFACEDATA, pccsd);
DDENTRY(CreateSurface32, LPDDHAL_CREATESURFACEDATA, pcsd);
DDENTRY(DestroySurface32, LPDDHAL_DESTROYSURFACEDATA, lpd);

DDENTRY(Flip32, LPDDHAL_FLIPDATA, pfd);
DDENTRY(GetFlipStatus32, LPDDHAL_GETFLIPSTATUSDATA, pfd);
DDENTRY(Lock32, LPDDHAL_LOCKDATA, pld);
DDENTRY(Unlock32, LPDDHAL_UNLOCKDATA, pld);
DDENTRY(WaitForVerticalBlank32, LPDDHAL_WAITFORVERTICALBLANKDATA, pwd);
DDENTRY(Blt32, LPDDHAL_BLTDATA, pbd);
DDENTRY(GetBltStatus32, LPDDHAL_GETBLTSTATUSDATA, pbd);
DDENTRY(SetExclusiveMode32, LPDDHAL_SETEXCLUSIVEMODEDATA, psem);
DDENTRY(SetMode32, LPDDHAL_SETMODEDATA, psmod);
DDENTRY(GetDriverInfo32, LPDDHAL_GETDRIVERINFODATA, lpInput);
DDENTRY(SetColorKey32, LPDDHAL_SETCOLORKEYDATA, lpSetColorKey);
DDENTRY(AddAttachedSurface32, LPDDHAL_ADDATTACHEDSURFACEDATA, lpAddSurfData);

DDENTRY(DestroyDriver32, LPDDHAL_DESTROYDRIVERDATA, pdstr);

BOOL __stdcall D3DHALCreateDriver(DWORD *lplpGlobal, DWORD *lplpHALCallbacks, LPDDHAL_DDEXEBUFCALLBACKS lpHALExeBufCallbacks, VMDAHAL_D3DCAPS_t *lpHALFlags);

/* FBHDA */
BOOL FBHDA_load_ex(VMDAHAL_t *pHal);

/* memory and proc management */
BOOL ProcessExists(DWORD pid);

/* framebuffer */
#define INVALID_OFFSET 0xFFFFFFFF
BOOL IsInFront(VMDAHAL_t *ddhal, void *ptr);
DWORD GetOffset(VMDAHAL_t *ddhal, void *ptr);
BOOL FlipPrimary(VMDAHAL_t *ddhal, void *to);

/* modes */
void UpdateCustomMode(VMDAHAL_t *hal);

/* globals */
extern VMDAHAL_t *globalHal;
extern BOOL halVSync;
extern uint64_t last_flip_time;
#define SCREEN_TIME ((uint64_t)(10000/60))

/* debug */
void SetExceptionHandler();

/* timing */
uint64_t GetTimeTMS();

/* mesa */
void Mesa3DCleanProc();
void Mesa3DCalibrate(BOOL loadonly);

void SurfaceCtxLock();
void SurfaceCtxUnlock();

typedef DWORD surface_id;

#define MAX_SURFACES 65535

DWORD SurfaceCreate(LPDDRAWI_DDRAWSURFACE_LCL surf);
BOOL SurfaceDelete(surface_id sid);
void SurfaceDeleteAll();
void SurfaceToMesaTex(surface_id sid);
void SurfaceToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL texonly);
void SurfaceZToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD color);
void SurfaceSetFormat(surface_id sid, DDPIXELFORMAT *fmt, DWORD screen_bpp);

void SurfaceFromMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL texonly);
BOOL SurfaceIsEmpty(surface_id sid);
void SurfaceApplyColorKey(surface_id sid, DWORD low, DWORD hi, DWORD low_pal, DWORD hi_pal);
void SurfaceClearEmpty(surface_id sid);
void SurfaceClearData(surface_id sid);
DWORD SurfaceDataSize(LPDDRAWI_DDRAWSURFACE_GBL gbl, DWORD *outPitch);

void SurfaceLock(LPDDRAWI_DDRAWSURFACE_LCL surf);
void SurfaceUnlock(LPDDRAWI_DDRAWSURFACE_LCL surf);

inline static DWORD SurfacePitch(DWORD width, DWORD bpp)
{
	DWORD bp = (bpp + 7) / 8;
	return (bp * width + (FBHDA_ROW_ALIGN-1)) & (~((DWORD)FBHDA_ROW_ALIGN-1));
}

#define VMHAL_DSTR2(_x) #_x
#define VMHAL_DSTR(_x) VMHAL_DSTR2(_x)

typedef struct _VMHAL_enviroment
{
	BOOL scanned;
	BOOL only2d;
	BOOL forceos;
	BOOL dx5;
	BOOL dx6; // latch for runtime >= dx6
	BOOL dx7; // ... dx7
	BOOL dx8; // ... dx8
	BOOL dx9; // ... dx9
	DWORD ddi;
	DWORD hwtl_ddi;
	BOOL  readback; // readback surface color/depth
	BOOL  touchdepth;
	DWORD texture_max_width;
	DWORD texture_max_height;
	DWORD texture_num_units;
	DWORD num_light;
	DWORD num_clips;
	BOOL zfloat;
	DWORD max_anisotropy;
	BOOL vertexblend;
	BOOL allow_palette;
	BOOL filter_bug;
	BOOL s3tc_bug;
	BOOL sysmem;
} VMHAL_enviroment_t;

#define DX7_SURFACE_NEST_TYPES (DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER)

BOOL GetVMHALenv(VMHAL_enviroment_t *dst);
void VMHALenv_RuntimeVer(int ver);
VMHAL_enviroment_t *GlobalVMHALenv();

#endif /* VERSION_ONLY */

#endif /* __VMHAL9X_H__INCLUDED__ */
