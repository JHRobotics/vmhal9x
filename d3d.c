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
#include <initguid.h> /* this is only one file using GUID */
#include <ddraw.h>
#include <ddrawi.h>
#include "d3dhal_ddk.h"
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"

#include "vmhal9x.h"
#include "mesa3d.h"

#include "nocrt.h"

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

extern HANDLE hSharedHeap;

static BOOL ValidateCtx(DWORD dwhContext)
{
	if(dwhContext != 0)
	{
		mesa3d_ctx_t *ctx = MESA_HANDLE_TO_CTX(dwhContext);
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
		return DDHAL_DRIVER_HANDLED;}

DWORD __stdcall SetRenderTarget32(LPD3DHAL_SETRENDERTARGETDATA lpSetRenderData)
{
	TRACE_ENTRY
	
	VALIDATE(lpSetRenderData)
	
	TOPIC("GL", "SetRenderTarget32(0x%X, 0x%X)",
		lpSetRenderData->lpDDS,
		lpSetRenderData->lpDDSZ
	);
	
	GL_BLOCK_BEGIN(lpSetRenderData->dwhContext)
		MesaSetTarget(ctx,
			(LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDS,
			(LPDDRAWI_DDRAWSURFACE_INT)lpSetRenderData->lpDDSZ
		);
	GL_BLOCK_END
	
	lpSetRenderData->ddrval = DD_OK;

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall Clear32(LPD3DHAL_CLEARDATA lpClearData)
{
	TRACE_ENTRY
	
	VALIDATE(lpClearData)

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
			MesaSetRenderState(ctx, state);
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
	
	lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;

	if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DCallbacks2))
	{
		DWORD dwSize;		
		D3DHAL_CALLBACKS2 D3DCallbacks2;
		TRACE("D3DHAL_CALLBACKS2 size %d/%d", lpInput->dwExpectedSize, sizeof(D3DHAL_CALLBACKS2));

		memset(&D3DCallbacks2, 0, sizeof(D3DHAL_CALLBACKS2));

		dwSize = min(lpInput->dwExpectedSize, sizeof(D3DHAL_CALLBACKS2));
		lpInput->dwActualSize = sizeof(D3DHAL_CALLBACKS2);

		D3DCallbacks2.dwSize = dwSize;
		D3DCallbacks2.dwFlags = D3DHAL2_CB32_SETRENDERTARGET  | D3DHAL2_CB32_DRAWONEPRIMITIVE | D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE | D3DHAL2_CB32_DRAWPRIMITIVES;
		D3DCallbacks2.SetRenderTarget = SetRenderTarget32;
		D3DCallbacks2.DrawOnePrimitive = DrawOnePrimitive32;
		D3DCallbacks2.DrawOneIndexedPrimitive = DrawOneIndexedPrimitive32;
		D3DCallbacks2.DrawPrimitives = DrawPrimitives32;
		//D3DCallbacks2.Clear = Clear32;

		memcpy(lpInput->lpvData, &D3DCallbacks2, dwSize);
		lpInput->ddRVal = DD_OK;
		TRACE("GUID_D3DCallbacks2 success");
	}
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DExtendedCaps))
	{
		D3DHAL_D3DEXTENDEDCAPS dx5caps;
		DWORD dwSize = sizeof(D3DHAL_D3DEXTENDEDCAPS);
		
		memset(&dx5caps, 0, dwSize);
		
		dx5caps.dwSize = dwSize;
		dx5caps.dwMinTextureWidth  = 1;
		dx5caps.dwMinTextureHeight = 1;
		dx5caps.dwMaxTextureWidth  = 2048; // TODO: query by GL_MAX_TEXTURE_SIZE
		dx5caps.dwMaxTextureHeight = 2048;
		dx5caps.dwMinStippleWidth  = 32;
		dx5caps.dwMinStippleHeight = 32;
		dx5caps.dwMaxStippleWidth  = 32;
		dx5caps.dwMaxStippleHeight = 32;
		
		memcpy(lpInput->lpvData, &dx5caps, dwSize);
		lpInput->ddRVal = DD_OK;
		TRACE("GUID_D3DExtendedCaps success");
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
	}
	
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextCreate32(LPD3DHAL_CONTEXTCREATEDATA pccd)
{
	TRACE_ENTRY

	pccd->ddrval = D3DHAL_OUTOFCONTEXTS; /* error state */

	mesa3d_entry_t *entry = Mesa3DGet(GetCurrentProcessId());
	if(entry)
	{
		LPDDRAWI_DDRAWSURFACE_INT dss = (LPDDRAWI_DDRAWSURFACE_INT)pccd->lpDDS;
		LPDDRAWI_DDRAWSURFACE_INT dsz = (LPDDRAWI_DDRAWSURFACE_INT)pccd->lpDDSZ;

		mesa3d_ctx_t *ctx = MesaCreateCtx(entry, dss, dsz);
		if(ctx)
		{
			TOPIC("GL", "MesaCreateCtx(entry, %X, %X)", dss, dsz);
			pccd->dwhContext = MESA_CTX_TO_HANDLE(ctx);
			pccd->ddrval = DD_OK;
			ctx->dd = pccd->lpDDGbl;
		}
	}

	if(pccd->ddrval != DD_OK)
	{
		ERR("ContextCreate32 FAILED");
	}

	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextDestroy32(LPD3DHAL_CONTEXTDESTROYDATA pcdd)
{
	TRACE_ENTRY
	
	MesaDestroyCtx(MESA_HANDLE_TO_CTX(pcdd->dwhContext));
	pcdd->dwhContext = 0;
	
	pcdd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall ContextDestroyAllCallback32(LPVOID lpData, HDDRVITEM hItem, DWORD dwData)
{
	TRACE_ENTRY
	
	return DDRV_SUCCESS_CONTINUE;
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
			MesaSetRenderState(ctx, lpState);
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
		
		if(ctx->state.zvisible)
		{
			/* DDK98: If you don't implement Z visibility testing, just do this. */
			prd->dwStatus &= ~D3DSTATUS_ZNOTVISIBLE;
			break;
		}
		
		switch (lpIns->bOpcode)
		{
			case D3DOP_POINT:
				LPD3DPOINT point = (LPD3DPOINT)prim;
				break;
			case D3DOP_SPAN:
				LPD3DSPAN span = (LPD3DSPAN)prim;
				break;
			case D3DOP_LINE:
				
				break;
			case D3DOP_TRIANGLE:
				D3DTRIANGLE *triPtr = (D3DTRIANGLE *)prim;
				
				break;
		}
	GL_BLOCK_END
	
	prd->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall TextureCreate32(LPD3DHAL_TEXTURECREATEDATA ptcd)
{
	TRACE_ENTRY
	
	VALIDATE(ptcd)
	
	ptcd->ddrval = DDERR_OUTOFVIDEOMEMORY;

	GL_BLOCK_BEGIN(ptcd->dwhContext)
		mesa3d_texture_t *tex = MesaCreateTexture(ctx, (LPDDRAWI_DDRAWSURFACE_INT)ptcd->lpDDS);
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
		MesaDestroyTexture(MESA_HANDLE_TO_TEX(ptcd->dwHandle));
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
		mesa3d_texture_t cp = *tex1;
		*tex1 = *tex2;
		*tex2 = cp;

		ptsd->ddrval = DD_OK;
	}
	else
	{
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
		ptgd->lpDDS  = (DWORD)tex->ddsurf;
		ptgd->ddrval = DD_OK;
	}
	else
	{
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
	// JH: ^this is REAL code from S3 driver inclusing the comment

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
			// TODO: reload system memory surfaces here?
			//MesaReadback(ctx);
			break;
		case D3DHAL_SCENE_CAPTURE_END:
			MesaRender(ctx);
			break;
	}
	GL_BLOCK_END

	scdata->ddrval = DD_OK;
	return DDHAL_DRIVER_HANDLED;
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
	SceneCapture32,			// Optional. (JH: required when driver or HW do some buffering)
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

#include "d3d_caps.h"

BOOL __stdcall D3DHALCreateDriver(DWORD *lplpGlobal, DWORD *lplpHALCallbacks, VMDAHAL_D3DCAPS_t *lpHALFlags)
{
	memset(&myGlobalD3DHal, 0, sizeof(D3DHAL_GLOBALDRIVERDATA));
	myGlobalD3DHal.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);
	myGlobalD3DHal.hwCaps = myCaps;
	myGlobalD3DHal.dwNumVertices = 0;
	myGlobalD3DHal.dwNumClipVertices = 0;
	myGlobalD3DHal.dwNumTextureFormats = (sizeof(myTextureFormats) / sizeof(DDSURFACEDESC));
	myGlobalD3DHal.lpTextureFormats = &myTextureFormats[0];
 
	*lplpGlobal = (DWORD)&myGlobalD3DHal;
	*lplpHALCallbacks = (DWORD)&myD3DHALCallbacks;
	
	lpHALFlags->ddscaps = DDSCAPS_3DDEVICE | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER | DDSCAPS_MIPMAP;
 	lpHALFlags->zcaps = DDBD_16 | DDBD_24; // | DDBD_32;

	return TRUE;
}
