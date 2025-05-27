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
#include <math.h>
#include "ddrawi_ddk.h"
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

NUKED_LOCAL void MesaVSCreate(mesa3d_ctx_t *ctx, D3DHAL_DP2CREATEVERTEXSHADER *shader, const BYTE *buffer)
{
	mesa_dx_shader_t **last_ptr = &ctx->shader.vs;
	mesa_dx_shader_t *next = NULL;

	while((*last_ptr) != NULL)
	{
		if((*last_ptr)->handle == shader->dwHandle)
		{
			next = (*last_ptr)->next;
			break;
		}

		last_ptr = &((*last_ptr)->next);
	}

	size_t s = sizeof(mesa_dx_shader_t) + shader->dwDeclSize + shader->dwCodeSize;
	BYTE *buf = hal_alloc(HEAP_NORMAL, s, 0);
	if(buf)
	{
		if((*last_ptr) != NULL)
		{
			hal_free(HEAP_NORMAL, *last_ptr);
		}

		mesa_dx_shader_t *vs = (mesa_dx_shader_t *)buf;
		buf += sizeof(mesa_dx_shader_t);
		vs->decl_size = shader->dwDeclSize;
		vs->decl = buf;
		buf += shader->dwDeclSize;
		vs->code_size = shader->dwCodeSize;
		vs->code = buf;
		vs->handle = shader->dwHandle;

		memcpy(vs->decl, buffer, vs->decl_size);
		memcpy(vs->code, buffer+vs->decl_size, vs->code_size);

		vs->next = next;
		*last_ptr = vs;
	}
}

NUKED_LOCAL void MesaVSDestroy(mesa3d_ctx_t *ctx, DWORD handle)
{
	mesa_dx_shader_t **ptr = &ctx->shader.vs;

	while((*ptr) != NULL)
	{
		if((*ptr)->handle == handle)
		{
			mesa_dx_shader_t *vs = (*ptr);
			*ptr = vs->next;
			hal_free(HEAP_NORMAL, vs);
		}
		else
		{
			ptr = &((*ptr)->next);
		}
	}
}

NUKED_LOCAL void MesaVSDestroyAll(mesa3d_ctx_t *ctx)
{
	while(ctx->shader.vs != NULL)
	{
		mesa_dx_shader_t *vs = ctx->shader.vs;
		ctx->shader.vs = vs->next;
		hal_free(HEAP_NORMAL, vs);
	}
}

NUKED_LOCAL mesa_dx_shader_t *MesaVSGet(mesa3d_ctx_t *ctx, DWORD handle)
{
	mesa_dx_shader_t *vs = ctx->shader.vs;
	while(vs != NULL)
	{
		if(vs->handle == handle)
		{
			return vs;
		}
		vs = vs->next;
	}

	return NULL;
}
