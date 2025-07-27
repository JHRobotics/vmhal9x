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
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

/* when 1 invert projection matrix, 0 invert viewmodel matrix */
#define DX_INVERT_PROJECTION 1

#define Mgl(_m, _y, _x) _m[4*((_y)-1) + ((_x)-1)]
#define Mdx(_m, _y, _x) _m->_ ## _y  ## _x

/* mostly borrowed from libglu/src/libutilproject.c */
inline static void matmultvecf(const GLfloat matrix[16], const GLfloat in[4], GLfloat out[4])
{
	int i;

	for(i=0; i<4; i++)
	{
		out[i] = 
	    in[0] * matrix[0*4+i] +
	    in[1] * matrix[1*4+i] +
	    in[2] * matrix[2*4+i] +
	    in[3] * matrix[3*4+i];
	}
}

inline static void matmultdotf(const GLfloat matrix[16], GLfloat n, GLfloat out[16])
{
	int i;
	for(i = 0; i < 16; i++)
	{
		out[i] = matrix[i] * n;
	}
}

inline static void matadddotf(const GLfloat matrix[16], GLfloat n, GLfloat out[16])
{
	int i;
	for(i = 0; i < 16; i++)
	{
		out[i] = matrix[i] + n;
	}
}

inline static void mataddf(const GLfloat a[16], const GLfloat b[16], GLfloat out[16])
{
	int i;
	for(i = 0; i < 16; i++)
	{
		out[i] = a[i] + b[i];
	}
}

inline static BOOL matinvf(const GLfloat m[16], GLfloat invOut[16])
{
	GLfloat inv[16], det;
	int i;

	inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
	         + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
	         - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
	         + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
	         - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
	         - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
	         + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
	         - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
	         + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
	         + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
	inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
	         - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
	inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
	         + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
	inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
	         - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
	inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
	         - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
	inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
	         + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
	inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
	         - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
	inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
	         + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

	det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
	if (det == 0)
		return FALSE;

	det = 1.0 / det;

	for(i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;

	return TRUE;
}

inline static void matmultf(const GLfloat a[16], const GLfloat b[16], GLfloat r[16])
{
	int i, j;
	GLfloat tmp[16];
	for(i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp[i*4+j] = 
				a[i*4+0]*b[0*4+j] +
				a[i*4+1]*b[1*4+j] +
				a[i*4+2]*b[2*4+j] +
				a[i*4+3]*b[3*4+j];
		}
	}

	for(i = 0; i < 16; i++)
		r[i] = tmp[i];
}

#if 0
NUKED_LOCAL BOOL MesaUnprojectf(GLfloat winx, GLfloat winy, GLfloat winz, GLfloat clipw,
	const GLfloat modelMatrix[16], 
	const GLfloat projMatrix[16],
	const GLint viewport[4],
	GLfloat *objx, GLfloat *objy, GLfloat *objz, GLfloat *objw)
{
	GLfloat finalMatrix[16];
	GLfloat in[4];
	GLfloat out[4];

	matmultf(modelMatrix, projMatrix, finalMatrix);
	if(!matinvf(finalMatrix, finalMatrix))
		return FALSE;

	in[0]=winx;
	in[1]=winy;
	in[2]=winz;
	in[3]=clipw;

	/* Map x and y from window coordinates */
	in[0] = (in[0] - viewport[0]) / viewport[2];
	in[1] = (in[1] - viewport[1]) / viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	matmultvecf(finalMatrix, in, out);

	if(out[3] == 0.0)
		return FALSE;

	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];

	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
	
	if(objw != NULL)
	{
		*objw = out[3];
	}

	return TRUE;
}
#endif

#ifdef DEBUG
NUKED_LOCAL void MesaMatrixDumpName(GLfloat *m, const char *n, const char *f, int line)
{
	TOPIC("MATRIX", "Matrix[4x4] %s at %s:%d = ", n, f, line);
	TOPIC("MATRIX", "[%f, %f, %f, %f]", m[0*4+0], m[0*4+1], m[0*4+2], m[0*4+3]);
	TOPIC("MATRIX", "[%f, %f, %f, %f]", m[1*4+0], m[1*4+1], m[1*4+2], m[1*4+3]);
	TOPIC("MATRIX", "[%f, %f, %f, %f]", m[2*4+0], m[2*4+1], m[2*4+2], m[2*4+3]);
	TOPIC("MATRIX", "[%f, %f, %f, %f]", m[3*4+0], m[3*4+1], m[3*4+2], m[3*4+3]);
}
#define MesaMatrixDump(_m) MesaMatrixDumpName(_m, #_m, __FILE__, __LINE__)
#endif

NUKED_FAST BOOL MesaIsIdentity(GLfloat matrix[16])
{
	int i = 0;
	for(i = 0; i < 16; i++)
	{
		if(i % 5 == 0)
		{
			if(matrix[i] != 1.0f)
				return FALSE;
		}
		else
		{
			if(matrix[i] != 0.0f)
				return FALSE;
		}
	}
	
	return TRUE;
}

NUKED_LOCAL void MesaIdentity(GLfloat matrix[16])
{
	int i;
	for(i = 0; i < 16; i++)
	{
		matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
	}
}

#ifdef TRACE_ON
void printmtx(const char *name, GLfloat m[16])
{
	TOPIC("MATRIX", "Matrix: %s", name);
	TOPIC("MATRIX", "[%f %f %f %f]",  m[0],  m[1],  m[2],  m[3]);
	TOPIC("MATRIX", "[%f %f %f %f]",  m[4],  m[5],  m[6],  m[7]);
	TOPIC("MATRIX", "[%f %f %f %f]",  m[8],  m[9], m[10], m[11]);
	TOPIC("MATRIX", "[%f %f %f %f]", m[12], m[13], m[14], m[15]);
}
#endif

NUKED_LOCAL void MesaApplyViewport(mesa3d_ctx_t *ctx, GLint x, GLint y, GLint w, GLint h, BOOL stateset)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	GL_CHECK(entry->proc.pglViewport(x, y, w, h));

	//ctx->entry->proc.pglGetIntegerv(GL_VIEWPORT, &ctx->matrix.viewport[0]);
	ctx->matrix.viewport[0] = x;
	ctx->matrix.viewport[1] = y;
	ctx->matrix.viewport[2] = w;
	ctx->matrix.viewport[3] = h;

	TOPIC("SHADER", "Viewport: [%d %d %d %d]", ctx->matrix.viewport[0], ctx->matrix.viewport[1], ctx->matrix.viewport[2], ctx->matrix.viewport[3]);

	ctx->matrix.vpnorm[0] = ctx->matrix.viewport[0];
	ctx->matrix.vpnorm[1] = ctx->matrix.viewport[1];
	ctx->matrix.vpnorm[2] = 2.0f / ctx->matrix.viewport[2];
	ctx->matrix.vpnorm[3] = 2.0f / ctx->matrix.viewport[3];

	if(stateset)
	{
		ctx->state.current.viewport.dwX = x;
		ctx->state.current.viewport.dwY = y;
		ctx->state.current.viewport.dwWidth = w;
		ctx->state.current.viewport.dwHeight = h;
		ctx->state.current.extraset[0] |= 1 << MESA_REC_EXTRA_VIEWPORT;
		TOPIC("STATESET", "save current viewport = %d %d %d %d", x, y, w, h);
	}
}

/**
 * DX to GL transformation discussion
 * https://community.khronos.org/t/converting-directx-transformations-to-opengl/62074/2
 *
 * GL TF = projection * modelview
 *
 * DX TF = projection * view * world0 (* worldN * betaN)
 *
 * Also require flip Y axis (!)
 *
 * I'm multiplying matices this way:
 *   GL_projection = flip_y * DX_projection
 *   GL_modelview  = DX_view * DX_world
 *
 * There was attempt to use GL_modelview for DX_word matrix only, but
 * glFog using GL_modelview to compute distance.
 *
 * Vertex Blending is possible with ARB_VERTEX_BLEND_ARB:
 *   https://registry.khronos.org/OpenGL/extensions/ARB/ARB_vertex_blend.txt
 * but looks unsuported on Mesa VMware driver.
 *
 **/
static const GLfloat initmatrix[16] = 
{
	1.0f,  0.0f,  0.0f,  0.0f,
	0.0f, -1.0f,  0.0f,  0.0f,
	0.0f,  0.0f,  1.0f,  0.0f,
	0.0f,  0.0f,  0.0f,  1.0f
};

NUKED_LOCAL void MesaApplyTransform(mesa3d_ctx_t *ctx, DWORD changes)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	TOPIC("MATRIX", "new matrix, changes=0x%X", changes);

	if(ctx->matrix.identity_mode)
	{
		ctx->matrix.outdated_stack |= changes;
		return;
	}

	if(changes & MESA_TF_PROJECTION)
	{
#if DX_INVERT_PROJECTION
		matmultf(ctx->matrix.proj, initmatrix, ctx->matrix.projfix);
#else
		memcpy(ctx->matrix.projfix, ctx->matrix.proj, sizeof(GLfloat[16]));
#endif
	}

	if(changes & (MESA_TF_PROJECTION))
	{
		entry->proc.pglMatrixMode(GL_PROJECTION);
		entry->proc.pglLoadMatrixf(&ctx->matrix.projfix[0]);
	}

	if(changes & (MESA_TF_WORLD | MESA_TF_VIEW))
	{
		entry->proc.pglMatrixMode(GL_MODELVIEW);
#if DX_INVERT_PROJECTION
		entry->proc.pglLoadMatrixf(&ctx->matrix.view[0]);
#else
		entry->proc.pglLoadMatrixf(&initmatrix[0]);
		entry->proc.pglMultMatrixf(&ctx->matrix.view[0]);
#endif
		if(changes & MESA_TF_VIEW)
		{
			/* when view matrix is changed, we need recalculate lights and clip planes */
			MesaTLRecalcModelview(ctx);
		}

		if(ctx->matrix.weight == 0)
		{
			entry->proc.pglMultMatrixf(&ctx->matrix.world[0][0]);
		}
#if 0
		else
		{
			int i = 0;
			entry->proc.pglEnable(GL_VERTEX_BLEND_ARB);
			entry->exts.pglVertexBlendARB(ctx->matrix.weight);
			
			for(i = 1; i <= ctx->matrix.weight; i++)
			{
				GLenum mtx_type;
				switch(ctx->matrix.weight)
				{
					case 1: mtx_type = MODELVIEW1_ARB; break;
					default: mtx_type = MODELVIEW2_ARB; break;
				}
				entry->proc.pglMatrixMode(mtx_type);
				entry->proc.pgLoadMatrixf(&ctx->matrix.world[i][0]);
			}
		}
		else
		{
			entry->proc.pglDisable(GL_VERTEX_BLEND_ARB);
		}
#endif
	}
}

NUKED_LOCAL void MesaSpaceIdentitySet(mesa3d_ctx_t *ctx)
{
	int tmu;
	mesa3d_entry_t *entry = ctx->entry;
	GL_CHECK(entry->proc.pglMatrixMode(GL_MODELVIEW));
	GL_CHECK(entry->proc.pglPushMatrix());
	GL_CHECK(entry->proc.pglLoadIdentity());
	GL_CHECK(entry->proc.pglMatrixMode(GL_PROJECTION));
	GL_CHECK(entry->proc.pglPushMatrix());
	GL_CHECK(entry->proc.pglLoadIdentity());

	for(tmu = 0; tmu < ctx->tmu_count; tmu++)
	{
		if(!ctx->state.tmu[tmu].matrix_idx)
		{
			GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
			GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
			GL_CHECK(entry->proc.pglLoadIdentity());
		}
	}

	ctx->matrix.identity_mode = TRUE;
}

NUKED_LOCAL void MesaSpaceIdentityReset(mesa3d_ctx_t *ctx)
{
	if(ctx->matrix.identity_mode)
	{
		int tmu;
		mesa3d_entry_t *entry = ctx->entry;
		GL_CHECK(entry->proc.pglMatrixMode(GL_PROJECTION));
		GL_CHECK(entry->proc.pglPopMatrix());
		GL_CHECK(entry->proc.pglMatrixMode(GL_MODELVIEW));
		GL_CHECK(entry->proc.pglPopMatrix());

		for(tmu = 0; tmu < ctx->tmu_count; tmu++)
		{
			//if(!ctx->state.tmu[tmu].matrix_idx)
			{
				//MesaSpaceModelviewSet(ctx);

				GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
				GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
				MesaTMUApplyMatrix(ctx, tmu);
				//GL_CHECK(entry->proc.pglLoadMatrixf(&ctx->state.tmu[tmu].matrix[0]));

				//MesaSpaceModelviewReset(ctx);
			}
		}

		ctx->matrix.identity_mode = FALSE;

		if(ctx->matrix.outdated_stack)
		{
			MesaApplyTransform(ctx, ctx->matrix.outdated_stack);
			ctx->matrix.outdated_stack = 0;
		}
	}
}

NUKED_LOCAL void MesaSpaceModelviewSet(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	GL_CHECK(entry->proc.pglMatrixMode(GL_MODELVIEW));
	GL_CHECK(entry->proc.pglPushMatrix());
	GL_CHECK(entry->proc.pglLoadMatrixf(&ctx->matrix.view[0]));
}

NUKED_LOCAL void MesaSpaceModelviewReset(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	GL_CHECK(entry->proc.pglMatrixMode(GL_MODELVIEW));
	GL_CHECK(entry->proc.pglPopMatrix());
}

NUKED_LOCAL void MesaVetexBlend(mesa3d_ctx_t *ctx, GLfloat coords[3], GLfloat *betas, int betas_cnt, GLfloat out[4])
{
	int i;
	GLfloat in4[4] = {coords[0], coords[1], coords[2], 1.0f};
	GLfloat a_betas[MESA_WORLDS_MAX] = {1.0f};
	GLfloat w[16];
	GLfloat m[16] = {
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	for(i = 0; i < ctx->matrix.weight; i++)
	{
		GLfloat b1 = 0.0;
		if(i < betas_cnt)
		{
			b1 = 1.0 - betas[i];
		}
		
		a_betas[0] -= b1;
		a_betas[1+i] = b1;
	}
	
	for(i = 0; i <= ctx->matrix.weight; i++)
	{
		matmultdotf(ctx->matrix.world[i], a_betas[i], w);
		mataddf(m, w, m);
	}
	
	matmultvecf(m, in4, out);
}

NUKED_FAST void MesaSetTextureMatrix(mesa3d_ctx_t *ctx, int tmu, GLfloat matrix[16])
{
	memcpy(ctx->state.tmu[tmu].matrix, matrix, sizeof(GLfloat[16]));
	ctx->state.tmu[tmu].matrix_idx = MesaIsIdentity(matrix);
}

NUKED_FAST void MesaTMUApplyMatrix(mesa3d_ctx_t *ctx, int tmu)
{
	mesa3d_entry_t *entry = ctx->entry;
	struct mesa3d_tmustate *ts = &ctx->state.tmu[tmu];
	if(ts->coordscalc_used >= 2)
	{
		GLfloat m[16];
		memcpy(m, ts->matrix, sizeof(m));
		if(ts->projected)
		{
#if 0
			switch(ts->coordscalc_used)
			{
				case 2:
					Mgl(m, 1, 4) = Mgl(m, 1, 2);
					Mgl(m, 2, 4) = Mgl(m, 2, 2);
					Mgl(m, 3, 4) = Mgl(m, 3, 2);
					Mgl(m, 4, 4) = Mgl(m, 4, 2);
					Mgl(m, 1, 2) = Mgl(m, 2, 2) = Mgl(m, 3, 2) = Mgl(m, 4, 2) = 0.0f;
					break;
				case 3:
					Mgl(m, 1, 4) = Mgl(m, 1, 3);
					Mgl(m, 2, 4) = Mgl(m, 2, 3);
					Mgl(m, 3, 4) = Mgl(m, 3, 3);
					Mgl(m, 4, 4) = Mgl(m, 4, 3);
					Mgl(m, 1, 3) = Mgl(m, 2, 3) = Mgl(m, 3, 3) = Mgl(m, 4, 3) = 0.0f;
					break;
			}
#endif
			GL_CHECK(entry->proc.pglLoadMatrixf(&m[0]));
		}
		else
		{
#if 0
			switch(ts->coordscalc_used)
			{
				case 2:
					//mat._13 = mat._23 = mat._33 = mat._43 = 0.0f;
					Mgl(m, 1, 3) = 0.0;
					Mgl(m, 2, 3) = 0.0;
					Mgl(m, 3, 3) = 0.0;
					Mgl(m, 4, 3) = 0.0;
					/* TRU */
				default:
					//mat._14 = mat._24 = mat._34 = 0.0f; mat._44 = 1.0f;
					Mgl(m, 1, 4) = 0;
					Mgl(m, 2, 4) = 0;
					Mgl(m, 3, 4) = 0;
					Mgl(m, 4, 4) = 1.0f;
					break;
			}
#endif
			GL_CHECK(entry->proc.pglLoadMatrixf(&m[0]));
		}

		TOPIC("MATRIX", "ts->projected=%d ts->coordscalc=%d ts->mapping=0x%X", ts->projected, ts->coordscalc, ts->mapping);

#ifdef DEBUG
		MesaMatrixDump(ts->matrix);
		MesaMatrixDump(m);
#endif
	}
	else
	{
		GL_CHECK(entry->proc.pglLoadIdentity());
	}
}

#define COPY_MATRIX(_srcdx, _dst) do{ \
	Mgl(_dst, 1, 1) = Mdx(_srcdx, 1, 1); Mgl(_dst, 1, 2) = Mdx(_srcdx, 1, 2); Mgl(_dst, 1, 3) = Mdx(_srcdx, 1, 3); Mgl(_dst, 1, 4) = Mdx(_srcdx, 1, 4); \
	Mgl(_dst, 2, 1) = Mdx(_srcdx, 2, 1); Mgl(_dst, 2, 2) = Mdx(_srcdx, 2, 2); Mgl(_dst, 2, 3) = Mdx(_srcdx, 2, 3); Mgl(_dst, 2, 4) = Mdx(_srcdx, 2, 4); \
	Mgl(_dst, 3, 1) = Mdx(_srcdx, 3, 1); Mgl(_dst, 3, 2) = Mdx(_srcdx, 3, 2); Mgl(_dst, 3, 3) = Mdx(_srcdx, 3, 3); Mgl(_dst, 3, 4) = Mdx(_srcdx, 3, 4); \
	Mgl(_dst, 4, 1) = Mdx(_srcdx, 4, 1); Mgl(_dst, 4, 2) = Mdx(_srcdx, 4, 2); Mgl(_dst, 4, 3) = Mdx(_srcdx, 4, 3); Mgl(_dst, 4, 4) = Mdx(_srcdx, 4, 4); \
	}while(0)

NUKED_LOCAL void MesaSetTransform(mesa3d_ctx_t *ctx, DWORD xtype, D3DMATRIX *matrix)
{
	TOPIC("MATRIX", "MesaSetTransform(ctx, 0x%X, %p)", xtype, matrix);

	/**
		This is undocumented and comes from reference driver.
		Original note:
		  BUGBUG is there a define for 0x80000000?
	**/
	BOOL set_identity = (xtype & 0x80000000) == 0 ? FALSE : TRUE;
	DWORD state = xtype & 0x7FFFFFFF;
	DWORD state_mtx = 0;
	GLfloat m[16];
	if(!set_identity && matrix != NULL)
	{
		COPY_MATRIX(matrix, m);
	}
	else
	{
		MesaIdentity(m);
	}
	
	switch(state)
	{
		case D3DTRANSFORMSTATE_WORLD:
		case D3DTS_WORLD:
			memcpy(ctx->matrix.world[0], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			state_mtx = D3DTRANSFORMSTATE_WORLD;
			break;
		case D3DTRANSFORMSTATE_VIEW:
			if(memcmp(ctx->matrix.view, m, sizeof(m)) != 0)
			{
				memcpy(ctx->matrix.view, m, sizeof(m));
				MesaApplyTransform(ctx, MESA_TF_VIEW);
			}
			state_mtx = D3DTRANSFORMSTATE_VIEW;
			break;
		case D3DTRANSFORMSTATE_PROJECTION:
			if(memcmp(ctx->matrix.proj, m, sizeof(m)) != 0)
			{
				memcpy(ctx->matrix.proj, m, sizeof(m));
				MesaApplyTransform(ctx, MESA_TF_PROJECTION);
			}
			state_mtx = D3DTRANSFORMSTATE_PROJECTION;
			break;
		case D3DTRANSFORMSTATE_WORLD1:
		case D3DTS_WORLD1:
			memcpy(ctx->matrix.world[1], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			state_mtx = D3DTRANSFORMSTATE_WORLD1;
			break;
		case D3DTRANSFORMSTATE_WORLD2:
		case D3DTS_WORLD2:
			memcpy(ctx->matrix.world[2], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_WORLD3:
		case D3DTS_WORLD3:
			memcpy(ctx->matrix.world[3], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			state_mtx = D3DTRANSFORMSTATE_WORLD2;
			break;
#if 1 /* disabled for now */
		case D3DTRANSFORMSTATE_TEXTURE0...D3DTRANSFORMSTATE_TEXTURE7:
		{
			DWORD tmu = state-D3DTRANSFORMSTATE_TEXTURE0;
			if(memcmp(ctx->state.tmu[tmu].matrix, m, sizeof(m)) != 0)
			{
				MesaSetTextureMatrix(ctx, tmu, m);
				ctx->state.tmu[tmu].update = TRUE;
				MesaDrawRefreshState(ctx);
			}
			state_mtx = state;
			break;
		}
#else
		case D3DTRANSFORMSTATE_TEXTURE0...D3DTRANSFORMSTATE_TEXTURE7:
			break;
#endif
		default:
			/* MSDN: https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtransformstatetype		
				The transform states in the range 256 through 511 are reserved to
				store up to 256 world matrices that can be indexed using the
				D3DTS_WORLDMATRIX and D3DTS_WORLD macros.
			*/
			if(state >= 256 && state <= 511)
			{
#if 0
				DWORD w_index = state-256;
				//memcpy(ctx->matrix.stored[w_index], m, sizeof(m));
				
				TRACE("MesaSetTransform: saved matrix %d", w_index);
				// TODO: are these state usefull in this driver? Or we can safely ignore them and save memory?
				if(w_index == 0)
				{
					//memcpy(ctx->matrix.world[0], m, sizeof(m));
					//MesaApplyTransform(ctx, MESA_TF_WORLD);
				}
#endif
			}
			else
			{
				WARN("MesaSetTransform: invalid state=%d, xtype=%X", state, xtype);
			}
			break;
	} // switch(state)
	
	if(state_mtx > 0 && state_mtx <= MESA_REC_MAX_MATICES)
	{
		memcpy(&ctx->state.current.matices[state_mtx], matrix, sizeof(D3DMATRIX));
		ctx->state.current.maticesset[0] |= 1 << state_mtx;
	}
}

NUKED_LOCAL void MesaMultTransform(mesa3d_ctx_t *ctx, DWORD xtype, D3DMATRIX *matrix)
{
	BOOL set_identity = (xtype & 0x80000000) == 0 ? FALSE : TRUE;
	DWORD state = xtype & 0x7FFFFFFF;
	GLfloat m[16];
	if(!set_identity && matrix != NULL)
	{
		COPY_MATRIX(matrix, m);
	}
	else
	{
		MesaIdentity(m);
	}
	TOPIC("MATRIX", "MesaMultTransform %d", xtype);

	switch(state)
	{
		case D3DTRANSFORMSTATE_WORLD:
		case D3DTS_WORLD:
			matmultf(ctx->matrix.world[0], m, ctx->matrix.world[0]);
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_VIEW:
			matmultf(ctx->matrix.view, m, ctx->matrix.view);
			MesaApplyTransform(ctx, MESA_TF_VIEW);
			break;
		case D3DTRANSFORMSTATE_PROJECTION:
			matmultf(ctx->matrix.proj, m, ctx->matrix.proj);
			break;
		case D3DTRANSFORMSTATE_WORLD1:
		case D3DTS_WORLD1:
			matmultf(ctx->matrix.world[1], m, ctx->matrix.world[1]);
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_WORLD2:
		case D3DTS_WORLD2:
			matmultf(ctx->matrix.world[2], m, ctx->matrix.world[2]);
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_WORLD3:
		case D3DTS_WORLD3:
			matmultf(ctx->matrix.world[3], m, ctx->matrix.world[3]);
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
#if 1 /* disabled for now */
		case D3DTRANSFORMSTATE_TEXTURE0...D3DTRANSFORMSTATE_TEXTURE7:
		{
			DWORD tmu = state-D3DTRANSFORMSTATE_TEXTURE0;
			matmultf(ctx->state.tmu[tmu].matrix, m, m);

			MesaSetTextureMatrix(ctx, tmu, m);

			ctx->state.tmu[tmu].update = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		}
#else
		case D3DTRANSFORMSTATE_TEXTURE0...D3DTRANSFORMSTATE_TEXTURE7:
			break;
#endif
		default:
			// ...
			break;
	}
}
