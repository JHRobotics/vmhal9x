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

#include "nocrt.h"
#endif

/* FROM ddrvmem.h */
typedef DWORD HDDRVITEM, * LPHDDRVITEM;

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
	if(dwhContext != 0)
	{
		mesa3d_ctx_t *ctx = MESA_HANDLE_TO_CTX(dwhContext);
		if(ctx->entry->pid != GetCurrentProcessId())
		{
			return FALSE;
		}
		
		VMDAHAL_t *dd = GetHAL(ctx->dd);
		if(!dd->invalid)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

#define VALIDATE(_d3d) if(!ValidateCtx((_d3d)->dwhContext)){ \
		(_d3d)->ddrval = D3DHAL_CONTEXT_BAD; \
		ERR("Invalid context");\
		return DDHAL_DRIVER_HANDLED;}

#include "d3d_caps.h"

DWORD __stdcall SetRenderTarget32(LPD3DHAL_SETRENDERTARGETDATA lpSetRenderData)
{
	TRACE_ENTRY

	VALIDATE(lpSetRenderData)

	if(lpSetRenderData->lpDDS != NULL)
	{
		TOPIC("GL", "SetRenderTarget32(0x%X, 0x%X)",
			lpSetRenderData->lpDDS,
			lpSetRenderData->lpDDSZ
		);
		
		surface_id dds_sid = 0;
		surface_id ddz_sid = 0;
		
		if(lpSetRenderData->lpDDS != NULL)
		{
			dds_sid = ((LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDS)->lpLcl->dwReserved1;
		}
		
		if(lpSetRenderData->lpDDSZ)
		{
			ddz_sid = ((LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDSZ)->lpLcl->dwReserved1;
		}
		
		if(dds_sid)
		{
			GL_BLOCK_BEGIN(lpSetRenderData->dwhContext)
				MesaSetTarget(ctx, dds_sid, ddz_sid, FALSE);
			GL_BLOCK_END

			lpSetRenderData->ddrval = DD_OK;
		}
		else
		{
			lpSetRenderData->ddrval = DDERR_INVALIDPARAMS;
			WARN("SetRenderTarget32: DDERR_INVALIDPARAMS");
		}
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall Clear32(LPD3DHAL_CLEARDATA lpClearData)
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

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawOnePrimitive32(LPD3DHAL_DRAWONEPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)
	
	GL_BLOCK_BEGIN(lpDrawData->dwhContext)
		MesaDraw(ctx, lpDrawData->PrimitiveType, lpDrawData->VertexType, lpDrawData->lpvVertices, lpDrawData->dwNumVertices);
	GL_BLOCK_END

	lpDrawData->ddrval = DD_OK;

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawOneIndexedPrimitive32(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)

	GL_BLOCK_BEGIN(lpDrawData->dwhContext)
		MesaDrawIndex(ctx, lpDrawData->PrimitiveType, lpDrawData->VertexType,
			lpDrawData->lpvVertices, lpDrawData->dwNumVertices,
			lpDrawData->lpwIndices, lpDrawData->dwNumIndices
		);
	GL_BLOCK_END

	lpDrawData->ddrval = DD_OK;

	return DDHAL_DRIVER_HANDLED;
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
DWORD __stdcall DrawPrimitives32(LPD3DHAL_DRAWPRIMITIVESDATA lpDrawData)
{
	TRACE_ENTRY
	
	VALIDATE(lpDrawData)
	
	LPBYTE lpData = (LPBYTE)lpDrawData->lpvData;
	LPD3DHAL_DRAWPRIMCOUNTS	drawPrimitiveCounts;
	int j;
	
	GL_BLOCK_BEGIN(lpDrawData->dwhContext)

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
			
			MesaDraw(ctx, drawPrimitiveCounts->wPrimitiveType, drawPrimitiveCounts->wVertexType, (LPD3DTLVERTEX)lpData, drawPrimitiveCounts->wNumVertices);
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

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ValidateTextureStageState32(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd)
{
	TRACE_ENTRY
	
	VALIDATE(lpvtssd)

	lpvtssd->dwNumPasses = 1;
	lpvtssd->ddrval = DD_OK;
	
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawPrimitives2_32(LPD3DHAL_DRAWPRIMITIVES2DATA pd)
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
		return DDHAL_DRIVER_HANDLED;
	}
	
	if(pd->dwFlags & (D3DHALDP2_REQVERTEXBUFSIZE | D3DHALDP2_REQCOMMANDBUFSIZE))
	{
		WARN("vertex buffer flags: pd->dwFlags=0x%X", pd->dwFlags);
		/*
		pd->dwErrorOffset   = 0;
		pd->ddrval          = DDERR_INVALIDPARAMS;
		return DDHAL_DRIVER_HANDLED;*/
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
	BOOL rc = FALSE;

	GL_BLOCK_BEGIN(pd->dwhContext)
		MesaFVFSet(ctx, pd->dwVertexType, pd->dwVertexSize);
		if((pd->dwVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
		{
			MesaSpaceIdentitySet(ctx);
		}
		MesaApplyLighting(ctx);
		if(!ctx->state.recording)
		{
			rc = MesaDraw6(ctx, cmdBufferStart, cmdBufferEnd, vertices, &pd->dwErrorOffset, RStates);
		}
		else
		{
			rc = MesaRecord6(ctx, cmdBufferStart, cmdBufferEnd, vertices, &pd->dwErrorOffset, RStates);
		}
		MesaSpaceIdentityReset(ctx);
	GL_BLOCK_END

	if(rc)
	{
		pd->ddrval = DD_OK;
	}
	else
	{
		pd->ddrval = D3DERR_COMMAND_UNPARSED;
	}
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

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall Clear2_32(LPD3DHAL_CLEAR2DATA cd)
{
	TRACE_ENTRY
	
	VALIDATE(cd)
	
	GL_BLOCK_BEGIN(cd->dwhContext)
		MesaClear(ctx, cd->dwFlags,
			cd->dwFillColor, cd->dvFillDepth, cd->dwFillStencil,
			cd->dwNumRects, (LPRECT)cd->lpRects);
	GL_BLOCK_END
	
	cd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

/**
 *
 * Function:    D3DGetDriverState
 *
 * Description: This callback is used by both the DirectDraw and Direct3D 
 *              runtimes to obtain information from the driver about its 
 *              current state.
 *
 * Parameters
 *
 *     lpgdsd   
 *           pointer to GetDriverState data structure
 *
 *           dwFlags
 *                   Flags to indicate the data required
 *           dwhContext
 *                   The ID of the context for which information 
 *                   is being requested
 *           lpdwStates
 *                   Pointer to the state data to be filled in by the driver
 *           dwLength
 *                   Length of the state data buffer to be filled 
 *                   in by the driver
 *           ddRVal
 *                   Return value
 *
 * Return Value
 *
 *      DDHAL_DRIVER_HANDLED
 *      DDHAL_DRIVER_NOTHANDLED
 */
DWORD __stdcall GetDriverState32(LPDDHAL_GETDRIVERSTATEDATA pGDSData)
{
	TRACE_ENTRY
	
	// Example:
	// D3DDEVINFOID_TEXTUREMANAGER
	// D3DDEVINFO_TEXTUREMANAGER
	//pGDSData->ddRVal = DD_OK;
	
	return DDHAL_DRIVER_NOTHANDLED;
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
DWORD __stdcall CreateSurfaceEx32(LPDDHAL_CREATESURFACEEXDATA lpcsxd)
{
	TRACE_ENTRY

	lpcsxd->ddRVal = DD_OK;

	if(lpcsxd->lpDDSLcl == NULL || lpcsxd->lpDDLcl == NULL)
	{
		return DDHAL_DRIVER_HANDLED;
	}

	mesa3d_entry_t *entry = Mesa3DGet(GetCurrentProcessId(), TRUE);

	if(entry == NULL)
	{
		return DDHAL_DRIVER_HANDLED;
	}

	LPDDRAWI_DDRAWSURFACE_LCL surf = lpcsxd->lpDDSLcl;

	TRACE("CreateSurfaceEx32 lpDDLcl=0x%X vidmem=0x%X", lpcsxd->lpDDLcl, surf->lpGbl->fpVidMem);

	/* this is from 3dlabs driver... */
	if(surf->lpGbl->fpVidMem == 0)
	{
		if(surf->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
		{
			// this is a system memory destroy notification
			// so go ahead free the slot for this surface if we have it
			SurfaceFree(entry, lpcsxd->lpDDLcl, surf);
		}

		// wait for DestroySurface to remove vram surface
		return DDHAL_DRIVER_HANDLED;
	}
	
	if(surf->ddsCaps.dwCaps & DX7_SURFACE_NEST_TYPES)
	{
		SurfaceExInsert(entry, lpcsxd->lpDDLcl, surf);
	
		if(!((surf->ddsCaps.dwCaps & DDSCAPS_MIPMAP) || (surf->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP)))
		{
			/* 
			 * from DDK ME:
			 * for some surfaces other than MIPMAP or CUBEMAP, such as
			 * flipping chains, we make a slot for every surface, as
			 * they are not as interleaved
			 */
			
			LPATTACHLIST curr = surf->lpAttachList;
			while(curr)
			{
				if(curr->lpAttached == surf)
				{
					break;
				}
				
				if(curr->lpAttached)
				{
					TRACE("LOOP %d", curr->lpAttached->lpSurfMore->dwSurfaceHandle);
					if(curr->lpAttached->ddsCaps.dwCaps & DX7_SURFACE_NEST_TYPES)
					{
						SurfaceExInsert(entry, lpcsxd->lpDDLcl, curr->lpAttached);
					}
					
					curr = curr->lpAttached->lpAttachList;
				}
				else
				{
					curr = NULL;
					//curr = curr->lpLink;
				}
				//curr = curr->lpLink;
				
			}
		}
	}
	else
	{
		if(surf->ddsCaps.dwCaps & DDSCAPS_RESERVED2)
		{
			TOPIC("EXEBUF", "surface 0x%X, vram=0x%X", surf->lpSurfMore->dwSurfaceHandle, surf->lpGbl->fpVidMem);
		}
		else
		{
			WARN("CreateSurfaceEx32: ignoring type 0x%X, id %d", surf->ddsCaps.dwCaps, surf->lpSurfMore->dwSurfaceHandle);
		}
	}
	
	return DDHAL_DRIVER_HANDLED;
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
DWORD __stdcall DestroyDDLocal32(LPDDHAL_DESTROYDDLOCALDATA lpdddd)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = Mesa3DGet(GetCurrentProcessId(), FALSE);
	if(entry)
	{
		MesaSurfacesTableRemoveDDLcl(entry, lpdddd->pDDLcl);
	}

	lpdddd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

static void GetDriverInfo2(DD_GETDRIVERINFO2DATA* pgdi2, LONG *lpRVal, DWORD *lpActualSize, void *lpvData)
{
	switch (pgdi2->dwType)
	{
		case D3DGDI2_TYPE_DXVERSION:
		{
			TRACE("D3DGDI2_TYPE_DXVERSION");
			// This is a way for a driver on NT to find out the DX-Runtime 
			// version. This information is provided to a new driver (i.e. 
			// one that  exposes GETDRIVERINFO2) for DX7 applications and 
			// DX8 applications. And you should get x0000800 for 
			// dwDXVersion; or more accurately, you should get
			// DD_RUNTIME_VERSION which is defined in ddrawi.h.
			DD_DXVERSION *pdxv = (DD_DXVERSION*)pgdi2;
			if(pdxv->dwDXVersion >= 0x700)
			{
				VMHALenv.dx6 = TRUE;
				VMHALenv.dx7 = TRUE;
			}

			if(pdxv->dwDXVersion >= 0x800)
				VMHALenv.dx8 = TRUE;

			if(pdxv->dwDXVersion >= 0x900)
				VMHALenv.dx9 = TRUE;

			TRACE("pdxv->dwDXVersion=0x%X", pdxv->dwDXVersion);

			*lpActualSize = sizeof(DD_DXVERSION);
			*lpRVal = DD_OK;
			break;
		}
		case D3DGDI2_TYPE_GETFORMATCOUNT:
		{
			TRACE("D3DGDI2_TYPE_GETFORMATCOUNT");
			// Its a request for the number of texture formats
			// we support. Get the extended data structure so
			// we can fill in the format count field.
			DD_GETFORMATCOUNTDATA *pgfcd = (DD_GETFORMATCOUNTDATA*)pgdi2;
			pgfcd->dwFormatCount = MYTEXTUREFORMATSDX8_COUNT;
			// Note : DX9 runtime passes in 0x900 in dwReserved
			*lpActualSize = sizeof(DD_GETFORMATCOUNTDATA);
			*lpRVal = DD_OK;
			break;
		}
		case D3DGDI2_TYPE_GETFORMAT:
		{
			TRACE("D3DGDI2_TYPE_GETFORMAT");
			// Its a request for a particular format we support.
			// Get the extended data structure so we can fill in
			// the format field.
			DD_GETFORMATDATA *pgfd = (DD_GETFORMATDATA*)pgdi2;
			if(pgfd->dwFormatIndex >= MYTEXTUREFORMATSDX8_COUNT)
			{
				*lpActualSize = 0;
				*lpRVal       = DDERR_INVALIDPARAMS;
				break;
			}
			memcpy(&pgfd->format, &myTextureFormatsDX8[pgfd->dwFormatIndex], sizeof(pgfd->format));
			*lpActualSize = sizeof(DD_GETFORMATDATA);
			*lpRVal = DD_OK;
			break;
		}
		case D3DGDI2_TYPE_GETD3DCAPS8:
		{
			TRACE("D3DGDI2_TYPE_GETD3DCAPS8");
			// The runtime is requesting the DX8 D3D caps 
			D3DCAPS8 caps;
			memset(&caps, 0, sizeof(D3DCAPS8));
			caps.DeviceType = D3DDEVTYPE_HAL;
			caps.AdapterOrdinal = 0;

			caps.Caps = DDCAPS_GDI | /* HW is shared with GDI */
				DDCAPS_BLT | /* BLT is supported */
				DDCAPS_BLTDEPTHFILL | /* depth fill */
				DDCAPS_BLTCOLORFILL | /* color fill */
				DDCAPS_BLTSTRETCH   | /* stretching blt */
				DDCAPS_COLORKEY     | /* transparentBlt */
				DDCAPS_CANBLTSYSMEM | /* from to sysmem blt */
				DDCAPS_3D           |
			0;
			caps.Caps2 = DDSCAPS2_CUBEMAP | DDCAPS2_WIDESURFACES | D3DCAPS2_CANRENDERWINDOWED; // D3DCAPS2_FULLSCREENGAMMA | D3DCAPS2_NO2DDURING3DSCENE
			caps.Caps3 = 0;
			/* JH: ^should we copy here DDCAPS_* flags or use only D3DCAPS_* flags?
			 * permedia driver do first one, from DX8 SDK doc say's the second...
			 */
			
			caps.PresentationIntervals = 0;
			caps.CursorCaps = 0;//D3DCURSORCAPS_COLOR | D3DCURSORCAPS_LOWRES;
			caps.DevCaps =
				//D3DDEVCAPS_CANBLTSYSTONONLOCAL | // Device supports blits from system-memory textures to nonlocal video-memory textures. */
				D3DDEVCAPS_CANRENDERAFTERFLIP | // Device can queue rendering commands after a page flip. Applications do not change their behavior if this flag is set; this capability simply means that the device is relatively fast.
				D3DDEVCAPS_DRAWPRIMTLVERTEX  | //Device exports a DrawPrimitive-aware hardware abstraction layer (HAL). 
				D3DDEVCAPS_EXECUTESYSTEMMEMORY | // Device can use execute buffers from system memory. 
				D3DDEVCAPS_EXECUTEVIDEOMEMORY | // Device can use execute buffers from video memory. 
				D3DDEVCAPS_HWRASTERIZATION | // Device has hardware acceleration for scene rasterization. 
				D3DDEVCAPS_HWTRANSFORMANDLIGHT | // Device can support transformation and lighting in hardware. 
				//D3DDEVCAPS_NPATCHES | // Device supports N patches. 
				//D3DDEVCAPS_PUREDEVICE | // Device can support rasterization, transform, lighting, and shading in hardware. (no need for final version of runtime)
				//D3DDEVCAPS_QUINTICRTPATCHES | // Device supports quintic béziers and B-splines. 
				//D3DDEVCAPS_RTPATCHES | // Device supports rectangular and triangular patches. 
				//D3DDEVCAPS_RTPATCHHANDLEZERO | // When this device capability is set, the hardware architecture does not require caching of any information, and uncached patches (handle zero) will be drawn as efficiently as cached ones. Note that setting D3DDEVCAPS_RTPATCHHANDLEZERO does not mean that a patch with handle zero can be drawn. A handle-zero patch can always be drawn whether this cap is set or not. 
				//D3DDEVCAPS_SEPARATETEXTUREMEMORIES | // Device is texturing from separate memory pools. 
				//D3DDEVCAPS_TEXTURENONLOCALVIDMEM | // Device can retrieve textures from non-local video memory. 
				D3DDEVCAPS_TEXTURESYSTEMMEMORY | // Device can retrieve textures from system memory. 
				D3DDEVCAPS_TEXTUREVIDEOMEMORY | // Device can retrieve textures from device memory. 
				D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | // Device can use buffers from system memory for transformed and lit vertices. 
				D3DDEVCAPS_TLVERTEXVIDEOMEMORY | // Device can use buffers from video memory for transformed and lit vertices. 
			0;
			caps.PrimitiveMiscCaps =
				D3DPMISCCAPS_BLENDOP | // Device supports the alpha-blending operations defined in the D3DBLENDOP enumerated type. 
				D3DPMISCCAPS_CLIPPLANESCALEDPOINTS | // Device correctly clips scaled points of size greater than 1.0 to user-defined clipping planes. 
				//D3DPMISCCAPS_CLIPTLVERTS | // Device clips post-transformed vertex primitives. 
				//D3DPMISCCAPS_COLORWRITEENABLE | // Device supports per-channel writes for the render target color buffer through the D3DRS_COLORWRITEENABLE state. 
				D3DPMISCCAPS_CULLCCW | // The driver supports counterclockwise culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CCW member of the D3DCULL enumerated type. 
				D3DPMISCCAPS_CULLCW | // The driver supports clockwise triangle culling through the D3DRS_CULLMODE state. (This applies only to triangle primitives.) This flag corresponds to the D3DCULL_CW member of the D3DCULL enumerated type. 
				D3DPMISCCAPS_CULLNONE | // The driver does not perform triangle culling. This corresponds to the D3DCULL_NONE member of the D3DCULL enumerated type. 
				D3DPMISCCAPS_LINEPATTERNREP | // The driver can handle values other than 1 in the wRepeatFactor member of the D3DLINEPATTERN structure. (This applies only to line-drawing primitives.) 
				D3DPMISCCAPS_MASKZ | // Device can enable and disable modification of the depth buffer on pixel operations. 
				//D3DPMISCCAPS_TSSARGTEMP | // Device supports D3DTA_TEMP for temporary register. 
			0;
			caps.RasterCaps =
				D3DPRASTERCAPS_ANISOTROPY | // Device supports anisotropic filtering. 
				D3DPRASTERCAPS_ANTIALIASEDGES | // Device can anti-alias lines forming the convex outline of objects. For more information, see D3DRS_EDGEANTIALIAS. 
				D3DPRASTERCAPS_COLORPERSPECTIVE | // Device iterates colors perspective correct. 
				D3DPRASTERCAPS_DITHER | // Device can dither to improve color resolution. 
				D3DPRASTERCAPS_FOGRANGE | // Device supports range-based fog. In range-based fog, the distance of an object from the viewer is used to compute fog effects, not the depth of the object (that is, the z-coordinate) in the scene. 
				D3DPRASTERCAPS_FOGTABLE | // Device calculates the fog value by referring to a lookup table containing fog values that are indexed to the depth of a given pixel. 
				D3DPRASTERCAPS_FOGVERTEX | // Device calculates the fog value during the lighting operation, and interpolates the fog value during rasterization. 
				D3DPRASTERCAPS_MIPMAPLODBIAS | // Device supports level-of-detail (LOD) bias adjustments. These bias adjustments enable an application to make a mipmap appear crisper or less sharp than it normally would. For more information about LOD bias in mipmaps, see D3DTSS_MIPMAPLODBIAS.
				D3DPRASTERCAPS_PAT | // The driver can perform patterned drawing lines or fills with D3DRS_LINEPATTERN for the primitive being queried. 
				// D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE | // Device provides limited multisample support through a stretch-blt implementation. When this capability is set, D3DRS_MULTISAMPLEANTIALIAS cannot be turned on and off in the middle of a scene. Multisample masking cannot be performed if this flag is set. 
				D3DPRASTERCAPS_WBUFFER | // Device supports depth buffering using w. 
				D3DPRASTERCAPS_WFOG | // Device supports w-based fog. W-based fog is used when a perspective projection matrix is specified, but affine projections still use z-based fog. The system considers a projection matrix that contains a nonzero value in the [3][4] element to be a perspective projection matrix. 
				D3DPRASTERCAPS_ZBIAS | // Device supports z-bias values. These are integer values assigned to polygons that allow physically coplanar polygons to appear separate. For more information, see D3DRS_ZBIAS. 
				// D3DPRASTERCAPS_ZBUFFERLESSHSR | // Device can perform hidden-surface removal (HSR) without requiring the application to sort polygons and without requiring the allocation of a depth-buffer. This leaves more video memory for textures. The method used to perform HSR is hardware-dependent and is transparent to the application. 
				// Z-bufferless HSR is performed if no depth-buffer surface is associated with the rendering-target surface and the depth-buffer comparison test is enabled (that is, when the state value associated with the D3DRS_ZENABLE enumeration constant is set to TRUE). 
				// D3DPRASTERCAPS_ZFOG | // Device supports z-based fog. 
				D3DPRASTERCAPS_ZTEST | // Device can perform z-test operations. This effectively renders a primitive and indicates whether any z pixels have been rendered. 
			0;
			caps.ZCmpCaps = myCaps6.dpcTriCaps.dwZCmpCaps;
			caps.SrcBlendCaps = myCaps6.dpcTriCaps.dwSrcBlendCaps;
			caps.DestBlendCaps = myCaps6.dpcTriCaps.dwDestBlendCaps;
			caps.AlphaCmpCaps = myCaps6.dpcTriCaps.dwAlphaCmpCaps;
			caps.TextureCaps  = myCaps6.dpcTriCaps.dwTextureCaps;
			caps.TextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			caps.CubeTextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			caps.VolumeTextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			caps.LineCaps = D3DLINECAPS_ALPHACMP | D3DLINECAPS_BLEND | D3DLINECAPS_FOG;
			caps.MaxTextureWidth = 16384;
			caps.MaxTextureHeight = 16384;
			caps.MaxVolumeExtent = 2048;
			caps.MaxTextureRepeat = 2048;
			caps.MaxTextureAspectRatio = 2048;
			
			/* some happy values from ref driver */
			caps.MaxVertexW      = 1.0e10;
			caps.GuardBandLeft   = -32768.0f;
			caps.GuardBandTop    = -32768.0f;
			caps.GuardBandRight  = 32767.0f;
			caps.GuardBandBottom = 32767.0f;
			caps.ExtentsAdjust   = 0.0f; //  AA kernel is 1.0 x 1.0
			caps.StencilCaps = MYSTENCIL_CAPS;
			caps.FVFCaps = 8;
			caps.TextureOpCaps = MYTEXOPCAPS;
			caps.MaxTextureBlendStages = MESA_TMU_CNT();
			caps.MaxSimultaneousTextures = MESA_TMU_CNT();
			caps.VertexProcessingCaps = MYVERTEXPROCCAPS;

			caps.MaxActiveLights = VMHALenv.num_light;
			caps.MaxUserClipPlanes = VMHALenv.num_clips;
			caps.MaxVertexBlendMatrices = 0;
			caps.MaxVertexBlendMatrixIndex = 0;
			if(VMHALenv.vertexblend)
			{
			 	caps.MaxVertexBlendMatrices = MESA_WORLDS_MAX;
			 	caps.MaxVertexBlendMatrixIndex = 3;
			}
			caps.MaxPointSize = 1.0f;
			caps.MaxPrimitiveCount = 0x000FFFFF;
			caps.MaxVertexIndex = 0x000FFFFF;
			caps.MaxStreams = 1;
			caps.MaxStreamStride = 256;
			caps.VertexShaderVersion = D3DVS_VERSION(0, 0);
			caps.MaxVertexShaderConst = 0;
			caps.PixelShaderVersion = D3DPS_VERSION(0, 0); // DX8
			caps.MaxPixelShaderValue = 0;
			
			TRACE("sizeof(D3DCAPS8) = %d, pgdi2->dwExpectedSize = %d",
				 sizeof(D3DCAPS8), pgdi2->dwExpectedSize);

			DWORD copySize = min(sizeof(D3DCAPS8), pgdi2->dwExpectedSize);
			memcpy(lpvData, &caps, copySize);
			*lpActualSize = copySize;
			*lpRVal = DD_OK;
			break;
		}
		case D3DGDI2_TYPE_GETD3DQUERYCOUNT:
			// Its a request for the number of asynchronous queries
			// we support. Get the extended data structure so
			// we can fill in the format count field.
			/* tru */
		case D3DGDI2_TYPE_GETD3DQUERY:
			// Its a request for a particular asynchronous query 
			// we support. Get the extended data structure so 
			// we can fill in the format field.
			/* tru */
		default:
			TRACE("pgdi2->dwType = %d DDERR_CURRENTLYNOTAVAIL", pgdi2->dwType);
			*lpActualSize = 0;
			*lpRVal = DDERR_CURRENTLYNOTAVAIL;
			break;
	}
}

#define COPY_INFO(_in, _s, _t) do{ \
	DWORD size = min(_in->dwExpectedSize, sizeof(_t)); \
	_in->dwActualSize = sizeof(_t); \
	memcpy(_in->lpvData, &_s, size); \
	_in->ddRVal = DD_OK; \
	}while(0)


/**
 * From DDK98:
 *  DESCRIPTION: DirectDraw has had many compatability problems
 *               in the past, particularly from adding or modifying
 *               members of public structures.  GetDriverInfo is an extension
 *               architecture that intends to allow DirectDraw to
 *               continue evolving, while maintaining backward compatability.
 *               This function is passed a GUID which represents some DirectDraw
 *               extension.  If the driver recognises and supports this extension,
 *               it fills out the required data and returns.
 *
 **/ 
DWORD __stdcall GetDriverInfo32(LPDDHAL_GETDRIVERINFODATA lpInput)
{
	TRACE_ENTRY

#ifdef DEBUG
	SetExceptionHandler();
#endif

	lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;

/*	TRACE("GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
			lpInput->guidInfo.Data1,
			lpInput->guidInfo.Data2,
			lpInput->guidInfo.Data3,
			lpInput->guidInfo.Data4[0],
			lpInput->guidInfo.Data4[1],
			lpInput->guidInfo.Data4[2],
			lpInput->guidInfo.Data4[3],
			lpInput->guidInfo.Data4[4],
			lpInput->guidInfo.Data4[5],
			lpInput->guidInfo.Data4[6],
			lpInput->guidInfo.Data4[7]
		);*/

	if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DCallbacks2))
	{
		/* DX5 */
		D3DHAL_CALLBACKS2 D3DCallbacks2;
		memset(&D3DCallbacks2, 0, sizeof(D3DHAL_CALLBACKS2));

		D3DCallbacks2.dwSize = sizeof(D3DHAL_CALLBACKS2);
		D3DCallbacks2.dwFlags = D3DHAL2_CB32_SETRENDERTARGET | D3DHAL2_CB32_DRAWONEPRIMITIVE | D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE | D3DHAL2_CB32_DRAWPRIMITIVES;
		D3DCallbacks2.SetRenderTarget = SetRenderTarget32;
		D3DCallbacks2.DrawOnePrimitive = DrawOnePrimitive32;
		D3DCallbacks2.DrawOneIndexedPrimitive = DrawOneIndexedPrimitive32;
		D3DCallbacks2.DrawPrimitives = DrawPrimitives32;

		D3DCallbacks2.Clear = Clear32;
		D3DCallbacks2.dwFlags |= D3DHAL2_CB32_CLEAR;

		COPY_INFO(lpInput, D3DCallbacks2, D3DHAL_CALLBACKS2);
		TRACE("GUID_D3DCallbacks2 success");
	}
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DCallbacks3) && VMHALenv.ddi >= 6)
	{
		/* DX6 */
		D3DHAL_CALLBACKS3 D3DCallbacks3;
  	memset(&D3DCallbacks3, 0, sizeof(D3DHAL_CALLBACKS3));

		D3DCallbacks3.dwSize = sizeof(D3DHAL_CALLBACKS3);
		D3DCallbacks3.dwFlags = D3DHAL3_CB32_DRAWPRIMITIVES2 | D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE;
    D3DCallbacks3.ValidateTextureStageState = ValidateTextureStageState32;
		D3DCallbacks3.DrawPrimitives2           = DrawPrimitives2_32;
		
		// optional, but needed for stencil support
		D3DCallbacks3.Clear2                    = Clear2_32;
		D3DCallbacks3.dwFlags                  |= D3DHAL3_CB32_CLEAR2;

		VMHALenv.dx6 = TRUE;

  	COPY_INFO(lpInput, D3DCallbacks3, D3DHAL_CALLBACKS3);
    TRACE("GUID_D3DCallbacks3 success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_MiscellaneousCallbacks))
	{
		/* DX5 */
		DDHAL_DDMISCELLANEOUSCALLBACKS misccb;
		memset(&misccb, 0, sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS));
		misccb.dwSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
		misccb.dwFlags = DDHAL_MISCCB32_GETSYSMEMBLTSTATUS;
		misccb.GetSysmemBltStatus = GetBltStatus32;
		
		COPY_INFO(lpInput, misccb, DDHAL_DDMISCELLANEOUSCALLBACKS);
		TRACE("GUID_MiscellaneousCallbacks success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_Miscellaneous2Callbacks) && VMHALenv.ddi >= 7)
	{
		/* DX7 */
		DDHAL_DDMISCELLANEOUS2CALLBACKS misccb2;
		memset(&misccb2, 0, sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS));
		misccb2.dwSize = sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS);
		misccb2.dwFlags = DDHAL_MISC2CB32_CREATESURFACEEX 
			| DDHAL_MISC2CB32_GETDRIVERSTATE
			| DDHAL_MISC2CB32_DESTROYDDLOCAL;

		misccb2.GetDriverState  = GetDriverState32;
		misccb2.CreateSurfaceEx = CreateSurfaceEx32;
		misccb2.DestroyDDLocal  = DestroyDDLocal32;

		VMHALenv.dx7 = TRUE;
		VMHALenv.dx6 = TRUE;
		
		COPY_INFO(lpInput, misccb2, DDHAL_DDMISCELLANEOUS2CALLBACKS);
		TRACE("GUID_Miscellaneous2Callbacks success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_DDMoreSurfaceCaps))
	{
		DDMORESURFACECAPS DDMoreSurfaceCaps;
		DDSCAPSEX emptycaps[2];
		DWORD i;
		BYTE *ptr = lpInput->lpvData;

		memset(&DDMoreSurfaceCaps, 0, sizeof(DDMORESURFACECAPS));
		memset(&emptycaps[0], 0, sizeof(DDSCAPSEX)*2);

		if(lpInput->dwExpectedSize < sizeof(DDMORESURFACECAPS))
		{
			return DDHAL_DRIVER_HANDLED;
		}

		DWORD extra_heaps = (lpInput->dwExpectedSize - sizeof(DDMORESURFACECAPS)) / (sizeof(DDSCAPSEX)*2);
		DDMoreSurfaceCaps.dwSize = sizeof(DDMORESURFACECAPS) + extra_heaps * sizeof(DDSCAPSEX) * 2;

		memcpy(ptr, &DDMoreSurfaceCaps, sizeof(DDMORESURFACECAPS));
		ptr += sizeof(DDMORESURFACECAPS);
		for(i = 0; i < extra_heaps; i++)
		{
			memcpy(ptr, &emptycaps[0], sizeof(DDSCAPSEX)*2);
			ptr += sizeof(DDSCAPSEX)*2;
		}

		lpInput->dwActualSize = DDMoreSurfaceCaps.dwSize;
		lpInput->ddRVal = DD_OK;

		TRACE("lpInput->dwExpectedSize=%d, lpInput->dwActualSize = %d",
			lpInput->dwExpectedSize, lpInput->dwActualSize);

		TRACE("GUID_DDMoreSurfaceCaps success");
	}
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DExtendedCaps))
	{
		D3DHAL_D3DEXTENDEDCAPS7 dxcaps;

		memset(&dxcaps, 0, sizeof(D3DHAL_D3DEXTENDEDCAPS7));
		dxcaps.dwMinTextureWidth  = 1;
		dxcaps.dwMinTextureHeight = 1;
		dxcaps.dwMaxTextureWidth  = VMHALenv.texture_max_width;
		dxcaps.dwMaxTextureHeight = VMHALenv.texture_max_height;
		dxcaps.dwMinStippleWidth  = 32;
		dxcaps.dwMinStippleHeight = 32;
		dxcaps.dwMaxStippleWidth  = 32;
		dxcaps.dwMaxStippleHeight = 32;

		dxcaps.dwFVFCaps                   = MESA_TMU_CNT(); /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    dxcaps.wMaxTextureBlendStages      = MESA_TMU_CNT();
    dxcaps.wMaxSimultaneousTextures    = MESA_TMU_CNT();
    dxcaps.dwMaxTextureRepeat          = 2048;
    dxcaps.dwTextureOpCaps = MYTEXOPCAPS;
  	dxcaps.wMaxTextureBlendStages      = MESA_TMU_CNT();
    dxcaps.wMaxSimultaneousTextures    = MESA_TMU_CNT();

		// this need also Clear2 callback
		dxcaps.dwStencilCaps = MYSTENCIL_CAPS;

		/* some happy values from ref driver */
		dxcaps.dvGuardBandLeft   = -32768.0f;
		dxcaps.dvGuardBandTop    = -32768.0f;
		dxcaps.dvGuardBandRight  = 32767.0f;
		dxcaps.dvGuardBandBottom = 32767.0f;
		dxcaps.dvExtentsAdjust = 0.0f; //  AA kernel is 1.0 x 1.0
		dxcaps.dvMaxVertexW = 1.0e10;

		/*
    T&L:
    dxcaps.dwMaxActiveLights = 0;
    */
    if(VMHALenv.hw_tl && VMHALenv.ddi >= 7)
  	{
			dxcaps.dwMaxActiveLights = VMHALenv.num_light;
			dxcaps.wMaxUserClipPlanes = VMHALenv.num_clips; // ref driver = 6
			dxcaps.wMaxVertexBlendMatrices = 0;

			if(VMHALenv.vertexblend)
				dxcaps.wMaxVertexBlendMatrices = MESA_WORLDS_MAX;
				//^ this need GL_ARB_vertex_blend or some extra CPU power

			dxcaps.dwVertexProcessingCaps = MYVERTEXPROCCAPS;
		}

		if(VMHALenv.max_anisotropy > 1)
		{
			dxcaps.dwMaxAnisotropy = VMHALenv.max_anisotropy;
		}

		TRACE("lpInput->dwExpectedSize=%d, D3DHAL_D3DEXTENDEDCAPS5=%d, D3DHAL_D3DEXTENDEDCAPS6=%d, D3DHAL_D3DEXTENDEDCAPS7=%d",
			lpInput->dwExpectedSize,
			sizeof(D3DHAL_D3DEXTENDEDCAPS5),
			sizeof(D3DHAL_D3DEXTENDEDCAPS6),
			sizeof(D3DHAL_D3DEXTENDEDCAPS7));

		if(VMHALenv.ddi <= 5)
		{
			dxcaps.dwSize = sizeof(D3DHAL_D3DEXTENDEDCAPS5);
			COPY_INFO(lpInput, dxcaps, D3DHAL_D3DEXTENDEDCAPS5);
		}
		else if(VMHALenv.ddi <= 6)
		{
			dxcaps.dwSize = sizeof(D3DHAL_D3DEXTENDEDCAPS6);
			COPY_INFO(lpInput, dxcaps, D3DHAL_D3DEXTENDEDCAPS6);
		}
		else
		{
			dxcaps.dwSize = sizeof(D3DHAL_D3DEXTENDEDCAPS7);
			COPY_INFO(lpInput, dxcaps, D3DHAL_D3DEXTENDEDCAPS7);
		}

		TRACE("GUID_D3DExtendedCaps success");
	}
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_ZPixelFormats) && VMHALenv.ddi >= 6)
	{
#pragma pack(push)
#pragma pack(1)
		struct {
			DWORD cnt;
			DDPIXELFORMAT pixfmt[8];
		} zformats;
#pragma pack(pop)
		int i = 0;

		memset(&zformats, 0, sizeof(zformats));

		zformats.pixfmt[i].dwSize             = sizeof(DDPIXELFORMAT);
		zformats.pixfmt[i].dwFlags            = DDPF_ZBUFFER;
		zformats.pixfmt[i].dwFourCC           = 0;
		zformats.pixfmt[i].dwZBufferBitDepth  = 16;
		zformats.pixfmt[i].dwStencilBitDepth  = 0;
		zformats.pixfmt[i].dwZBitMask         = 0xFFFF;
		zformats.pixfmt[i].dwStencilBitMask   = 0x0000;
		zformats.pixfmt[i].dwRGBZBitMask      = 0;
		i++;

		zformats.pixfmt[i].dwSize             = sizeof(DDPIXELFORMAT);
		zformats.pixfmt[i].dwFlags            = DDPF_ZBUFFER;
		zformats.pixfmt[i].dwFourCC           = 0;
		zformats.pixfmt[i].dwZBufferBitDepth  = 24;
		zformats.pixfmt[i].dwStencilBitDepth  = 0;
		zformats.pixfmt[i].dwZBitMask         = 0x00FFFFFF;
		zformats.pixfmt[i].dwStencilBitMask   = 0;
		zformats.pixfmt[i].dwRGBZBitMask      = 0;
		i++;

		zformats.pixfmt[i].dwSize             = sizeof(DDPIXELFORMAT);
		zformats.pixfmt[i].dwFlags            = DDPF_ZBUFFER;
		zformats.pixfmt[i].dwFourCC           = 0;
		zformats.pixfmt[i].dwZBufferBitDepth  = 32;
		zformats.pixfmt[i].dwStencilBitDepth  = 0;
		zformats.pixfmt[i].dwZBitMask         = 0xFFFFFFFF;
		zformats.pixfmt[i].dwStencilBitMask   = 0x00000000;
		zformats.pixfmt[i].dwRGBZBitMask      = 0;
		i++;

		zformats.pixfmt[i].dwSize             = sizeof(DDPIXELFORMAT);
		zformats.pixfmt[i].dwFlags            = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
		zformats.pixfmt[i].dwFourCC           = 0;
		zformats.pixfmt[i].dwZBufferBitDepth  = 32; // The sum of the z buffer bit depth AND the stencil depth 
		zformats.pixfmt[i].dwStencilBitDepth  = 8;
		zformats.pixfmt[i].dwZBitMask         = 0xFFFFFF00;
		zformats.pixfmt[i].dwStencilBitMask   = 0x000000FF;
		zformats.pixfmt[i].dwRGBZBitMask      = 0;
		i++;

		zformats.cnt = i;
		DWORD real_size = sizeof(DWORD) + sizeof(DDPIXELFORMAT)*zformats.cnt;
		lpInput->dwActualSize = real_size;
		memcpy(lpInput->lpvData, &zformats, min(lpInput->dwExpectedSize, real_size));

		lpInput->ddRVal = DD_OK;
		TRACE("GUID_ZPixelFormats success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_D3DParseUnknownCommandCallback) && VMHALenv.ddi >= 6) 
	{
		mesa3d_entry_t *entry = Mesa3DGet(GetCurrentProcessId(), TRUE);
		if(entry)
		{
			entry->D3DParseUnknownCommand = (DWORD)(lpInput->lpvData);
			lpInput->ddRVal = DD_OK;
			TRACE("GUID_D3DParseUnknownCommandCallback loaded");
		}
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_NonLocalVidMemCaps) && VMHALenv.ddi >= 6)
	{
		DDNONLOCALVIDMEMCAPS DDNonLocalVidMemCaps;
		memset(&DDNonLocalVidMemCaps, 0, sizeof(DDNonLocalVidMemCaps));
		DDNonLocalVidMemCaps.dwSize = sizeof(DDNONLOCALVIDMEMCAPS);

		TRACE("lpInput->dwExpectedSize=%d", lpInput->dwExpectedSize);

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, DDNonLocalVidMemCaps, DDNONLOCALVIDMEMCAPS);
		TRACE("GUID_NonLocalVidMemCaps set");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_VideoPortCallbacks))
	{
		DDHAL_DDVIDEOPORTCALLBACKS vpCB;
		memset(&vpCB, 0, sizeof(DDHAL_DDVIDEOPORTCALLBACKS));
		vpCB.dwSize = sizeof(DDHAL_DDVIDEOPORTCALLBACKS);

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, vpCB, DDHAL_DDVIDEOPORTCALLBACKS);
		TRACE("GUID_VideoPortCallbacks zero");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_ColorControlCallbacks))
	{
		DDHAL_DDCOLORCONTROLCALLBACKS ccCB;
		memset(&ccCB, 0, sizeof(DDHAL_DDCOLORCONTROLCALLBACKS));
		ccCB.dwSize = sizeof(DDHAL_DDCOLORCONTROLCALLBACKS);

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, ccCB, DDHAL_DDCOLORCONTROLCALLBACKS);
		TRACE("GUID_ColorControlCallbacks zero");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_VideoPortCaps))
	{
		DDVIDEOPORTCAPS VideoPortCaps;
		memset(&VideoPortCaps, 0, sizeof(DDVIDEOPORTCAPS));
		VideoPortCaps.dwSize = sizeof(DDVIDEOPORTCAPS);

		if(lpInput->dwExpectedSize != sizeof(DDVIDEOPORTCAPS))
			return DDHAL_DRIVER_HANDLED;

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, VideoPortCaps, DDVIDEOPORTCAPS);
		TRACE("GUID_VideoPortCaps zero");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_KernelCallbacks))
	{
		DDHAL_DDKERNELCALLBACKS kCB;
		memset(&kCB, 0, sizeof(DDHAL_DDKERNELCALLBACKS));
		kCB.dwSize = sizeof(DDHAL_DDKERNELCALLBACKS);

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, kCB, DDHAL_DDKERNELCALLBACKS);
		TRACE("GUID_KernelCallbacks zero");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_KernelCaps))
	{
		DDKERNELCAPS kcaps;
		memset(&kcaps, 0, sizeof(DDKERNELCAPS));
		kcaps.dwSize = sizeof(DDKERNELCAPS);

		if(lpInput->dwExpectedSize != sizeof(DDKERNELCAPS))
			return DDHAL_DRIVER_HANDLED;

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, kcaps, DDKERNELCAPS);
		TRACE("GUID_KernelCaps zero");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_MotionCompCallbacks))
	{
		DD_MOTIONCOMPCALLBACKS mcc;
		memset(&mcc, 0, sizeof(DD_MOTIONCOMPCALLBACKS));
		mcc.dwSize = sizeof(DD_MOTIONCOMPCALLBACKS);

		if(lpInput->dwExpectedSize != sizeof(DD_MOTIONCOMPCALLBACKS))
			return DDHAL_DRIVER_HANDLED;

		lpInput->ddRVal = DD_OK;
		COPY_INFO(lpInput, mcc, DD_MOTIONCOMPCALLBACKS);
		TRACE("GUID_MotionCompCallbacks zero");	
	}
	// NOTE: GUID_GetDriverInfo2 has the same value as GUID_DDStereoMode
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_GetDriverInfo2) && VMHALenv.ddi >= 8) /*  */
	{
		if(((DD_GETDRIVERINFO2DATA*)(lpInput->lpvData))->dwMagic == D3DGDI2_MAGIC) /* GUID_GetDriverInfo2 */
		{
			DD_GETDRIVERINFO2DATA* pgdi2 = lpInput->lpvData;
			GetDriverInfo2(pgdi2, &lpInput->ddRVal, &lpInput->dwExpectedSize, lpInput->lpvData);
			lpInput->dwActualSize = lpInput->dwExpectedSize;
			TRACE("GUID_GetDriverInfo2 success %d %d", lpInput->dwExpectedSize, lpInput->dwActualSize);
		}
		else /* GUID_DDStereoMode */
		{
			lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;
			TRACE("GUID_DDStereoMode ignored");
		}
	}
	else
	{
		TRACE("Not handled GUID: %08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
			lpInput->guidInfo.Data1,
			lpInput->guidInfo.Data2,
			lpInput->guidInfo.Data3,
			lpInput->guidInfo.Data4[0],
			lpInput->guidInfo.Data4[1],
			lpInput->guidInfo.Data4[2],
			lpInput->guidInfo.Data4[3],
			lpInput->guidInfo.Data4[4],
			lpInput->guidInfo.Data4[5],
			lpInput->guidInfo.Data4[6],
			lpInput->guidInfo.Data4[7]
		);
		lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;
	}
	/* also can be set:
		CLSID_DirectDraw
		
	 */
	
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextCreate32(LPD3DHAL_CONTEXTCREATEDATA pccd)
{
	TRACE_ENTRY

	pccd->ddrval = D3DHAL_OUTOFCONTEXTS; /* error state */

	mesa3d_entry_t *entry = Mesa3DGet(pccd->dwPID, TRUE);
	if(entry)
	{
		mesa3d_ctx_t *ctx = NULL;
		
		entry->runtime_ver = 5;
		if(VMHALenv.dx6 && VMHALenv.ddi >= 6)
		{
			entry->runtime_ver = 6;
		}

		if(VMHALenv.dx7 && VMHALenv.ddi >= 7)
		{
			entry->runtime_ver = 7;
		}
		
		surface_id dds_sid = 0;
		surface_id ddz_sid = 0;

		if(entry->runtime_ver >= 7)
		{
			TRACE("ContextCreate32 DX7+");
			if(pccd->lpDDSLcl)
			{
				dds_sid = pccd->lpDDSLcl->dwReserved1;
			}

			if(pccd->lpDDSZLcl)
			{
				ddz_sid = pccd->lpDDSZLcl->dwReserved1;
			}
			
			if(dds_sid)
			{
				ctx = MesaCreateCtx(entry, dds_sid,  ddz_sid);
				ctx->surfaces = MesaSurfacesTableGet(entry, pccd->lpDDLcl, SURFACES_TABLE_POOL-1);
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
			SurfaceAttachCtx(ctx);
			pccd->dwhContext = MESA_CTX_TO_HANDLE(ctx);
			pccd->ddrval = DD_OK;

			if(entry->runtime_ver >= 7)
				ctx->dd = NULL;
			else
				ctx->dd = pccd->lpDDGbl;
		}
	}

	if(pccd->ddrval != DD_OK || pccd->dwhContext == 0)
	{
		ERR("ContextCreate32 FAILED");
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextDestroy32(LPD3DHAL_CONTEXTDESTROYDATA pcdd)
{
	TRACE_ENTRY

	mesa3d_ctx_t *ctx = MESA_HANDLE_TO_CTX(pcdd->dwhContext);
	SurfaceDeattachCtx(ctx);
	MesaDestroyCtx(ctx);	

	pcdd->dwhContext = 0;

	pcdd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextDestroyAllCallback32(LPVOID lpData, HDDRVITEM hItem, DWORD dwData)
{
	TRACE_ENTRY
	
	return DDRV_SUCCESS_CONTINUE; // FIXME: check documentation
}

DWORD __stdcall ContextDestroyAll32(LPD3DHAL_CONTEXTDESTROYALLDATA pcdd)
{
	TRACE_ENTRY
	
	pcdd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall RenderState32(LPD3DHAL_RENDERSTATEDATA prd)
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
	GL_BLOCK_END
	
	prd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall RenderPrimitive32(LPD3DHAL_RENDERPRIMITIVEDATA prd)
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
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall TextureCreate32(LPD3DHAL_TEXTURECREATEDATA ptcd)
{
	TRACE_ENTRY
	
	VALIDATE(ptcd)
	
	surface_id sid = 0;
	
	ptcd->ddrval = DDERR_OUTOFVIDEOMEMORY;
	LPDDRAWI_DDRAWSURFACE_INT dds = (LPDDRAWI_DDRAWSURFACE_INT)ptcd->lpDDS;
	if(dds && dds->lpLcl)
	{
		sid = dds->lpLcl->dwReserved1;
	}

	if(sid)
	{
		GL_BLOCK_BEGIN(ptcd->dwhContext)
			mesa3d_texture_t *tex = MesaCreateTexture(ctx, sid);
			if(tex)
			{
				ptcd->dwHandle = MESA_TEX_TO_HANDLE(tex);
				TRACE("new texture: %X", ptcd->dwHandle);
				ptcd->ddrval = DD_OK;
			}
			else
			{
				/* set NULL handle */
				ptcd->dwHandle = 0;
				ptcd->ddrval = DDERR_GENERIC;
			}
		GL_BLOCK_END
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall TextureDestroy32(LPD3DHAL_TEXTUREDESTROYDATA ptcd)
{
	TRACE_ENTRY

	VALIDATE(ptcd)
	
	if(!ptcd->dwHandle)
	{
		ptcd->ddrval = DDERR_GENERIC;
		return DDHAL_DRIVER_HANDLED;
	}

	GL_BLOCK_BEGIN(ptcd->dwhContext)
		MesaDestroyTexture(MESA_HANDLE_TO_TEX(ptcd->dwHandle), FALSE, 0);
	GL_BLOCK_END
	
	ptcd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall TextureSwap32(LPD3DHAL_TEXTURESWAPDATA ptsd)
{
	TRACE_ENTRY
	
	VALIDATE(ptsd)
	
	mesa3d_texture_t *tex1 = MESA_HANDLE_TO_TEX(ptsd->dwHandle1);
	mesa3d_texture_t *tex2 = MESA_HANDLE_TO_TEX(ptsd->dwHandle2);
	
	if(tex1 && tex2)
	{
		mesa3d_texture_t cp;
		memcpy(&cp, tex2, sizeof(mesa3d_texture_t));
		memcpy(tex2, tex1, sizeof(mesa3d_texture_t));
		memcpy(tex1, &cp, sizeof(mesa3d_texture_t));
		ptsd->ddrval = DD_OK;
	}
	else
	{
		WARN("TextureSwap32: DDERR_INVALIDPARAMS");
		ptsd->ddrval = DDERR_INVALIDPARAMS;
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall TextureGetSurf32(LPD3DHAL_TEXTUREGETSURFDATA ptgd)
{
	TRACE_ENTRY
	
	VALIDATE(ptgd)
	
	mesa3d_texture_t *tex = MESA_HANDLE_TO_TEX(ptgd->dwHandle);
	if(tex)
	{
		ptgd->lpDDS  = (DWORD)SurfaceGetLCL(tex->data_sid[0][0]);
		ptgd->ddrval = DD_OK;
	}
	else
	{
		WARN("TextureGetSurf32: DDERR_INVALIDPARAMS");
		ptgd->ddrval = DDERR_INVALIDPARAMS;
	}

	return DDHAL_DRIVER_HANDLED;
}

#if 0
/* Templates for DDI 3, not ever called! */
#define GL_MATRIX_SIZE sizeof(GLfloat[16])
DWORD __stdcall MatrixCreate32(LPD3DHAL_MATRIXCREATEDATA pmcd)
{
	TRACE_ENTRY
	
	GLfloat *ptr = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, GL_MATRIX_SIZE);
	if(ptr)
	{
		pmcd->ddrval = DD_OK;
		pmcd->dwHandle = MESA_MTX_TO_HANDLE(ptr);
	}
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MatrixDestroy32(LPD3DHAL_MATRIXDESTROYDATA pmdd)
{
	TRACE_ENTRY
	
	GLfloat *ptr = MESA_HANDLE_TO_MTX(pmdd->dwHandle);
	if(ptr)
	{
		pmdd->ddrval = DD_OK;
		HeapFree(hSharedHeap, 0, ptr);
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MatrixSetData32(LPD3DHAL_MATRIXSETDATADATA pmsd)
{
	TRACE_ENTRY
	
	GLfloat *ptr = MESA_HANDLE_TO_MTX(pmsd->dwHandle);
	if(ptr)
	{
		memcpy(ptr, &pmsd->dmMatrix._11, GL_MATRIX_SIZE);
		
		pmsd->ddrval = DD_OK;
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MatrixGetData32(LPD3DHAL_MATRIXGETDATADATA pmsd)
{
	TRACE_ENTRY
	
	GLfloat *ptr = MESA_HANDLE_TO_MTX(pmsd->dwHandle);
	if(ptr)
	{
		memcpy(&pmsd->dmMatrix._11, ptr, GL_MATRIX_SIZE);
		
		pmsd->ddrval = DD_OK;
	}
	
	return DDHAL_DRIVER_HANDLED;

}

DWORD __stdcall SetViewportData32(LPD3DHAL_SETVIEWPORTDATADATA psvd)
{
	TRACE_ENTRY
	
	psvd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MaterialCreate32(LPD3DHAL_MATERIALCREATEDATA pmcd)
{
	TRACE_ENTRY
	
	pmcd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MaterialDestroy32(LPD3DHAL_MATERIALDESTROYDATA pmdd)
{
	TRACE_ENTRY
	
	pmdd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MaterialSetData32(LPD3DHAL_MATERIALSETDATADATA pmsd)
{
	TRACE_ENTRY
	
	pmsd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall MaterialGetData32(LPD3DHAL_MATERIALGETDATADATA pmgd)
{
	TRACE_ENTRY
	
	pmgd->ddrval = DD_OK;
	return (DDHAL_DRIVER_HANDLED);
}
#endif /* unused templates */

DWORD __stdcall GetState32(LPD3DHAL_GETSTATEDATA pgsd)
{
	TRACE_ENTRY
	
	VALIDATE(pgsd)

	if (pgsd->dwWhich != D3DHALSTATE_GET_RENDER)
	{
		// You must be able to do transform/lighting
	}
	// JH: ^this is REAL code from S3 driver including the comment

	pgsd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall SceneCapture32(LPD3DHAL_SCENECAPTUREDATA scdata)
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
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall CanCreateExecuteBuffer32(LPDDHAL_CANCREATESURFACEDATA csd)
{
	TRACE_ENTRY
	TOPIC("EXEBUF", "CanCreateExecuteBuffer32");

	/* asume we can create the buffer every time */

	csd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall CreateExecuteBuffer32(LPDDHAL_CREATESURFACEDATA csd)
{
	TRACE_ENTRY
	TOPIC("EXEBUF", "CreateExecuteBuffer32");
	
	int i;
	LPDDRAWI_DDRAWSURFACE_LCL *lplpSList = csd->lplpSList;

	for(i = 0; i < (int)csd->dwSCnt; i++)
	{
		LPDDRAWI_DDRAWSURFACE_LCL surf = lplpSList[i];
		TOPIC("EXEBUF", "CreateExecuteBuffer32 dwLinearSize = %d", surf->lpGbl->dwLinearSize);

		if(surf->lpGbl->dwLinearSize < 1*1024*1024)
		{
			surf->lpGbl->dwLinearSize = 1*1024*1024;
		}

		surf->lpGbl->fpVidMem = (DWORD)HeapAlloc(hSharedLargeHeap, HEAP_ZERO_MEMORY, surf->lpGbl->dwLinearSize);
		if(surf->lpGbl->fpVidMem == 0)
		{
			csd->ddRVal = DDERR_OUTOFVIDEOMEMORY;
			return DDHAL_DRIVER_HANDLED;
		}
	}

	csd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DestroyExecuteBuffer32(LPDDHAL_DESTROYSURFACEDATA dsd)
{
	TRACE_ENTRY
	TOPIC("EXEBUF", "DestroyExecuteBuffer32");

	if(dsd->lpDDSurface->lpGbl->fpVidMem != 0)
	{
		HeapFree(hSharedLargeHeap, 0, (void*)dsd->lpDDSurface->lpGbl->fpVidMem);
		dsd->lpDDSurface->lpGbl->fpVidMem = 0;
	}

	dsd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall LockExecuteBuffer32(LPDDHAL_LOCKDATA lock)
{
	TRACE_ENTRY
	TOPIC("EXEBUF", "LockExecuteBuffer32");
	/* nop */

	return DDHAL_DRIVER_NOTHANDLED; /* let the lock processed */
}

DWORD __stdcall UnlockExecuteBuffer32(LPDDHAL_UNLOCKDATA lock)
{
	TRACE_ENTRY
	TOPIC("EXEBUF", "UnlockExecuteBuffer32");
	/* nop */

	return DDHAL_DRIVER_NOTHANDLED; /* let the unlock processed */
}


/* GLOBAL hal */
static D3DHAL_GLOBALDRIVERDATA myGlobalD3DHal;

static D3DHAL_CALLBACKS myD3DHALCallbacks = {
	sizeof(D3DHAL_CALLBACKS),
    // Device context
	ContextCreate32,            // Required.
	ContextDestroy32,           // Required.
	ContextDestroyAll32,	// Required.

	// Scene capture
	SceneCapture32,			// Optional. (JH: required when driver or HW do some buffering. ... JH #2: Not work for DX6+)
	// Execution
#ifdef IMPLEMENT_EXECUTE
	Execute32,
#else
	NULL,			// Optional.  Don't implement if just rasterization.
#endif
	NULL,
	RenderState32,		// Required if no Execute
	RenderPrimitive32,		// Required if no Execute
	0L,				// Reserved, must be zero

	// Textures
	TextureCreate32,            // If any of these calls are supported,
	TextureDestroy32,           // they must all be.
	TextureSwap32,              // ditto - but can always fail.
	TextureGetSurf32,           // ditto - but can always fail.
	
	/* For next exports is important this DDK98s note:
	 * 	No driver currently exports this function and no run-time file ever call it. 
	 * 	Future Direct3D exports will be done through the GetDriverInfo mechanism.
	 */
	// Transform - must be supported if lighting is supported.
	NULL, //MatrixCreate32,             // If any of these calls are supported,
	NULL, //MatrixDestroy32,            // they must all be.
	NULL, //MatrixSetData32,            // ditto
	NULL, //MatrixGetData32,            // ditto
	NULL, //SetViewportData32,          // ditto

	// Lighting
	NULL,                       // If any of these calls are supported,
	NULL, //myMaterialCreate,           // they must all be.
	NULL, //myMaterialDestroy,          // ditto
	NULL, //myMaterialSetData,          // ditto
	NULL, //myMaterialGetData,          // ditto

	// Pipeline state
	GetState32,			// Required if implementing Execute.

	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
	0L,				// Reserved, must be zero
};

static DDHAL_DDEXEBUFCALLBACKS myD3DHALExeBufCallbacks = {
	sizeof(DDHAL_DDEXEBUFCALLBACKS),
	0, /* flags, auto sets in DRV */
	CanCreateExecuteBuffer32, /* CanCreateExecuteBuffer */
	CreateExecuteBuffer32,    /* CreateExecuteBuffer */
	DestroyExecuteBuffer32,   /* DestroyExecuteBuffer */
	LockExecuteBuffer32,      /* LockExecuteBuffer */
	UnlockExecuteBuffer32     /* UnlockExecuteBuffer */
};

static D3DHAL_CALLBACKS myD3DHALCallbacks7;

BOOL __stdcall D3DHALCreateDriver(DWORD *lplpGlobal, DWORD *lplpHALCallbacks, LPDDHAL_DDEXEBUFCALLBACKS lpHALExeBufCallbacks, VMDAHAL_D3DCAPS_t *lpHALFlags)
{
	memset(&myGlobalD3DHal, 0, sizeof(D3DHAL_GLOBALDRIVERDATA));
	myGlobalD3DHal.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);
	myGlobalD3DHal.hwCaps = myCaps;
	myGlobalD3DHal.dwNumVertices = 0;
	myGlobalD3DHal.dwNumClipVertices = 0;
	myGlobalD3DHal.dwNumTextureFormats = (sizeof(myTextureFormats) / sizeof(DDSURFACEDESC));
	myGlobalD3DHal.lpTextureFormats = &myTextureFormats[0];
	
	if(VMHALenv.ddi >= 7)
	{
		myGlobalD3DHal.hwCaps = myCaps6;
		if(VMHALenv.ddi < 8)
		{
			myGlobalD3DHal.hwCaps.dwDevCaps &= ~(CAPS_DX8);
		}
		
		if(VMHALenv.hw_tl)
		{
			myGlobalD3DHal.hwCaps.dwDevCaps |= 
				D3DDEVCAPS_HWTRANSFORMANDLIGHT /* Device can support transformation and lighting in hardware and DRAWPRIMITIVES2EX must be also */
				/*| D3DDEVCAPS_CANBLTSYSTONONLOCAL*/ /* Device supports a Tex Blt from system memory to non-local vidmem */
				| D3DDEVCAPS_HWRASTERIZATION; /* Device has HW acceleration for rasterization */
		}
	}
	else if(VMHALenv.ddi >= 6)
	{
		myGlobalD3DHal.hwCaps = myCaps6;
		myGlobalD3DHal.hwCaps.dwDevCaps &= ~(CAPS_DX7|CAPS_DX8);
	}

	*lplpGlobal = (DWORD)&myGlobalD3DHal;
	*lplpHALCallbacks = (DWORD)&myD3DHALCallbacks;
	
	if(VMHALenv.ddi >= 7)
	{
		memcpy(&myD3DHALCallbacks7, &myD3DHALCallbacks, sizeof(myD3DHALCallbacks));
		myD3DHALCallbacks7.SceneCapture = NULL;
		*lplpHALCallbacks = (DWORD)&myD3DHALCallbacks7;
	}
	else
	{
		*lplpHALCallbacks = (DWORD)&myD3DHALCallbacks;
	}

	memcpy(lpHALExeBufCallbacks, &myD3DHALExeBufCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

	lpHALFlags->ddscaps = DDSCAPS_3DDEVICE | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER | DDSCAPS_MIPMAP;
 	lpHALFlags->zcaps = DDBD_16 | DDBD_24 | DDBD_32;
 	lpHALFlags->caps2 = DDSCAPS2_CUBEMAP | DDCAPS2_WIDESURFACES;
//	lpHALFlags->caps2 = DDCAPS2_NO2DDURING3DSCENE;

	/* buffer allocation is done in driver only when this flag is set */
	lpHALFlags->ddscaps |= DDSCAPS_EXECUTEBUFFER;

	return TRUE;
}
