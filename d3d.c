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
#include "ids.h"
#include "d3dhal.h"

#include "nocrt.h"

/* FROM ddrvmem.h */
typedef DWORD HDDRVITEM, * LPHDDRVITEM;

#ifndef D3DHAL2_CB32_SETRENDERTARGET
#define D3DHAL2_CB32_SETRENDERTARGET    0x00000001L
#define D3DHAL2_CB32_CLEAR              0x00000002L
#define D3DHAL2_CB32_DRAWONEPRIMITIVE   0x00000004L
#define D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE 0x00000008L
#define D3DHAL2_CB32_DRAWPRIMITIVES     0x00000010L
#endif

#define VALIDATE(_d3d) if(_d3d->dwhContext == 0){ \
		(_d3d)->ddrval = D3DHAL_CONTEXT_BAD; \
		ERR("Invalid context");\
		return DDHAL_DRIVER_HANDLED;}

typedef void (__stdcall *userlib_fn_t)(void *dataptr);

static hal9x_callbacks_t hal9x_cbs =
{
	hal_env,
	{
		id_assign,
		id_free,
		id_destroy
	}
};

#define USER_LIB(_fn, _par) { \
	HMODULE m = GetModuleHandleA(HAL3D_USERLIB_NAME); \
	if(m == NULL){m = LoadLibraryA(HAL3D_USERLIB_NAME); if(m){\
		hal3d_init_t init_p = (hal3d_init_t)GetProcAddress(m, "hal3d_init"); \
		init_p(&hal9x_cbs); \
	} } \
	if(m != NULL){ \
		userlib_fn_t fn = (userlib_fn_t)GetProcAddress(m, HAL3D_USERLIB_PREFIX #_fn); \
		if(fn != NULL){ \
			fn(_par); \
	} } }

#include "d3d_caps.h"

DWORD __stdcall SetRenderTarget32(LPD3DHAL_SETRENDERTARGETDATA lpSetRenderData)
{
	TRACE_ENTRY
	USER_LIB(SetRenderTarget32, lpSetRenderData);
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall Clear32(LPD3DHAL_CLEARDATA lpClearData)
{
	TRACE_ENTRY
	USER_LIB(Clear32, lpClearData);
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawOnePrimitive32(LPD3DHAL_DRAWONEPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	USER_LIB(DrawOnePrimitive32, lpDrawData);
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawOneIndexedPrimitive32(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA lpDrawData)
{
	TRACE_ENTRY
	USER_LIB(DrawOneIndexedPrimitive32, lpDrawData);
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall DrawPrimitives32(LPD3DHAL_DRAWPRIMITIVESDATA lpDrawData)
{
	TRACE_ENTRY
	USER_LIB(DrawPrimitives32, lpDrawData);
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
	USER_LIB(DrawPrimitives2_32, pd);
	return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall Clear2_32(LPD3DHAL_CLEAR2DATA cd)
{
	TRACE_ENTRY
	USER_LIB(Clear2_32, cd);
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
	
	TOPIC("TARGET", "GetDriverState32(dwFlags=%d)", pGDSData->dwFlags);
	
	// Example:
	// D3DDEVINFOID_TEXTUREMANAGER
	// D3DDEVINFO_TEXTUREMANAGER
	//pGDSData->ddRVal = DD_OK;
	
	return DDHAL_DRIVER_NOTHANDLED;
}

DDENTRY_FPUSAVE(CreateSurfaceEx32, LPDDHAL_CREATESURFACEEXDATA, lpcsxd)
{
	TRACE_ENTRY
	USER_LIB(CreateSurfaceEx32, lpcsxd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY_FPUSAVE(DestroyDDLocal32, LPDDHAL_DESTROYDDLOCALDATA, lpdddd)
{
	TRACE_ENTRY
	USER_LIB(DestroyDDLocal32, lpdddd);
	return DDHAL_DRIVER_HANDLED;
}

static void GetDriverInfo2(DD_GETDRIVERINFO2DATA* pgdi2, LONG *lpRVal, DWORD *lpActualSize, void *lpvData)
{
	VMHAL_enviroment_t env;
	GetVMHALenv(&env);
	
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
				VMHALenv_RuntimeVer(7);

			if(pdxv->dwDXVersion >= 0x800)
				VMHALenv_RuntimeVer(8);

			if(pdxv->dwDXVersion >= 0x900)
				VMHALenv_RuntimeVer(9);

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

#if 0
			caps.Caps = DDCAPS_GDI | /* HW is shared with GDI */
				DDCAPS_BLT | /* BLT is supported */
				DDCAPS_BLTDEPTHFILL | /* depth fill */
				DDCAPS_BLTCOLORFILL | /* color fill */
				DDCAPS_BLTSTRETCH   | /* stretching blt */
				DDCAPS_COLORKEY     | /* transparentBlt */
				DDCAPS_CANBLTSYSMEM | /* from to sysmem blt */
				DDCAPS_3D           |
			0;
			caps.Caps2 = DDCAPS2_WIDESURFACES | D3DCAPS2_CANRENDERWINDOWED; // D3DCAPS2_FULLSCREENGAMMA | D3DCAPS2_NO2DDURING3DSCENE
			caps.Caps3 = 0;
			/* JH: ^should we copy here DDCAPS_* flags or use only D3DCAPS_* flags?
			 * permedia driver do first one, from DX8 SDK doc say's the second...
			 */
#else
			caps.Caps = 0; /* DX7 */
			caps.Caps2 = D3DCAPS2_CANRENDERWINDOWED;
			caps.Caps3 = 0;
#endif
			
			caps.PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;
			caps.CursorCaps = D3DCURSORCAPS_COLOR | D3DCURSORCAPS_LOWRES;
			caps.DevCaps =
				//D3DDEVCAPS_CANBLTSYSTONONLOCAL | // Device supports blits from system-memory textures to nonlocal video-memory textures. (HWTL) */
				D3DDEVCAPS_CANRENDERAFTERFLIP | // Device can queue rendering commands after a page flip. Applications do not change their behavior if this flag is set; this capability simply means that the device is relatively fast.
				D3DDEVCAPS_DRAWPRIMTLVERTEX  | //Device exports a DrawPrimitive-aware hardware abstraction layer (HAL). 
				D3DDEVCAPS_EXECUTESYSTEMMEMORY | // Device can use execute buffers from system memory. 
				D3DDEVCAPS_EXECUTEVIDEOMEMORY | // Device can use execute buffers from video memory. 
				//D3DDEVCAPS_NPATCHES | // Device supports N patches. 
				D3DDEVCAPS_PUREDEVICE | // Device can support rasterization, transform, lighting, and shading in hardware. (no need for final version of runtime)
				//D3DDEVCAPS_QUINTICRTPATCHES | // Device supports quintic béziers and B-splines. 
				//D3DDEVCAPS_RTPATCHES | // Device supports rectangular and triangular patches. 
				//D3DDEVCAPS_RTPATCHHANDLEZERO | // When this device capability is set, the hardware architecture does not require caching of any information, and uncached patches (handle zero) will be drawn as efficiently as cached ones. Note that setting D3DDEVCAPS_RTPATCHHANDLEZERO does not mean that a patch with handle zero can be drawn. A handle-zero patch can always be drawn whether this cap is set or not. 
				//D3DDEVCAPS_SEPARATETEXTUREMEMORIES | // Device is texturing from separate memory pools. (HWTL)
				//D3DDEVCAPS_TEXTURENONLOCALVIDMEM | // Device can retrieve textures from non-local video memory.  (= AGP memory)
				//D3DDEVCAPS_TEXTURESYSTEMMEMORY | // Device can retrieve textures from system memory. 
				D3DDEVCAPS_TEXTUREVIDEOMEMORY | // Device can retrieve textures from device memory. 
				D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | // Device can use buffers from system memory for transformed and lit vertices. 
				D3DDEVCAPS_TLVERTEXVIDEOMEMORY | // Device can use buffers from video memory for transformed and lit vertices. 
				D3DDEVCAPS_DRAWPRIMITIVES2 |
				D3DDEVCAPS_DRAWPRIMITIVES2EX |
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
				//D3DPRASTERCAPS_ZTEST | // Device can perform z-test operations. This effectively renders a primitive and indicates whether any z pixels have been rendered. 
			0;
			caps.ZCmpCaps = myCaps6.dpcTriCaps.dwZCmpCaps;
			caps.SrcBlendCaps = myCaps6.dpcTriCaps.dwSrcBlendCaps;
			caps.DestBlendCaps = myCaps6.dpcTriCaps.dwDestBlendCaps;
			caps.AlphaCmpCaps = myCaps6.dpcTriCaps.dwAlphaCmpCaps;
			caps.TextureCaps  = myCaps6.dpcTriCaps.dwTextureCaps | D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_CUBEMAP_POW2 | D3DPTEXTURECAPS_MIPCUBEMAP;
#if 1
			caps.TextureFilterCaps =
				D3DPTFILTERCAPS_MAGFLINEAR |
				D3DPTFILTERCAPS_MAGFANISOTROPIC |
				D3DPTFILTERCAPS_MAGFPOINT |
				D3DPTFILTERCAPS_MINFANISOTROPIC |
				D3DPTFILTERCAPS_MINFLINEAR |
				D3DPTFILTERCAPS_MINFPOINT |
				D3DPTFILTERCAPS_MIPFLINEAR |
				D3DPTFILTERCAPS_MIPFPOINT;
#endif
			//caps.TextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			//caps.CubeTextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			//caps.VolumeTextureFilterCaps = myCaps6.dpcTriCaps.dwTextureFilterCaps;
			caps.CubeTextureFilterCaps = caps.TextureFilterCaps;
			caps.VolumeTextureFilterCaps = 0;
			caps.LineCaps = D3DLINECAPS_ALPHACMP | D3DLINECAPS_BLEND | D3DLINECAPS_FOG;
			caps.TextureAddressCaps = myCaps6.dpcTriCaps.dwTextureAddressCaps | D3DPTADDRESSCAPS_MIRRORONCE;
			caps.PresentationIntervals = 0;
			caps.MaxTextureWidth = env.texture_max_width;
			caps.MaxTextureHeight = env.texture_max_width;
			caps.MaxVolumeExtent = 2048;
			caps.MaxTextureRepeat = 8192;
			caps.MaxTextureAspectRatio = 0; // no limit
			
			/* some happy values from ref driver */
			caps.MaxVertexW      = 1.0e10;
			caps.GuardBandLeft   = -8192.0f;
			caps.GuardBandTop    = -8192.0f;
			caps.GuardBandRight  = 8192.0f;
			caps.GuardBandBottom = 8192.0f;
			caps.ExtentsAdjust   = 0.0f; //  AA kernel is 1.0 x 1.0
			caps.StencilCaps = MYSTENCIL_CAPS;
			caps.FVFCaps = 8;
			caps.TextureOpCaps = MYTEXOPCAPS;
			caps.MaxTextureBlendStages = HAL3D_TMU_CNT;
			caps.MaxSimultaneousTextures = HAL3D_TMU_CNT;
			caps.VertexProcessingCaps = MYVERTEXPROCCAPS_DX8;

			caps.MaxActiveLights = 0; /* when TL sets this below */
			caps.MaxUserClipPlanes = env.num_clips;
			caps.MaxVertexBlendMatrices = 0;
			caps.MaxVertexBlendMatrixIndex = 0;
			if(env.vertexblend)
			{
			 	caps.MaxVertexBlendMatrices = HAL3D_WORLDS_MAX;
			 	caps.MaxVertexBlendMatrixIndex = 3;
			}
			caps.MaxPointSize = 1.0f;
			caps.MaxPrimitiveCount = 0x0000FFFFF;
			caps.MaxVertexIndex = 0x002000000;
			/* A driver reports support for 32-bit indices by setting the value of the MaxVertexIndex field of D3DCAPS8 (currently also in D3DHAL_D3DEXTENDEDCAPS) to a value greater than 0xFFFF */
			caps.MaxStreams = HAL3D_MAX_STREAM;
			caps.MaxStreamStride = 65536;
			caps.VertexShaderVersion = D3DVS_VERSION(0, 0);
			caps.MaxVertexShaderConst = 0;
			caps.PixelShaderVersion = D3DPS_VERSION(0, 0); // DX8
			caps.MaxPixelShaderValue = 0;
			caps.ShadeCaps = myCaps6.dpcTriCaps.dwShadeCaps;

			if(env.hwtl_ddi >= 8)
			{
				caps.DevCaps |=
					D3DDEVCAPS_HWRASTERIZATION | // Device has hardware acceleration for scene rasterization. 
					D3DDEVCAPS_HWTRANSFORMANDLIGHT | // Device can support transformation and lighting in hardware. 
					D3DDEVCAPS_CANBLTSYSTONONLOCAL |
					0;
				caps.MaxActiveLights = env.num_light;

				/*
				 * DDK 2k3:
				 * If a driver sets the D3DDEVCAPS_SEPARATETEXTUREMEMORIES flag in the DevCaps member of the
				 * D3DCAPS8 structure, it indicates to DirectX 8.0 and later versions of applications that
				 * they are disabled from simultaneously using multiple textures
				 */
				if(env.texture_num_units == 1)
				{
					caps.DevCaps |= D3DDEVCAPS_SEPARATETEXTUREMEMORIES;
				}
			}

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

DDENTRY_FPUSAVE(GetAvailDriverMemory32, LPDDHAL_GETAVAILDRIVERMEMORYDATA, pgadmd)
{
	/* this function only report size off private heaps, that not expose to DD via heap */
	pgadmd->dwTotal = 0;
	pgadmd->dwFree = 0;

	pgadmd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
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
DDENTRY_FPUSAVE(GetDriverInfo32, LPDDHAL_GETDRIVERINFODATA, lpInput)
{
	TRACE_ENTRY
	VMHAL_enviroment_t env;
	GetVMHALenv(&env);

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
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_D3DCallbacks3) && env.ddi >= 6)
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

		VMHALenv_RuntimeVer(6);

  	COPY_INFO(lpInput, D3DCallbacks3, D3DHAL_CALLBACKS3);
    TRACE("GUID_D3DCallbacks3 success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_MiscellaneousCallbacks))
	{
		/* DX5 */
		DDHAL_DDMISCELLANEOUSCALLBACKS misccb;
		memset(&misccb, 0, sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS));
		misccb.dwSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
		misccb.dwFlags = DDHAL_MISCCB32_GETSYSMEMBLTSTATUS | DDHAL_MISCCB32_GETAVAILDRIVERMEMORY;
		misccb.GetSysmemBltStatus = GetBltStatus32;
		misccb.GetAvailDriverMemory = GetAvailDriverMemory32;
		
		VMHALenv_RuntimeVer(5);

		COPY_INFO(lpInput, misccb, DDHAL_DDMISCELLANEOUSCALLBACKS);
		TRACE("GUID_MiscellaneousCallbacks success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_Miscellaneous2Callbacks) && env.ddi >= 7)
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

		VMHALenv_RuntimeVer(7);

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
		/* OK, DDS dwCaps is passed by 16bit driver, but DDS dwCaps2 is passed here... */
		DDMoreSurfaceCaps.ddsCapsMore.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_VERTEXBUFFER | DDSCAPS2_COMMANDBUFFER;

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
		dxcaps.dwMaxTextureWidth  = env.texture_max_width;
		dxcaps.dwMaxTextureHeight = env.texture_max_height;
		dxcaps.dwMinStippleWidth  = 32;
		dxcaps.dwMinStippleHeight = 32;
		dxcaps.dwMaxStippleWidth  = 32;
		dxcaps.dwMaxStippleHeight = 32;

		dxcaps.dwFVFCaps                   = HAL3D_TMU_CNT; /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    dxcaps.wMaxTextureBlendStages      = HAL3D_TMU_CNT;
    dxcaps.wMaxSimultaneousTextures    = HAL3D_TMU_CNT;
    dxcaps.dwMaxTextureRepeat          = env.texture_max_width;
    dxcaps.dwMaxTextureAspectRatio     = env.texture_max_height;
    dxcaps.dwTextureOpCaps = MYTEXOPCAPS;
  	dxcaps.wMaxTextureBlendStages      = HAL3D_TMU_CNT;
    dxcaps.wMaxSimultaneousTextures    = HAL3D_TMU_CNT;

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
    if(env.hwtl_ddi >= 7 && env.ddi >= 7)
  	{
			dxcaps.dwMaxActiveLights = env.num_light;
			dxcaps.wMaxUserClipPlanes = env.num_clips; // ref driver = 6
			dxcaps.wMaxVertexBlendMatrices = 0;

			if(env.vertexblend)
				dxcaps.wMaxVertexBlendMatrices = HAL3D_WORLDS_MAX;
				//^ this need GL_ARB_vertex_blend or some extra CPU power

			dxcaps.dwVertexProcessingCaps = MYVERTEXPROCCAPS;
		}

		if(env.max_anisotropy > 1)
		{
			dxcaps.dwMaxAnisotropy = env.max_anisotropy;
		}

		TRACE("lpInput->dwExpectedSize=%d, D3DHAL_D3DEXTENDEDCAPS5=%d, D3DHAL_D3DEXTENDEDCAPS6=%d, D3DHAL_D3DEXTENDEDCAPS7=%d",
			lpInput->dwExpectedSize,
			sizeof(D3DHAL_D3DEXTENDEDCAPS5),
			sizeof(D3DHAL_D3DEXTENDEDCAPS6),
			sizeof(D3DHAL_D3DEXTENDEDCAPS7));

		if(env.ddi <= 5)
		{
			dxcaps.dwSize = sizeof(D3DHAL_D3DEXTENDEDCAPS5);
			COPY_INFO(lpInput, dxcaps, D3DHAL_D3DEXTENDEDCAPS5);
		}
		else if(env.ddi <= 6)
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
	else if(IsEqualIID(&lpInput->guidInfo, &GUID_ZPixelFormats) && env.ddi >= 6)
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
		zformats.pixfmt[i].dwZBufferBitDepth  = 32;
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
		/*zformats.pixfmt[i].dwZBitMask         = 0xFFFFFF00;
		zformats.pixfmt[i].dwStencilBitMask   = 0x000000FF;*/
		zformats.pixfmt[i].dwZBitMask         = 0x00FFFFFF;
		zformats.pixfmt[i].dwStencilBitMask   = 0xFF000000;
		zformats.pixfmt[i].dwRGBZBitMask      = 0;
		i++;

		zformats.cnt = i;
		DWORD real_size = sizeof(DWORD) + sizeof(DDPIXELFORMAT)*zformats.cnt;
		lpInput->dwActualSize = real_size;
		memcpy(lpInput->lpvData, &zformats, min(lpInput->dwExpectedSize, real_size));

		lpInput->ddRVal = DD_OK;
		TRACE("GUID_ZPixelFormats success");
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_D3DParseUnknownCommandCallback) && env.ddi >= 6) 
	{
		USER_LIB(ParseUnknownCommandCallback, lpInput->lpvData);
		lpInput->ddRVal = DD_OK;
	}
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_NonLocalVidMemCaps) && env.ddi >= 6)
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
	else if(IsEqualIID(&(lpInput->guidInfo), &GUID_GetDriverInfo2) && env.ddi >= 8) /*  */
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

DDENTRY(ContextCreate32, LPD3DHAL_CONTEXTCREATEDATA, pccd)
{
	TRACE_ENTRY

	pccd->ddrval = D3DHAL_OUTOFCONTEXTS; /* error state */
	USER_LIB(ContextCreate32, pccd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(ContextDestroy32, LPD3DHAL_CONTEXTDESTROYDATA, pcdd)
{
	TRACE_ENTRY
	VALIDATE(pcdd)
	USER_LIB(ContextDestroy32, pcdd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(ContextDestroyAll32, LPD3DHAL_CONTEXTDESTROYALLDATA, pcdd)
{
	TRACE_ENTRY
	USER_LIB(ContextDestroyAll32, pcdd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(RenderState32, LPD3DHAL_RENDERSTATEDATA, prd)
{
	TRACE_ENTRY
	VALIDATE(prd)
	USER_LIB(RenderState32, prd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(RenderPrimitive32, LPD3DHAL_RENDERPRIMITIVEDATA, prd)
{
	TRACE_ENTRY
	VALIDATE(prd)
	USER_LIB(RenderPrimitive32, prd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(TextureCreate32, LPD3DHAL_TEXTURECREATEDATA, ptcd)
{
	TRACE_ENTRY
	VALIDATE(ptcd)
	USER_LIB(TextureCreate32, ptcd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(TextureDestroy32, LPD3DHAL_TEXTUREDESTROYDATA, ptcd)
{
	TRACE_ENTRY
	VALIDATE(ptcd)
	USER_LIB(TextureDestroy32, ptcd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(TextureSwap32, LPD3DHAL_TEXTURESWAPDATA, ptsd)
{
	TRACE_ENTRY
	VALIDATE(ptsd)
	USER_LIB(TextureSwap32, ptsd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(TextureGetSurf32, LPD3DHAL_TEXTUREGETSURFDATA, ptgd)
{
	TRACE_ENTRY
	VALIDATE(ptgd)
	USER_LIB(TextureGetSurf32, ptgd);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(GetState32, LPD3DHAL_GETSTATEDATA, pgsd)
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

DDENTRY(SceneCapture32, LPD3DHAL_SCENECAPTUREDATA, scdata)
{
	TRACE_ENTRY
	VALIDATE(scdata)
	USER_LIB(SceneCapture32, scdata);
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(CanCreateExecuteBuffer32, LPDDHAL_CANCREATESURFACEDATA, csd)
{
	TRACE_ENTRY
	/* asume we can create the buffer every time */
	csd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY_FPUSAVE(CreateExecuteBuffer32, LPDDHAL_CREATESURFACEDATA, csd)
{
	TRACE_ENTRY

	int i;
	LPDDRAWI_DDRAWSURFACE_LCL *lplpSList = csd->lplpSList;
	for(i = 0; i < (int)csd->dwSCnt; i++)
	{
		LPDDRAWI_DDRAWSURFACE_LCL surf = lplpSList[i];
		TOPIC("EXEBUF", "CreateExecuteBuffer32 dwLinearSize = %d, wWidth = %d, wHeight = %d", surf->lpGbl->dwLinearSize,
		surf->lpGbl->wWidth,  surf->lpGbl->wHeight);

		BOOL alloc_vram = FALSE;
		if(surf->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VERTEXBUFFER)
		{
			if(surf->ddsCaps.dwCaps & DDSCAPS_WRITEONLY)
			{
				/* DDK 2K3:
				 * If neither flag is set, the driver should allocate an implicit
				 * vertex buffer. Implicit vertex buffers should not be placed in
				 * video memory since they are expected to be read/write. Only
				 * explicit vertex buffers with the DDSCAPS_WRITEONLY flag set
				 * can be safely placed in video memory
				 */
				alloc_vram = TRUE;
			}
		}

/*	if(surf->lpGbl->dwLinearSize < 1*1024*1024)
		{
			surf->lpGbl->dwLinearSize = 1*1024*1024;
		}
		^ This is wasn't good idea, beacuse this exhaust memory very quickly
*/
		/* alloc buffer in video memory */
		surf->lpGbl->dwBlockSizeX = surf->lpGbl->dwLinearSize;
		surf->lpGbl->dwBlockSizeY = 1;
		if(alloc_vram)
		{
			surf->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
		}
		else
		{
			surf->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_USERMEM;
		}
	} // for

	csd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

static BOOL IsVram(void *flat)
{
	FBHDA_t *hda = FBHDA_setup();
	if(hda)
	{
		if((BYTE*)flat >= (BYTE*)hda->vram_pm32 && 
			(BYTE*)flat < ((BYTE*)hda->vram_pm32 + hda->vram_size_virt))
		{
			return TRUE;
		}
	}
	return FALSE;
}

DDENTRY_FPUSAVE(DestroyExecuteBuffer32, LPDDHAL_DESTROYSURFACEDATA, dsd)
{
	TRACE_ENTRY

	TOPIC("MEMORY", "DestroyExecuteBuffer32 mem=0x%X", 
		dsd->lpDDSurface->lpGbl->fpVidMem
		);

	void *flat = (void*)dsd->lpDDSurface->lpGbl->fpVidMem;
	if(IsVram(flat))
	{
		FBHDA_DD_surface_delete(flat);
	}

	dsd->ddRVal = DD_OK;
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(LockExecuteBuffer32, LPDDHAL_LOCKDATA, lock)
{
	TRACE_ENTRY
	/* nop */
	
	lock->lpSurfData = (void*)lock->lpDDSurface->lpGbl->fpVidMem;
	lock->ddRVal = DD_OK;
	
	return DDHAL_DRIVER_HANDLED;
}

DDENTRY(UnlockExecuteBuffer32, LPDDHAL_UNLOCKDATA, lock)
{
	TRACE_ENTRY
	lock->ddRVal = DD_OK;
	
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

	VMHAL_enviroment_t env;
	GetVMHALenv(&env);

	if(env.allow_palette == FALSE)
	{
		myGlobalD3DHal.dwNumTextureFormats -= NUM_PALETTE_FORMATS;
	}

	if(env.ddi >= 7)
	{
		myGlobalD3DHal.hwCaps = myCaps6;
		if(env.ddi < 8)
		{
			myGlobalD3DHal.hwCaps.dwDevCaps &= ~(CAPS_DX8);
		}
		
		if(env.hwtl_ddi >= 7)
		{
			myGlobalD3DHal.hwCaps.dwDevCaps |= 
				D3DDEVCAPS_HWTRANSFORMANDLIGHT /* Device can support transformation and lighting in hardware and DRAWPRIMITIVES2EX must be also */
				/*| D3DDEVCAPS_CANBLTSYSTONONLOCAL*/ /* Device supports a Tex Blt from system memory to non-local vidmem */
				| D3DDEVCAPS_HWRASTERIZATION; /* Device has HW acceleration for rasterization */
		}
	}
	else if(env.ddi >= 6)
	{
		myGlobalD3DHal.hwCaps = myCaps6;
		myGlobalD3DHal.hwCaps.dwDevCaps &= ~(CAPS_DX7|CAPS_DX8);
	}

	*lplpGlobal = (DWORD)&myGlobalD3DHal;
	*lplpHALCallbacks = (DWORD)&myD3DHALCallbacks;

	if(env.ddi >= 7)
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

	lpHALFlags->ddscaps =
		DDSCAPS_3DDEVICE |
		DDSCAPS_TEXTURE |
		DDSCAPS_ZBUFFER |
		DDSCAPS_MIPMAP |
		DDSCAPS_FRONTBUFFER |
		DDSCAPS_BACKBUFFER |
		DDSCAPS_COMPLEX |
		DDSCAPS_FLIP |
		DDSCAPS_LOCALVIDMEM |
		DDSCAPS_OFFSCREENPLAIN |
		DDSCAPS_PRIMARYSURFACE |
		DDSCAPS_VIDEOMEMORY |
	0;
	lpHALFlags->zcaps = DDBD_16 | DDBD_24 | DDBD_32;
	lpHALFlags->caps2 = DDCAPS2_WIDESURFACES | D3DCAPS2_FULLSCREENGAMMA | /*DDCAPS2_COPYFOURCC |*/ DDCAPS2_FLIPNOVSYNC /* | DDCAPS2_CANMANAGETEXTURE*/;
	/*
		cap DDCAPS2_NO2DDURING3DSCENE should be theoretically safer to set,
		BUT some games forbid to start with this flag set.
	*/
	
//	lpHALFlags->caps2 = DDCAPS2_NO2DDURING3DSCENE | DDCAPS2_CANMANAGETEXTURE;

	/* buffer allocation is done in driver only when this flag is set */
	//lpHALFlags->ddscaps |= DDSCAPS_EXECUTEBUFFER;

	return TRUE;
}
