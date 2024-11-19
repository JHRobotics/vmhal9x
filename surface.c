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
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"

#include "nocrt.h"

/* 
	JH: when driver needs save information of surface it can use
		_DDRAWI_DDRAWSURFACE_GBL.dwReserved1 (not works for system memory surfaces)
	or
		_DDRAWI_DDRAWSURFACE_GBL_MORE.dwDriverReserved
	
	But problem is, that pointer to video ram is changing - for example after
	FLIP operation ponter system will exchange pointer between surfaces.
	
	So I write these function to store information relatively to flat address.
	
*/

extern HANDLE hSharedHeap;

#define SURF_HT_MOD 113

static SurfaceInfo_t *surtface_ht[SURF_HT_MOD];

void SurfaceInfoInit()
{
	memset(&surtface_ht[0], 0, sizeof(surtface_ht));
}

static inline SurfaceInfo_t *SurfaceInfoGetInline(FLATPTR lin_address, BOOL create)
{
	SurfaceInfo_t **pptr = &surtface_ht[lin_address % SURF_HT_MOD];
	
	while(*pptr != NULL)
	{
		if((*pptr)->lin_address == lin_address)
		{
			return *pptr;
		}
		
		pptr = &((*pptr)->next);
	}
	
	if(create)
	{
		TOPIC("GL", "Allocated info for addr:%X", lin_address);
		SurfaceInfo_t *info = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(SurfaceInfo_t));
		if(info != NULL)
		{
			info->lin_address = lin_address;
			info->texture_dirty = TRUE;
			info->next = NULL; /* for all cases */
		
			*pptr = info;
		
			return info;
		}
	}
	
	return NULL;
}

void SurfaceInfoErase(FLATPTR lin_address)
{
	SurfaceInfo_t **pptr = &surtface_ht[lin_address % SURF_HT_MOD];
	
	while(*pptr != NULL)
	{
		if((*pptr)->lin_address == lin_address)
		{
			SurfaceInfo_t *info = *pptr;
			*pptr = ((*pptr)->next);
			
			info->next = NULL;
			HeapFree(hSharedHeap, 0, info);
		}
		else
		{
			pptr = &((*pptr)->next);
		}
	}
}

SurfaceInfo_t *SurfaceInfoGet(FLATPTR lin_address, BOOL create)
{
	return SurfaceInfoGetInline(lin_address, create);
}

void SurfaceInfoMakeDirty(FLATPTR lin_address)
{
	SurfaceInfo_t *info = SurfaceInfoGetInline(lin_address, FALSE);
	if(info)
	{
		info->texture_dirty = TRUE;
	}
}

void SurfaceInfoMakeClean(FLATPTR lin_address)
{
	SurfaceInfo_t *info = SurfaceInfoGetInline(lin_address, TRUE);
	if(info)
	{
		info->texture_dirty = FALSE;
	}
}

static LONG sharedLock = 0;

void SurfaceCtxLock()
{
	LONG tmp;
	do
	{
		tmp = InterlockedExchange(&sharedLock, 1);
	} while(tmp == 1);
}

void SurfaceCtxUnlock()
{
	InterlockedExchange(&sharedLock, 0);
}
