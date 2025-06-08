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

NUKED_LOCAL void MesaFVFSet(mesa3d_ctx_t *ctx, DWORD fvf_code)
{
	if(ctx->state.vertex.code == fvf_code)
	{
		/* not need to recalculate */
		return;
	}

	ctx->state.vertex.code = fvf_code;
	MesaFVFRecalc(ctx);

	TOPIC("SHADER", "MesaFVFSet type=0x%X, realsize=%d",
		fvf_code, ctx->state.vertex.stride
	);
}

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

		if(fvf_code & D3DFVF_NORMAL)
		{
			ctx->state.vertex.pos.normal = offset;
			ctx->state.vertex.type.normal = MESA_VDT_FLOAT3;
			offset += 3;
		}
		else
		{
			ctx->state.vertex.type.normal = MESA_VDT_NONE;
		}

		if(fvf_code & D3DFVF_RESERVED1)
		{
			offset++;
		}

		if(fvf_code & D3DFVF_DIFFUSE)
		{
			ctx->state.vertex.pos.diffuse = offset++;
			ctx->state.vertex.type.diffuse = MESA_VDT_D3DCOLOR;
		}
		else
		{
			ctx->state.vertex.type.diffuse = MESA_VDT_NONE;
		}

		if(fvf_code & D3DFVF_SPECULAR)
		{
			ctx->state.vertex.pos.specular = offset++;
			ctx->state.vertex.type.specular = MESA_VDT_D3DCOLOR;
		}
		else
		{
			ctx->state.vertex.type.specular = MESA_VDT_NONE;
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
			}
			else
			{
				ctx->state.vertex.type.texcoords[i] = MESA_VDT_NONE;
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
		MesaVSDump(vs);
		MesaVSSetVertex(ctx, vs);
	}

	MesaFVFRecalcCoords(ctx);
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
#define DEF_SPECULAR 0x00000000

NUKED_INLINE void MesaDrawFVF_internal(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, FVF_t *vertex)
{
	int i;
	DWORD *dw = vertex->dw;
	GLfloat *fv = vertex->fv;

	for(i = 0; i < ctx->tmu_count; i++)
	{
		//if(ctx->state.tmu[i].image)
		if(ctx->state.tmu[i].active)
		{
			int coordindex               = ctx->state.tmu[i].coordindex;
			mesa_vertex_data_t coordtype = ctx->state.vertex.type.texcoords[coordindex];
			int coordpos                 = ctx->state.vertex.pos.texcoords[coordindex];

			if(ctx->state.tmu[i].projected)
			{
				GLfloat s, t, r, w;
				switch(coordtype)
				{
					case MESA_VDT_FLOAT2: 
						s = fv[coordpos+0];
						w = fv[coordpos+1];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord1f(GL_TEXTURE0 + i, s/w);
						}
						break;
					case MESA_VDT_FLOAT3:
						s = fv[coordpos+0];
						t = fv[coordpos+1];
						w = fv[coordpos+2];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord2f(GL_TEXTURE0 + i, s/w, t/w);
						}
						break;
					case MESA_VDT_FLOAT4:
						s = fv[coordpos+0];
						t = fv[coordpos+1];
						r = fv[coordpos+2];
						w = fv[coordpos+3];
						if(w != 0.0)
						{
							entry->proc.pglMultiTexCoord3f(GL_TEXTURE0 + i, s/w, t/w, r/w);
						}
						break;
					default:
						break;
/*
					case MESA_VDT_NONE:
					case MESA_VDT_FLOAT1:
					default:
						entry->proc.pglMultiTexCoord2fv(GL_TEXTURE0 + i, def_texcoords);
						break;
*/
				}
			}
			else
			{
				switch(coordtype)
				{
					case MESA_VDT_FLOAT1:
						entry->proc.pglMultiTexCoord1fv(GL_TEXTURE0 + i, &fv[coordpos]);
						break;
					case MESA_VDT_FLOAT2:
						entry->proc.pglMultiTexCoord2fv(GL_TEXTURE0 + i, &fv[coordpos]);
						break;
					case MESA_VDT_FLOAT3:
						entry->proc.pglMultiTexCoord3fv(GL_TEXTURE0 + i, &fv[coordpos]);
						break;
					case MESA_VDT_FLOAT4:
						entry->proc.pglMultiTexCoord4fv(GL_TEXTURE0 + i, &fv[coordpos]);
						break;
/*					case MESA_VDT_NONE:
					default:
						entry->proc.pglMultiTexCoord2fv(GL_TEXTURE0 + i, def_texcoords);
						break;*/
					default:
						break;
				}
			}
		} // active
	}

	if(ctx->state.vertex.type.diffuse == MESA_VDT_D3DCOLOR)
	{
		LoadColor1(entry, ctx, dw[ctx->state.vertex.pos.diffuse]);
	}
/*
	else
	{
		if(ctx->dxif >= 8)
		{
			LoadColor1(entry, ctx, DEF_DIFFUSE);
		}
	}*/

	if(ctx->state.vertex.type.specular == MESA_VDT_D3DCOLOR)
	{
		LoadColor2(entry, ctx, dw[ctx->state.vertex.pos.specular]);
	}
/*
	else
	{
		// FIXME: optimize for load black
		//LoadColor2(entry, ctx, DEF_SPECULAR);
	}*/

	if(ctx->state.vertex.xyzrhw)
	{
		GLfloat v[4];
		SV_UNPROJECT(v, fv[FVF_X], fv[FVF_Y], fv[FVF_Z], fv[FVF_RHW]);
		entry->proc.pglVertex4fv(&v[0]);
	}
	else
	{
		if(ctx->matrix.weight == 0)
		{
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					entry->proc.pglNormal3fv(&fv[ctx->state.vertex.pos.normal]);
					break;
				case MESA_VDT_FLOAT1:
				{
					GLfloat n1 = fv[ctx->state.vertex.pos.normal];
					entry->proc.pglNormal3f(n1, n1, 1);
					break;
				}
				case MESA_VDT_FLOAT2:
				{
					GLfloat n1 = fv[ctx->state.vertex.pos.normal+0];
					GLfloat n2 = fv[ctx->state.vertex.pos.normal+1];
					entry->proc.pglNormal3f(n1, n2, 1);
					break;
				}
				default:
					//entry->proc.pglNormal3f(0, 0, 1);
					break;
			}

			entry->proc.pglVertex3fv(&fv[ctx->state.vertex.pos.xyzw]);
		}
		else
		{
			GLfloat coords[4];
			GLfloat normals[4];
			switch(ctx->state.vertex.type.normal)
			{
				case MESA_VDT_FLOAT3:
				case MESA_VDT_FLOAT4:
					memcpy(normals, &fv[ctx->state.vertex.pos.normal], sizeof(GLfloat)*3);
					normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT1:
					normals[0] = fv[ctx->state.vertex.pos.normal];
					normals[1] = normals[0];
					normals[2] = normals[3] = 1.0f;
					break;
				case MESA_VDT_FLOAT2:
					normals[0] = fv[ctx->state.vertex.pos.normal + 0];
					normals[1] = fv[ctx->state.vertex.pos.normal + 1];
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
				&fv[ctx->state.vertex.pos.xyzw + 3], 
				ctx->state.vertex.betas, coords);
			entry->proc.pglNormal3fv(&coords[0]);

			MesaVetexBlend(ctx,
				&fv[ctx->state.vertex.pos.xyzw], 
				&fv[ctx->state.vertex.pos.xyzw + 3], 
				ctx->state.vertex.betas, coords);

			entry->proc.pglVertex3fv(&coords[0]);
		}
	}
}

NUKED_LOCAL void MesaDrawFVFIndex(mesa3d_ctx_t *ctx, void *vertices, int index)
{
	FVF_t *fvf = (FVF_t *)(((BYTE *)vertices) + (index * ctx->state.vertex.stride));
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
	DWORD stride = ctx->state.vertex.stride;
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

NUKED_FAST void MesaDrawFVFdefaults(mesa3d_ctx_t *ctx)
{
	int i;
	mesa3d_entry_t *entry = ctx->entry;

	for(i = 0; i < ctx->tmu_count; i++)
	{
		int coordindex = ctx->state.tmu[i].coordindex;

		switch(ctx->state.vertex.type.texcoords[coordindex])
		{
			case MESA_VDT_FLOAT2:
			case MESA_VDT_FLOAT3:
			case MESA_VDT_FLOAT4:
				break;
			case MESA_VDT_FLOAT1:
			default:
				GL_CHECK(entry->proc.pglMultiTexCoord4f(GL_TEXTURE0 + i, MESA_DEF_TEXCOORDS));
				break;
		}
	}

	if(ctx->state.vertex.type.diffuse != MESA_VDT_D3DCOLOR)
	{
		LoadColor1(entry, ctx, DEF_DIFFUSE);
	}

	if(ctx->state.vertex.type.specular != MESA_VDT_D3DCOLOR)
	{
		LoadColor2(entry, ctx, DEF_SPECULAR);
	}

	switch(ctx->state.vertex.type.normal)
	{
		case MESA_VDT_FLOAT1:
		case MESA_VDT_FLOAT2:
		case MESA_VDT_FLOAT3:
		case MESA_VDT_FLOAT4:
			break;
		default:
			GL_CHECK(entry->proc.pglNormal3f(0, 0, 1));
			break;
	}
}
