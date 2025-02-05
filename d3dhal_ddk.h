/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3dhal.h
 *  Content:	Direct3D HAL include file
 *
 ***************************************************************************/

#ifndef _D3DHAL_H_
#define _D3DHAL_H_

#include "ddraw.h"
#include "d3dtypes.h"
#include "d3dcaps.h"
#include "ddrawi.h"

/*
 * If the HAL driver does not implement clipping, it must reserve at least
 * this much space at the end of the LocalVertexBuffer for use by the HEL
 * clipping.  I.e. the vertex buffer contain dwNumVertices+dwNumClipVertices
 * vertices.  No extra space is needed by the HEL clipping in the
 * LocalHVertexBuffer.
 */
#define D3DHAL_NUMCLIPVERTICES	20

/*
 * If no dwNumVertices is given, this is what will be used.
 */
#define D3DHAL_DEFAULT_TL_NUM	((32 * 1024) / sizeof (D3DTLVERTEX))
#define D3DHAL_DEFAULT_H_NUM	((32 * 1024) / sizeof (D3DHVERTEX))

/*
 * Description for a device.
 * This is used to describe a device that is to be created or to query
 * the current device.
 *
 * For DX5 and subsequent runtimes, D3DDEVICEDESC is a user-visible
 * structure that is not seen by the device drivers. The runtime
 * stitches a D3DDEVICEDESC together using the D3DDEVICEDESC_V1
 * embedded in the GLOBALDRIVERDATA and the extended caps queried
 * from the driver using GetDriverInfo.
 */

typedef struct _D3DDeviceDesc_V1 {
    DWORD	     dwSize;		     /* Size of D3DDEVICEDESC structure */
    DWORD	     dwFlags;	             /* Indicates which fields have valid data */
    D3DCOLORMODEL    dcmColorModel;	     /* Color model of device */
    DWORD	     dwDevCaps;	             /* Capabilities of device */
    D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
    BOOL	     bClipping;	             /* Device can do 3D clipping */
    D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD	     dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
    DWORD	     dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
    DWORD	     dwMaxBufferSize;        /* Maximum execute buffer size */
    DWORD	     dwMaxVertexCount;       /* Maximum vertex count */
} D3DDEVICEDESC_V1, *LPD3DDEVICEDESC_V1;

#define D3DDEVICEDESCSIZE_V1 (sizeof(D3DDEVICEDESC_V1))

/* --------------------------------------------------------------
 * Instantiated by the HAL driver on driver connection.
 */
typedef struct _D3DHAL_GLOBALDRIVERDATA {
    DWORD		dwSize;			// Size of this structure
    D3DDEVICEDESC_V1    hwCaps;            // Capabilities of the hardware
    DWORD		dwNumVertices;		// see following comment
    DWORD		dwNumClipVertices;	// see following comment
    DWORD		dwNumTextureFormats;	// Number of texture formats
    LPDDSURFACEDESC	lpTextureFormats;	// Pointer to texture formats
} D3DHAL_GLOBALDRIVERDATA;

typedef D3DHAL_GLOBALDRIVERDATA *LPD3DHAL_GLOBALDRIVERDATA;

#define D3DHAL_GLOBALDRIVERDATASIZE (sizeof(D3DHAL_GLOBALDRIVERDATA))

/* --------------------------------------------------------------
 * Extended caps introduced with DX5 and queried with
 * GetDriverInfo (GUID_D3DExtendedCaps).
 */
typedef struct _D3DHAL_D3DEXTENDEDCAPS5 {
    DWORD		dwSize;			// Size of this structure
    DWORD       dwMinTextureWidth, dwMaxTextureWidth;
    DWORD       dwMinTextureHeight, dwMaxTextureHeight;
    DWORD       dwMinStippleWidth, dwMaxStippleWidth;
    DWORD       dwMinStippleHeight, dwMaxStippleHeight;
} D3DHAL_D3DEXTENDEDCAPS5;

typedef struct _D3DHAL_D3DEXTENDEDCAPS6 {
    DWORD       dwSize;         // Size of this structure

    DWORD       dwMinTextureWidth, dwMaxTextureWidth;
    DWORD       dwMinTextureHeight, dwMaxTextureHeight;
    DWORD       dwMinStippleWidth, dwMaxStippleWidth;
    DWORD       dwMinStippleHeight, dwMaxStippleHeight;

    /* fields added for DX6 */
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;  /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;

} D3DHAL_D3DEXTENDEDCAPS6;

typedef struct _D3DHAL_D3DEXTENDEDCAPS7 {
    DWORD       dwSize;         // Size of this structure

    DWORD       dwMinTextureWidth, dwMaxTextureWidth;
    DWORD       dwMinTextureHeight, dwMaxTextureHeight;
    DWORD       dwMinStippleWidth, dwMaxStippleWidth;
    DWORD       dwMinStippleHeight, dwMaxStippleHeight;

    /* fields added for DX6 */
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;  /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;

    /* fields added for DX7 */
    DWORD       dwMaxActiveLights;
    D3DVALUE    dvMaxVertexW;

    WORD        wMaxUserClipPlanes;
    WORD        wMaxVertexBlendMatrices;

    DWORD       dwVertexProcessingCaps;

    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
} D3DHAL_D3DEXTENDEDCAPS7;


/* --------------------------------------------------------------
 * Argument to the HAL functions.
 */

typedef DWORD D3DI_BUFFERHANDLE, *LPD3DI_BUFFERHANDLE;

/*
 * Internal version of executedata
 */
typedef struct _D3DI_ExecuteData {
    DWORD       dwSize;
    D3DI_BUFFERHANDLE dwHandle;		/* Handle allocated by driver */
    DWORD       dwVertexOffset;
    DWORD       dwVertexCount;
    DWORD       dwInstructionOffset;
    DWORD       dwInstructionLength;
    DWORD       dwHVertexOffset;
    D3DSTATUS   dsStatus;		/* Status after execute */
} D3DI_EXECUTEDATA, *LPD3DI_EXECUTEDATA;

/*
 * Internal version of lightdata and constants for flags
 */

#define D3DLIGHTI_ATT0_IS_NONZERO	(0x00010000)	
#define D3DLIGHTI_ATT1_IS_NONZERO	(0x00020000)
#define D3DLIGHTI_ATT2_IS_NONZERO	(0x00040000)
#define D3DLIGHTI_LINEAR_FALLOFF	(0x00080000)
#define D3DLIGHTI_UNIT_SCALE		(0x00100000)
#define D3DLIGHTI_LIGHT_AT_EYE		(0x00200000)

typedef struct _D3DI_LIGHT {
    D3DLIGHTTYPE	type;
	DWORD			version;	/* matches number on D3DLIGHT struct */
    BOOL			valid;
    D3DVALUE		red, green, blue, shade;
    D3DVECTOR		position;
    D3DVECTOR		model_position;
    D3DVECTOR		direction;
    D3DVECTOR		model_direction;
    D3DVECTOR		halfway;
	D3DVECTOR		model_eye;		/* direction from eye in model space */
	D3DVECTOR		model_scale;	/* model scale for proper range computations */
    D3DVALUE		range;
    D3DVALUE		range_squared;
    D3DVALUE		falloff;
    D3DVALUE		attenuation0;
    D3DVALUE		attenuation1;
    D3DVALUE		attenuation2;
    D3DVALUE		cos_theta_by_2;
    D3DVALUE		cos_phi_by_2;
	DWORD			flags;
} D3DI_LIGHT, *LPD3DI_LIGHT;

 
typedef struct _D3DHAL_CONTEXTCREATEDATA {
	union
	{
		LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;    // in:  Driver struct (legacy)
		LPDDRAWI_DIRECTDRAW_LCL lpDDLcl;    // in:  For DX7 driver onwards
	};
	union
	{
		LPDIRECTDRAWSURFACE lpDDS;      // in:  Surface to be used as target
		LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl; // For DX7 onwards
	};
	union
	{
		LPDIRECTDRAWSURFACE lpDDSZ;     // in:  Surface to be used as Z
		LPDDRAWI_DDRAWSURFACE_LCL lpDDSZLcl; // For DX7 onwards
	};
	union
	{
		DWORD	dwPID;      // in:  Current process id
		ULONG_PTR dwrstates;  // Sundown: hack so we can send ptr back
	};
	ULONG_PTR dwhContext; // out: Context handle
	HRESULT   ddrval;     // out: Return value
} D3DHAL_CONTEXTCREATEDATA;
typedef D3DHAL_CONTEXTCREATEDATA *LPD3DHAL_CONTEXTCREATEDATA;

typedef struct _D3DHAL_CONTEXTDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    HRESULT		ddrval;		// out: Return value
} D3DHAL_CONTEXTDESTROYDATA;
typedef D3DHAL_CONTEXTDESTROYDATA *LPD3DHAL_CONTEXTDESTROYDATA;

typedef struct _D3DHAL_CONTEXTDESTROYALLDATA {
    DWORD		dwPID;		// in:  Process id to destroy contexts for
    HRESULT		ddrval;		// out: Return value
} D3DHAL_CONTEXTDESTROYALLDATA;
typedef D3DHAL_CONTEXTDESTROYALLDATA *LPD3DHAL_CONTEXTDESTROYALLDATA;

typedef struct _D3DHAL_SCENECAPTUREDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwFlag;		// in:  Indicates beginning or end
    HRESULT		ddrval;		// out: Return value
} D3DHAL_SCENECAPTUREDATA;
typedef D3DHAL_SCENECAPTUREDATA *LPD3DHAL_SCENECAPTUREDATA;

typedef struct _D3DHAL_EXECUTEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in/out: Where to start/error occured
    DWORD		dwFlags;	// in:  Flags for this execute
    DWORD		dwStatus;	// in/out: Condition branch status
    D3DI_EXECUTEDATA	deExData;	// in:  Execute data describing buffer
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    					//	Only provided if HEL performing transform
    D3DINSTRUCTION	diInstruction;	// in:  Optional one off instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_EXECUTEDATA;
typedef D3DHAL_EXECUTEDATA *LPD3DHAL_EXECUTEDATA;

typedef struct _D3DHAL_EXECUTECLIPPEDDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in/out: Where to start/error occured
    DWORD		dwFlags;	// in:  Flags for this execute
    DWORD		dwStatus;	// in/out: Condition branch status
    D3DI_EXECUTEDATA	deExData;	// in:  Execute data describing buffer
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    					//	Only provided if HEL performing transform
    LPDIRECTDRAWSURFACE	lpHBuf;		// in:  Execute buffer containing HVertex data
    					//	Only provided if HEL performing transform
    D3DINSTRUCTION	diInstruction;	// in:  Optional one off instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_EXECUTECLIPPEDDATA;
typedef D3DHAL_EXECUTECLIPPEDDATA *LPD3DHAL_EXECUTECLIPPEDDATA;

typedef struct _D3DHAL_RENDERSTATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in:  Where to find states in buffer
    DWORD		dwCount;	// in:  How many states to process
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_RENDERSTATEDATA;
typedef D3DHAL_RENDERSTATEDATA *LPD3DHAL_RENDERSTATEDATA;

typedef struct _D3DHAL_RENDERPRIMITIVEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwOffset;	// in:  Where to find primitive data in buffer
    DWORD		dwStatus;	// in/out: Condition branch status
    LPDIRECTDRAWSURFACE	lpExeBuf;	// in:  Execute buffer containing data
    DWORD		dwTLOffset;	// in:  Byte offset in lpTLBuf for start of vertex data
    LPDIRECTDRAWSURFACE	lpTLBuf;	// in:  Execute buffer containing TLVertex data
    D3DINSTRUCTION	diInstruction;	// in:  Primitive instruction
    HRESULT		ddrval;		// out: Return value
} D3DHAL_RENDERPRIMITIVEDATA;
typedef D3DHAL_RENDERPRIMITIVEDATA *LPD3DHAL_RENDERPRIMITIVEDATA;

typedef struct _D3DHAL_TEXTURECREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    LPDIRECTDRAWSURFACE	lpDDS;		// in:  Pointer to surface object
    DWORD		dwHandle;	// out: Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTURECREATEDATA;
typedef D3DHAL_TEXTURECREATEDATA *LPD3DHAL_TEXTURECREATEDATA;

typedef struct _D3DHAL_TEXTUREDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTUREDESTROYDATA;
typedef D3DHAL_TEXTUREDESTROYDATA *LPD3DHAL_TEXTUREDESTROYDATA;

typedef struct _D3DHAL_TEXTURESWAPDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle1;	// in:  Handle to texture 1
    DWORD		dwHandle2;	// in:  Handle to texture 2
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTURESWAPDATA;
typedef D3DHAL_TEXTURESWAPDATA *LPD3DHAL_TEXTURESWAPDATA;

typedef struct _D3DHAL_TEXTUREGETSURFDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		lpDDS;		// out: Pointer to surface object
    DWORD		dwHandle;	// in:  Handle to texture
    HRESULT		ddrval;		// out: Return value
} D3DHAL_TEXTUREGETSURFDATA;
typedef D3DHAL_TEXTUREGETSURFDATA *LPD3DHAL_TEXTUREGETSURFDATA;

typedef struct _D3DHAL_MATRIXCREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// out: Handle to matrix
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXCREATEDATA;
typedef D3DHAL_MATRIXCREATEDATA *LPD3DHAL_MATRIXCREATEDATA;

typedef struct _D3DHAL_MATRIXDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXDESTROYDATA;
typedef D3DHAL_MATRIXDESTROYDATA *LPD3DHAL_MATRIXDESTROYDATA;

typedef struct _D3DHAL_MATRIXSETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    D3DMATRIX		dmMatrix;	// in:  Matrix data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXSETDATADATA;
typedef D3DHAL_MATRIXSETDATADATA *LPD3DHAL_MATRIXSETDATADATA;

typedef struct _D3DHAL_MATRIXGETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to matrix
    D3DMATRIX		dmMatrix;	// out: Matrix data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATRIXGETDATADATA;
typedef D3DHAL_MATRIXGETDATADATA *LPD3DHAL_MATRIXGETDATADATA;

typedef struct _D3DHAL_SETVIEWPORTDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwViewportID;	// in:	ID of viewport
    D3DVIEWPORT2		dvViewData;	// in:  Viewport data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_SETVIEWPORTDATADATA;
typedef D3DHAL_SETVIEWPORTDATADATA *LPD3DHAL_SETVIEWPORTDATADATA;

typedef struct _D3DHAL_LIGHTSETDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwLight;	// in:  Which light to set
    D3DI_LIGHT		dlLight;	// in:  Light data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_LIGHTSETDATA;
typedef D3DHAL_LIGHTSETDATA *LPD3DHAL_LIGHTSETDATA;

typedef struct _D3DHAL_MATERIALCREATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// out: Handle to material
    D3DMATERIAL		dmMaterial;	// in:  Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALCREATEDATA;
typedef D3DHAL_MATERIALCREATEDATA *LPD3DHAL_MATERIALCREATEDATA;

typedef struct _D3DHAL_MATERIALDESTROYDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALDESTROYDATA;
typedef D3DHAL_MATERIALDESTROYDATA *LPD3DHAL_MATERIALDESTROYDATA;

typedef struct _D3DHAL_MATERIALSETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    D3DMATERIAL		dmMaterial;	// in:  Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALSETDATADATA;
typedef D3DHAL_MATERIALSETDATADATA *LPD3DHAL_MATERIALSETDATADATA;

typedef struct _D3DHAL_MATERIALGETDATADATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwHandle;	// in:  Handle to material
    D3DMATERIAL		dmMaterial;	// out: Material data
    HRESULT		ddrval;		// out: Return value
} D3DHAL_MATERIALGETDATADATA;
typedef D3DHAL_MATERIALGETDATADATA *LPD3DHAL_MATERIALGETDATADATA;

typedef struct _D3DHAL_GETSTATEDATA {
    DWORD		dwhContext;	// in:  Context handle
    DWORD		dwWhich;	// in:  Transform, lighting or render?
    D3DSTATE		ddState;	// in/out: State.
    HRESULT		ddrval;		// out: Return value
} D3DHAL_GETSTATEDATA;
typedef D3DHAL_GETSTATEDATA *LPD3DHAL_GETSTATEDATA;


/* --------------------------------------------------------------
 * Direct3D HAL Table.
 * Instantiated by the HAL driver on connection.
 *
 * Calls take the form of:
 *	retcode = HalCall(HalCallData* lpData);
 */
 
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTCREATECB)	(LPD3DHAL_CONTEXTCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTDESTROYCB)	(LPD3DHAL_CONTEXTDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_CONTEXTDESTROYALLCB) (LPD3DHAL_CONTEXTDESTROYALLDATA);
typedef DWORD	(__stdcall *LPD3DHAL_SCENECAPTURECB)	(LPD3DHAL_SCENECAPTUREDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECB)		(LPD3DHAL_EXECUTEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECLIPPEDCB)	(LPD3DHAL_EXECUTECLIPPEDDATA);
typedef DWORD	(__stdcall *LPD3DHAL_RENDERSTATECB)	(LPD3DHAL_RENDERSTATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_RENDERPRIMITIVECB)	(LPD3DHAL_RENDERPRIMITIVEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_EXECUTECLIPPEDCB)	(LPD3DHAL_EXECUTECLIPPEDDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTURECREATECB)	(LPD3DHAL_TEXTURECREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTUREDESTROYCB)	(LPD3DHAL_TEXTUREDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTURESWAPCB)	(LPD3DHAL_TEXTURESWAPDATA);
typedef DWORD	(__stdcall *LPD3DHAL_TEXTUREGETSURFCB)	(LPD3DHAL_TEXTUREGETSURFDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXCREATECB)	(LPD3DHAL_MATRIXCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXDESTROYCB)	(LPD3DHAL_MATRIXDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXSETDATACB)	(LPD3DHAL_MATRIXSETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATRIXGETDATACB)	(LPD3DHAL_MATRIXGETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_SETVIEWPORTDATACB)	(LPD3DHAL_SETVIEWPORTDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_LIGHTSETCB)	(LPD3DHAL_LIGHTSETDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALCREATECB)	(LPD3DHAL_MATERIALCREATEDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALDESTROYCB)	(LPD3DHAL_MATERIALDESTROYDATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALSETDATACB)	(LPD3DHAL_MATERIALSETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_MATERIALGETDATACB)	(LPD3DHAL_MATERIALGETDATADATA);
typedef DWORD	(__stdcall *LPD3DHAL_GETSTATECB)	(LPD3DHAL_GETSTATEDATA);



/*
 * Regarding dwNumVertices, specify 0 if you are relying on the HEL to do
 * everything and you do not need the resultant TLVertex buffer to reside
 * in device memory.
 * The HAL driver will be asked to allocate dwNumVertices + dwNumClipVertices
 * in the case described above.
 */

typedef struct _D3DHAL_CALLBACKS {
    DWORD			dwSize;
    
    // Device context
    LPD3DHAL_CONTEXTCREATECB	ContextCreate;
    LPD3DHAL_CONTEXTDESTROYCB	ContextDestroy;
    LPD3DHAL_CONTEXTDESTROYALLCB ContextDestroyAll;

    // Scene Capture
    LPD3DHAL_SCENECAPTURECB	SceneCapture;
    
    // Execution
    LPD3DHAL_EXECUTECB		Execute;
    LPD3DHAL_EXECUTECLIPPEDCB	ExecuteClipped;
    LPD3DHAL_RENDERSTATECB	RenderState;
    LPD3DHAL_RENDERPRIMITIVECB	RenderPrimitive;
    
    DWORD			dwReserved;		// Must be zero

    // Textures
    LPD3DHAL_TEXTURECREATECB	TextureCreate;
    LPD3DHAL_TEXTUREDESTROYCB	TextureDestroy;
    LPD3DHAL_TEXTURESWAPCB	TextureSwap;
    LPD3DHAL_TEXTUREGETSURFCB	TextureGetSurf;
    
    // Transform
    LPD3DHAL_MATRIXCREATECB	MatrixCreate;
    LPD3DHAL_MATRIXDESTROYCB	MatrixDestroy;
    LPD3DHAL_MATRIXSETDATACB	MatrixSetData;
    LPD3DHAL_MATRIXGETDATACB	MatrixGetData;
    LPD3DHAL_SETVIEWPORTDATACB	SetViewportData;
    
    // Lighting
    LPD3DHAL_LIGHTSETCB		LightSet;
    LPD3DHAL_MATERIALCREATECB	MaterialCreate;
    LPD3DHAL_MATERIALDESTROYCB	MaterialDestroy;
    LPD3DHAL_MATERIALSETDATACB	MaterialSetData;
    LPD3DHAL_MATERIALGETDATACB	MaterialGetData;

    // Pipeline state
    LPD3DHAL_GETSTATECB		GetState;

    DWORD			dwReserved0;		// Must be zero
    DWORD			dwReserved1;		// Must be zero
    DWORD			dwReserved2;		// Must be zero
    DWORD			dwReserved3;		// Must be zero
    DWORD			dwReserved4;		// Must be zero
    DWORD			dwReserved5;		// Must be zero
    DWORD			dwReserved6;		// Must be zero
    DWORD			dwReserved7;		// Must be zero
    DWORD			dwReserved8;		// Must be zero
    DWORD			dwReserved9;		// Must be zero

} D3DHAL_CALLBACKS;
typedef D3DHAL_CALLBACKS *LPD3DHAL_CALLBACKS;

#define D3DHAL_SIZE_V1 sizeof( D3DHAL_CALLBACKS )


typedef struct _D3DHAL_SETRENDERTARGETDATA {
    DWORD               dwhContext;     // in:  Context handle
    LPDIRECTDRAWSURFACE lpDDS;          // in:  new render target
    LPDIRECTDRAWSURFACE lpDDSZ;         // in:  new Z buffer
    HRESULT             ddrval;         // out: Return value
} D3DHAL_SETRENDERTARGETDATA;
typedef D3DHAL_SETRENDERTARGETDATA FAR *LPD3DHAL_SETRENDERTARGETDATA;

typedef struct _D3DHAL_CLEARDATA {
    DWORD               dwhContext;     // in:  Context handle

    // dwFlags can contain D3DCLEAR_TARGET or D3DCLEAR_ZBUFFER
    DWORD               dwFlags;        // in:  surfaces to clear

    DWORD               dwFillColor;    // in:  Color value for rtarget
    DWORD               dwFillDepth;    // in:  Depth value for Z buffer

    LPD3DRECT           lpRects;        // in:  Rectangles to clear
    DWORD               dwNumRects;     // in:  Number of rectangles

    HRESULT             ddrval;         // out: Return value
} D3DHAL_CLEARDATA;
typedef D3DHAL_CLEARDATA FAR *LPD3DHAL_CLEARDATA;


typedef struct _D3DHAL_DRAWONEPRIMITIVEDATA {

    DWORD               dwhContext;     // in:  Context handle

    DWORD               dwFlags;        // in:  flags

    D3DPRIMITIVETYPE    PrimitiveType;  // in:  type of primitive to draw
    D3DVERTEXTYPE       VertexType;     // in:  type of vertices
    LPVOID              lpvVertices;    // in:  pointer to vertices
    DWORD               dwNumVertices;  // in:  number of vertices

    DWORD               dwReserved;     // in:  reserved

    HRESULT             ddrval;         // out: Return value

} D3DHAL_DRAWONEPRIMITIVEDATA;
typedef D3DHAL_DRAWONEPRIMITIVEDATA *LPD3DHAL_DRAWONEPRIMITIVEDATA;


typedef struct _D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA {
    DWORD               dwhContext;     // in: Context handle

    DWORD               dwFlags;        // in: flags word

    // Primitive and vertex type
    D3DPRIMITIVETYPE    PrimitiveType;  // in: primitive type
    D3DVERTEXTYPE       VertexType;     // in: vertex type

    // Vertices
    LPVOID              lpvVertices;    // in: vertex data
    DWORD               dwNumVertices;  // in: vertex count

    // Indices
    LPWORD              lpwIndices;     // in: index data
    DWORD               dwNumIndices;   // in: index count

    HRESULT             ddrval;         // out: Return value
} D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA;
typedef D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA *LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA;


typedef struct _D3DHAL_DRAWPRIMCOUNTS {
    WORD wNumStateChanges;
    WORD wPrimitiveType;
    WORD wVertexType;
    WORD wNumVertices;
} D3DHAL_DRAWPRIMCOUNTS, *LPD3DHAL_DRAWPRIMCOUNTS;

typedef struct _D3DHAL_DRAWPRIMITIVESDATA {
    DWORD               dwhContext;     // in:  Context handle

    DWORD               dwFlags;

    //
    // Data block:
    //
    // Consists of interleaved D3DHAL_DRAWPRIMCOUNTS, state change pairs,
    // and primitive drawing commands.
    //
    //  D3DHAL_DRAWPRIMCOUNTS: gives number of state change pairs and
    //          the information on the primitive to draw.
    //          wPrimitiveType is of type D3DPRIMITIVETYPE. Drivers
    //              must support all 7 of the primitive types specified
    //              in the DrawPrimitive API.
    //          Currently, wVertexType will always be D3DVT_TLVERTEX.
    //          If the wNumVertices member is 0, then the driver should
    //              return after doing the state changing. This is the
    //              terminator for the command stream.
    // state change pairs: DWORD pairs specify the state changes that
    //          the driver should effect before drawing the primitive.
    //          wNumStateChanges can be 0, in which case the next primitive
    //          should be drawn without any state changes in between.
    //          If present, the state change pairs are NOT aligned, they
    //          immediately follow the PRIMCOUNTS structure.
    // vertex data (if any): is 32-byte aligned.
    //
    // If a primcounts structure follows (i.e. if wNumVertices was nonzero
    // in the previous one), then it will immediately follow the state
    // changes or vertex data with no alignment padding.
    //

    LPVOID              lpvData;

    DWORD               dwReserved;     // in: reserved

    HRESULT             ddrval;         // out: Return value
} D3DHAL_DRAWPRIMITIVESDATA;
typedef D3DHAL_DRAWPRIMITIVESDATA *LPD3DHAL_DRAWPRIMITIVESDATA;

typedef DWORD (FAR PASCAL *LPD3DHAL_SETRENDERTARGETCB) (LPD3DHAL_SETRENDERTARGETDATA);
typedef DWORD (FAR PASCAL *LPD3DHAL_CLEARCB)           (LPD3DHAL_CLEARDATA);
typedef DWORD (FAR PASCAL *LPD3DHAL_DRAWONEPRIMITIVECB)   (LPD3DHAL_DRAWONEPRIMITIVEDATA);
typedef DWORD (FAR PASCAL *LPD3DHAL_DRAWONEINDEXEDPRIMITIVECB) (LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA);
typedef DWORD (FAR PASCAL *LPD3DHAL_DRAWPRIMITIVESCB)   (LPD3DHAL_DRAWPRIMITIVESDATA);

typedef struct _D3DHAL_CALLBACKS2
{
    DWORD                       dwSize;                 // size of struct
    DWORD                       dwFlags;                // flags for callbacks

    LPD3DHAL_SETRENDERTARGETCB  SetRenderTarget;
    LPD3DHAL_CLEARCB            Clear;
    LPD3DHAL_DRAWONEPRIMITIVECB DrawOnePrimitive;
    LPD3DHAL_DRAWONEINDEXEDPRIMITIVECB DrawOneIndexedPrimitive;
    LPD3DHAL_DRAWPRIMITIVESCB    DrawPrimitives;

} D3DHAL_CALLBACKS2;
typedef D3DHAL_CALLBACKS2 *LPD3DHAL_CALLBACKS2;

#define D3DHAL2_CB32_SETRENDERTARGET    0x00000001L
#define D3DHAL2_CB32_CLEAR              0x00000002L
#define D3DHAL2_CB32_DRAWONEPRIMITIVE   0x00000004L
#define D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE 0x00000008L
#define D3DHAL2_CB32_DRAWPRIMITIVES     0x00000010L

/* --------------------------------------------------------------
 * Flags for the data parameters.
 */

/*
 * SceneCapture()
 * This is used as an indication to the driver that a scene is about to
 * start or end, and that it should capture data if required.
 */
#define D3DHAL_SCENE_CAPTURE_START	0x00000000L
#define D3DHAL_SCENE_CAPTURE_END	0x00000001L
 
/*
 * Execute()
 */
 
/*
 * Use the instruction stream starting at dwOffset.
 */
#define D3DHAL_EXECUTE_NORMAL		0x00000000L

/*
 * Use the optional instruction override (diInstruction) and return
 * after completion.  dwOffset is the offset to the first primitive.
 */
#define D3DHAL_EXECUTE_OVERRIDE		0x00000001L
 
/*
 * GetState()
 * The driver will get passed a flag in dwWhich specifying which module
 * the state must come from.  The driver then fills in ulArg[1] with the
 * appropriate value depending on the state type given in ddState.
 */

/*
 * The following are used to get the state of a particular stage of the
 * pipeline.
 */
#define D3DHALSTATE_GET_TRANSFORM	0x00000001L
#define D3DHALSTATE_GET_LIGHT		0x00000002L
#define D3DHALSTATE_GET_RENDER		0x00000004L


/* --------------------------------------------------------------
 * Return values from HAL functions.
 */
 
/*
 * The context passed in was bad.
 */
#define D3DHAL_CONTEXT_BAD		0x000000200L

/*
 * No more contexts left.
 */
#define D3DHAL_OUTOFCONTEXTS		0x000000201L

/*
 * Execute() and ExecuteClipped()
 */
 
/*
 * Executed to completion via early out.
 * 	(e.g. totally clipped)
 */
#define D3DHAL_EXECUTE_ABORT		0x00000210L

/*
 * An unhandled instruction code was found (e.g. D3DOP_TRANSFORM).
 * The dwOffset parameter must be set to the offset of the unhandled
 * instruction.
 *
 * Only valid from Execute()
 */
#define D3DHAL_EXECUTE_UNHANDLED	0x00000211L



/*************** DX6/7 ****************/


/* --------------------------------------------------------------
 * D3DCallbacks3 - queried with GetDriverInfo (GUID_D3DCallbacks3).
 *
 * Clear2 - enables stencil clears (exposed to the API in
 *      IDirect3DViewport3::Clear2
 * ValidateTextureStageState - evaluates the context's current state (including
 *      multitexture) and returns an error if the hardware cannot
 *      accelerate the current state vector.
 * DrawPrimitives2 - Renders primitives, and changes device state specified
 *                   in the command buffer.
 *
 * Multitexture-aware drivers must implement both ValidateTextureStageState.
 */

typedef struct _D3DHAL_CLEAR2DATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

  // dwFlags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, and/or D3DCLEAR_STENCIL
    DWORD               dwFlags;        // in:  surfaces to clear

    DWORD               dwFillColor;    // in:  Color value for rtarget
    D3DVALUE            dvFillDepth;    // in:  Depth value for Z buffer (0.0-1.0)
    DWORD               dwFillStencil;  // in:  value used to clear stencil buffer

    LPD3DRECT           lpRects;        // in:  Rectangles to clear
    DWORD               dwNumRects;     // in:  Number of rectangles

    HRESULT             ddrval;         // out: Return value
} D3DHAL_CLEAR2DATA;
typedef D3DHAL_CLEAR2DATA FAR *LPD3DHAL_CLEAR2DATA;

typedef struct _D3DHAL_VALIDATETEXTURESTAGESTATEDATA
{
    ULONG_PTR           dwhContext; // in:  Context handle
    DWORD           dwFlags;    // in:  Flags, currently set to 0
    DWORD           dwReserved; //
    DWORD           dwNumPasses;// out: Number of passes the hardware
                                //      can perform the operation in
    HRESULT         ddrval;     // out: return value
} D3DHAL_VALIDATETEXTURESTAGESTATEDATA;
typedef D3DHAL_VALIDATETEXTURESTAGESTATEDATA *LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA;


typedef struct _D3DHAL_DRAWPRIMITIVES2DATA {
    ULONG_PTR             dwhContext;           // in: Context handle
    DWORD             dwFlags;              // in: flags
    DWORD             dwVertexType;         // in: vertex type
    LPDDRAWI_DDRAWSURFACE_LCL lpDDCommands; // in: vertex buffer command data
    DWORD             dwCommandOffset;      // in: offset to start of vertex buffer commands
    DWORD             dwCommandLength;      // in: number of bytes of command data
    union
    { // based on D3DHALDP2_USERMEMVERTICES flag
       LPDDRAWI_DDRAWSURFACE_LCL lpDDVertex;// in: surface containing vertex data
       LPVOID lpVertices;                   // in: User mode pointer to vertices
    };
    DWORD             dwVertexOffset;       // in: offset to start of vertex data
    DWORD             dwVertexLength;       // in: number of vertices of vertex data
    DWORD             dwReqVertexBufSize;   // in: number of bytes required for the next vertex buffer
    DWORD             dwReqCommandBufSize;  // in: number of bytes required for the next commnand buffer
    LPDWORD           lpdwRStates;          // in: Pointer to the array where render states are updated
    union 
    {
       DWORD          dwVertexSize;         // in: Size of each vertex in bytes
       HRESULT        ddrval;               // out: return value
    };
    DWORD             dwErrorOffset;        // out: offset in lpDDCommands to
                                            //      first D3DHAL_COMMAND not handled
} D3DHAL_DRAWPRIMITIVES2DATA, *LPD3DHAL_DRAWPRIMITIVES2DATA;

// Indicates that the lpVertices field in the DrawPrimitives2 data is
// valid, i.e. user allocated memory.
#define D3DHALDP2_USERMEMVERTICES   0x00000001L
// Indicates that the command buffer and vertex buffer are a system memory execute buffer
// resulting from the use of the Execute buffer API.
#define D3DHALDP2_EXECUTEBUFFER     0x00000002L
// The swap flags indicate if it is OK for the driver to swap the submitted buffers with new
// buffers and asyncronously work on the submitted buffers.
#define D3DHALDP2_SWAPVERTEXBUFFER  0x00000004L
#define D3DHALDP2_SWAPCOMMANDBUFFER 0x00000008L
// The requested flags are present if the new buffers which the driver can allocate need to be
// of atleast a given size. If any of these flags are set, the corresponding dwReq* field in
// D3DHAL_DRAWPRIMITIVES2DATA will also be set with the requested size in bytes.
#define D3DHALDP2_REQVERTEXBUFSIZE  0x00000010L
#define D3DHALDP2_REQCOMMANDBUFSIZE 0x00000020L
// These flags are set by the driver upon return from DrawPrimitives2 indicating if the new
// buffers are not in system memory.
#define D3DHALDP2_VIDMEMVERTEXBUF   0x00000040L
#define D3DHALDP2_VIDMEMCOMMANDBUF  0x00000080L


// Used by the driver to ask runtime to parse the execute buffer
#define D3DERR_COMMAND_UNPARSED              MAKE_DDHRESULT(3000)

typedef DWORD (__stdcall *LPD3DHAL_CLEAR2CB)        (LPD3DHAL_CLEAR2DATA);
typedef DWORD (__stdcall *LPD3DHAL_VALIDATETEXTURESTAGESTATECB)(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (__stdcall *LPD3DHAL_DRAWPRIMITIVES2CB)  (LPD3DHAL_DRAWPRIMITIVES2DATA);

typedef struct _D3DHAL_CALLBACKS3
{
    DWORD   dwSize;         // size of struct
    DWORD   dwFlags;        // flags for callbacks
    LPD3DHAL_CLEAR2CB                       Clear2;
    LPVOID                                  lpvReserved;
    LPD3DHAL_VALIDATETEXTURESTAGESTATECB    ValidateTextureStageState;
    LPD3DHAL_DRAWPRIMITIVES2CB              DrawPrimitives2;
} D3DHAL_CALLBACKS3;
typedef D3DHAL_CALLBACKS3 *LPD3DHAL_CALLBACKS3;
#define D3DHAL_CALLBACKS3SIZE       sizeof(D3DHAL_CALLBACKS3)

//  bit definitions for D3DHAL
#define D3DHAL3_CB32_CLEAR2                      0x00000001L
#define D3DHAL3_CB32_RESERVED                    0x00000002L
#define D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE   0x00000004L
#define D3DHAL3_CB32_DRAWPRIMITIVES2             0x00000008L

//-----------------------------------------------------------------------------
// DrawPrimitives2 DDI
//-----------------------------------------------------------------------------

//
// Command structure for vertex buffer rendering
//

typedef struct _D3DHAL_DP2COMMAND
{
	BYTE bCommand;           // vertex command
	BYTE bReserved;
	union
	{
		WORD wPrimitiveCount;   // primitive count for unconnected primitives
		WORD wStateCount;     // count of render states to follow
	};
} D3DHAL_DP2COMMAND, *LPD3DHAL_DP2COMMAND;

//
// DrawPrimitives2 commands:
//

typedef enum _D3DHAL_DP2OPERATION
{
    D3DDP2OP_POINTS               = 1,
    D3DDP2OP_INDEXEDLINELIST      = 2,
    D3DDP2OP_INDEXEDTRIANGLELIST  = 3,
    D3DDP2OP_RESERVED0            = 4,      // Used by the front-end only
    D3DDP2OP_RENDERSTATE          = 8,
    D3DDP2OP_LINELIST             = 15,
    D3DDP2OP_LINESTRIP            = 16,
    D3DDP2OP_INDEXEDLINESTRIP     = 17,
    D3DDP2OP_TRIANGLELIST         = 18,
    D3DDP2OP_TRIANGLESTRIP        = 19,
    D3DDP2OP_INDEXEDTRIANGLESTRIP = 20,
    D3DDP2OP_TRIANGLEFAN          = 21,
    D3DDP2OP_INDEXEDTRIANGLEFAN   = 22,
    D3DDP2OP_TRIANGLEFAN_IMM      = 23,
    D3DDP2OP_LINELIST_IMM         = 24,
    D3DDP2OP_TEXTURESTAGESTATE    = 25,     // Has edge flags and called from Execute
    D3DDP2OP_INDEXEDTRIANGLELIST2 = 26,
    D3DDP2OP_INDEXEDLINELIST2     = 27,
    D3DDP2OP_VIEWPORTINFO         = 28,
    D3DDP2OP_WINFO                = 29,
// two below are for pre-DX7 interface apps running DX7 driver
    D3DDP2OP_SETPALETTE           = 30,
    D3DDP2OP_UPDATEPALETTE        = 31,
//#if(DIRECT3D_VERSION >= 0x0700)
    // New for DX7
    D3DDP2OP_ZRANGE               = 32,
    D3DDP2OP_SETMATERIAL          = 33,
    D3DDP2OP_SETLIGHT             = 34,
    D3DDP2OP_CREATELIGHT          = 35,
    D3DDP2OP_SETTRANSFORM         = 36,
    D3DDP2OP_EXT                  = 37,
    D3DDP2OP_TEXBLT               = 38,
    D3DDP2OP_STATESET             = 39,
    D3DDP2OP_SETPRIORITY          = 40,
//#endif /* DIRECT3D_VERSION >= 0x0700 */
    D3DDP2OP_SETRENDERTARGET      = 41,
    D3DDP2OP_CLEAR                = 42,
//#if(DIRECT3D_VERSION >= 0x0700)
    D3DDP2OP_SETTEXLOD            = 43,
    D3DDP2OP_SETCLIPPLANE         = 44
//#endif /* DIRECT3D_VERSION >= 0x0700 */
} D3DHAL_DP2OPERATION;

//
// DrawPrimitives2 point primitives
//

typedef struct _D3DHAL_DP2POINTS
{
    WORD wCount;
    WORD wVStart;
} D3DHAL_DP2POINTS, *LPD3DHAL_DP2POINTS;

//
// DrawPrimitives2 line primitives
//

typedef struct _D3DHAL_DP2STARTVERTEX
{
    WORD wVStart;
} D3DHAL_DP2STARTVERTEX, *LPD3DHAL_DP2STARTVERTEX;

typedef struct _D3DHAL_DP2LINELIST
{
    WORD wVStart;
} D3DHAL_DP2LINELIST, *LPD3DHAL_DP2LINELIST;

typedef struct _D3DHAL_DP2INDEXEDLINELIST
{
    WORD wV1;
    WORD wV2;
} D3DHAL_DP2INDEXEDLINELIST, *LPD3DHAL_DP2INDEXEDLINELIST;

typedef struct _D3DHAL_DP2LINESTRIP
{
    WORD wVStart;
} D3DHAL_DP2LINESTRIP, *LPD3DHAL_DP2LINESTRIP;

typedef struct _D3DHAL_DP2INDEXEDLINESTRIP
{
    WORD wV[2];
} D3DHAL_DP2INDEXEDLINESTRIP, *LPD3DHAL_DP2INDEXEDLINESTRIP;

//
// DrawPrimitives2 triangle primitives
//

typedef struct _D3DHAL_DP2TRIANGLELIST
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLELIST, *LPD3DHAL_DP2TRIANGLELIST;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLELIST
{
    WORD wV1;
    WORD wV2;
    WORD wV3;
    WORD wFlags;
} D3DHAL_DP2INDEXEDTRIANGLELIST, *LPD3DHAL_DP2INDEXEDTRIANGLELIST;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLELIST2
{
    WORD wV1;
    WORD wV2;
    WORD wV3;
} D3DHAL_DP2INDEXEDTRIANGLELIST2, *LPD3DHAL_DP2INDEXEDTRIANGLELIST2;

typedef struct _D3DHAL_DP2TRIANGLESTRIP
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLESTRIP, *LPD3DHAL_DP2TRIANGLESTRIP;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLESTRIP
{
    WORD wV[3];
} D3DHAL_DP2INDEXEDTRIANGLESTRIP, *LPD3DHAL_DP2INDEXEDTRIANGLESTRIP;

typedef struct _D3DHAL_DP2TRIANGLEFAN
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLEFAN, *LPD3DHAL_DP2TRIANGLEFAN;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLEFAN
{
    WORD wV[3];
} D3DHAL_DP2INDEXEDTRIANGLEFAN, *LPD3DHAL_DP2INDEXEDTRIANGLEFAN;

typedef struct _D3DHAL_DP2TRIANGLEFAN_IMM
{
    DWORD dwEdgeFlags;
} D3DHAL_DP2TRIANGLEFAN_IMM, *LPD3DHAL_DP2TRIANGLEFAN_IMM;

//
// DrawPrimitives2 Renderstate changes
//

typedef struct _D3DHAL_DP2RENDERSTATE
{
    D3DRENDERSTATETYPE RenderState;
    union
    {
        D3DVALUE dvState;
        DWORD dwState;
    };
} D3DHAL_DP2RENDERSTATE, *LPD3DHAL_DP2RENDERSTATE;

typedef struct _D3DHAL_DP2TEXTURESTAGESTATE
{
    WORD wStage;
    WORD TSState;
    DWORD dwValue;
} D3DHAL_DP2TEXTURESTAGESTATE, *LPD3DHAL_DP2TEXTURESTAGESTATE;

typedef struct _D3DHAL_DP2VIEWPORTINFO
{
    DWORD dwX;
    DWORD dwY;
    DWORD dwWidth;
    DWORD dwHeight;
} D3DHAL_DP2VIEWPORTINFO, *LPD3DHAL_DP2VIEWPORTINFO;

typedef struct _D3DHAL_DP2WINFO
{
    D3DVALUE        dvWNear;
    D3DVALUE        dvWFar;
} D3DHAL_DP2WINFO, *LPD3DHAL_DP2WINFO;

typedef struct _D3DHAL_DP2SETPALETTE
{
    DWORD dwPaletteHandle;
    DWORD dwPaletteFlags;
    DWORD dwSurfaceHandle;
} D3DHAL_DP2SETPALETTE, *LPD3DHAL_DP2SETPALETTE;

typedef struct _D3DHAL_DP2UPDATEPALETTE
{
    DWORD dwPaletteHandle;
        WORD  wStartIndex;
        WORD  wNumEntries;
} D3DHAL_DP2UPDATEPALETTE, *LPD3DHAL_DP2UPDATEPALETTE;

typedef struct _D3DHAL_DP2SETRENDERTARGET
{
    DWORD hRenderTarget;
    DWORD hZBuffer;
} D3DHAL_DP2SETRENDERTARGET, *LPD3DHAL_DP2SETRENDERTARGET;

// Values for dwOperations in the D3DHAL_DP2STATESET
#define D3DHAL_STATESETBEGIN     0
#define D3DHAL_STATESETEND       1
#define D3DHAL_STATESETDELETE    2
#define D3DHAL_STATESETEXECUTE   3
#define D3DHAL_STATESETCAPTURE   4

typedef struct _D3DHAL_DP2STATESET
{
    DWORD               dwOperation;
    DWORD               dwParam;  // State set handle passed with D3DHAL_STATESETBEGIN,
                                  // D3DHAL_STATESETEXECUTE, D3DHAL_STATESETDELETE
                                  // D3DHAL_STATESETCAPTURE
    D3DSTATEBLOCKTYPE   sbType;   // Type use with D3DHAL_STATESETBEGIN/END
} D3DHAL_DP2STATESET, *LPD3DHAL_DP2STATESET;
//
// T&L Hal specific stuff
//
typedef struct _D3DHAL_DP2ZRANGE
{
    D3DVALUE    dvMinZ;
    D3DVALUE    dvMaxZ;
} D3DHAL_DP2ZRANGE, *LPD3DHAL_DP2ZRANGE;

typedef D3DMATERIAL7 D3DHAL_DP2SETMATERIAL, *LPD3DHAL_DP2SETMATERIAL;

// Values for dwDataType in D3DHAL_DP2SETLIGHT
#define D3DHAL_SETLIGHT_ENABLE   0
#define D3DHAL_SETLIGHT_DISABLE  1
// If this is set, light data will be passed in after the
// D3DLIGHT7 structure
#define D3DHAL_SETLIGHT_DATA     2

typedef struct _D3DHAL_DP2SETLIGHT
{
    DWORD     dwIndex;
    DWORD     dwDataType;
} D3DHAL_DP2SETLIGHT, *LPD3DHAL_DP2SETLIGHT;

typedef struct _D3DHAL_DP2SETCLIPPLANE
{
    DWORD     dwIndex;
    D3DVALUE  plane[4];
} D3DHAL_DP2SETCLIPPLANE, *LPD3DHAL_DP2SETCLIPPLANE;

typedef struct _D3DHAL_DP2CREATELIGHT
{
    DWORD dwIndex;
} D3DHAL_DP2CREATELIGHT, *LPD3DHAL_DP2CREATELIGHT;

typedef struct _D3DHAL_DP2SETTRANSFORM
{
    D3DTRANSFORMSTATETYPE xfrmType;
    D3DMATRIX matrix;
} D3DHAL_DP2SETTRANSFORM, *LPD3DHAL_DP2SETTRANSFORM;

typedef struct _D3DHAL_DP2EXT
{
    DWORD dwExtToken;
    DWORD dwSize;
} D3DHAL_DP2EXT, *LPD3DHAL_DP2EXT;

typedef struct _D3DHAL_DP2TEXBLT
{
    DWORD   dwDDDestSurface;// dest surface
    DWORD   dwDDSrcSurface; // src surface
    POINT   pDest;
    RECTL   rSrc;       // src rect
    DWORD   dwFlags;    // blt flags
} D3DHAL_DP2TEXBLT, *LPD3DHAL_DP2TEXBLT;

typedef struct _D3DHAL_DP2SETPRIORITY
{
    DWORD dwDDSurface;
    DWORD dwPriority;
} D3DHAL_DP2SETPRIORITY, *LPD3DHAL_DP2SETPRIORITY;

typedef struct _D3DHAL_DP2CLEAR
{
  // dwFlags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, and/or D3DCLEAR_STENCIL
    DWORD               dwFlags;        // in:  surfaces to clear
    DWORD               dwFillColor;    // in:  Color value for rtarget
    D3DVALUE            dvFillDepth;    // in:  Depth value for Z buffer (0.0-1.0)
    DWORD               dwFillStencil;  // in:  value used to clear stencil buffer
    RECT                Rects[1];       // in:  Rectangles to clear
} D3DHAL_DP2CLEAR, *LPD3DHAL_DP2CLEAR;

typedef struct _D3DHAL_DP2SETTEXLOD
{
    DWORD dwDDSurface;
    DWORD dwLOD;
} D3DHAL_DP2SETTEXLOD, *LPD3DHAL_DP2SETTEXLOD;

// typedef for the Callback that the drivers can use to parse unknown commands
// passed to them via the DrawPrimitives2 callback. The driver obtains this
// callback thru a GetDriverInfo call with GUID_D3DParseUnknownCommandCallback
// made by ddraw somewhere around the initialization time.
typedef HRESULT (__stdcall *PFND3DPARSEUNKNOWNCOMMAND) (LPVOID lpvCommands, LPVOID *lplpvReturnedCommand);

/*
 * Texture handle's offset into the 32-DWORD cascade state vector
 */
#define D3DTSS_TEXTUREMAP 0

/* exec bufer flags */
#define D3DDEVCAPS_HWVERTEXBUFFER 0x02000000L /* Device supports Driver Allocated Vertex Buffers*/
#define D3DDEVCAPS_HWINDEXBUFFER  0x04000000L /* Device supports Driver Allocated Index Buffers*/
#define D3DDEVCAPS_SUBVOLUMELOCK  0x08000000L /* Device supports locking a part of volume texture*/

#endif /* _D3DHAL_H */
