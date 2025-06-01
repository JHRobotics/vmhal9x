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
#ifndef NUKED_SKIP
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "ddrawi_ddk.h"
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

#define SV_UNPROJECT(_v, _x, _y, _z, _rhw) \
	_v[0] = (((((_x) - ctx->matrix.viewport[0]) / ctx->matrix.viewport[2]) * 2.0f) - 1.0f)/(_rhw); \
	_v[1] = (((((_y) - ctx->matrix.viewport[1]) / ctx->matrix.viewport[3]) * 2.0f) - 1.0f)/(_rhw); \
	_v[2] = ((_z)*ctx->matrix.zscale[10])/(_rhw); \
	_v[3] = 1.0f/(_rhw);

#define SV_UNPROJECT_NTP(_v, _x, _y, _z) \
	_v[3] = 1.0f; \
	_v[0] = ((((_x) - ctx->matrix.vpnorm[0]) * ctx->matrix.vpnorm[2]) - 1.0f); \
	_v[1] = ((((_y) - ctx->matrix.vpnorm[1]) * ctx->matrix.vpnorm[3]) - 1.0f); \
	_v[2] = (_z)

#define SV_UNPROJECTD(_v, _x, _y, _z, _w) \
	_v[3] = 2.0/((double)_w); \
	_v[0] = (((((double)_x) - ctx->matrix.vpnorm[0]) * ctx->matrix.vpnorm[2]) - 1.0)*_v[3]; \
	_v[1] = (((((double)_y) - ctx->matrix.vpnorm[1]) * ctx->matrix.vpnorm[3]) - 1.0)*_v[3]; \
	_v[2] = ((double)_z)*_v[3]

#define FVF_X 0
#define FVF_Y 1
#define FVF_Z 2
#define FVF_BETA1 3
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

static void LoadColor1(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD color)
{
	GLfloat cv[4];
	MESA_D3DCOLOR_TO_FV(color, cv);
	entry->proc.pglColor4fv(&cv[0]);

	if(ctx->state.material.lighting && ctx->state.material.color_vertex)
	{
		if((ctx->state.material.untracked & (MESA_MAT_DIFFUSE_C1 | MESA_MAT_AMBIENT_C1)) == 
			(MESA_MAT_DIFFUSE_C1 | MESA_MAT_AMBIENT_C1))
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &cv[0]);
		}
		else if(ctx->state.material.untracked & MESA_MAT_DIFFUSE_C1)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &cv[0]);
		}
		else if(ctx->state.material.untracked & MESA_MAT_AMBIENT_C1)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &cv[0]);
		}

		if(ctx->state.material.untracked & MESA_MAT_EMISSIVE_C1)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &cv[0]);
		}

		if(ctx->state.material.untracked & MESA_MAT_SPECULAR_C1)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &cv[0]);
		}
	}
}

static void LoadColor2(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD color)
{
	GLfloat cv[4];
	MESA_D3DCOLOR_TO_FV(color, cv);

	if(ctx->state.material.lighting && ctx->state.material.color_vertex)
	{
		if((ctx->state.material.untracked & (MESA_MAT_DIFFUSE_C2 | MESA_MAT_AMBIENT_C2)) ==
			(MESA_MAT_DIFFUSE_C2 | MESA_MAT_AMBIENT_C2))
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &cv[0]);
		}
		else if(ctx->state.material.untracked & MESA_MAT_DIFFUSE_C2)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &cv[0]);
		}
		else if(ctx->state.material.untracked & MESA_MAT_AMBIENT_C2)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &cv[0]);
		}

		if(ctx->state.material.untracked & MESA_MAT_EMISSIVE_C2)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &cv[0]);
		}

		if(ctx->state.specular_vertex)
		{
			entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &cv[0]);
			//entry->proc.pglSecondaryColor3fv(&cv[0]);
		}
	}
}

NUKED_LOCAL void MesaDrawTLVertex(mesa3d_ctx_t *ctx, LPD3DTLVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->state.tmu[0].image)
	{
		entry->proc.pglMultiTexCoord2f(GL_TEXTURE0, CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
		TOPIC("TEX", "glTexCoord2f(%f, %f)", vertex->tu, vertex->tv);
	}

	LoadColor1(entry, ctx, vertex->color);
	LoadColor2(entry, ctx, vertex->specular);

	GLfloat v[4];
	SV_UNPROJECT(v, vertex->sx, vertex->sy, vertex->sz, vertex->rhw);
	entry->proc.pglVertex4fv(&v[0]);
}

NUKED_LOCAL void MesaDrawLVertex(mesa3d_ctx_t *ctx, LPD3DLVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;

	if(ctx->state.tmu[0].image)
	{
		entry->proc.pglMultiTexCoord2f(GL_TEXTURE0, CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
	}

	LoadColor1(entry, ctx, vertex->color);
	LoadColor2(entry, ctx, vertex->specular);

	entry->proc.pglVertex3f(vertex->x, vertex->y, vertex->z);
}

NUKED_LOCAL void MesaDrawVertex(mesa3d_ctx_t *ctx, LPD3DVERTEX vertex)
{
	mesa3d_entry_t *entry = ctx->entry;

	if(ctx->state.tmu[0].image)
	{
		entry->proc.pglMultiTexCoord2f(GL_TEXTURE0, CONV_U_TO_S(vertex->tu), CONV_V_TO_T(vertex->tv));
	}

	entry->proc.pglVertex3f(vertex->x, vertex->y, vertex->z);
}

NUKED_LOCAL void MesaFVFSet(mesa3d_ctx_t *ctx, DWORD type/*, DWORD size*/)
{
	if(ctx->state.fvf.type == type)
	{
		/* not need to recalculate */
		return;
	}

	ctx->state.fvf.type = type;
	MesaFVFRecalc(ctx);
/*
	if(size != ctx->state.fvf.stride)
	{
		WARN("WRONG fvf calculation real size=%d, calculated=%d", size, ctx->state.fvf.stride);
	}
*/
	TOPIC("MATRIX", "MesaFVFSet type=0x%X, realsize=%d",
		type, ctx->state.fvf.stride
	);
}

NUKED_LOCAL void MesaFVFRecalc(mesa3d_ctx_t *ctx)
{
	int offset = 0; // in DW
	int i;
	BOOL refresh = FALSE;
	
	DWORD type = ctx->state.fvf.type;
	
	switch(type & D3DFVF_POSITION_MASK)
	{
		case D3DFVF_XYZ:
			offset = 3; // sizeof(D3DVECTOR) == 12
			ctx->state.fvf.betas = 0;
			break;
		case D3DFVF_XYZRHW:
			offset = 4;
			ctx->state.fvf.betas = 0;
			break;
		case D3DFVF_XYZB1:
			offset = 3 + 1;
			ctx->state.fvf.betas = 1;
			break;
		case D3DFVF_XYZB2:
			offset = 3 + 2;
			ctx->state.fvf.betas = 2;
			break;
		case D3DFVF_XYZB3:
			offset = 3 + 3;
			ctx->state.fvf.betas = 3;
			break;
		case D3DFVF_XYZB4:
			offset = 3 + 4;
			ctx->state.fvf.betas = 4;
			break;
		case D3DFVF_XYZB5:
			offset = 3 + 5;
			ctx->state.fvf.betas = 5;
			break;
	}
	
	if(type & D3DFVF_RESERVED0)
	{
		ctx->state.fvf.begin = 1;
		//offset++;
	}
	else
	{
		ctx->state.fvf.begin = 0;
	}
	
	if(type & D3DFVF_NORMAL)
	{
		ctx->state.fvf.pos_normal = offset;
		offset += 3;
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
	
	int tc = (type & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT; /* 0 = no coord, 1 = one tex coord */
	for(i = 0; i < MESA_TMU_MAX; i++)
	{
		if(i < tc)
		{
			ctx->state.fvf.pos_coords[i] = offset;
			DWORD coords_format = (type >> ((2*i) + 16)) & 3;
			switch(coords_format)
			{
				case D3DFVF_TEXTUREFORMAT1:
					ctx->state.fvf.num_coords[i] = 1;
					break;
				case D3DFVF_TEXTUREFORMAT2:
					ctx->state.fvf.num_coords[i] = 2;
					break;
				case D3DFVF_TEXTUREFORMAT3:
					ctx->state.fvf.num_coords[i] = 3;
					break;
				case D3DFVF_TEXTUREFORMAT4:
					ctx->state.fvf.num_coords[i] = 4;
					break;
			}
			offset += ctx->state.fvf.num_coords[i];
		}
		else
		{
			ctx->state.fvf.num_coords[i] = 0;
			ctx->state.fvf.pos_coords[i] = 0;
		}
	}

	for(i = 0; i < ctx->tmu_count; i++)
	{
		if(ctx->state.tmu[i].coordindex < tc)
		{
			if(ctx->state.tmu[i].nocoords)
			{
				ctx->state.tmu[i].nocoords = FALSE;
				ctx->state.tmu[i].update = TRUE;
				refresh = TRUE;
			}
		}
		else
		{
			if(!ctx->state.tmu[i].nocoords)
			{
				ctx->state.tmu[i].nocoords = TRUE;
				ctx->state.tmu[i].update = TRUE;
				refresh = TRUE;
			}
		}
	} // for
	
	if(refresh)
	{
		MesaDrawRefreshState(ctx);
	}

	ctx->state.fvf.stride = offset * sizeof(D3DVALUE);
}

#define DEF_DIFFUSE 0xFF000000
#define DEF_SPECULAR 0xFFFFFFFF

static GLfloat def_texcoords[4] = {0.0f, 0.0f, 0.0f, 0.0f};

NUKED_INLINE void MesaDrawFVF_internal(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, FVF_t *vertex)
{
	int i;
	DWORD *dw = &vertex->dw[ctx->state.fvf.begin];
	GLfloat *fv = &vertex->fv[ctx->state.fvf.begin];

	for(i = 0; i < ctx->tmu_count; i++)
	{
		//if(ctx->state.tmu[i].image)
		if(ctx->state.tmu[i].active)
		{
			int coordindex = ctx->state.tmu[i].coordindex;
			int coordnum = ctx->state.fvf.num_coords[coordindex];
			int coordpos = ctx->state.fvf.pos_coords[coordindex];
			GLfloat *coords = def_texcoords;

			if(coordpos > 0 && coordnum > 0)
			{
				coords = &fv[coordpos];
			}
			else if(coordnum == 0)
			{
				coordnum = 2;
			}

			if(ctx->state.tmu[i].projected)
			{
				GLfloat s, t, r, w;
				switch(coordnum)
				{
					case 2:
						s = coords[0];
						w = coords[1];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord1f(GL_TEXTURE0 + i, s/w);
						}
						break;
					case 3:
						s = coords[0];
						t = coords[1];
						w = coords[2];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord2f(GL_TEXTURE0 + i, s/w, t/w);
						}
						break;
					case 4:
						s = coords[0];
						t = coords[1];
						r = coords[2];
						w = coords[3];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord3f(GL_TEXTURE0 + i, s/w, t/w, r/w);
						}
						break;
				}
			}
			else
			{
				switch(coordnum)
				{
					case 1:
						entry->proc.pglMultiTexCoord1fv(GL_TEXTURE0 + i, coords);
						break;
					case 2:
						entry->proc.pglMultiTexCoord2fv(GL_TEXTURE0 + i, coords);
						break;
					case 3:
						entry->proc.pglMultiTexCoord3fv(GL_TEXTURE0 + i, coords);
						break;
					case 4:
						entry->proc.pglMultiTexCoord4fv(GL_TEXTURE0 + i, coords);
						break;
				} // switch(coordnum)
			}
		} // image
	}

	if(ctx->state.fvf.pos_diffuse)
	{
		LoadColor1(entry, ctx, dw[ctx->state.fvf.pos_diffuse]);
	}
	else
	{
		if(ctx->dxif >= 8)
		{
			LoadColor1(entry, ctx, DEF_DIFFUSE);
		}
	}

	if(ctx->state.fvf.pos_specular)
	{
		LoadColor2(entry, ctx, dw[ctx->state.fvf.pos_specular]);
	}
	else
	{
		// FIXME: optimize for load black
		//LoadColor2(entry, ctx, DEF_SPECULAR);
	}

	if((ctx->state.fvf.type & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
	{
		GLfloat v[4];
		SV_UNPROJECT(v, fv[FVF_X], fv[FVF_Y], fv[FVF_Z], fv[FVF_RHW]);
		entry->proc.pglVertex4fv(&v[0]);
	}
	else
	{
		if(ctx->matrix.weight == 0)
		{
			if(ctx->state.fvf.pos_normal)
			{
				entry->proc.pglNormal3fv(&fv[ctx->state.fvf.pos_normal]);
			}

			entry->proc.pglVertex3fv(&fv[FVF_X]);
		}
		else
		{
			GLfloat coords[4];
			if(ctx->state.fvf.pos_normal)
			{
				MesaVetexBlend(ctx,
					&fv[ctx->state.fvf.pos_normal],
					&fv[FVF_BETA1], 
					ctx->state.fvf.betas, coords);
				entry->proc.pglNormal3fv(&coords[0]);
			}
			MesaVetexBlend(ctx,
				&fv[FVF_X],
				&fv[FVF_BETA1], 
				ctx->state.fvf.betas, coords);
			entry->proc.pglVertex3fv(&coords[0]);
		}
	}
}

NUKED_LOCAL void MesaDrawFVFIndex(mesa3d_ctx_t *ctx, void *vertices, int index)
{
	FVF_t *fvf = (FVF_t *)(((BYTE *)vertices) + (index * ctx->state.fvf.stride));
	MesaDrawFVF_internal(ctx->entry, ctx, fvf);
}

NUKED_LOCAL void MesaDrawFVFBlock(mesa3d_ctx_t *ctx, GLenum gl_ptype, void *vertices, DWORD offset, DWORD cnt, DWORD stride)
{
	mesa3d_entry_t *entry = ctx->entry;
	//DWORD stride = ctx->state.fvf.stride;
	//BYTE *vb = ((BYTE *)vertices) + ((start + cnt - 1) * stride);
	BYTE *vb = ((BYTE *)vertices) + offset;
	
	TOPIC("GL", "glBegin(%d)", gl_ptype);
	switch(gl_ptype)
	{
		case GL_TRIANGLES:
		{
			entry->proc.pglBegin(GL_TRIANGLES);
			while(cnt >= 3)
			{
				MesaDrawFVF_internal(entry, ctx, (FVF_t *)(vb + stride*2));
				MesaDrawFVF_internal(entry, ctx, (FVF_t *)(vb + stride*1));
				MesaDrawFVF_internal(entry, ctx, (FVF_t *)(vb));
				vb += stride*3;
				cnt -= 3;
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
		case GL_TRIANGLE_FAN:
		{
			vb = ((BYTE *)vertices) + offset + ((cnt - 1) * stride);
			entry->proc.pglBegin(GL_TRIANGLE_FAN);
			BYTE *first = ((BYTE *)vertices) + offset;
			MesaDrawFVF_internal(entry, ctx, (FVF_t *)first);
			while(cnt > 1)
			{
				MesaDrawFVF_internal(entry, ctx, (FVF_t *)vb);
				vb -= stride;
				cnt--;
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
		case GL_TRIANGLE_STRIP:
		{
			if(cnt >= 3)
			{
				MesaReverseCull(ctx);

				entry->proc.pglBegin(GL_TRIANGLE_STRIP);
				while(cnt > 0)
				{
					MesaDrawFVF_internal(entry, ctx, (FVF_t *)vb);
					vb += stride;
					cnt--;
				}
				GL_CHECK(entry->proc.pglEnd());

				MesaSetCull(ctx);
			}
			break;
		}
		default:
		{
			entry->proc.pglBegin(gl_ptype);
			while(cnt > 0)
			{
				MesaDrawFVF_internal(entry, ctx, (FVF_t *)vb);
				vb += stride;
				cnt--;
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
	}
}

NUKED_LOCAL void MesaDrawFVFs(mesa3d_ctx_t *ctx, GLenum gl_ptype, void *vertices, DWORD start, DWORD cnt)
{
	DWORD stride = ctx->state.fvf.stride;
	MesaDrawFVFBlock(ctx, gl_ptype, vertices, start*stride, cnt, stride);
}

#define D8INDEX(_n) (index_stride==2 ? ((WORD*)(index))[_n] : ((DWORD*)(index))[_n])
#define FVFINDEX(_n) (FVF_t*)(((BYTE*)vertices) + stride*D8INDEX(_n))

NUKED_LOCAL void MesaDrawFVFBlockIndex(
	mesa3d_ctx_t *ctx, GLenum gl_ptype,
	void *vertices,  DWORD stride,
	void *index, DWORD index_stride,
	DWORD start, DWORD cnt)
{
	mesa3d_entry_t *entry = ctx->entry;

	TRACE("MesaDrawFVFBlockIndex(vertives=0x%X, stride=%d index=0x%X, index_stride=%d)", 
		vertices, stride, index, index_stride);

	TOPIC("GL", "glBegin(%d)", gl_ptype);

	switch(gl_ptype)
	{
		case GL_TRIANGLES:
		{
			DWORD i;
			entry->proc.pglBegin(GL_TRIANGLES);
			for(i = 0; i < cnt; i += 3)
			{
				MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+i+2));
				MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+i+1));
				MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+i+0));
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
		case GL_TRIANGLE_FAN:
		{
			entry->proc.pglBegin(GL_TRIANGLE_FAN);
			MesaDrawFVF_internal(entry, ctx, FVFINDEX(start));
			while(cnt > 1)
			{
				MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+cnt-1));
				cnt--;
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
		case GL_TRIANGLE_STRIP:
		{
			if(cnt >= 3)
			{
				DWORD i;
				MesaReverseCull(ctx);
				entry->proc.pglBegin(GL_TRIANGLE_STRIP);
				for(i = 0; i < cnt; i++)
				{
					MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+i));
				}
				GL_CHECK(entry->proc.pglEnd());

				MesaSetCull(ctx);
			}
			break;
		}
		default:
		{
			DWORD i;
			entry->proc.pglBegin(gl_ptype);
			for(i = 0; i < cnt; i++)
			{
				MesaDrawFVF_internal(entry, ctx, FVFINDEX(start+i));
			}
			GL_CHECK(entry->proc.pglEnd());
			break;
		}
	}
}
