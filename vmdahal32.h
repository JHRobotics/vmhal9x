#ifndef __VMDAHAL32_H__INCLUDED__
#define __VMDAHAL32_H__INCLUDED__

#include "3d_accel.h"

#pragma pack(push)
#pragma pack(1)

typedef struct VMDAHALCB32
{
	LPDDHAL_CREATESURFACE             CreateSurface;
	LPDDHAL_CANCREATESURFACE          CanCreateSurface;
	LPDDHALSURFCB_DESTROYSURFACE      DestroySurface;
	LPDDHALSURFCB_FLIP                Flip;
	LPDDHALSURFCB_SETCLIPLIST         SetClipList;
	LPDDHALSURFCB_LOCK                Lock;
	LPDDHALSURFCB_UNLOCK              Unlock;
	LPDDHALSURFCB_BLT                 Blt;
	LPDDHALSURFCB_SETCOLORKEY         SetColorKey;
	LPDDHALSURFCB_ADDATTACHEDSURFACE  AddAttachedSurface;
	LPDDHALSURFCB_GETBLTSTATUS        GetBltStatus;
	LPDDHALSURFCB_GETFLIPSTATUS       GetFlipStatus;
	LPDDHALSURFCB_UPDATEOVERLAY       UpdateOverlay;
	LPDDHALSURFCB_SETOVERLAYPOSITION  SetOverlayPosition;
	LPDDHAL_GETDRIVERINFO             GetDriverInfo;
	LPDDHAL_WAITFORVERTICALBLANK      WaitForVerticalBlank;
	LPDDHAL_SETMODE		                SetMode;
	LPDDHAL_SETEXCLUSIVEMODE          SetExclusiveMode;
} VMDAHALCB32_t;

struct VXD_pair
{
	DWORD pid;
	DWORD vxd;
};

#define VXD_PAIRS_CNT 32

typedef struct VMDAHAL
{
	DWORD dwSize;
	DWORD vramLinear;
	DWORD vramSize;
	
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwBpp;
	DWORD dwPitch;
	
  DDPIXELFORMAT ddpf;
  DDHALINFO     ddHALInfo;
  
  VMDAHALCB32_t cb32;
  
  DWORD hInstance;
  
  FBHDA_t *pFBHDA32;
  DWORD pFBHDA16;
  DWORD FBHDA_version;
  
  HDC hDC;
  
  struct VXD_pair vxd_table[VXD_PAIRS_CNT];
  
} VMDAHAL_t;
#pragma pack(pop)


#endif
