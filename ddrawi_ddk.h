#ifndef __DDRAWI_DDK_H__INCLUDED__
#define __DDRAWI_DDK_H__INCLUDED__

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

#endif /* __DDRAWI_DDK_H__INCLUDED__ */
