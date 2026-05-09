/******************************************************************************
 * Copyright (c) 2026 Jaroslav Hensl                                          *
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
#include <initguid.h> /* this is only one file using GUID */
#include <ddraw.h>
#include <ddrawi.h>
#include "ddrawi_ddk.h"
#include "d3dhal_ddk.h"
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include <d3d8caps.h>
#include "vmhal9x.h"
#include "mesa3d.h"
#include "d3dhal.h"

#define VMHAL9X_LIB
#include "vmsetup.h"

#include "nocrt.h"
#endif

/* FROM ddrvmem.h */
typedef DWORD HDDRVITEM, * LPHDDRVITEM;

#define MESA_TMU_CNT() ((env.texture_num_units > MESA_TMU_MAX) ? MESA_TMU_MAX : env.texture_num_units)

enum DDRV_RETURN {
	DDRV_SUCCESS_STOP,
	DDRV_SUCCESS_CONTINUE,
	DDRV_ERROR_CONTINUE,
	DDRV_ERROR_STOP
};

#ifndef D3DHAL2_CB32_SETRENDERTARGET
#define D3DHAL2_CB32_SETRENDERTARGET    0x00000001L
#define D3DHAL2_CB32_CLEAR              0x00000002L
#define D3DHAL2_CB32_DRAWONEPRIMITIVE   0x00000004L
#define D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE 0x00000008L
#define D3DHAL2_CB32_DRAWPRIMITIVES     0x00000010L
#endif

static BOOL ValidateCtx(DWORD dwhContext)
{
	BOOL rc = FALSE;
	NONGL_BLOCK_BEGIN(dwhContext)
		rc = TRUE;
	NONGL_BLOCK_END

	return rc;
}

#define VALIDATE(_d3d) if(!ValidateCtx((_d3d)->dwhContext)){ \
		(_d3d)->ddrval = D3DHAL_CONTEXT_BAD; \
		ERR("Invalid context");\
		return;}

void __stdcall hal3d_SetRenderTarget32(LPD3DHAL_SETRENDERTARGETDATA lpSetRenderData)
{
	TRACE_ENTRY

	VALIDATE(lpSetRenderData)

	if(lpSetRenderData->lpDDS != NULL)
	{
		TOPIC("GL", "SetRenderTarget32(0x%X, 0x%X)",
			lpSetRenderData->lpDDS,
			lpSetRenderData->lpDDSZ
		);
		
		DWORD dds_id = 0;
		DWORD ddz_id = 0;
		
		if(lpSetRenderData->lpDDS != NULL)
		{
			dds_id = ((LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDS)->lpLcl->dwReserved1;
		}
		
		if(lpSetRenderData->lpDDSZ)
		{
			ddz_id = ((LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDSZ)->lpLcl->dwReserved1;
		}
		
		if(dds_id)
		{
			GL_BLOCK_BEGIN(lpSetRenderData->dwhContext)
				MesaSetTarget(ctx, dds_id, ddz_id, FALSE);
			GL_BLOCK_END

			lpSetRenderData->ddrval = DD_OK;
		}
		else
		{
			lpSetRenderData->ddrval = DDERR_INVALIDPARAMS;
			WARN("SetRenderTarget32: DDERR_INVALIDPARAMS");
		}
	}
}

void __stdcall hal3d_Clear32(LPD3DHAL_CLEARDATA lpClearData)
{
	TRACE_ENTRY

	VALIDATE(lpClearData)
	
	/* DDK98 - DX5: dwFillDepth will always be set to 0xffffffff which mens
	fill to the maximum z-buffer value. This is currently the only supported z fill mode. */
	GL_BLOCK_BEGIN(lpClearData->dwhContext)
		MesaClear(ctx, lpClearData->dwFlags,
			lpClearData->dwFillColor, 1.0f, 0x0,
			lpClearData->dwNumRects, (LPRECT)lpClearData->lpRects);
	GL_BLOCK_END

	lpClearData->ddrval = DD_OK;
}

void __stdcall hal3d_DrawOnePrimitive32(LPD3DHAL_DRAWONEPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)
	
	GL_BLOCK_BEGIN(lpDrawData->dwhContext)
		MesaBufferTexturesChanges(ctx);
		MesaDraw5(ctx, lpDrawData->PrimitiveType, lpDrawData->VertexType, lpDrawData->lpvVertices, lpDrawData->dwNumVertices);
	GL_BLOCK_END

	lpDrawData->ddrval = DD_OK;
}

void __stdcall hal3d_DrawOneIndexedPrimitive32(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)

	GL_BLOCK_BEGIN(lpDrawData->dwhContext)
		MesaBufferTexturesChanges(ctx);
		MesaDraw5Index(ctx, lpDrawData->PrimitiveType, lpDrawData->VertexType,
			lpDrawData->lpvVertices, lpDrawData->dwNumVertices,
			lpDrawData->lpwIndices, lpDrawData->dwNumIndices
		);
	GL_BLOCK_END

	lpDrawData->ddrval = DD_OK;
}

/**
 * From DDK98:
 *  Data block:
 * 
 *  Consists of interleaved D3DHAL_DRAWPRIMCOUNTS, state change pairs,
 *  and primitive drawing commands.
 *  
 *  D3DHAL_DRAWPRIMCOUNTS: gives number of state change pairs and
 *            the information on the primitive to draw.
 *            wPrimitiveType is of type D3DPRIMITIVETYPE. Drivers
 *                must support all 7 of the primitive types specified
 *                in the DrawPrimitive API.
 *            Currently, wVertexType will always be D3DVT_TLVERTEX.
 *            If the wNumVertices member is 0, then the driver should
 *                return after doing the state changing. This is the
 *                terminator for the command stream.
 *   state change pairs: DWORD pairs specify the state changes that
 *            the driver should effect before drawing the primitive.
 *            wNumStateChanges can be 0, in which case the next primitive
 *            should be drawn without any state changes in between.
 *            If present, the state change pairs are NOT aligned, they
 *            immediately follow the PRIMCOUNTS structure.
 *   vertex data (if any): is 32-byte aligned.
 *  
 *   If a primcounts structure follows (i.e. if wNumVertices was nonzero
 *   in the previous one), then it will immediately follow the state
 *   changes or vertex data with no alignment padding.
 **/
void __stdcall hal3d_DrawPrimitives32(LPD3DHAL_DRAWPRIMITIVESDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)
	
	LPBYTE lpData = (LPBYTE)lpDrawData->lpvData;
	LPD3DHAL_DRAWPRIMCOUNTS	drawPrimitiveCounts;
	int j;
	
	GL_BLOCK_BEGIN(lpDrawData->dwhContext)
	MesaBufferTexturesChanges(ctx);

	TOPIC("TEX", "DrawPrimitives32");

	do
	{
		drawPrimitiveCounts = (LPD3DHAL_DRAWPRIMCOUNTS)(lpData);
		lpData += sizeof(D3DHAL_DRAWPRIMCOUNTS);
		
		/* state block */
		LPD3DSTATE state;
		for(j = drawPrimitiveCounts->wNumStateChanges; j > 0; j--)
		{
			state = (LPD3DSTATE)lpData;
			D3DHAL_DP2RENDERSTATE rstate;
			rstate.RenderState = state->drstRenderStateType;
			rstate.dwState = state->dwArg[0];
			MesaSetRenderState(ctx, &rstate, NULL);
			lpData += sizeof(D3DSTATE);
		}
		
		/* padding */
		lpData += 31;
		lpData = (LPBYTE)((ULONG)lpData & (~31));
		
		/* draw block */
		if(drawPrimitiveCounts->wNumVertices)
		{
			TOPIC("TEX", "batch %d, type: %d", drawPrimitiveCounts->wNumVertices, drawPrimitiveCounts->wPrimitiveType);
			
			MesaDraw5(ctx, drawPrimitiveCounts->wPrimitiveType, drawPrimitiveCounts->wVertexType, (LPD3DTLVERTEX)lpData, drawPrimitiveCounts->wNumVertices);
			/* wVertexType should be only D3DVT_TLVERTEX, but for all cases... */
			switch(drawPrimitiveCounts->wVertexType)
			{
				case D3DVT_VERTEX:
					lpData += drawPrimitiveCounts->wNumVertices * sizeof(D3DVERTEX);
					break;
				case D3DVT_LVERTEX:
					lpData += drawPrimitiveCounts->wNumVertices * sizeof(D3DLVERTEX);
					break;
				case D3DVT_TLVERTEX:
				default:
					lpData += drawPrimitiveCounts->wNumVertices * sizeof(D3DTLVERTEX);
					break;
			}
		}
	} while(drawPrimitiveCounts->wNumVertices);

	GL_BLOCK_END

	lpDrawData->ddrval = DD_OK;
}

/*
void __stdcall hal3d_ValidateTextureStageState32(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd)
{
	TRACE_ENTRY
	
	VALIDATE(lpvtssd)

	lpvtssd->dwNumPasses = 1;
	lpvtssd->ddrval = DD_OK;
}*/

void __stdcall hal3d_DrawPrimitives2_32(LPD3DHAL_DRAWPRIMITIVES2DATA pd)
{
	TRACE_ENTRY
	
	TRACE("context id=%X", pd->dwhContext);

#ifdef DEBUG
	SetExceptionHandler();
#endif

	VALIDATE(pd)

	LPBYTE insStart;
	LPBYTE vertices = NULL;
	
	insStart = (LPBYTE)(pd->lpDDCommands->lpGbl->fpVidMem);
	if (insStart == NULL)
	{
		ERR("DrawPrimitives2_32: insStart == NULL, pd->dwFlags=%X", pd->dwFlags);
		
		pd->dwErrorOffset   = 0;
		pd->ddrval          = DDERR_INVALIDPARAMS;
		WARN("DrawPrimitives2_32: DDERR_INVALIDPARAMS");
		return;
	}
	
	if(pd->lpVertices)
	{
		if(pd->dwFlags & D3DHALDP2_USERMEMVERTICES)
		{
			vertices = ((LPBYTE)pd->lpVertices) + pd->dwVertexOffset;
		}
		else
		{
			vertices = ((LPBYTE)pd->lpDDVertex->lpGbl->fpVidMem) + pd->dwVertexOffset;
		}
  }
  
  TRACE("DrawPrimitives2_32: dwFlags=0x%X, dwVertexType=0x%X",
  	pd->dwFlags, pd->dwVertexType);
  
	LPDWORD RStates = NULL;
	if(pd->dwFlags & D3DHALDP2_EXECUTEBUFFER)
		RStates = pd->lpdwRStates;

	LPBYTE cmdBufferStart    = insStart + pd->dwCommandOffset;
	LPBYTE cmdBufferEnd      = cmdBufferStart + pd->dwCommandLength;
	DWORD rc = DD_OK;
	
	GL_BLOCK_BEGIN(pd->dwhContext)
		MesaBufferTexturesChanges(ctx);
	
		LPBYTE UMVertices = pd->lpVertices;
		if(UMVertices != NULL)
		{
			UMVertices += pd->dwVertexOffset;
		}

		rc = MesaDraw6(ctx, cmdBufferStart, cmdBufferEnd, vertices, UMVertices, pd->dwVertexType, &pd->dwErrorOffset, RStates, pd->dwVertexLength);

		//MesaSpaceIdentityReset(ctx);
	GL_BLOCK_END

	pd->ddrval = rc;
	TRACE("MesaDraw6(...) = %d", rc);

#if 0
	if(pd->dwFlags & D3DHALDP2_REQVERTEXBUFSIZE)
	{
		TRACE("want resize vertex buffer to %d bytes", pd->dwReqVertexBufSize);
		pd->dwFlags &= ~(D3DHALDP2_SWAPVERTEXBUFFER | D3DHALDP2_REQVERTEXBUFSIZE);
		pd->dwReqVertexBufSize = 1*1024*1024;
	}
	
	if(pd->dwFlags & D3DHALDP2_REQCOMMANDBUFSIZE)
	{
		TRACE("want command vertex buffer to %d bytes", pd->dwReqCommandBufSize);
		pd->dwFlags &= ~(D3DHALDP2_SWAPCOMMANDBUFFER | D3DHALDP2_REQCOMMANDBUFSIZE);
	}
#endif
}

void __stdcall hal3d_Clear2_32(LPD3DHAL_CLEAR2DATA cd)
{
	TRACE_ENTRY
	
	VALIDATE(cd)
	
	GL_BLOCK_BEGIN(cd->dwhContext)
		MesaClear(ctx, cd->dwFlags,
			cd->dwFillColor, cd->dvFillDepth, cd->dwFillStencil,
			cd->dwNumRects, (LPRECT)cd->lpRects);
	GL_BLOCK_END
	
	cd->ddrval = DD_OK;
}

/*
 * 
 * Function:    D3DCreateSurfaceEx
 * Description: D3dCreateSurfaceEx creates a Direct3D surface from a DirectDraw  
 *              surface and associates a requested handle value to it.
 * 
 *              All Direct3D drivers must support D3dCreateSurfaceEx.
 * 
 *              D3dCreateSurfaceEx creates an association between a  
 *              DirectDraw surface and a small integer surface handle. 
 *              By creating these associations between a
 *              handle and a DirectDraw surface, D3dCreateSurfaceEx allows a 
 *              surface handle to be imbedded in the Direct3D command stream.
 *              For example when the D3DDP2OP_TEXBLT command token is sent
 *              to D3dDrawPrimitives2 to load a texture map, it uses a source 
 *              handle and destination handle which were associated
 *              with a DirectDraw surface through D3dCreateSurfaceEx.
 *
 *              For every DirectDraw surface created under the DirectDrawLocal 
 *              object, the runtime generates a valid handle that uniquely
 *              identifies the surface and places it in
 *              pcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle. This handle value
 *              is also used with the D3DRENDERSTATE_TEXTUREHANDLE render state 
 *              to enable texturing, and with the D3DDP2OP_SETRENDERTARGET and 
 *              D3DDP2OP_CLEAR commands to set and/or clear new rendering and 
 *              depth buffers. The driver should fail the call and return
 *              DDHAL_DRIVER_HANDLED if it cannot create the Direct3D
 *              surface. If the DDHAL_CREATESURFACEEX_SWAPHANDLES flag is set, 
 *              the handles should be swapped over two sequential calls to 
 *              D3dCreateSurfaceEx. As appropriate, the driver should also 
 *              store any surface-related information that it will subsequently 
 *              need when using the surface. The driver must create
 *              a new surface table for each new lpDDLcl and implicitly grow 
 *              the table when necessary to accommodate more surfaces. 
 *              Typically this is done with an exponential growth algorithm 
 *              so that you don't have to grow the table too often. Direct3D 
 *              calls D3dCreateSurfaceEx after the surface is created by
 *              DirectDraw by request of the Direct3D runtime or the application.
 *
 * Parameters
 *
 *      lpcsxd
 *           pointer to CreateSurfaceEx structure that contains the information
 *           required for the driver to create the surface (described below). 
 *
 *           dwFlags
 *           lpDDLcl
 *                   Handle to the DirectDraw object created by the application.
 *                   This is the scope within which the lpDDSLcl handles exist.
 *                   A DD_DIRECTDRAW_LOCAL structure describes the driver.
 *           lpDDSLcl
 *                   Handle to the DirectDraw surface we are being asked to
 *                   create for Direct3D. These handles are unique within each
 *                   different DD_DIRECTDRAW_LOCAL. A DD_SURFACE_LOCAL structure
 *                   represents the created surface object.
 *           ddRVal
 *                   Specifies the location in which the driver writes the return
 *                   value of the D3dCreateSurfaceEx callback. A return code of
 *                   DD_OK indicates success.
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLE
 *      DDHAL_DRIVER_NOTHANDLE
 *
 * JH: pcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle is prealocated by
 *     runtime/system/HEL/whatever so don't write to this variable your own numbers.
 *
 */
void __stdcall hal3d_CreateSurfaceEx32(LPDDHAL_CREATESURFACEEXDATA lpcsxd)
{
	TRACE_ENTRY
	
	lpcsxd->ddRVal = DD_OK;

	if(lpcsxd->lpDDSLcl == NULL || lpcsxd->lpDDLcl == NULL)
	{
		return;
	}

	mesa3d_entry_t *entry = Mesa3DGet(TRUE);

	if(entry == NULL)
	{
		ERR("Mesa3DGet() failed");
		return;
	}

	LPDDRAWI_DDRAWSURFACE_LCL surf = lpcsxd->lpDDSLcl;

	TOPIC("TARGET", "CreateSurfaceEx32 lpDDLcl=0x%X vidmem=0x%X handle=%d sid=%d", lpcsxd->lpDDLcl, surf->lpGbl->fpVidMem, surf->lpSurfMore->dwSurfaceHandle, surf->dwReserved1);

	/* this is from 3dlabs driver... */
	if(surf->lpGbl->fpVidMem == 0)
	{
		ddsurf_unregister(entry, 0, surf->lpSurfMore->dwSurfaceHandle);
		lpcsxd->ddRVal = DD_OK;
		return;
	}
	
	TOPIC("TARGET", "CreateSurfaceEx32 dwCaps=0x%X dwCaps2=0x%X", surf->ddsCaps.dwCaps,
		surf->lpSurfMore->ddsCapsEx.dwCaps2
	);
	
	if(surf->ddsCaps.dwCaps & DX7_SURFACE_NEST_TYPES)
	{
		if(ddsurf_register(entry, surf, TRUE) != NULL)
		{
			lpcsxd->ddRVal = DD_OK;
		}
	}
	else
	{
		/* is exec buffer */
		if(surf->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
		{
			TOPIC("EXEBUF", "surface 0x%X, vram=0x%X", surf->lpSurfMore->dwSurfaceHandle, surf->lpGbl->fpVidMem);
			/* DX8+ need register buffer too for usage with streams */	
			if(ddsurf_register(entry, surf, TRUE) != NULL)
			{
				lpcsxd->ddRVal = DD_OK;
			}
		}
		else
		{
			WARN("CreateSurfaceEx32: ignoring type 0x%X, handle=%d", surf->ddsCaps.dwCaps, surf->lpSurfMore->dwSurfaceHandle);
		}
	}
}

/**
 *
 * Function:    D3DDestroyDDLocal
 *
 * Description: D3dDestroyDDLocal destroys all the Direct3D surfaces previously 
 *              created by D3DCreateSurfaceEx that belong to the same given 
 *              local DirectDraw object.
 *
 *              All Direct3D drivers must support D3dDestroyDDLocal.
 *              Direct3D calls D3dDestroyDDLocal when the application indicates 
 *              that the Direct3D context is no longer required and it will be 
 *              destroyed along with all surfaces associated to it. 
 *              The association comes through the pointer to the local 
 *              DirectDraw object. The driver must free any memory that the
 *              driver's D3dCreateSurfaceEx callback allocated for
 *              each surface if necessary. The driver should not destroy 
 *              the DirectDraw surfaces associated with these Direct3D surfaces; 
 *              this is the application's responsibility.
 *
 * Parameters
 *
 *      lpdddd
 *            Pointer to the DestoryLocalDD structure that contains the
 *            information required for the driver to destroy the surfaces.
 *
 *            dwFlags
 *                  Currently unused
 *            pDDLcl
 *                  Pointer to the local Direct Draw object which serves as a
 *                  reference for all the D3D surfaces that have to be destroyed.
 *            ddRVal
 *                  Specifies the location in which the driver writes the return
 *                  value of D3dDestroyDDLocal. A return code of DD_OK indicates
 *                   success.
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLED
 *      DDHAL_DRIVER_NOTHANDLED
 */
void __stdcall hal3d_DestroyDDLocal32(LPDDHAL_DESTROYDDLOCALDATA lpdddd)
{
	TRACE_ENTRY

	// ---

	TRACE("DestroyDDLocal32 SUCCESS");
	lpdddd->ddRVal = DD_OK;
}

void __stdcall hal3d_ContextCreate32(LPD3DHAL_CONTEXTCREATEDATA pccd)
{
	TRACE_ENTRY

	pccd->ddrval = D3DHAL_OUTOFCONTEXTS; /* error state */

	mesa3d_entry_t *entry = Mesa3DGet(TRUE);
	if(entry)
	{
		mesa3d_ctx_t *ctx = NULL;
		
		entry->runtime_ver = 5;
		if(entry->env.dx6 && entry->env.ddi >= 6)
		{
			entry->runtime_ver = 6;
		}

		if(entry->env.dx7 && entry->env.ddi >= 7)
		{
			entry->runtime_ver = 7;
		}
		
		DWORD dds_sid = 0;
		DWORD ddz_sid = 0;

		if(entry->runtime_ver >= 7)
		{
			TRACE("ContextCreate32 DX7+");
			if(pccd->lpDDSLcl)
			{
				dds_sid = pccd->lpDDSLcl->lpSurfMore->dwSurfaceHandle;
			}

			if(pccd->lpDDSZLcl)
			{
				ddz_sid = pccd->lpDDSZLcl->lpSurfMore->dwSurfaceHandle;
			}
			
			if(dds_sid)
			{
				ctx = MesaCreateCtx(entry, dds_sid,  ddz_sid);
				TRACE("ContextCreate32 lpDDLcl=%X", pccd->lpDDLcl);
			}
		}
		else
		{
			TRACE("ContextCreate32 -DX6");
			
			LPDDRAWI_DDRAWSURFACE_INT dds_int = (LPDDRAWI_DDRAWSURFACE_INT)pccd->lpDDS;
			LPDDRAWI_DDRAWSURFACE_INT ddz_int = (LPDDRAWI_DDRAWSURFACE_INT)pccd->lpDDSZ;
			
			if(dds_int && dds_int->lpLcl)
			{
				dds_sid = dds_int->lpLcl->dwReserved1;
			}
			
			if(ddz_int && ddz_int->lpLcl)
			{
				ddz_sid = ddz_int->lpLcl->dwReserved1;
			}

			if(dds_sid)
			{
				ctx = MesaCreateCtx(entry, dds_sid, ddz_sid);
			}
		}

		if(ctx)
		{
			//SurfaceAttachCtx(ctx);
			// Return to the runtime the D3D context id that will be used to
			// identify calls for this context from now on. Store prev value
			// since that tells us which API are we being called from
			// (5=DX9, 4=DX8, 3=DX7, 2=DX6, 1=DX5, 0=DX3)
			ctx->dxif = pccd->dwhContext; // in: DX API version
			
			pccd->dwhContext = ctx->id;
			pccd->ddrval = DD_OK;

			if(entry->runtime_ver >= 7)
				ctx->dd = NULL;
			else
				ctx->dd = pccd->lpDDGbl;

			if(ctx->dxif < MESA_CTX_IF_DX7)
			{
				ctx->matrix.zscale = 0.99;
			}
		}
	}

	if(pccd->ddrval != DD_OK || pccd->dwhContext == 0)
	{
		ERR("ContextCreate32 FAILED");
	}
}

void __stdcall hal3d_ContextDestroy32(LPD3DHAL_CONTEXTDESTROYDATA pcdd)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = Mesa3DGet(FALSE);
	if(entry)
	{
		mesa3d_ctx_t *ctx = HT_LOOKUP_T(mesa3d_ctx_t, entry->ht_ctx, pcdd->dwhContext);
		if(ctx)
		{
			MesaDestroyCtx(ctx, FALSE);
		}
	}

	pcdd->dwhContext = 0;
	pcdd->ddrval = DD_OK;
}

void __stdcall hal3d_ContextDestroyAll32(LPD3DHAL_CONTEXTDESTROYALLDATA pcdd)
{
	TRACE_ENTRY
	
	mesa3d_entry_t *mesa = Mesa3DGet(FALSE);
	if(mesa)
	{
		MesaDestroyAllCtx(mesa);
	}
	pcdd->ddrval = DD_OK;
}

void __stdcall hal3d_RenderState32(LPD3DHAL_RENDERSTATEDATA prd)
{
	TRACE_ENTRY
	
	VALIDATE(prd)
	
	int i;

	GL_BLOCK_BEGIN(prd->dwhContext)
		LPBYTE lpData = (LPBYTE)(((LPDDRAWI_DDRAWSURFACE_INT)prd->lpExeBuf)->lpLcl->lpGbl->fpVidMem);
		LPD3DSTATE lpState = (LPD3DSTATE) (lpData + prd->dwOffset);
		
		for(i = 0; i < prd->dwCount; i++)
		{
			D3DHAL_DP2RENDERSTATE rstate;
			rstate.RenderState = lpState->drstRenderStateType;
			rstate.dwState = lpState->dwArg[0];
			MesaSetRenderState(ctx, &rstate, NULL);
			lpState++;
		}
		MesaDrawRefreshState(ctx);
	GL_BLOCK_END
	
	prd->ddrval = DD_OK;
}

void __stdcall hal3d_RenderPrimitive32(LPD3DHAL_RENDERPRIMITIVEDATA prd)
{
	TRACE_ENTRY

	VALIDATE(prd)

	GL_BLOCK_BEGIN(prd->dwhContext)
		LPBYTE lpData = (LPBYTE)(((LPDDRAWI_DDRAWSURFACE_INT)prd->lpExeBuf)->lpLcl->lpGbl->fpVidMem);
  	LPD3DINSTRUCTION lpIns = &prd->diInstruction;
  	LPBYTE prim = lpData + prd->dwOffset;
  	LPBYTE vertices = NULL;

  	if(prd->lpTLBuf != NULL)
  	{
  		LPBYTE lpVData = (LPBYTE)(((LPDDRAWI_DDRAWSURFACE_INT)prd->lpTLBuf)->lpLcl->lpGbl->fpVidMem);
  		vertices = lpVData + prd->dwTLOffset;
  	}

		if(ctx->state.zvisible)
		{
			/* DDK98: If you don't implement Z visibility testing, just do this. */
			prd->dwStatus &= ~D3DSTATUS_ZNOTVISIBLE;
			break;
		}
		
		MesaDraw3(ctx, lpIns->bOpcode, prim, vertices);
	GL_BLOCK_END

	prd->ddrval = DD_OK;
}

void __stdcall hal3d_TextureCreate32(LPD3DHAL_TEXTURECREATEDATA ptcd)
{
	TRACE_ENTRY
	
	VALIDATE(ptcd)
	
	ptcd->dwHandle = 0;
	ptcd->ddrval = DDERR_GENERIC;
	
	LPDDRAWI_DDRAWSURFACE_INT dds = (LPDDRAWI_DDRAWSURFACE_INT)ptcd->lpDDS;

	NONGL_BLOCK_BEGIN(ptcd->dwhContext)
		ddsurface_t *dd;
		if((dd = ddsurf_register(entry, dds->lpLcl, FALSE)) != NULL)
		{
			dd->lcl = dds->lpLcl;
			ptcd->dwHandle = dds->lpLcl->dwReserved1;
			ptcd->ddrval = DD_OK;
		}
	NONGL_BLOCK_END
}

void __stdcall hal3d_TextureDestroy32(LPD3DHAL_TEXTUREDESTROYDATA ptcd)
{
	TRACE_ENTRY

	VALIDATE(ptcd)
	
	ptcd->ddrval = DDERR_GENERIC;

	NONGL_BLOCK_BEGIN(ptcd->dwhContext)
		ddsurf_unregister(ctx->entry, ptcd->dwhContext, ptcd->dwHandle);
		ptcd->ddrval = DD_OK;
	NONGL_BLOCK_END
}

void __stdcall hal3d_TextureSwap32(LPD3DHAL_TEXTURESWAPDATA ptsd)
{
	TRACE_ENTRY
	
	VALIDATE(ptsd)
	
	ptsd->ddrval = DDERR_GENERIC;

	NONGL_BLOCK_BEGIN(ptsd->dwhContext)
		if(ddsurf_swap(ctx, ptsd->dwHandle1, ptsd->dwHandle2))
		{
			ptsd->ddrval = DD_OK;
		}
	NONGL_BLOCK_END
}

void __stdcall hal3d_TextureGetSurf32(LPD3DHAL_TEXTUREGETSURFDATA ptgd)
{
	TRACE_ENTRY
	
	VALIDATE(ptgd)

	ptgd->ddrval = DDERR_INVALIDPARAMS;
	
	NONGL_BLOCK_BEGIN(ptgd->dwhContext)
		ddsurface_t *dd = ddsurf_get_by_id(ctx, ptgd->dwHandle);
		if(dd)
		{
			ptgd->lpDDS  = (DWORD)dd->lcl;
			ptgd->ddrval = DD_OK;
		}
		
	NONGL_BLOCK_END
}

void __stdcall hal3d_SceneCapture32(LPD3DHAL_SCENECAPTUREDATA scdata)
{
	TRACE_ENTRY

	VALIDATE(scdata)
	
	TOPIC("GL", "SceneCapture32: %d", scdata->dwFlag);
	TOPIC("TEX", "SceneCapture32: %d", scdata->dwFlag);
	
	GL_BLOCK_BEGIN(scdata->dwhContext)
	switch(scdata->dwFlag)
	{
		case D3DHAL_SCENE_CAPTURE_START:
			MesaSceneBegin(ctx);
			break;
		case D3DHAL_SCENE_CAPTURE_END:
			MesaSceneEnd(ctx);
			break;
	}
	GL_BLOCK_END

	scdata->ddrval = DD_OK;
}

void __stdcall hal3d_ParseUnknownCommandCallback(void *data)
{
	mesa3d_entry_t *entry = Mesa3DGet(TRUE);
	if(entry)
	{
		entry->D3DParseUnknownCommand = (DWORD)data;
	}
}

static VMHAL_enviroment_t VMHALenvLocal;

void UpdateVMHALenv(VMHAL_enviroment_t *dst)
{
	if(vmhal_setup_str("hal", "forceos", FALSE) != NULL)
	{
		dst->forceos = vmhal_setup_dw("hal", "forceos") ? TRUE : FALSE; 
	}
	
	if(vmhal_setup_str("hal", "readback", FALSE) != NULL)
	{
		dst->readback = vmhal_setup_dw("hal", "readback") ? TRUE : FALSE;
	}
	
	if(vmhal_setup_str("hal", "touchdepth", FALSE) != NULL)
	{
		dst->touchdepth = vmhal_setup_dw("hal", "touchdepth") ? TRUE : FALSE;
	}
	
	if(vmhal_setup_str("hal", "lowdetail", FALSE) != NULL)
	{
		dst->lowdetail = vmhal_setup_dw("hal", "lowdetail");
	}
}

static void ReadEnv(VMHAL_enviroment_t *dst)
{
	if(vmhal_setup_str("hal", "ddi", FALSE) != NULL)
	{
		dst->ddi = vmhal_setup_dw("hal", "ddi"); 
	}
	
	if(vmhal_setup_str("hal", "hwtl", FALSE) != NULL)
	{
		dst->hwtl_ddi = vmhal_setup_dw("hal", "hwtl") ;
	}

	if(vmhal_setup_str("hal", "vertexblend", FALSE) != NULL)
	{
		dst->vertexblend = vmhal_setup_dw("hal", "vertexblend") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "palette", FALSE) != NULL)
	{
		dst->allow_palette = vmhal_setup_dw("hal", "palette") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "filter_bug", FALSE) != NULL)
	{
		dst->filter_bug = vmhal_setup_dw("hal", "filter_bug") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "s3tc_bug", FALSE) != NULL)
	{
		dst->s3tc_bug = vmhal_setup_dw("hal", "s3tc_bug") ? TRUE : FALSE;
	}

	if(vmhal_setup_str("hal", "sysmem", FALSE) != NULL)
	{
		dst->sysmem = vmhal_setup_dw("hal", "sysmem") ? TRUE : FALSE;
	}
	
	if(vmhal_setup_str("hal", "reduce_tex_units", FALSE) != NULL)
	{
		DWORD t = vmhal_setup_dw("hal", "reduce_tex_units");
		if(t == 0)
		{
			dst->texture_num_units = 8;
		}
	}
	
	UpdateVMHALenv(dst);
}

ids_proc_t *ids_procs = NULL;

void __stdcall hal3d_init(hal9x_callbacks_t *callbacks)
{
	TOPIC("INIT", "hal3d_init call");
	VMHAL_enviroment_t *glb = callbacks->env_p();
	memcpy(&VMHALenvLocal, glb, sizeof(VMHAL_enviroment_t));
	ReadEnv(&VMHALenvLocal);
	
	ids_procs = &(callbacks->ids);
	TOPIC("INIT", "hal3d_init succ");
}

BOOL GetVMHALenv(VMHAL_enviroment_t *dst)
{
	if(dst == NULL) return FALSE;

	memcpy(dst, &VMHALenvLocal, sizeof(VMHAL_enviroment_t));

	return TRUE;
}

VMHAL_enviroment_t *GlobalVMHALenv()
{
	return &VMHALenvLocal;
}

BOOL halVSync = FALSE;

BOOL FBHDA_valid_obj();

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	switch( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);
			PERF_INIT
			FBHDA_load();
			break;
		case DLL_PROCESS_DETACH:
			if(FBHDA_valid_obj())
			{
				FBHDA_free();
			}
			PERF_DUMP
			PERF_DESTROY
			break;
	}

	return TRUE;
}
