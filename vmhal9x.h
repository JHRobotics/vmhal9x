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

#define VMHAL9X_MAJOR 0
#define VMHAL9X_MINOR 4
#define VMHAL9X_PATCH 2024
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

/* debuging macros */
#ifndef DDDEBUG
#define DDDEBUG 0
#endif

void dbg_prefix_printf(const char *prefix, const char *file, int line, const char *fmt, ...);

#if DDDEBUG >= 4
# define dbg_printf(_fmt, ...) dbg_prefix_printf("D|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
#else
# define dbg_printf(_fmt, ...)
#endif

#if DDDEBUG >= 3
# define TRACE(_fmt, ...) dbg_prefix_printf("T|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define TRACE_ENTRY TRACE("%s", __FUNCTION__);
# define TRACE_ON
#else
# define TRACE(_fmt, ...)
# define TRACE_ENTRY
#endif

#if DDDEBUG >= 2
# define WARN(_fmt, ...) dbg_prefix_printf("W|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define WARN_ON
#else
# define WARN(_fmt, ...)
#endif

#if DDDEBUG >= 1
# define ERR(_fmt, ...) dbg_prefix_printf("E|", __FILE__, __LINE__, _fmt __VA_OPT__(,) __VA_ARGS__)
# define ERR_ON
#else
# define ERR(_fmt, ...)
#endif


/* calls */
VMDAHAL_t *GetHAL(LPDDRAWI_DIRECTDRAW_GBL lpDD);

DWORD __stdcall CanCreateSurface(LPDDHAL_CANCREATESURFACEDATA pccsd);
DWORD __stdcall CreateSurface(LPDDHAL_CREATESURFACEDATA pcsd);
DWORD __stdcall DestroySurface( LPDDHAL_DESTROYSURFACEDATA lpd);

DWORD __stdcall Flip32(LPDDHAL_FLIPDATA pfd);
DWORD __stdcall GetFlipStatus32(LPDDHAL_GETFLIPSTATUSDATA pfd);
DWORD __stdcall Lock32(LPDDHAL_LOCKDATA pld);
DWORD __stdcall Unlock32(LPDDHAL_UNLOCKDATA pld);
DWORD __stdcall WaitForVerticalBlank32(LPDDHAL_WAITFORVERTICALBLANKDATA pwd);
DWORD __stdcall Blt32(LPDDHAL_BLTDATA pbd);
DWORD __stdcall GetBltStatus32(LPDDHAL_GETBLTSTATUSDATA pbd);
DWORD __stdcall SetExclusiveMode32(LPDDHAL_SETEXCLUSIVEMODEDATA psem);
DWORD __stdcall SetMode32(LPDDHAL_SETMODEDATA psmod);

/* FBHDA */
BOOL FBHDA_load_ex(VMDAHAL_t *pHal);

/* framebuffer */
#define INVALID_OFFSET 0xFFFFFFFF
BOOL IsInFront(VMDAHAL_t *ddhal, void *ptr);
DWORD GetOffset(VMDAHAL_t *ddhal, void *ptr);

/* globals */
extern VMDAHAL_t *globalHal;

#endif /* __VMHAL9X_H__INCLUDED__ */
