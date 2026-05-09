/******************************************************************************
 * Copyright (c) 2026 Jaroslav Hensl                                          *
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
#include "wine.h"

#include "vmsetup.h"

#include "nocrt.h"

static VMHAL_enviroment_t VMHALenv = {
	FALSE, /* scanned */
	FALSE, /* only2d */
	FALSE, /* forceos */
	FALSE, /* runtime dx5 */
	FALSE, /* runtime dx6 */
	FALSE, /* runtime dx7 */
	FALSE, /* runtime dx8 */
	FALSE, /* runtime dx9 */
	8, // DDI (maximum)
	8, // HW T&L
	FALSE, // readback
	FALSE, // touchdepth
	16384, // tex w  (can be query by GL_MAX_TEXTURE_SIZE)
	16384, // tex h
	4, // tex units
	8, // lights (GL min. is 8)
	6, // clip planes (GL min. is 6), GL_MAX_CLIP_PLANES
	TRUE, // use float32 in Z buffer (eg 64-bit F32_S8_X24 depth plane), on FALSE 32-bit S24_S8 depth plane
	16, // max anisotropy
	FALSE, // vertexblend
	FALSE, // use palette
	FALSE,  // filter bug
	FALSE, // s3tc bug
	FALSE, ///TRUE,  // textures in sysmem
	0,     // low detail
};

VMHAL_enviroment_t* __stdcall hal_env()
{
	return &VMHALenv;
}

BOOL GetVMHALenv(VMHAL_enviroment_t *dst)
{
	if(dst == NULL) return FALSE;

	memcpy(dst, &VMHALenv, sizeof(VMHAL_enviroment_t));

	return TRUE;
}

void VMHALenv_RuntimeVer(int ver)
{
	if(ver >= 9) VMHALenv.dx9 = TRUE;
	if(ver >= 8) VMHALenv.dx8 = TRUE;
	if(ver >= 7) VMHALenv.dx7 = TRUE;
	if(ver >= 6) VMHALenv.dx6 = TRUE;
	if(ver >= 5) VMHALenv.dx5 = TRUE;
}
