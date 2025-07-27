#ifndef __DDRAWI_DDK_H__INCLUDED__
#define __DDRAWI_DDK_H__INCLUDED__

DEFINE_GUID( GUID_VideoPortCallbacks,       0xefd60cc1, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_ColorControlCallbacks,    0xefd60cc2, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_VideoPortCaps,            0xefd60cc3, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_KernelCallbacks,      0x80863800, 0x6B06, 0x11D0, 0x9B, 0x06, 0x0, 0xA0, 0xC9, 0x03, 0xA3, 0xB8);
DEFINE_GUID( GUID_KernelCaps,           0xFFAA7540, 0x7AA8, 0x11D0, 0x9B, 0x06, 0x00, 0xA0, 0xC9, 0x03, 0xA3, 0xB8);

/* extra defines which missing in MinGW ddrawi.h */

/*
 * structure for passing information to DDHAL GetFlipStatus fn
 */
typedef struct _DDHAL_GETFLIPSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_GETFLIPSTATUS	GetFlipStatus;	// PRIVATE: ptr to callback
} DDHAL_GETFLIPSTATUSDATA;

/*
 * structure for passing information to DDHAL WaitForVerticalBlank fn
 */
typedef struct _DDHAL_WAITFORVERTICALBLANKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL		lpDD;		// driver struct
    DWORD				dwFlags;	// flags
    DWORD				bIsInVB;	// is in vertical blank
    DWORD				hEvent;		// event
    HRESULT				ddRVal;		// return value
    LPDDHAL_WAITFORVERTICALBLANK	WaitForVerticalBlank; // PRIVATE: ptr to callback
} DDHAL_WAITFORVERTICALBLANKDATA;

/*
 * Return if the vertical blank is in progress
 */
#define DDWAITVB_I_TESTVB			0x80000006l

/*
 * structure for passing information to DDHAL GetBltStatus fn
 */
typedef struct _DDHAL_GETBLTSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_GETBLTSTATUS	GetBltStatus;	// PRIVATE: ptr to callback
} DDHAL_GETBLTSTATUSDATA;

/*
 * value in the fpVidMem; indicates dwBlockSize is valid (surface object)
 */
#define DDHAL_PLEASEALLOC_BLOCKSIZE	0x00000002l

/*
 * Values in fpVidMem: Indicates dwLinearSizde is valid.
 * THIS VALUE CAN ONLY BE USED BY A D3D Optimize DRIVER FUNCTION
 * IT IS INVALID FOR A DRIVER TO RETURN THIS VALUE FROM CreateSurface32.
 */
#define DDHAL_PLEASEALLOC_LINEARSIZE	0x00000003l

// This structure can be queried from the driver from DX5 onward
// using GetDriverInfo with GUID_MiscellaneousCallbacks
#define DDHAL_MISCCB32_GETAVAILDRIVERMEMORY    0x00000001l
#define DDHAL_MISCCB32_UPDATENONLOCALHEAP      0x00000002l
#define DDHAL_MISCCB32_GETHEAPALIGNMENT        0x00000004l
#define DDHAL_MISCCB32_GETSYSMEMBLTSTATUS      0x00000008l

typedef struct _DDHAL_GETAVAILDRIVERMEMORYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL lpDD;        // driver struct
    DDSCAPS                 DDSCaps;     // caps for type of surface memory
    DWORD                   dwTotal;     // total memory for this kind of surface
    DWORD                   dwFree;      // free memory for this kind of surface
    HRESULT                 ddRVal;      // return value
    LPDDHAL_GETAVAILDRIVERMEMORY   GetAvailDriverMemory; // PRIVATE: ptr to callback
    DDSCAPSEX               ddsCapsEx;       // Added in V6. Driver should check DDVERSION info
                                                 // to see if this field is present.
} DDHAL_GETAVAILDRIVERMEMORYDATA;

/*
 * More driver surface capabilities (in addition to those described in DDCORECAPS).
 * This struct contains the caps bits added to the DDCAPS.ddsCaps structure in DX6.
 */
typedef struct _DDMORESURFACECAPS
{
    DWORD       dwSize;             // size of DDMORESURFACECAPS structure
    DDSCAPSEX   ddsCapsMore;
    /*
     * The DDMORESURFACECAPS struct is of variable size. The following list may be
     * filled in by DX6-aware drivers (see DDVERSIONINFO) to restrict their
     * video memory heaps (those which are exposed to DirectDraw) to
     * certain sets of DDSCAPS_ bits. Thse entries are exactly analogous to
     * the ddsCaps and ddsCapsAlt members of the VIDMEM structures listed in
     * the VIDMEMINFO.pvmList member of DDHALINFO.vmiData. There should be
     * exactly DDHALINFO.vmiData.dwNumHeaps copies of tagExtendedHeapRestrictions
     * in this struct. The size of this struct is thus:
     *  DDMORESURFACECAPS.dwSize = sizeof(DDMORESURFACECAPS) +
     *          (DDHALINFO.vmiData.dwNumHeaps-1) * sizeof(DDSCAPSEX)*2;
     * Note the -1 accounts for the fact that DDMORESURFACECAPS is declared to have 1
     * tagExtendedHeapRestrictions member.
     */
    struct tagExtendedHeapRestrictions
    {
        DDSCAPSEX   ddsCapsEx;
        DDSCAPSEX   ddsCapsExAlt;
    } ddsExtendedHeapRestrictions[1];
} DDMORESURFACECAPS, *LPDDMORESURFACECAPS;

typedef struct _DDHAL_CREATESURFACEEXDATA {
    DWORD                       dwFlags;    // Currently always 0 and not used
    LPDDRAWI_DIRECTDRAW_LCL     lpDDLcl;    // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl;   // list of created surface objects
    HRESULT			ddRVal;     // return value
} DDHAL_CREATESURFACEEXDATA;

typedef struct _DDHAL_GETDRIVERSTATEDATA {
    DWORD                       dwFlags;        // Flags to indicate the data
                                                // required
    union
    {
        // LPDDRAWI_DIRECTDRAW_GBL     lpDD;           // driver struct
        DWORD                       dwhContext;     // d3d context
    };
    LPDWORD                     lpdwStates;     // ptr to the state data
                                                // to be filled in by the
                                                // driver
    DWORD                       dwLength;
    HRESULT                     ddRVal;         // return value
} DDHAL_GETDRIVERSTATEDATA;

typedef struct _DDHAL_DESTROYDDLOCALDATA
{
    DWORD dwFlags;
    LPDDRAWI_DIRECTDRAW_LCL pDDLcl;
    HRESULT  ddRVal;
} DDHAL_DESTROYDDLOCALDATA;

#define DDHAL_MISC2CB32_CREATESURFACEEX        0x00000002l
#define DDHAL_MISC2CB32_GETDRIVERSTATE         0x00000004l
#define DDHAL_MISC2CB32_DESTROYDDLOCAL         0x00000008l

/*
 * structure for passing information to DDHAL SetColorKey fn
 */
typedef struct _DDHAL_SETCOLORKEYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD 			dwFlags;	// flags
    DDCOLORKEY 			ckNew;		// new color key
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_SETCOLORKEY	SetColorKey;	// PRIVATE: ptr to callback
} DDHAL_SETCOLORKEYDATA;

/*
 * structure for passing information to DDHAL AddAttachedSurface fn
 */
typedef struct _DDHAL_ADDATTACHEDSURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL         lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL       lpDDSurface;    // surface struct
    LPDDRAWI_DDRAWSURFACE_LCL       lpSurfAttached; // surface to attach
    HRESULT                         ddRVal;     // return value
    LPDDHALSURFCB_ADDATTACHEDSURFACE    AddAttachedSurface; // PRIVATE: ptr to callback
} DDHAL_ADDATTACHEDSURFACEDATA;

/*
 * structure for passing information to DDHAL CanCreateVideoPort fn
 */
typedef struct _DDHAL_CANCREATEVPORTDATA *LPDDHAL_CANCREATEVPORTDATA;
typedef struct _DDHAL_CREATEVPORTDATA *LPDDHAL_CREATEVPORTDATA;
typedef struct _DDHAL_FLIPVPORTDATA *LPDDHAL_FLIPVPORTDATA;
typedef struct _DDHAL_GETVPORTCONNECTDATA *LPDDHAL_GETVPORTCONNECTDATA;
typedef struct _DDHAL_GETVPORTBANDWIDTHDATA *LPDDHAL_GETVPORTBANDWIDTHDATA;
typedef struct _DDHAL_GETVPORTINPUTFORMATDATA *LPDDHAL_GETVPORTINPUTFORMATDATA;
typedef struct _DDHAL_GETVPORTOUTPUTFORMATDATA *LPDDHAL_GETVPORTOUTPUTFORMATDATA;
typedef struct _DDHAL_GETVPORTFIELDDATA *LPDDHAL_GETVPORTFIELDDATA;
typedef struct _DDHAL_GETVPORTLINEDATA *LPDDHAL_GETVPORTLINEDATA;
typedef struct _DDHAL_DESTROYVPORTDATA *LPDDHAL_DESTROYVPORTDATA;
typedef struct _DDHAL_GETVPORTFLIPSTATUSDATA *LPDDHAL_GETVPORTFLIPSTATUSDATA;
typedef struct _DDHAL_UPDATEVPORTDATA *LPDDHAL_UPDATEVPORTDATA;
typedef struct _DDHAL_WAITFORVPORTSYNCDATA *LPDDHAL_WAITFORVPORTSYNCDATA;
typedef struct _DDHAL_GETVPORTSIGNALDATA *LPDDHAL_GETVPORTSIGNALDATA;
typedef struct _DDHAL_VPORTCOLORDATA *LPDDHAL_VPORTCOLORDATA;

/*
 * DIRECTVIDEOPORT object callbacks
 */
typedef DWORD (__stdcall *LPDDHALVPORTCB_CANCREATEVIDEOPORT)(LPDDHAL_CANCREATEVPORTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_CREATEVIDEOPORT)(LPDDHAL_CREATEVPORTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_FLIP)(LPDDHAL_FLIPVPORTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETBANDWIDTH)(LPDDHAL_GETVPORTBANDWIDTHDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETINPUTFORMATS)(LPDDHAL_GETVPORTINPUTFORMATDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETOUTPUTFORMATS)(LPDDHAL_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETFIELD)(LPDDHAL_GETVPORTFIELDDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETLINE)(LPDDHAL_GETVPORTLINEDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETVPORTCONNECT)(LPDDHAL_GETVPORTCONNECTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_DESTROYVPORT)(LPDDHAL_DESTROYVPORTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETFLIPSTATUS)(LPDDHAL_GETVPORTFLIPSTATUSDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_UPDATE)(LPDDHAL_UPDATEVPORTDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_WAITFORSYNC)(LPDDHAL_WAITFORVPORTSYNCDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_GETSIGNALSTATUS)(LPDDHAL_GETVPORTSIGNALDATA);
typedef DWORD (__stdcall *LPDDHALVPORTCB_COLORCONTROL)(LPDDHAL_VPORTCOLORDATA);

typedef struct _DDHAL_DDVIDEOPORTCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALVPORTCB_CANCREATEVIDEOPORT   CanCreateVideoPort;
    LPDDHALVPORTCB_CREATEVIDEOPORT      CreateVideoPort;
    LPDDHALVPORTCB_FLIP                 FlipVideoPort;
    LPDDHALVPORTCB_GETBANDWIDTH         GetVideoPortBandwidth;
    LPDDHALVPORTCB_GETINPUTFORMATS      GetVideoPortInputFormats;
    LPDDHALVPORTCB_GETOUTPUTFORMATS     GetVideoPortOutputFormats;
    LPVOID              lpReserved1;
    LPDDHALVPORTCB_GETFIELD             GetVideoPortField;
    LPDDHALVPORTCB_GETLINE              GetVideoPortLine;
    LPDDHALVPORTCB_GETVPORTCONNECT      GetVideoPortConnectInfo;
    LPDDHALVPORTCB_DESTROYVPORT         DestroyVideoPort;
    LPDDHALVPORTCB_GETFLIPSTATUS        GetVideoPortFlipStatus;
    LPDDHALVPORTCB_UPDATE               UpdateVideoPort;
    LPDDHALVPORTCB_WAITFORSYNC          WaitForVideoPortSync;
    LPDDHALVPORTCB_GETSIGNALSTATUS      GetVideoSignalStatus;
    LPDDHALVPORTCB_COLORCONTROL         ColorControl;
} DDHAL_DDVIDEOPORTCALLBACKS;

/*
 * DIRECTDRAWCOLORCONTROL object callbacks
 */
typedef struct _DDHAL_COLORCONTROLDATA *LPDDHAL_COLORCONTROLDATA;

typedef DWORD (__stdcall *LPDDHALCOLORCB_COLORCONTROL)(LPDDHAL_COLORCONTROLDATA);

typedef struct _DDHAL_DDCOLORCONTROLCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALCOLORCB_COLORCONTROL         ColorControl;
} DDHAL_DDCOLORCONTROLCALLBACKS;

/*
 * DIRECTDRAWSURFACEKERNEL object callbacks
 * This structure can be queried from the driver from DX5 onward
 * using GetDriverInfo with GUID_KernelCallbacks
 */
typedef struct _DDHAL_SYNCSURFACEDATA *LPDDHAL_SYNCSURFACEDATA;
typedef struct _DDHAL_SYNCVIDEOPORTDATA *LPDDHAL_SYNCVIDEOPORTDATA;
typedef DWORD (__stdcall *LPDDHALKERNELCB_SYNCSURFACE)(LPDDHAL_SYNCSURFACEDATA);
typedef DWORD (__stdcall *LPDDHALKERNELCB_SYNCVIDEOPORT)(LPDDHAL_SYNCVIDEOPORTDATA);

typedef struct _DDHAL_DDKERNELCALLBACKS
{
    DWORD                               dwSize;
    DWORD                               dwFlags;
    LPDDHALKERNELCB_SYNCSURFACE     SyncSurfaceData;
    LPDDHALKERNELCB_SYNCVIDEOPORT   SyncVideoPortData;
} DDHAL_DDKERNELCALLBACKS, *LPDDHAL_DDKERNELCALLBACKS;

/*
 * DDKERNELCAPS
 */
typedef struct _DDKERNELCAPS
{
    DWORD dwSize;			// size of the DDKERNELCAPS structure
    DWORD dwCaps;                       // Contains the DDKERNELCAPS_XXX flags
    DWORD dwIRQCaps;                    // Contains the DDIRQ_XXX flags
} DDKERNELCAPS, *LPDDKERNELCAPS;

/* originaly in dvp.h */
/*
 * DDVIDEOPORTCAPS
 */
typedef struct _DDVIDEOPORTCAPS
{
    DWORD dwSize;			// size of the DDVIDEOPORTCAPS structure
    DWORD dwFlags;			// indicates which fields contain data
    DWORD dwMaxWidth;			// max width of the video port field
    DWORD dwMaxVBIWidth;		// max width of the VBI data
    DWORD dwMaxHeight; 			// max height of the video port field
    DWORD dwVideoPortID;		// Video port ID (0 - (dwMaxVideoPorts -1))
    DWORD dwCaps;			// Video port capabilities
    DWORD dwFX;				// More video port capabilities
    DWORD dwNumAutoFlipSurfaces;	// Max number of autoflippable surfaces allowed
    DWORD dwAlignVideoPortBoundary;	// Byte restriction of placement within the surface
    DWORD dwAlignVideoPortPrescaleWidth;// Byte restriction of width after prescaling
    DWORD dwAlignVideoPortCropBoundary;	// Byte restriction of left cropping
    DWORD dwAlignVideoPortCropWidth;	// Byte restriction of cropping width
    DWORD dwPreshrinkXStep;		// Width can be shrunk in steps of 1/x
    DWORD dwPreshrinkYStep;		// Height can be shrunk in steps of 1/x
    DWORD dwNumVBIAutoFlipSurfaces;	// Max number of VBI autoflippable surfaces allowed
    DWORD dwNumPreferredAutoflip;	// Optimal number of autoflippable surfaces for hardware
    WORD  wNumFilterTapsX;              // Number of taps the prescaler uses in the X direction (0 - no prescale, 1 - replication, etc.)
    WORD  wNumFilterTapsY;              // Number of taps the prescaler uses in the Y direction (0 - no prescale, 1 - replication, etc.)
} DDVIDEOPORTCAPS;

/*
 * The dwMaxWidth and dwMaxVBIWidth members are valid
 */
#define DDVPD_WIDTH		0x00000001l

/*
 * The dwMaxHeight member is valid
 */
#define DDVPD_HEIGHT		0x00000002l

/*
 * The dwVideoPortID member is valid
 */
#define DDVPD_ID		0x00000004l

/*
 * The dwCaps member is valid
 */
#define DDVPD_CAPS		0x00000008l

/*
 * The dwFX member is valid
 */
#define DDVPD_FX		0x00000010l

/*
 * The dwNumAutoFlipSurfaces member is valid
 */
#define DDVPD_AUTOFLIP		0x00000020l

/*
 * All of the alignment members are valid
 */
#define DDVPD_ALIGN		0x00000040l

/*
 * The dwNumPreferredAutoflip member is valid
 */
#define DDVPD_PREFERREDAUTOFLIP 0x00000080l

/*
 * The wNumFilterTapsX and wNumFilterTapsY fields are valid
 */
#define DDVPD_FILTERQUALITY     0x00000100l

/* DDHAL_DDEXEBUFCALLBACKS flags */
#define DDHAL_EXEBUFCB32_CANCREATEEXEBUF    0x00000001l
#define DDHAL_EXEBUFCB32_CREATEEXEBUF       0x00000002l
#define DDHAL_EXEBUFCB32_DESTROYEXEBUF      0x00000004l
#define DDHAL_EXEBUFCB32_LOCKEXEBUF         0x00000008l
#define DDHAL_EXEBUFCB32_UNLOCKEXEBUF       0x00000010l

/*
 * This DDPF flag is used to indicate a DX8+ format capability entry in
 * the texture format list. It is not visible to applications.
 */
#define DDPF_D3DFORMAT                                          0x00200000l

/*
 * List of operations supported on formats in DX8+ texture list.
 * See the DX8 DDK for a complete description of these flags.
 */
#define D3DFORMAT_OP_TEXTURE                    0x00000001L
#define D3DFORMAT_OP_VOLUMETEXTURE              0x00000002L
#define D3DFORMAT_OP_CUBETEXTURE                0x00000004L
#define D3DFORMAT_OP_OFFSCREEN_RENDERTARGET     0x00000008L
#define D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET   0x00000010L
#define D3DFORMAT_OP_ZSTENCIL                   0x00000040L
#define D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH 0x00000080L

// This format can be used as a render target if the current display mode
// is the same depth if the alpha channel is ignored. e.g. if the device 
// can render to A8R8G8B8 when the display mode is X8R8G8B8, then the
// format op list entry for A8R8G8B8 should have this cap. 
#define D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET 0x00000100L

// This format contains DirectDraw support (including Flip).  This flag
// should not to be set on alpha formats.
#define D3DFORMAT_OP_DISPLAYMODE                0x00000400L

// The rasterizer can support some level of Direct3D support in this format
// and implies that the driver can create a Context in this mode (for some 
// render target format).  When this flag is set, the D3DFORMAT_OP_DISPLAYMODE
// flag must also be set.
#define D3DFORMAT_OP_3DACCELERATION             0x00000800L

// If the driver needs a private format to be D3D or driver manageable,
// then it needs to tell D3D the pixelsize in bits per pixel by setting
// dwPrivateFormatBitCount in DDPIXELFORMAT and by setting the below
// format op. If the below format op is not set, then D3D or the driver
// will NOT be allowed to manage the format.
#define D3DFORMAT_OP_PIXELSIZE                  0x00001000L

// Indicates that this format can be converted to any RGB format for which
// D3DFORMAT_MEMBEROFGROUP_ARGB is specified
#define D3DFORMAT_OP_CONVERT_TO_ARGB            0x00002000L

// Indicates that this format can be used to create offscreen plain surfaces.
#define D3DFORMAT_OP_OFFSCREENPLAIN             0x00004000L

// Indicated that this format can be read as an SRGB texture (meaning that the
// sampler will linearize the looked up data)
#define D3DFORMAT_OP_SRGBREAD                   0x00008000L

// Indicates that this format can be used in the bumpmap instructions
#define D3DFORMAT_OP_BUMPMAP                    0x00010000L

// Indicates that this format can be sampled by the displacement map sampler
#define D3DFORMAT_OP_DMAP                       0x00020000L

// Indicates that this format cannot be used with texture filtering
#define D3DFORMAT_OP_NOFILTER                   0x00040000L

// Indicates that format conversions are supported to this RGB format if
// D3DFORMAT_OP_CONVERT_TO_ARGB is specified in the source format.
#define D3DFORMAT_MEMBEROFGROUP_ARGB            0x00080000L

// Indicated that this format can be written as an SRGB target (meaning that the
// pixel pipe will DE-linearize data on output to format)
#define D3DFORMAT_OP_SRGBWRITE                  0x00100000L

// Indicates that this format cannot be used with alpha blending
#define D3DFORMAT_OP_NOALPHABLEND               0x00200000L

//Indicates that the device can auto-generated sublevels for resources
//of this format
#define D3DFORMAT_OP_AUTOGENMIPMAP              0x00400000L

// Indicates that this format cannot be used by vertex texture sampler
#define D3DFORMAT_OP_VERTEXTEXTURE              0x00800000L 

// Indicates that this format supports neither texture coordinate wrap modes, nor mipmapping
#define D3DFORMAT_OP_NOTEXCOORDWRAPNORMIP		0x01000000L

DEFINE_GUID( GUID_MotionCompCallbacks,          0xb1122b40, 0x5dA5, 0x11d1, 0x8f, 0xcF, 0x00, 0xc0, 0x4f, 0xc2, 0x9b, 0x4e);

typedef struct _DD_GETMOCOMPGUIDSDATA *PDD_GETMOCOMPGUIDSDATA;
typedef struct _DD_GETMOCOMPFORMATSDATA *PDD_GETMOCOMPFORMATSDATA;
typedef struct _DD_CREATEMOCOMPDATA *PDD_CREATEMOCOMPDATA;
typedef struct _DD_GETMOCOMPCOMPBUFFDATA *PDD_GETMOCOMPCOMPBUFFDATA;
typedef struct _DD_GETINTERNALMOCOMPDATA *PDD_GETINTERNALMOCOMPDATA;
typedef struct _DD_BEGINMOCOMPFRAMEDATA *PDD_BEGINMOCOMPFRAMEDATA;
typedef struct _DD_ENDMOCOMPFRAMEDATA *PDD_ENDMOCOMPFRAMEDATA;
typedef struct _DD_RENDERMOCOMPDATA *PDD_RENDERMOCOMPDATA;
typedef struct _DD_QUERYMOCOMPSTATUSDATA *PDD_QUERYMOCOMPSTATUSDATA;
typedef struct _DD_DESTROYMOCOMPDATA *PDD_DESTROYMOCOMPDATA;

/*
 * DIRECTDRAWVIDEO object callbacks
 */
typedef DWORD (__stdcall *PDD_MOCOMPCB_GETGUIDS)( PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_GETFORMATS)( PDD_GETMOCOMPFORMATSDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_CREATE)( PDD_CREATEMOCOMPDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_GETCOMPBUFFINFO)( PDD_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_GETINTERNALINFO)( PDD_GETINTERNALMOCOMPDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_BEGINFRAME)( PDD_BEGINMOCOMPFRAMEDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_ENDFRAME)( PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_RENDER)( PDD_RENDERMOCOMPDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_QUERYSTATUS)( PDD_QUERYMOCOMPSTATUSDATA);
typedef DWORD (__stdcall *PDD_MOCOMPCB_DESTROY)( PDD_DESTROYMOCOMPDATA);

typedef struct DD_MOTIONCOMPCALLBACKS
{
    DWORD                           dwSize;
    DWORD                           dwFlags;
    PDD_MOCOMPCB_GETGUIDS           GetMoCompGuids;
    PDD_MOCOMPCB_GETFORMATS         GetMoCompFormats;
    PDD_MOCOMPCB_CREATE             CreateMoComp;
    PDD_MOCOMPCB_GETCOMPBUFFINFO    GetMoCompBuffInfo;
    PDD_MOCOMPCB_GETINTERNALINFO    GetInternalMoCompInfo;
    PDD_MOCOMPCB_BEGINFRAME         BeginMoCompFrame;
    PDD_MOCOMPCB_ENDFRAME           EndMoCompFrame;
    PDD_MOCOMPCB_RENDER             RenderMoComp;
    PDD_MOCOMPCB_QUERYSTATUS        QueryMoCompStatus;
    PDD_MOCOMPCB_DESTROY            DestroyMoComp;
} DD_MOTIONCOMPCALLBACKS;
typedef DD_MOTIONCOMPCALLBACKS *PDD_MOTIONCOMPCALLBACKS;

#define DDHAL_MOCOMP32_GETGUIDS                 0x00000001
#define DDHAL_MOCOMP32_GETFORMATS               0x00000002
#define DDHAL_MOCOMP32_CREATE                   0x00000004
#define DDHAL_MOCOMP32_GETCOMPBUFFINFO          0x00000008
#define DDHAL_MOCOMP32_GETINTERNALINFO          0x00000010
#define DDHAL_MOCOMP32_BEGINFRAME               0x00000020
#define DDHAL_MOCOMP32_ENDFRAME                 0x00000040
#define DDHAL_MOCOMP32_RENDER                   0x00000080
#define DDHAL_MOCOMP32_QUERYSTATUS              0x00000100
#define DDHAL_MOCOMP32_DESTROY                  0x00000200

#endif /* __DDRAWI_DDK_H__INCLUDED__ */
