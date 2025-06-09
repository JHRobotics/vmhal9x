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

NUKED_INLINE void LoadColor1(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD color, BOOL localonly)
{
	GLfloat cv[4];
	MESA_D3DCOLOR_TO_FV(color, cv);
	entry->proc.pglColor4fv(&cv[0]);

	if(!localonly && ctx->state.material.lighting && ctx->state.material.color_vertex)
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

NUKED_INLINE void LoadColor2(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD color)
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

	LoadColor1(entry, ctx, vertex->color, FALSE);
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

	LoadColor1(entry, ctx, vertex->color, FALSE);
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

NUKED_LOCAL void MesaFVFSet(mesa3d_ctx_t *ctx, DWORD fvf_code)
{
	if(ctx->state.vertex.code == fvf_code)
	{
		if(!ctx->state.fvf_shader_dirty)
		{
			/* not need to recalculate */
			return;
		}
	}

	ctx->state.vertex.code = fvf_code;
	MesaFVFRecalc(ctx);
	ctx->state.fvf_shader_dirty = FALSE;

	TOPIC("SHADER", "MesaFVFSet type=0x%X, realsize=%d",
		fvf_code, ctx->state.vertex.stride
	);
}

#define VSTREAM0_POS(_n, _pos, _t) \
	if(ctx->vstream[0].mem.ptr != NULL){ \
		ctx->state.vertex.ptr._n = &ctx->vstream[0].mem._t[_pos]; \
		ctx->state.vertex.ptr._n ## _stride32 = ctx->vstream[0].stride/4; \
	}else{ \
		ctx->state.vertex.ptr._n = NULL; \
		ctx->state.vertex.ptr._n ## _stride32 = 0; \
	}

#define VSTREAM0_NULL(_n) \
	ctx->state.vertex.ptr._n = NULL; \
	ctx->state.vertex.ptr._n ## _stride32 = 0;

NUKED_LOCAL void MesaFVFRecalc(mesa3d_ctx_t *ctx)
{
	int offset = 0; // in DW
	int i;
	int num_coords;

	DWORD fvf_code = ctx->state.vertex.code;

	if(RDVSD_ISLEGACY(fvf_code) || ctx->dxif < MESA_CTX_IF_DX8)
	{
		ctx->state.vertex.shader = FALSE;

		if(fvf_code & D3DFVF_RESERVED0)
		{
			offset++;
			ctx->state.vertex.pos.xyzw = 1;
		}
		else
		{
			ctx->state.vertex.pos.xyzw = 0;
		}

		switch(fvf_code & D3DFVF_POSITION_MASK)
		{
			case D3DFVF_XYZ:
				offset += 3;
				ctx->state.vertex.betas = 0;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
			case D3DFVF_XYZRHW:
				offset += 4;
				ctx->state.vertex.betas = 0;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT4;
				ctx->state.vertex.xyzrhw = TRUE;
				break;
			case D3DFVF_XYZB1:
				offset += 3 + 1;
				ctx->state.vertex.betas = 1;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
			case D3DFVF_XYZB2:
				offset += 3 + 2;
				ctx->state.vertex.betas = 2;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
			case D3DFVF_XYZB3:
				offset += 3 + 3;
				ctx->state.vertex.betas = 3;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
			case D3DFVF_XYZB4:
				offset += 3 + 4;
				ctx->state.vertex.betas = 4;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
			case D3DFVF_XYZB5:
				offset += 3 + 5;
				ctx->state.vertex.betas = 5;
				ctx->state.vertex.type.xyzw = MESA_VDT_FLOAT3;
				ctx->state.vertex.xyzrhw = FALSE;
				break;
		}

		VSTREAM0_POS(xyzw, ctx->state.vertex.pos.xyzw, fv);

		if(fvf_code & D3DFVF_NORMAL)
		{
			ctx->state.vertex.pos.normal = offset;
			ctx->state.vertex.type.normal = MESA_VDT_FLOAT3;
			VSTREAM0_POS(normal, offset, fv);
			offset += 3;
		}
		else
		{
			ctx->state.vertex.type.normal = MESA_VDT_NONE;
			VSTREAM0_NULL(normal);
		}

		if(fvf_code & D3DFVF_RESERVED1)
		{
			offset++;
		}

		if(fvf_code & D3DFVF_DIFFUSE)
		{
			ctx->state.vertex.pos.diffuse = offset++;
			ctx->state.vertex.type.diffuse = MESA_VDT_D3DCOLOR;
			VSTREAM0_POS(diffuse, ctx->state.vertex.pos.diffuse, dw);
		}
		else
		{
			ctx->state.vertex.type.diffuse = MESA_VDT_NONE;
			VSTREAM0_NULL(diffuse);
		}

		if(fvf_code & D3DFVF_SPECULAR)
		{
			ctx->state.vertex.pos.specular = offset++;
			ctx->state.vertex.type.specular = MESA_VDT_D3DCOLOR;
			VSTREAM0_POS(specular, ctx->state.vertex.pos.specular, dw);
		}
		else
		{
			ctx->state.vertex.type.specular = MESA_VDT_NONE;
			VSTREAM0_NULL(specular);
		}

		num_coords = (fvf_code & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT; /* 0 = no coord, 1 = one tex coord */
		for(i = 0; i < MESA_TMU_MAX; i++)
		{
			if(i < num_coords)
			{
				ctx->state.vertex.pos.texcoords[i] = offset;
				DWORD coords_format = (fvf_code >> ((2*i) + 16)) & 3;
				switch(coords_format)
				{
					case D3DFVF_TEXTUREFORMAT1:
						ctx->state.vertex.type.texcoords[i] = MESA_VDT_FLOAT1;
						offset += 1;
						break;
					case D3DFVF_TEXTUREFORMAT2:
						ctx->state.vertex.type.texcoords[i] = MESA_VDT_FLOAT2;
						offset += 2;
						break;
					case D3DFVF_TEXTUREFORMAT3:
						ctx->state.vertex.type.texcoords[i] = MESA_VDT_FLOAT3;
						offset += 3;
						break;
					case D3DFVF_TEXTUREFORMAT4:
						ctx->state.vertex.type.texcoords[i] = MESA_VDT_FLOAT4;
						offset += 4;
						break;
				}

				if(ctx->vstream[0].mem.ptr != NULL)
				{
					ctx->state.vertex.ptr.texcoords[i] = &ctx->vstream[0].mem.fv[ctx->state.vertex.pos.texcoords[i]];
					ctx->state.vertex.ptr.texcoords_stride32[i] = ctx->vstream[0].stride/4;
				}
				else
				{
					ctx->state.vertex.ptr.texcoords[i] = NULL;
					ctx->state.vertex.ptr.texcoords_stride32[i] = 0;
				}
			}
			else
			{
				ctx->state.vertex.type.texcoords[i] = MESA_VDT_NONE;
				ctx->state.vertex.ptr.texcoords[i] = NULL;
				ctx->state.vertex.ptr.texcoords_stride32[i] = 0;
			}
		}

		ctx->state.vertex.stride = offset * sizeof(D3DVALUE);
	}
	else /* load from shader */
	{
		mesa_dx_shader_t *vs = MesaVSGet(ctx, fvf_code);
		if(vs == NULL)
		{
			ERR("VS not exist (handle=0x%X)", fvf_code);
			return;
		}

		if(vs->code_size != 0)
		{
			ERR("VS contain code (handle=0x%X)", fvf_code);
			return;
		}

		TOPIC("SHADER", "shader handle = 0x%X", fvf_code);
#ifdef DEBUG
		MesaVSDump(vs);
#endif
		MesaVSSetVertex(ctx, vs);
	}

	MesaFVFRecalcCoords(ctx);
	
	ctx->state.vertex.fast_draw = FALSE;
}

NUKED_LOCAL void MesaFVFRecalcCoords(mesa3d_ctx_t *ctx)
{
	BOOL refresh = FALSE;
	int i;
	
	for(i = 0; i < ctx->tmu_count; i++)
	{
		//if(ctx->state.tmu[i].coordindex < ctx->state.vertex.texcoords)
		if(ctx->state.vertex.type.texcoords[ctx->state.tmu[i].coordindex] != MESA_VDT_NONE)
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
}

#define DEF_DIFFUSE  0xFFFFFFFF
#define DEF_DIFFUSE_DX8  0xFF000000

#define DEF_SPECULAR 0x00000000

NUKED_FAST void MesaVertexReadStream(mesa3d_ctx_t *ctx, mesa3d_vertex_t *v, int index)
{
	int i;

	for(i = 0; i < ctx->tmu_count; i++)
	{
		int coordindex               = ctx->state.tmu[i].coordindex;
		mesa_vertex_data_t coordtype = ctx->state.vertex.type.texcoords[coordindex];
		GLfloat *fv                  = ctx->state.vertex.ptr.texcoords[coordindex] + ctx->state.vertex.ptr.texcoords_stride32[coordindex]*index;

		if(ctx->state.tmu[i].projected)
		{
			GLfloat s, t, r, w;
			switch(coordtype)
			{
				case MESA_VDT_FLOAT2:
					s = fv[0];
					w = fv[1];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
					}
					else
					{
						v->texcoords[i][0] = 0;
					}
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT3:
					s = fv[0];
					t = fv[1];
					w = fv[2];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
					}
					else
					{
						v->texcoords[i][0] = 0;
						v->texcoords[i][1] = 0;
					}
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT4:
					s = fv[0];
					t = fv[1];
					r = fv[2];
					w = fv[3];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
						v->texcoords[i][2] = r/w;
					}
					else
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
						v->texcoords[i][2] = r/w;
					}
					v->texcoords[i][3] = 1;
					break;
				default:
					v->texcoords[i][0] = 0;
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
			}
		}
		else
		{
			switch(coordtype)
			{
				case MESA_VDT_FLOAT1:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT2:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT3:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = fv[2];
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT4:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = fv[2];
					v->texcoords[i][3] = fv[3];
					break;
				default:
					v->texcoords[i][0] = 0;
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
			}
		}
	} // for tmu

	if(ctx->state.vertex.type.diffuse == MESA_VDT_D3DCOLOR)
	{
		DWORD *dw = ctx->state.vertex.ptr.diffuse + ctx->state.vertex.ptr.diffuse_stride32*index;
		v->diffuse = *dw;
	}
	else
	{
		if(ctx->dxif >= MESA_CTX_IF_DX8)
		{
			v->diffuse = DEF_DIFFUSE_DX8;
		}
		else
		{
			v->diffuse = DEF_DIFFUSE;
		}
	}

	if(ctx->state.vertex.type.specular == MESA_VDT_D3DCOLOR)
	{
		DWORD *dw = ctx->state.vertex.ptr.specular + ctx->state.vertex.ptr.specular_stride32*index;
		v->specular = *dw;
	}
	else
	{
		v->specular = DEF_SPECULAR;
	}

	if(ctx->state.vertex.xyzrhw)
	{
		GLfloat *fv = ctx->state.vertex.ptr.xyzw + ctx->state.vertex.ptr.xyzw_stride32*index;
		SV_UNPROJECT(v->xyzw, fv[0], fv[1], fv[2], fv[3]);

		v->normal[0] = 0.0f;
		v->normal[1] = 0.0f;
		v->normal[2] = 1.0f;
	}
	else
	{
		if(ctx->matrix.weight == 0)
		{
			GLfloat *fv = ctx->state.vertex.ptr.normal + ctx->state.vertex.ptr.normal_stride32*index;
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					v->normal[0] = fv[0];
					v->normal[1] = fv[1];
					v->normal[2] = fv[2];
					break;
				case MESA_VDT_FLOAT1:
					v->normal[0] = fv[0];
					v->normal[1] = fv[0];
					v->normal[2] = 1.0f;
					break;
				case MESA_VDT_FLOAT2:
					v->normal[0] = fv[0];
					v->normal[1] = fv[1];
					v->normal[2] = 1.0f;
					break;
				default:
					v->normal[0] = 0.0f;
					v->normal[1] = 0.0f;
					v->normal[2] = 1.0f;
					break;
			}

			fv = ctx->state.vertex.ptr.xyzw + ctx->state.vertex.ptr.xyzw_stride32*index;
			switch(ctx->state.vertex.type.xyzw)
			{
				case MESA_VDT_FLOAT3:
					v->xyzw[0] = fv[0];
					v->xyzw[1] = fv[1];
					v->xyzw[2] = fv[2];
					v->xyzw[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT4:
					v->xyzw[0] = fv[0];
					v->xyzw[1] = fv[1];
					v->xyzw[2] = fv[2];
					v->xyzw[3] = fv[3];
					break;
				default:
					v->xyzw[0] = 0.0f;
					v->xyzw[1] = 0.0f;
					v->xyzw[2] = 0.0f;
					v->xyzw[3] = 1.0f;
					break;
			}
		}
		else
		{
			GLfloat *fv = ctx->state.vertex.ptr.normal + ctx->state.vertex.ptr.normal_stride32*index;

			GLfloat coords[4];
			GLfloat normals[4];
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					normals[0] = fv[0];
					normals[1] = fv[1];
					normals[2] = fv[2];
					normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT1:
					normals[0] = fv[0];
					normals[1] = fv[0];
					normals[2] = normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT2:
					normals[0] = fv[0];
					normals[1] = fv[1];
					normals[2] = normals[3] = 1.0f;
					break;
				default:
					normals[0] = 0.0f;
					normals[1] = 0.0f;
					normals[2] = 1.0f;
					normals[3] = 1.0f;
					break;
			}

			MesaVetexBlend(ctx,
				normals,
				fv+3,
				ctx->state.vertex.betas, coords);
			v->normal[0] = coords[0];
			v->normal[1] = coords[1];
			v->normal[2] = coords[2];

			fv = ctx->state.vertex.ptr.normal + ctx->state.vertex.ptr.normal_stride32*index;
			MesaVetexBlend(ctx,
				fv,
				fv+3,
				ctx->state.vertex.betas, coords);
			v->xyzw[0] = coords[0];
			v->xyzw[1] = coords[1];
			v->xyzw[2] = coords[2];
			v->xyzw[3] = 1.0f;
		}
	}
}

NUKED_FAST void MesaVertexReadBuffer(mesa3d_ctx_t *ctx, mesa3d_vertex_t *v, BYTE *buf, int index, DWORD stride8)
{
	int i;

	for(i = 0; i < ctx->tmu_count; i++)
	{
		int coordindex               = ctx->state.tmu[i].coordindex;
		mesa_vertex_data_t coordtype = ctx->state.vertex.type.texcoords[coordindex];
		GLfloat *fv                  = ((GLfloat*)(buf + stride8*index)) + ctx->state.vertex.pos.texcoords[coordindex];

		if(ctx->state.tmu[i].projected)
		{
			GLfloat s, t, r, w;
			switch(coordtype)
			{
				case MESA_VDT_FLOAT2:
					s = fv[0];
					w = fv[1];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
					}
					else
					{
						v->texcoords[i][0] = 0;
					}
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT3:
					s = fv[0];
					t = fv[1];
					w = fv[2];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
					}
					else
					{
						v->texcoords[i][0] = 0;
						v->texcoords[i][1] = 0;
					}
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT4:
					s = fv[0];
					t = fv[1];
					r = fv[2];
					w = fv[3];
					if(w != 0.0)
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
						v->texcoords[i][2] = r/w;
					}
					else
					{
						v->texcoords[i][0] = s/w;
						v->texcoords[i][1] = t/w;
						v->texcoords[i][2] = r/w;
					}
					v->texcoords[i][3] = 1;
					break;
				default:
					v->texcoords[i][0] = 0;
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
			}
		}
		else
		{
			switch(coordtype)
			{
				case MESA_VDT_FLOAT1:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT2:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT3:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = fv[2];
					v->texcoords[i][3] = 1;
					break;
				case MESA_VDT_FLOAT4:
					v->texcoords[i][0] = fv[0];
					v->texcoords[i][1] = fv[1];
					v->texcoords[i][2] = fv[2];
					v->texcoords[i][3] = fv[3];
					break;
				default:
					v->texcoords[i][0] = 0;
					v->texcoords[i][1] = 0;
					v->texcoords[i][2] = 0;
					v->texcoords[i][3] = 1;
					break;
			}
		}
	} // for tmu

	if(ctx->state.vertex.type.diffuse == MESA_VDT_D3DCOLOR)
	{
		DWORD *dw = ((DWORD*)(buf + index*stride8))+ctx->state.vertex.pos.diffuse;
		v->diffuse = *dw;
	}
	else
	{
		if(ctx->dxif >= MESA_CTX_IF_DX8)
		{
			v->diffuse = DEF_DIFFUSE_DX8;
		}
	}

	if(ctx->state.vertex.type.specular == MESA_VDT_D3DCOLOR)
	{
		DWORD *dw = ((DWORD*)(buf + index*stride8))+ctx->state.vertex.pos.specular;
		v->specular = *dw;
	}
	else
	{
		v->specular = DEF_SPECULAR;
	}

	if(ctx->state.vertex.xyzrhw)
	{
		GLfloat *fv = ((GLfloat*)(buf + index*stride8))+ctx->state.vertex.pos.xyzw;
		SV_UNPROJECT(v->xyzw, fv[0], fv[1], fv[2], fv[3]);

		v->normal[0] = 0.0f;
		v->normal[1] = 0.0f;
		v->normal[2] = 1.0f;
	}
	else
	{
		if(ctx->matrix.weight == 0)
		{
			GLfloat *fv = ((GLfloat*)(buf + index*stride8))+ctx->state.vertex.pos.normal;
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					v->normal[0] = fv[0];
					v->normal[1] = fv[1];
					v->normal[2] = fv[2];
					break;
				case MESA_VDT_FLOAT1:
					v->normal[0] = fv[0];
					v->normal[1] = fv[0];
					v->normal[2] = 1.0f;
					break;
				case MESA_VDT_FLOAT2:
					v->normal[0] = fv[0];
					v->normal[1] = fv[1];
					v->normal[2] = 1.0f;
					break;
				default:
					v->normal[0] = 0.0f;
					v->normal[1] = 0.0f;
					v->normal[2] = 1.0f;
					break;
			}

			fv = ((GLfloat*)(buf + index*stride8))+ctx->state.vertex.pos.xyzw;
			switch(ctx->state.vertex.type.xyzw)
			{
				case MESA_VDT_FLOAT3:
					v->xyzw[0] = fv[0];
					v->xyzw[1] = fv[1];
					v->xyzw[2] = fv[2];
					v->xyzw[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT4:
					v->xyzw[0] = fv[0];
					v->xyzw[1] = fv[1];
					v->xyzw[2] = fv[2];
					v->xyzw[3] = fv[3];
					break;
				default:
					v->xyzw[0] = 0.0f;
					v->xyzw[1] = 0.0f;
					v->xyzw[2] = 0.0f;
					v->xyzw[3] = 1.0f;
					break;
			}
		}
		else
		{
			GLfloat *fv = ((GLfloat*)(buf + index*stride8))+ctx->state.vertex.pos.normal;

			GLfloat coords[4];
			GLfloat normals[4];
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					normals[0] = fv[0];
					normals[1] = fv[1];
					normals[2] = fv[2];
					normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT1:
					normals[0] = fv[0];
					normals[1] = fv[0];
					normals[2] = normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT2:
					normals[0] = fv[0];
					normals[1] = fv[1];
					normals[2] = normals[3] = 1.0f;
					break;
				default:
					normals[0] = 0.0f;
					normals[1] = 0.0f;
					normals[2] = 1.0f;
					normals[3] = 1.0f;
					break;
			}

			MesaVetexBlend(ctx,
				normals,
				fv+3,
				ctx->state.vertex.betas, coords);
			v->normal[0] = coords[0];
			v->normal[1] = coords[1];
			v->normal[2] = coords[2];

			fv = ((GLfloat*)(buf + index*stride8))+ctx->state.vertex.pos.xyzw;
			MesaVetexBlend(ctx,
				fv,
				fv+3,
				ctx->state.vertex.betas, coords);
			v->xyzw[0] = coords[0];
			v->xyzw[1] = coords[1];
			v->xyzw[2] = coords[2];
			v->xyzw[3] = 1.0f;
		}
	}
}

NUKED_FAST void MesaVertexDraw(mesa3d_ctx_t *ctx, mesa3d_vertex_t *v)
{
	int i;
	mesa3d_entry_t *entry = ctx->entry;
	BOOL fast_draw = ctx->state.vertex.fast_draw;

	for(i = 0; i < ctx->tmu_count; i++)
	{
		if(ctx->state.tmu[i].image)
		{
			entry->proc.pglMultiTexCoord4fv(GL_TEXTURE0+i, &v->texcoords[i][0]);
		}
	}
	
	if(ctx->state.vertex.type.normal != MESA_VDT_NONE || fast_draw == FALSE)
	{
		entry->proc.pglNormal3fv(&v->normal[0]);
	}
	
	if(ctx->state.vertex.type.diffuse != MESA_VDT_NONE || fast_draw == FALSE)
	{
		LoadColor1(entry, ctx, v->diffuse, ctx->state.vertex.type.diffuse == MESA_VDT_D3DCOLOR ? FALSE : TRUE);
	}
	
	if(ctx->state.vertex.type.specular == MESA_VDT_D3DCOLOR)
	{
		LoadColor2(entry, ctx, v->specular);
	}
	
	if(ctx->state.vertex.type.xyzw == MESA_VDT_FLOAT3 && fast_draw == TRUE)
	{
		entry->proc.pglVertex3fv(&v->xyzw[0]);
	}
	else
	{
		entry->proc.pglVertex4fv(&v->xyzw[0]);
	}
	
	ctx->state.vertex.fast_draw = TRUE;
}

#define VETREX_DRAW_SWITCH \
	switch(gltype){ \
		case GL_TRIANGLES: \
			entry->proc.pglBegin(GL_TRIANGLES); \
			for(i = 0; i < cnt-2; i += 3){ \
				VERTEX_GET(start+i+2); \
				MesaVertexDraw(ctx, &v); \
				VERTEX_GET(start+i+1); \
				MesaVertexDraw(ctx, &v); \
				VERTEX_GET(start+i); \
				MesaVertexDraw(ctx, &v); \
			} \
			GL_CHECK(entry->proc.pglEnd()); \
			break; \
		case GL_TRIANGLE_FAN: \
			MesaReverseCull(ctx); \
			entry->proc.pglBegin(GL_TRIANGLE_FAN); \
			for(i = 0; i < cnt; i++){ \
				VERTEX_GET(start+i); \
				MesaVertexDraw(ctx, &v); \
			} \
			GL_CHECK(entry->proc.pglEnd()); \
			MesaSetCull(ctx); \
			break; \
		case GL_TRIANGLE_STRIP: \
			if(cnt >= 3){ \
				MesaReverseCull(ctx); \
				entry->proc.pglBegin(GL_TRIANGLE_STRIP); \
				for(i = 0; i < cnt; i++){ \
					VERTEX_GET(start+i); \
					MesaVertexDraw(ctx, &v); \
				} \
				GL_CHECK(entry->proc.pglEnd()); \
				MesaSetCull(ctx); \
			} \
			break; \
		default: \
			entry->proc.pglBegin(gltype); \
			for(i = cnt-1; i >= 0; i--){ \
				VERTEX_GET(start+i); \
				MesaVertexDraw(ctx, &v); \
			} \
			GL_CHECK(entry->proc.pglEnd()); \
			break; \
		} // switch

#define VERTEX_GET(_n) MesaVertexReadStream(ctx, &v, _n)

NUKED_LOCAL void MesaVertexDrawStream(mesa3d_ctx_t *ctx, GLenum gltype, DWORD start, DWORD cnt)
{	
	TOPIC("GL", "glBegin(%d)", gltype);
	mesa3d_entry_t *entry = ctx->entry;
	mesa3d_vertex_t v;
	int i;

	VETREX_DRAW_SWITCH
}

#undef VERTEX_GET

#define INDEX_GET(_p) (index_stride8 == 4 ? dindex[_p]+base : ((DWORD)windex[_p])+base)
#define VERTEX_GET(_n) MesaVertexReadStream(ctx, &v, INDEX_GET(_n))

NUKED_LOCAL void MesaVertexDrawStreamIndex(mesa3d_ctx_t *ctx, GLenum gltype, DWORD start, int base, DWORD cnt, void *index, DWORD index_stride8)
{
	TOPIC("GL", "glBegin(%d)", gltype);
	mesa3d_entry_t *entry = ctx->entry;
	mesa3d_vertex_t v;
	int i;

	WORD  *windex =  (WORD*)index;
	DWORD *dindex = (DWORD*)index;

	VETREX_DRAW_SWITCH
}

#undef VERTEX_GET
#undef INDEX_GET

#define VERTEX_GET(_n) MesaVertexReadBuffer(ctx, &v, ptr, _n, stride)

NUKED_LOCAL void MesaVertexDrawBlock(mesa3d_ctx_t *ctx, GLenum gltype, BYTE *ptr, DWORD start, DWORD cnt, DWORD stride)
{
	TOPIC("GL", "glBegin(%d)", gltype);
	mesa3d_entry_t *entry = ctx->entry;
	mesa3d_vertex_t v;
	int i;

	VETREX_DRAW_SWITCH
}

#undef VERTEX_GET

#define INDEX_GET(_p) (index_stride8 == 4 ? dindex[_p] : ((DWORD)windex[_p]))
#define VERTEX_GET(_n) MesaVertexReadBuffer(ctx, &v, ptr, INDEX_GET(_n), stride)

NUKED_LOCAL void MesaVertexDrawBlockIndex(mesa3d_ctx_t *ctx, GLenum gltype, BYTE *ptr, DWORD start, DWORD cnt, DWORD stride, void *index, DWORD index_stride8)
{
	TOPIC("GL", "glBegin(%d)", gltype);
	mesa3d_entry_t *entry = ctx->entry;
	mesa3d_vertex_t v;
	int i;

	WORD  *windex =  (WORD*)index;
	DWORD *dindex = (DWORD*)index;

	VETREX_DRAW_SWITCH
}

#undef VERTEX_GET
#undef INDEX_GET

#undef VETREX_DRAW_SWITCH
