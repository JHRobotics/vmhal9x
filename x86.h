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
#ifndef __VMHAL9X__X86_H__INCLUDED__
#define __VMHAL9X__X86_H__INCLUDED__

/*
From DDK:

The DirectX runtime saves and restores floating-point state as required
for the following Direct3D callback functions:

D3dContextCreate 
D3dContextDestroy 
D3dDrawPrimitives2 
D3dGetDriverState 
D3dValidateTextureStageState 

For the following callback functions, a Direct3D-supported display driver
must save floating-point state before performing floating-point operations,
and restore it when the operations are complete:

D3dCreateSurfaceEx 
D3dDestroyDDLocal 
D3DBuffer Callbacks
(and all DDRAW functions)
*/

#define FXSAVE_REGION_SIZE 512
#define FSAVE_REGION_SIZE 128

#ifdef __SSE__
/* save using FXSAVE */
/* todo: detect AVX/XSAVE and use XSAVE when available */
#define SAVE_X87_XMM \
	unsigned char saveregion[FXSAVE_REGION_SIZE] __attribute__((aligned(32))); \
	asm volatile ("FXSAVE %0" : : "m" (saveregion))

#define RESTORE_X87_XMM \
	asm volatile ("FXRSTOR %0" : : "m" (saveregion))

#else /* __SSE__ */
/* save only x87/MMX (FSAVE) */
#define SAVE_X87_XMM \
	unsigned char saveregion[FSAVE_REGION_SIZE]; \
	asm volatile ("FNSAVE %0" : : "m" (saveregion))

#define RESTORE_X87_XMM \
	asm volatile ("FRSTOR %0" : : "m" (saveregion))

#endif /* !__SSE__ */

#define DDENTRY_FPUSAVE(_name, _argtype, _argname) \
static DWORD _name ## _proc(_argtype _argname); \
DWORD __stdcall _name(_argtype _argname){ \
	DWORD rc; \
	SAVE_X87_XMM; \
	rc = _name ## _proc(_argname); \
	RESTORE_X87_XMM; \
	return rc;} \
static DWORD _name ## _proc(_argtype _argname)

#define DDENTRY(_name, _argtype, _argname) \
DWORD __stdcall _name(_argtype _argname)

#endif /* __VMHAL9X__X86_H__INCLUDED__ */
