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

NUKED_LOCAL void MesaVSDump(mesa_dx_shader_t *vs)
{
	size_t i;
	char *buf;

	if(vs->decl_size > 0)
	{
		buf = hal_alloc(HEAP_NORMAL, vs->decl_size * 3 + 1, 0);

		for(i = 0; i < vs->decl_size; i++)
		{
			sprintf(buf+i*3, "%02X ", vs->decl[i]);
		}

		TOPIC("SHADER", "DECL (%d): %s", vs->decl_size, buf);
		hal_free(HEAP_NORMAL, buf);
	}
	else
	{
		TOPIC("SHADER", "DECL (0): -");
	}

	if(vs->code_size > 0)
	{
		buf = hal_alloc(HEAP_NORMAL, vs->code_size * 3 + 1, 0);

		for(i = 0; i < vs->code_size; i++)
		{
			sprintf(buf+i*3, "%02X ", vs->code[i]);
		}

		TOPIC("SHADER", "CODE (%d): %s", vs->code_size, buf);
		hal_free(HEAP_NORMAL, buf);
	}
	else
	{
		TOPIC("SHADER", "CODE (0): -");
	}
}

NUKED_FAST void D3DVSD2Mesa(DWORD vsdt, mesa_vertex_data_t *out_dt, DWORD *out_dsize)
{
	mesa_vertex_data_t td;
	DWORD dsize;

	switch(vsdt)
	{
		case D3DVSDT_FLOAT1:
			td    = MESA_VDT_FLOAT1;
			dsize = 1;
			break;
		case D3DVSDT_FLOAT2:
			td    = MESA_VDT_FLOAT2;
			dsize = 2;
			break;
		case D3DVSDT_FLOAT3:
			td    = MESA_VDT_FLOAT3;
			dsize = 3;
			break;
		case D3DVSDT_FLOAT4:
			td    = MESA_VDT_FLOAT4;
			dsize = 4;
			break;
		case D3DVSDT_D3DCOLOR:
			td    = MESA_VDT_D3DCOLOR;
			dsize = 1;
			break;
		case D3DVSDT_UBYTE4:
			td    = MESA_VDT_UBYTE4;
			dsize = 1;
			break;
		case D3DVSDT_SHORT2:
			td    = MESA_VDT_USHORT2;
			dsize = 1;
			break;
		case D3DVSDT_SHORT4:
			td    = MESA_VDT_USHORT4;
			dsize = 2;
			break;
		default:
			td    = MESA_VDT_NONE;
			dsize = 0;
			break;
	}

	if(out_dt != NULL)
		*out_dt = td;

	if(out_dsize != NULL)
		*out_dsize = dsize;
}

NUKED_LOCAL BOOL MesaVSSetVertex(mesa3d_ctx_t *ctx, mesa_dx_shader_t *vs)
{
	DWORD *decl = (DWORD*)vs->decl;
	DWORD cnt = vs->decl_size / 4;
	DWORD i;
	DWORD offset = 0;
	int p;

	ctx->state.bind_vertices = 0;
	ctx->state.vertex.type.xyzw     = MESA_VDT_NONE;
	ctx->state.vertex.type.normal   = MESA_VDT_NONE;
	ctx->state.vertex.type.diffuse  = MESA_VDT_NONE;
	ctx->state.vertex.type.specular = MESA_VDT_NONE;
	for(i = 0; i < MESA_TMU_MAX; i++)
	{
		ctx->state.vertex.type.texcoords[i] = MESA_VDT_NONE;
	}

	ctx->state.vertex.texcoords = 0;
	ctx->state.vertex.betas = 0;

	for(i = 0; i < cnt; i++,decl++)
	{
		DWORD next = 0;
		DWORD type = (*decl) >> D3DVSD_TOKENTYPESHIFT;
		switch(type)
		{
			case D3DVSD_TOKEN_STREAM:
			{
				DWORD stream_id = (*decl) ^ (D3DVSD_TOKEN_STREAM << D3DVSD_TOKENTYPESHIFT);
				if(stream_id < MESA_MAX_STREAM)
				{
					/* this is only way how change active stream */
					ctx->state.bind_vertices = stream_id;
				}
				break;
			}
			case D3DVSD_TOKEN_STREAMDATA:
			{
				DWORD data_type = ((*decl) & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT;
				DWORD vertex_type = (*decl) & 0xFFFF;

				switch(vertex_type)
				{
					case D3DVSDE_POSITION:
						D3DVSD2Mesa(data_type, &ctx->state.vertex.type.xyzw, &next);
						ctx->state.vertex.pos.xyzw = offset;
						offset += next;
						break;
					case D3DVSDE_NORMAL:
						D3DVSD2Mesa(data_type, &ctx->state.vertex.type.normal, &next);
						ctx->state.vertex.pos.normal = offset;
						offset += next;
						break;
					case D3DVSDE_DIFFUSE:
						D3DVSD2Mesa(data_type, &ctx->state.vertex.type.diffuse, &next);
						ctx->state.vertex.pos.diffuse = offset;
						offset += next;
						break;
					case D3DVSDE_SPECULAR:
						D3DVSD2Mesa(data_type, &ctx->state.vertex.type.specular, &next);
						ctx->state.vertex.pos.specular = offset;
						offset += next;
						break;
					case D3DVSDE_TEXCOORD0...D3DVSDE_TEXCOORD7:
						p = vertex_type - D3DVSDE_TEXCOORD0;
						D3DVSD2Mesa(data_type, &ctx->state.vertex.type.texcoords[p], &next);
						ctx->state.vertex.pos.texcoords[p] = offset;
						offset += next;

						if(p+1 > ctx->state.vertex.texcoords)
						{
							ctx->state.vertex.texcoords = p+1;
						}
						break;
					default:
						D3DVSD2Mesa(data_type, NULL, &next);
						offset += next;
						break;
				} // switch(vertex_type)
				break;
			}
			case D3DVSD_TOKEN_END:
				i = cnt;
				break;
			default:
				ERR("Shader parse, unknown token 0x%X of type=%d", (*decl), type);
				return FALSE;
		}
	}

	ctx->state.vertex.stride = offset * sizeof(D3DVALUE);
	ctx->state.vertex.shader = TRUE;
	ctx->state.vertex.xyzrhw = FALSE;

	return TRUE;
}
