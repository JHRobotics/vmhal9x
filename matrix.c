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
	for(i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			r[i*4+j] = 
				a[i*4+0]*b[0*4+j] +
				a[i*4+1]*b[1*4+j] +
				a[i*4+2]*b[2*4+j] +
				a[i*4+3]*b[3*4+j];
		}
	}
}

#if 0
static BOOL unprojectf(GLfloat winx, GLfloat winy, GLfloat winz, GLfloat clipw,
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

static BOOL matisidx(GLfloat matrix[16])
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

void MesaUnprojectCalc(mesa3d_ctx_t *ctx)
{
	matmultf(ctx->matrix.model, ctx->matrix.model, ctx->matrix.fnorm);
	if(!matinvf(ctx->matrix.fnorm, ctx->matrix.fnorm))
	{
		/* OK use identity matrix */
		int i;
		for(i = 0; i < 16; i++)
		{
			ctx->matrix.fnorm[i] = (i % 5 == 0) ? 1.0f : 0.0f;
		}
		ctx->matrix.fnorm_is_idx = TRUE;
	}
	else
	{
		ctx->matrix.fnorm_is_idx = matisidx(ctx->matrix.fnorm);
	}
	
	ctx->matrix.vnorm[0] = ctx->matrix.viewport[0];
	ctx->matrix.vnorm[1] = ctx->matrix.viewport[1];
	ctx->matrix.vnorm[2] = 2.0f / ctx->matrix.viewport[2];
	ctx->matrix.vnorm[3] = 2.0f / ctx->matrix.viewport[3];
}

void MesaUnproject(mesa3d_ctx_t *ctx, GLfloat winx, GLfloat winy, GLfloat winz,
	GLfloat *objx, GLfloat *objy, GLfloat *objz)
{
/*	return unprojectf(winx, winy, winz, 1.0f, ctx->matrix.model, ctx->matrix.proj, ctx->matrix.viewport,
		objx, objy, objz, NULL);*/
	
	GLfloat in[4];
	
	/* Map x and y from window coordinates */
	in[0] = ((winx - ctx->matrix.vnorm[0]) * ctx->matrix.vnorm[2]) - 1.0f;
	in[1] = ((winy - ctx->matrix.vnorm[1]) * ctx->matrix.vnorm[3]) - 1.0f;
	in[2] =  (winz * 2.0f) - 1.0f;
	in[3] = 1.0f;

	if(ctx->matrix.fnorm_is_idx)
	{
		*objx = in[0];
		*objy = in[1];
		*objz = in[2];
		return;
	}

	GLfloat out[4];
	matmultvecf(ctx->matrix.fnorm, in, out);
	
	if(out[3] == 0.0)
	{
		*objx = in[0];
		*objy = in[1];
		*objz = in[2];
		return;
	}

	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];

	*objx = out[0];
	*objy = out[1];
	*objz = out[2];
}

#if 0
void MesaUnprojectw(mesa3d_ctx_t *ctx, GLfloat winx, GLfloat winy, GLfloat winz, GLfloat rw, GLfloat out[4])
{
	GLfloat in[4];
	
	/* Map x and y from window coordinates */
	in[0] = ((winx - ctx->matrix.vnorm[0]) * ctx->matrix.vnorm[2]) - 1.0f;
	in[1] = ((winy - ctx->matrix.vnorm[1]) * ctx->matrix.vnorm[3]) - 1.0f;
	in[2] =  (winz * 2.0f) - 1.0f;
	in[3] = rw;

	if(ctx->matrix.fnorm_is_idx)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
		return;
	}

	matmultvecf(ctx->matrix.fnorm, in, out);
	
	if(out[3] == 0.0)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
		return;
	}

	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
}
#endif
