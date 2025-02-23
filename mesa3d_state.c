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
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"
#include "d3dhal_ddk.h"

#include "nocrt.h"
#endif

NUKED_LOCAL void MesaRecState(mesa3d_ctx_t *ctx, DWORD state, DWORD value)
{
	if(state <= MESA_REC_MAX_STATE && ctx->state.record != NULL)
	{
		DWORD mask_byte = state >> 5; // div 32
		DWORD mask_bit  = 1 << (state & 31);
			
		ctx->state.record->state[state] = value;
		ctx->state.record->stateset[mask_byte] |= mask_bit;
	}
}

NUKED_LOCAL void MesaRecTMUState(mesa3d_ctx_t *ctx, DWORD tmu, DWORD state, DWORD value)
{
	if(state <= MESA_REC_MAX_TMU_STATE && ctx->state.record != NULL)
	{
		DWORD mask_byte = state >> 5; // div 32
		DWORD mask_bit  = 1 << (state & 31);

		ctx->state.record->tmu[tmu].state[state] = value;
		ctx->state.record->tmu[tmu].set[mask_byte] |= mask_bit;
	}
}

#define SET_BIT(_v, _dw) (_v)[(_dw) >> 5] |= 1 << ((_dw) & 31)

NUKED_LOCAL void state_apply_mask(mesa_rec_state_t *rec, D3DSTATEBLOCKTYPE sbType)
{
	DWORD state_mask[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	DWORD tmu_mask[1] = {0};

	if(sbType == D3DSBT_ALL)
	{
		/* DDK:
		 * Signals the driver to capture all state.
		 * When requested to capture all state in pure device mode the driver should capture
		 * all state with the exception of:
		 * - current vertex stream state,
		 * - current index stream state
		 * - currently realized textures.
		 * The state that should be captured is as follows;
		 * the render states listed below, the texture stage states listed below,
		 * the viewport, all the world transforms, the view transform, the projection transform,
		 * the texture transform for all texture stages, all user clip planes, the current material,
		 * all lights that have been used prior to the state block creation, the current vertex shader handle,
		 * the current pixel shader handle, the current vertex shader constants and
		 * the current pixel shader constants.
		 *
		 * The render states to record are as follows:
		 */
		SET_BIT(state_mask, D3DRENDERSTATE_SPECULARENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_ZENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_FILLMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_SHADEMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_LINEPATTERN);
		SET_BIT(state_mask, D3DRENDERSTATE_ZWRITEENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHATESTENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_LASTPIXEL);
		SET_BIT(state_mask, D3DRENDERSTATE_SRCBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_DESTBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_CULLMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_ZFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHAREF);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHAFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_DITHERENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_STIPPLEDALPHA);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGCOLOR);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGTABLEMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGSTART);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGEND);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGDENSITY);
		SET_BIT(state_mask, D3DRENDERSTATE_EDGEANTIALIAS);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHABLENDENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_ZBIAS);
		SET_BIT(state_mask, D3DRENDERSTATE_RANGEFOGENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILFAIL);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILZFAIL);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILPASS);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILREF);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILMASK);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILWRITEMASK);
		SET_BIT(state_mask, D3DRENDERSTATE_TEXTUREFACTOR);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP0);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP1);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP2);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP3);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP4);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP5);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP6);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP7);
		SET_BIT(state_mask, D3DRENDERSTATE_AMBIENT);
		SET_BIT(state_mask, D3DRENDERSTATE_COLORVERTEX);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGVERTEXMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_CLIPPING);
		SET_BIT(state_mask, D3DRENDERSTATE_LIGHTING);
		SET_BIT(state_mask, D3DRENDERSTATE_NORMALIZENORMALS);
		SET_BIT(state_mask, D3DRENDERSTATE_LOCALVIEWER);
		SET_BIT(state_mask, D3DRENDERSTATE_EMISSIVEMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_AMBIENTMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_DIFFUSEMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_SPECULARMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_VERTEXBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_CLIPPLANEENABLE);

		SET_BIT(state_mask, D3DRS_SOFTWAREVERTEXPROCESSING);
		SET_BIT(state_mask, D3DRS_POINTSIZE);
		SET_BIT(state_mask, D3DRS_POINTSIZE_MIN);
		SET_BIT(state_mask, D3DRS_POINTSPRITEENABLE);
		SET_BIT(state_mask, D3DRS_POINTSCALEENABLE);
		SET_BIT(state_mask, D3DRS_POINTSCALE_A);
		SET_BIT(state_mask, D3DRS_POINTSCALE_B);
		SET_BIT(state_mask, D3DRS_POINTSCALE_C);
		SET_BIT(state_mask, D3DRS_MULTISAMPLEANTIALIAS);
		SET_BIT(state_mask, D3DRS_MULTISAMPLEMASK);
		SET_BIT(state_mask, D3DRS_PATCHEDGESTYLE);
		SET_BIT(state_mask, D3DRS_PATCHSEGMENTS);
		SET_BIT(state_mask, D3DRS_POINTSIZE_MAX);
		SET_BIT(state_mask, D3DRS_INDEXEDVERTEXBLENDENABLE);
		SET_BIT(state_mask, D3DRS_COLORWRITEENABLE);
		SET_BIT(state_mask, D3DRS_TWEENFACTOR);
		SET_BIT(state_mask, D3DRS_BLENDOP);

		SET_BIT(tmu_mask, D3DTSS_COLOROP);
		SET_BIT(tmu_mask, D3DTSS_COLORARG1);
		SET_BIT(tmu_mask, D3DTSS_COLORARG2);
		SET_BIT(tmu_mask, D3DTSS_ALPHAOP);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG1);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG2);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT00);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT01);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT10);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT11);
		SET_BIT(tmu_mask, D3DTSS_TEXCOORDINDEX);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSU);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSV);
		SET_BIT(tmu_mask, D3DTSS_BORDERCOLOR);
		SET_BIT(tmu_mask, D3DTSS_MAGFILTER);
		SET_BIT(tmu_mask, D3DTSS_MINFILTER);
		SET_BIT(tmu_mask, D3DTSS_MIPFILTER);
		SET_BIT(tmu_mask, D3DTSS_MIPMAPLODBIAS);
		SET_BIT(tmu_mask, D3DTSS_MAXMIPLEVEL);
		SET_BIT(tmu_mask, D3DTSS_MAXANISOTROPY);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVLSCALE);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVLOFFSET);
		SET_BIT(tmu_mask, D3DTSS_TEXTURETRANSFORMFLAGS);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSW);
		SET_BIT(tmu_mask, D3DTSS_COLORARG0);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG0);
		SET_BIT(tmu_mask, D3DTSS_RESULTARG);
	}
	else if(sbType == D3DSBT_PIXELSTATE)
	{
		/*
		 * Signals the driver to capture pixel state only.
		 * When capturing pixel state in pure device mode the following state should be captured;
		 * the pixel processing related render states listed below, the pixel processing texture
		 * stage states listed below, the current pixel shader handle and the current pixel shader constants.
		 * The render states to record are as follows:
		 */
		SET_BIT(state_mask, D3DRENDERSTATE_ZENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_FILLMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_SHADEMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_LINEPATTERN);
		SET_BIT(state_mask, D3DRENDERSTATE_ZWRITEENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHATESTENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_LASTPIXEL);
		SET_BIT(state_mask, D3DRENDERSTATE_SRCBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_DESTBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_ZFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHAREF);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHAFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_DITHERENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_STIPPLEDALPHA);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGSTART);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGEND);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGDENSITY);
		SET_BIT(state_mask, D3DRENDERSTATE_EDGEANTIALIAS);
		SET_BIT(state_mask, D3DRENDERSTATE_ALPHABLENDENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_ZBIAS);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILFAIL);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILZFAIL);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILPASS);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILFUNC);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILREF);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILMASK);
		SET_BIT(state_mask, D3DRENDERSTATE_STENCILWRITEMASK);
		SET_BIT(state_mask, D3DRENDERSTATE_TEXTUREFACTOR);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP0);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP1);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP2);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP3);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP4);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP5);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP6);
		SET_BIT(state_mask, D3DRENDERSTATE_WRAP7);
		SET_BIT(state_mask, D3DRS_COLORWRITEENABLE);
		SET_BIT(state_mask, D3DRS_BLENDOP);

		SET_BIT(tmu_mask, D3DTSS_COLOROP);
		SET_BIT(tmu_mask, D3DTSS_COLORARG1);
		SET_BIT(tmu_mask, D3DTSS_COLORARG2);
		SET_BIT(tmu_mask, D3DTSS_ALPHAOP);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG1);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG2);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT00);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT01);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT10);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVMAT11);
		SET_BIT(tmu_mask, D3DTSS_TEXCOORDINDEX);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSU);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSV);
		SET_BIT(tmu_mask, D3DTSS_BORDERCOLOR);
		SET_BIT(tmu_mask, D3DTSS_MAGFILTER);
		SET_BIT(tmu_mask, D3DTSS_MINFILTER);
		SET_BIT(tmu_mask, D3DTSS_MIPFILTER);
		SET_BIT(tmu_mask, D3DTSS_MIPMAPLODBIAS);
		SET_BIT(tmu_mask, D3DTSS_MAXMIPLEVEL);
		SET_BIT(tmu_mask, D3DTSS_MAXANISOTROPY);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVLSCALE);
		SET_BIT(tmu_mask, D3DTSS_BUMPENVLOFFSET);
		SET_BIT(tmu_mask, D3DTSS_TEXTURETRANSFORMFLAGS);
		SET_BIT(tmu_mask, D3DTSS_ADDRESSW);
		SET_BIT(tmu_mask, D3DTSS_COLORARG0);
		SET_BIT(tmu_mask, D3DTSS_ALPHAARG0);
		SET_BIT(tmu_mask, D3DTSS_RESULTARG);
	}
	else if(sbType == D3DSBT_VERTEXSTATE)
	{
		/*
		 * Signals the driver to capture vertex state only.
		 * When capturing vertex state in pure device mode the following state should be captured;
		 * the vertex processing related render states listed below, the vertex processing texture
		 * stage states listed below, all lights that have been used prior to the state block creation,
		 * the current vertex shader handle and the current vertex shader constants.
		 * The render states to record are as follows:
		 */
		SET_BIT(state_mask, D3DRENDERSTATE_SHADEMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_SPECULARENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_CULLMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGCOLOR);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGTABLEMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGSTART);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGEND);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGDENSITY);
		SET_BIT(state_mask, D3DRENDERSTATE_RANGEFOGENABLE);
		SET_BIT(state_mask, D3DRENDERSTATE_AMBIENT);
		SET_BIT(state_mask, D3DRENDERSTATE_COLORVERTEX);
		SET_BIT(state_mask, D3DRENDERSTATE_FOGVERTEXMODE);
		SET_BIT(state_mask, D3DRENDERSTATE_CLIPPING);
		SET_BIT(state_mask, D3DRENDERSTATE_LIGHTING);
		SET_BIT(state_mask, D3DRENDERSTATE_NORMALIZENORMALS);
		SET_BIT(state_mask, D3DRENDERSTATE_LOCALVIEWER);
		SET_BIT(state_mask, D3DRENDERSTATE_EMISSIVEMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_AMBIENTMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_DIFFUSEMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_SPECULARMATERIALSOURCE);
		SET_BIT(state_mask, D3DRENDERSTATE_VERTEXBLEND);
		SET_BIT(state_mask, D3DRENDERSTATE_CLIPPLANEENABLE);
		SET_BIT(state_mask, D3DRS_SOFTWAREVERTEXPROCESSING);
		SET_BIT(state_mask, D3DRS_POINTSIZE);
		SET_BIT(state_mask, D3DRS_POINTSIZE_MIN);
		SET_BIT(state_mask, D3DRS_POINTSPRITEENABLE);
		SET_BIT(state_mask, D3DRS_POINTSCALEENABLE);
		SET_BIT(state_mask, D3DRS_POINTSCALE_A);
		SET_BIT(state_mask, D3DRS_POINTSCALE_B);
		SET_BIT(state_mask, D3DRS_POINTSCALE_C);
		SET_BIT(state_mask, D3DRS_MULTISAMPLEANTIALIAS);
		SET_BIT(state_mask, D3DRS_MULTISAMPLEMASK);
		SET_BIT(state_mask, D3DRS_PATCHEDGESTYLE);
		SET_BIT(state_mask, D3DRS_PATCHSEGMENTS);
		SET_BIT(state_mask, D3DRS_POINTSIZE_MAX);
		SET_BIT(state_mask, D3DRS_INDEXEDVERTEXBLENDENABLE);
		SET_BIT(state_mask, D3DRS_TWEENFACTOR);

		SET_BIT(tmu_mask, D3DTSS_TEXCOORDINDEX);
		SET_BIT(tmu_mask, D3DTSS_TEXTURETRANSFORMFLAGS);
	}
	
	DWORD i;
	for(i = 0; i < 8; i++)
	{
		rec->stateset[i] &= state_mask[i];
	}

	for(i = 0; i < MESA_TMU_MAX; i++)
	{
		rec->tmu->set[0] &= tmu_mask[0];
	}
}

#undef SET_BIT

NUKED_LOCAL void MesaApplyState(mesa3d_ctx_t *ctx, mesa_rec_state_t *rec)
{
	DWORD i, j;
	for(i = 0; i < MESA_REC_MAX_STATE; i++)
	{
		DWORD state_byte = i >> 5;
		DWORD state_bit = 1 << (i & 31);

		if(rec->stateset[state_byte] & state_bit)
		{
			D3DHAL_DP2RENDERSTATE rstate;
			rstate.RenderState = i;
			rstate.dwState = rec->state[i];
			MesaSetRenderState(ctx, &rstate, NULL);
		}
	}

	for(j = 0; j < MESA_TMU_MAX; j++)
	{
		for(i = 0; i < MESA_REC_MAX_TMU_STATE; i++)
		{
			DWORD state_byte = i >> 5;
			DWORD state_bit = 1 << (i & 31);

			if(rec->tmu[j].set[state_byte] & state_bit)
			{
				MesaSetTextureState(ctx, j, i, &rec->tmu[j].state[i]);
			}
		}
	}
	
	MesaStencilApply(ctx);
	MesaDrawRefreshState(ctx);
}

NUKED_LOCAL mesa_rec_state_t *MesaRecLookup(mesa3d_ctx_t *ctx, DWORD handle, BOOL create)
{
	int empty = -1;
	int i;
	for(i = 0; i < MESA_RECS_MAX; i++)
	{
		if(ctx->records[i] != NULL)
		{
			if(ctx->records[i]->handle == handle)
			{
				if(create)
				{
					memset(ctx->records[i], 0, sizeof(mesa_rec_state_t));
					ctx->records[i]->handle = handle;
				}

				return ctx->records[i];
			}
		}
		else if(empty < 0)
		{
			empty = i;
		}
	}

	if(empty >= MESA_RECS_MAX || !create)
	{
		return NULL;
	}

	ctx->records[empty] = hal_alloc(HEAP_NORMAL, sizeof(mesa_rec_state_t), 0);
	if(ctx->records[empty])
	{
		ctx->records[empty]->handle = handle;
	}

	return ctx->records[empty];
}

NUKED_LOCAL void MesaRecStart(mesa3d_ctx_t *ctx, DWORD handle, D3DSTATEBLOCKTYPE sbType)
{
	TRACE_ENTRY
	
	mesa_rec_state_t *rec = MesaRecLookup(ctx, handle, TRUE);
	
	if(rec)
	{
		ctx->state.record = rec;
		ctx->state.record_type = sbType;
	}
}

NUKED_LOCAL void MesaRecStop(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY
	
	if(ctx->state.record)
	{
		state_apply_mask(ctx->state.record, ctx->state.record_type);
		ctx->state.record = NULL;
	}
}

NUKED_LOCAL void MesaRecApply(mesa3d_ctx_t *ctx, DWORD handle)
{
	TRACE_ENTRY
	
	mesa_rec_state_t *rec = MesaRecLookup(ctx, handle, TRUE);
	if(rec != NULL)
	{
		if(rec == ctx->state.record)
		{
			state_apply_mask(rec, ctx->state.record_type);
		}

		MesaApplyState(ctx, rec);
	}
}

NUKED_LOCAL void MesaRecDelete(mesa3d_ctx_t *ctx, DWORD handle)
{
	TRACE_ENTRY
	
	DWORD i;
	for(i = 0; i < MESA_RECS_MAX; i++)
	{
		if(ctx->records[i] != NULL)
		{
			if(ctx->records[i]->handle == handle)
			{
				if(ctx->records[i] == ctx->state.record)
				{
					ctx->state.record = NULL;
					ctx->state.recording = FALSE;
				}
			}
			hal_free(HEAP_NORMAL, ctx->records[i]);
			ctx->records[i] = NULL;
		}
	} // for
}
