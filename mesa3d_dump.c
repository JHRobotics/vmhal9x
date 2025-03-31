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
#ifndef NUKED_SKIP
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

#ifdef DEBUG

static DWORD dump_id = 0;

NUKED_LOCAL int mesa_dump_key()
{
	static BYTE keys[256];
	
	if(GetKeyboardState(keys))
	{
		if(keys[VK_F12] & 0xF0)
		{
			if(keys[VK_CONTROL] & 0xF0)
			{
				return MESA_KEY_DUMP_MORE;
			}
			else
			{
				return MESA_KEY_DUMP;
			}
		}
	}
	
	SHORT t = GetAsyncKeyState(VK_F12);
	if(t & 0x8000)
	{
		return MESA_KEY_DUMP;
	}
	
	//return MESA_KEY_DUMP;
	return 0;
}

NUKED_LOCAL void mesa_dump_inc()
{
	dump_id++;
}

#define DUMP_D(_n) fprintf(fw, "%s=%d\r\n", #_n, ctx->_n);
#define DUMP_X(_n) fprintf(fw, "%s=0x%08X\r\n", #_n, ctx->_n);
#define DUMP_F(_n) fprintf(fw, "%s=%f\r\n", #_n, ctx->_n);
#define DUMP_V(_n) fprintf(fw, "%s=[%f %f %f %f]\r\n", #_n, ctx->_n[0], ctx->_n[1], ctx->_n[2], ctx->_n[3]);
#define DUMP_M(_n) fprintf(fw, "%s=[\r\n", #_n); \
	fprintf(fw, " %f %f %f %f\r\n",  ctx->_n[0],  ctx->_n[1],  ctx->_n[2],  ctx->_n[3]); \
	fprintf(fw, " %f %f %f %f\r\n",  ctx->_n[4],  ctx->_n[5],  ctx->_n[6],  ctx->_n[7]); \
	fprintf(fw, " %f %f %f %f\r\n",  ctx->_n[8],  ctx->_n[9], ctx->_n[10], ctx->_n[11]); \
	fprintf(fw, " %f %f %f %f\n]\r\n", ctx->_n[12], ctx->_n[13], ctx->_n[14], ctx->_n[15]);

NUKED_LOCAL void mesa_dump(mesa3d_ctx_t *ctx)
{
	FILE *fw;
	char fn[32];
	sprintf(fn, "C:\\vmhal\\hal_%04d.log", dump_id);
	
	fw = fopen(fn, "fw");
	if(fw)
	{
#include "mesa3d_dump.h"

		fclose(fw);
	}
}

#endif /* DEBUG */
