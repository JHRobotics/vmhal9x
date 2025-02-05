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
} DDMORESURFACECAPS, FAR * LPDDMORESURFACECAPS;

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

#endif /* __DDRAWI_DDK_H__INCLUDED__ */
