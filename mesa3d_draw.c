/******************************************************************************
 * Copyright (c) 2024 Jaroslav Hensl                                          *
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
#define MESA3D_DRAW_C

#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"

#define FVF_X 0
#define FVF_Y 1
#define FVF_Z 2
#define FVF_RHW 3

#define FVF_MAX 32

typedef struct _FVF
{
	union
	{
		D3DVALUE fv[32];
		DWORD    dw[32];
	};
} FVF_t;

void MesaDrawTLVertex(mesa3d_ctx_t *ctx, LPD3DTLVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->state.tex[0].image)
	{
		entry->proc.pglTexCoord2f(CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
		TOPIC("TEX", "glTexCoord2f(%f, %f)", vertex->tu, vertex->tv);
	}

	entry->proc.pglColor4ub(
		RGBA_GETRED(vertex->color),
		RGBA_GETGREEN(vertex->color),
		RGBA_GETBLUE(vertex->color),
		RGBA_GETALPHA(vertex->color)
	);

	if(ctx->state.specular)
	{
		entry->proc.pglSecondaryColor3ubEXT(
			RGBA_GETRED(vertex->specular),
			RGBA_GETGREEN(vertex->specular),
			RGBA_GETBLUE(vertex->specular)
		);
	}

	GLfloat x, y, z, w = 2.0;
	MesaUnproject(ctx, vertex->sx, vertex->sy, vertex->sz, &x, &y, &z);
	// w = (1.0/vertex->rhw) - 0.5; = currently best
	if(vertex->rhw != 0)
	{
		w = 2.0/vertex->rhw;
	}
	
	entry->proc.pglVertex4f(x*w, y*w, z*w, w);
	TOPIC("TEX", "glVertex4f(%f, %f, %f, %f)", x, y, z, w);	
}

void MesaDrawLVertex(mesa3d_ctx_t *ctx, LPD3DLVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->state.tex[0].image)
	{
		entry->proc.pglTexCoord2f(CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
	}

	entry->proc.pglColor4ub(
		RGBA_GETRED(vertex->color),
		RGBA_GETGREEN(vertex->color),
		RGBA_GETBLUE(vertex->color),
		RGBA_GETALPHA(vertex->color)
	);
	
	if(ctx->state.specular)
	{
		entry->proc.pglSecondaryColor3ubEXT(
			RGBA_GETRED(vertex->specular),
			RGBA_GETGREEN(vertex->specular),
			RGBA_GETBLUE(vertex->specular)
		);
	}
	
	entry->proc.pglVertex3f(vertex->x, vertex->y, vertex->z);
}

void MesaDrawVertex(mesa3d_ctx_t *ctx, LPD3DVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->state.tex[0].image)
	{
		entry->proc.pglTexCoord2f(CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
	}
					
	entry->proc.pglVertex3f(vertex->x, vertex->y, vertex->z);
}

void MesaFVFSet(mesa3d_ctx_t *ctx, DWORD type)
{
	ctx->state.fvf.type = type;
	
	int offset = 0; // in DW
	int i;
	
	switch(type & D3DFVF_POSITION_MASK)
	{
		case D3DFVF_XYZ:
			offset = 3;
			break;
		case D3DFVF_XYZRHW:
			offset = 4;
			break;
		case D3DFVF_XYZB1:
			offset = 3 + 1;
			break;
		case D3DFVF_XYZB2:
			offset = 3 + 2;
			break;
		case D3DFVF_XYZB3:
			offset = 3 + 3;
			break;
		case D3DFVF_XYZB4:
			offset = 3 + 4;
			break;
		case D3DFVF_XYZB5:
			offset = 3 + 5;
			break;
	}
	
	if(type & D3DFVF_NORMAL)
	{
		ctx->state.fvf.pos_normal = offset++;
	}
	else
	{
		ctx->state.fvf.pos_normal = 0;
	}
	
	if(type & D3DFVF_RESERVED1)
	{
		offset++;
	}
	
	if(type & D3DFVF_DIFFUSE)
	{
		ctx->state.fvf.pos_diffuse = offset++;
	}
	else
	{
		ctx->state.fvf.pos_diffuse = 0;
	}
	
	if(type & D3DFVF_SPECULAR)
	{
		ctx->state.fvf.pos_specular = offset++;
	}
	else
	{
		ctx->state.fvf.pos_specular = 0;
	}
	
	int tc = (type & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for(i = 0; i < MESA_ACTIVE_TEX_TOTAL; i++)
	{
		if(tc > i)
		{
			ctx->state.fvf.pos_tex[i] = offset;
			offset += 2;
		}
		else
		{
			ctx->state.fvf.pos_tex[i] = 0;
		}
	}
	
	ctx->state.fvf.stride = offset * sizeof(D3DVALUE);
}

inline static void MesaDrawFVF_internal(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, FVF_t *vertex)
{
	int i;
	
	for(i = 0; i < MESA_ACTIVE_TEX_TOTAL; i++)
	{
		if(ctx->state.fvf.pos_tex[i])
		{
			entry->proc.pglMultiTexCoord2f(GL_TEXTURE0 + i,
				CONV_U_TO_S(vertex->fv[ctx->state.fvf.pos_tex[i] + 0]),
				CONV_V_TO_T(vertex->fv[ctx->state.fvf.pos_tex[i] + 1])
			);
		}
	}

	if(ctx->state.fvf.pos_diffuse)
	{
		DWORD color = vertex->dw[ctx->state.fvf.pos_diffuse];
		entry->proc.pglColor4ub(
			RGBA_GETRED(color),
			RGBA_GETGREEN(color),
			RGBA_GETBLUE(color),
			RGBA_GETALPHA(color)
		);
	}

	if(ctx->state.specular && ctx->state.fvf.pos_specular)
	{
		DWORD color = vertex->dw[ctx->state.fvf.pos_specular];
		entry->proc.pglSecondaryColor3ubEXT(
			RGBA_GETRED(color),
			RGBA_GETGREEN(color),
			RGBA_GETBLUE(color)
		);
	}

	if(ctx->state.fvf.type & D3DFVF_XYZRHW)
	{
		GLfloat x, y, z, w = 2.0;
		MesaUnproject(ctx, vertex->fv[FVF_X], vertex->fv[FVF_Y], vertex->fv[FVF_Z], &x, &y, &z);
		if(vertex->fv[FVF_RHW] != 0)
		{
			w = 2.0/vertex->fv[FVF_RHW];
		}
	
		entry->proc.pglVertex4f(x*w, y*w, z*w, w);
	}
	else
	{
		entry->proc.pglVertex4f(vertex->fv[FVF_X], vertex->fv[FVF_Y], vertex->fv[FVF_Z], 1.0);
	}
}

void MesaDrawFVF(mesa3d_ctx_t *ctx, void *vertex)
{
	MesaDrawFVF_internal(ctx->entry, ctx, vertex);
}

void MesaDrawFVFIndex(mesa3d_ctx_t *ctx, void *vertices, int index)
{
	FVF_t *fvf = (FVF_t *)(((BYTE *)vertices) + (index * ctx->state.fvf.stride));
	MesaDrawFVF_internal(ctx->entry, ctx, fvf);
}

void MesaDrawFVFs(mesa3d_ctx_t *ctx, GLenum gl_ptype, void *vertices, DWORD start, DWORD cnt)
{
	mesa3d_entry_t *entry = ctx->entry;
	BYTE *vb = ((BYTE *)vertices) + (start * ctx->state.fvf.stride);
	
	if(gl_ptype != -1)
	{
		TOPIC("GL", "glBegin(%d)", gl_ptype);
		entry->proc.pglBegin(gl_ptype);
		while(cnt--)
		{
			MesaDrawFVF_internal(entry, ctx, (FVF_t *)vb);
			vb += ctx->state.fvf.stride;
		}
		GL_CHECK(entry->proc.pglEnd());
	}
}
