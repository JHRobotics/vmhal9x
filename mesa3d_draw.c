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

void MesaDrawTLVertex(mesa3d_ctx_t *ctx, LPD3DTLVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->state.tex)
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
	
	if(ctx->state.tex)
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
	
	if(ctx->state.tex)
	{
		entry->proc.pglTexCoord2f(CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
	}
					
	entry->proc.pglVertex3f(vertex->x, vertex->y, vertex->z);
}
