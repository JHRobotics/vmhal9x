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

void Mesa3DCalibrate(BOOL loadonly)
{
	VMHAL_enviroment_t *env = GlobalVMHALenv();

	if(env->scanned)
		return;
	
	FBHDA_t *hda = FBHDA_setup();
	DWORD pid = GetCurrentProcessId();
	mesa3d_ctx_t *ctx_ptr = NULL;
	BOOL restart_os = FALSE;
	BOOL only_2d = FALSE;
	mesa3d_entry_t *full_entry = Mesa3DGet(pid, TRUE);

	env->s3tc_bug = TRUE;

	if(hda->vram_size < hda->vram_bar_size)
	{
		env->sysmem = TRUE;
	}

	if((hda->flags & FB_VESA_MODES) != 0)
	{
		env->sysmem = TRUE;
	}

	do
	{
		if(!full_entry)
		{
			restart_os = TRUE;
			break;
		}

		if(!loadonly)
		{
			ctx_ptr = MesaCreateCtx(full_entry, 0, 0);
			if(!ctx_ptr)
			{
				restart_os = TRUE;
				break;
			}

			GL_BLOCK_BEGIN(ctx_ptr)
				if(entry->gl_major < 2 || (entry->gl_major == 2 && entry->gl_minor < 1))
				{
					restart_os = TRUE;
				}
			GL_BLOCK_END
		}
		else
		{
			if(!full_entry->os)
			{
				env->filter_bug = TRUE;
			}
		}
	}while(0);

	if(full_entry)
	{
		Mesa3DFree(pid, TRUE);
	}

	if(restart_os)
	{
		env->forceos = TRUE;
		full_entry = Mesa3DGet(pid, TRUE);
		
		do
		{
			if(!full_entry)
			{
				only_2d = TRUE;
				break;
			}

			if(!loadonly)
			{
				ctx_ptr = MesaCreateCtx(full_entry, 0, 0);
				if(!ctx_ptr)
				{
					only_2d = TRUE;
					break;
				}
			}
			// ...
		}while(0);

		if(full_entry)
		{
			Mesa3DFree(pid, TRUE);
		}
	}
	
	if(only_2d)
	{
		env->only2d = TRUE;
	}
	
	env->scanned = TRUE;
}

