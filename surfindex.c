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
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"

#include "nocrt.h"
#endif

static void ddsurf_pair(ddsurface_t *first, ddsurface_t *second)
{
	first->down = second;
	second->up  = first;
}

static void ddsurf_unpair(ddsurface_t *surf)
{
	if(surf->up != NULL)
	{
		surf->up->down = surf->down;
	}
	if(surf->down != NULL)
	{
		surf->down->up = surf->up;
	}
	surf->up   = NULL;
	surf->down = NULL;
}

static BOOL conv_fbsurf_in(LPDDRAWI_DDRAWSURFACE_LCL in, FBHDA_DD_surface_t *out, FBHDA_t *hda)
{
	DDPIXELFORMAT fmt = in->lpGbl->ddpfSurface;
	out->uid   = 0;
	out->width = in->lpGbl->wWidth;
	out->height = in->lpGbl->wHeight;
	out->data  = (void*)in->lpGbl->fpVidMem;

	memset(&(out->attrs), 0, sizeof(FBHDA_DD_surface_attrs_t));
	
	if(in->ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER | DDSCAPS_FLIP | DDSCAPS_OFFSCREENPLAIN))
	{
		if((in->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) == 0)
		{
			out->width  = hda->width;
			out->height = hda->height;
			out->pitch  = hda->pitch;
			out->size   = hda->stride;
		}
		else
		{
			out->pitch = in->lpGbl->lPitch;
			out->size = out->pitch*out->height;
		}
		
		switch(hda->bpp)
		{
			case 8:  out->four_cc = D3DFMT_P8;       break;
			case 15: out->four_cc = D3DFMT_X1R5G5B5; break;
			case 16: out->four_cc = D3DFMT_R5G6B5;   break;
			case 24: out->four_cc = D3DFMT_R8G8B8;   break;
			case 32: out->four_cc = D3DFMT_X8R8G8B8; break;
			default: return FALSE;
		}
	}
	else if(in->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
	{
		out->width = 0;
		out->height = 0;
		out->pitch = 0;
		out->size  = in->lpGbl->dwLinearSize;
		out->four_cc = 0;
	}
	else if(fmt.dwFlags & DDPF_FOURCC) /* defined with FOURCC code */
	{
		DWORD blksize = 0;
		BOOL align = TRUE;
		DWORD dx = in->lpGbl->wWidth;
		DWORD dy = in->lpGbl->wHeight;

		switch(fmt.dwFourCC)
		{
			case MAKEFOURCC('D', 'X', 'T', '1'):
				blksize = 8;
				dx = (in->lpGbl->wWidth  + 3) >> 2;
				dy = (in->lpGbl->wHeight + 3) >> 2;
				align = FALSE;
				break;
			case MAKEFOURCC('D', 'X', 'T', '2'):
			case MAKEFOURCC('D', 'X', 'T', '3'):
				blksize = 16;
				dx = (in->lpGbl->wWidth  + 3) >> 2;
				dy = (in->lpGbl->wHeight + 3) >> 2;
				align = FALSE;
				break;
			case MAKEFOURCC('D', 'X', 'T', '4'):
			case MAKEFOURCC('D', 'X', 'T', '5'):
				blksize = 16;
				dx = (in->lpGbl->wWidth  + 3) >> 2;
				dy = (in->lpGbl->wHeight + 3) >> 2;
				align = FALSE;
				break;
			/* MSDN: The dwFourCC member is valid and contains a FOURCC code
			 * that describes a non-RGB pixel format.
			 * JH: this mean all cases below is just for case
			 */
			case D3DFMT_R5G6B5:
			case D3DFMT_A1R5G5B5:
			case D3DFMT_X1R5G5B5:
			case D3DFMT_A4R4G4B4:
			case D3DFMT_X4R4G4B4:
			case D3DFMT_A8P8:
			case D3DFMT_A8L8:
			case D3DFMT_D16_LOCKABLE:
			case D3DFMT_D16:
				blksize = 2;
				break;
			case D3DFMT_X8R8G8B8:
			case D3DFMT_A8R8G8B8:
			case D3DFMT_D32:
			case D3DFMT_S8D24:
			case D3DFMT_D24X8:
				blksize = 4;
				break;
			case D3DFMT_R8G8B8:
				blksize = 3;
				break;
			case D3DFMT_R3G3B2:
			case D3DFMT_A8:
			case D3DFMT_L8:
				blksize = 1;
				break;
			default:
				return FALSE;
		}

		if(align)
		{
			out->pitch = ((dx * blksize) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
			out->size = dy * out->pitch;
		}
		else
		{
			out->pitch = dx * blksize;
			out->size = dx * dy * blksize;
		}
		out->four_cc = fmt.dwFourCC;
	}
	else if(fmt.dwFlags & DDPF_RGB)
	{
		DWORD bp = 0;
		if(fmt.dwFlags & DDPF_PALETTEINDEXED8)
		{
			if(fmt.dwRGBBitCount == 8)
				out->four_cc = D3DFMT_P8;
			else
				return FALSE;
			bp = 1;
		}
		else if(fmt.dwFlags & DDPF_ALPHAPIXELS)
		{
			switch(fmt.dwRGBBitCount)
			{
				case 16:
					if(fmt.dwRGBAlphaBitMask == 0xF000)
						out->four_cc = D3DFMT_A4R4G4B4;
					else if(fmt.dwRGBAlphaBitMask == 0x8000)
						out->four_cc = D3DFMT_A1R5G5B5;
					else if(fmt.dwRGBAlphaBitMask == 0xFF00)
						out->four_cc = D3DFMT_A8R3G3B2;
					else
						return FALSE;
					bp = 2;
					break;
				case 32:
					if(fmt.dwBBitMask == 0x00FF0000)
						out->four_cc = D3DFMT_A8B8G8R8;
					else if(fmt.dwBBitMask == 0x000000FF)
						out->four_cc = D3DFMT_A8R8G8B8;
					else if(fmt.dwBBitMask == 0x3FF00000)
						out->four_cc = D3DFMT_A2B10G10R10;
					else
						return FALSE;
					bp = 4;
					break;
				case 64:
					out->four_cc = D3DFMT_A16B16G16R16;
					bp = 8;
				default:
					return FALSE;
			}
		}
		else // no alpha bits
		{
			switch(fmt.dwRGBBitCount)
			{
				case 8:
					out->four_cc = D3DFMT_R3G3B2;
					bp = 1;
					break;
				case 16:
					if(fmt.dwRBitMask == 0x00007C00)
						out->four_cc = D3DFMT_X1R5G5B5;
					else if(fmt.dwRBitMask == 0x0000F800)
						out->four_cc = D3DFMT_R5G6B5;
					else if(fmt.dwRBitMask == 0x00000F00)
						out->four_cc = D3DFMT_X4R4G4B4;
					else
						return FALSE;
					bp = 2;
					break;
				case 24:
					out->four_cc = D3DFMT_R8G8B8;
					bp = 3;
					break;
				case 32:
					if(fmt.dwBBitMask == 0x00FF0000)
						out->four_cc = D3DFMT_X8R8G8B8;
					else if(fmt.dwBBitMask == 0x000000FF)
						out->four_cc = D3DFMT_X8B8G8R8;
					else
						return FALSE;
					bp = 4;
					break;
				default:
					return FALSE;
			}
		}
		
		out->pitch = ((in->lpGbl->wWidth*bp) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
		out->size = in->lpGbl->wHeight * out->pitch;
	}
	else if(fmt.dwFlags & DDPF_LUMINANCE)
	{
		DWORD bp = 0;
		switch(fmt.dwAlphaBitDepth)
		{
			case 0:
				out->four_cc = D3DFMT_L8;
				bp = 1;
				break;
			case 4:
				out->four_cc = D3DFMT_A4L4;
				bp = 1;
				break;
			case 8:
				out->four_cc = D3DFMT_A8L8;
				bp = 2;
				break;
			case 16:
				out->four_cc = D3DFMT_L16;
				bp = 2;
				break;
			default:
				return FALSE;
		}
		out->pitch = ((in->lpGbl->wWidth*bp) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
		out->size = in->lpGbl->wHeight * out->pitch;
	}
	else if(fmt.dwFlags & DDPF_ALPHA)
	{
		DWORD bp = 0;
		switch(fmt.dwAlphaBitDepth)
		{
			case 8:
				if(fmt.dwFlags & DDPF_ALPHAPIXELS)
				{
					out->four_cc = D3DFMT_A8P8;
					bp = 2;
				}
				else
				{
					out->four_cc = D3DFMT_A8;
					bp = 1;
				}
				break;
			default:
				return FALSE;
		}
		out->pitch = ((in->lpGbl->wWidth*bp) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
		out->size = in->lpGbl->wHeight * out->pitch;
	}
	else if(fmt.dwFlags & (DDPF_ZBUFFER | DDPF_ZPIXELS))
	{
		DWORD bp = 0;
		switch(fmt.dwZBufferBitDepth)
		{
			case 15:
				out->four_cc = D3DFMT_D15S1;
				bp = 2;
				break;
			case 16:
				out->four_cc = D3DFMT_D16;
				bp = 2;
				break;
			case 24:
				if(fmt.dwFlags & DDPF_STENCILBUFFER)
				{
					if(fmt.dwStencilBitDepth == 8)
						out->four_cc = D3DFMT_D24S8;
					else if(fmt.dwStencilBitDepth == 4)
						out->four_cc = D3DFMT_D24X4S4;
					else
						return FALSE;
				}
				else
				{
					out->four_cc = D3DFMT_D24X8;
				}
				bp = 4;
				break;
			case 32:
				out->four_cc = D3DFMT_D32;
				bp = 4;
				break;
			default:
				return FALSE;
		}
		out->pitch = ((in->lpGbl->wWidth*bp) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
		out->size = in->lpGbl->wHeight * out->pitch;
	}
	else
	{
		return FALSE;
	}
	
	if(in->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
	{
		out->attrs.flags |= FBHDA_DD_FLAG_TEXTURE;
	}
	

	return TRUE;
}

static BOOL conv_fbsurf(LPDDRAWI_DDRAWSURFACE_LCL in, FBHDA_DD_surface_t *out, FBHDA_t *hda)
{
	if(conv_fbsurf_in(in, out, hda))
	{
		return TRUE;
	}
	
	ERR("Failed convert surface dwSurfaceHandle=%d, dwReserved1=%d; dwCaps: 0x%X, dwCaps2: 0x%X, dwFlags: 0x%X, bits:%d, R:0x%X, G:0x%X, B:0x%X",
		in->lpSurfMore->dwSurfaceHandle, in->dwReserved1,
		in->ddsCaps.dwCaps, in->lpSurfMore->ddsCapsEx.dwCaps2,
		in->lpGbl->ddpfSurface.dwFlags, in->lpGbl->ddpfSurface.dwRGBBitCount,
		in->lpGbl->ddpfSurface.dwRBitMask, in->lpGbl->ddpfSurface.dwGBitMask, in->lpGbl->ddpfSurface.dwBBitMask
	);
	return FALSE;
}

static int conv_dxcube(DWORD dwCaps2)
{
	if(dwCaps2 & DDSCAPS2_CUBEMAP)
	{
		if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX)
		{
			return DDSURFACE_CUBE_SIDE_1;
		}
		else if(dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY)
		{
			return DDSURFACE_CUBE_SIDE_2;
		}
		else if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY)
		{
			return DDSURFACE_CUBE_SIDE_3;
		}
		else if(dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ)
		{
			return DDSURFACE_CUBE_SIDE_4;
		}
		else if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ)
		{
			return DDSURFACE_CUBE_SIDE_5;
		}
		
		return DDSURFACE_CUBE_SIDE_0;
	}
	return 0;
}

/*
	Here is MS code how correctly unroll surface attachments
	https://learn.microsoft.com/en-us/previous-versions/ms890929(v=msdn.10)
	
	Algorithm is like this:
	
	1) loop mimmaps
  	mipmap = GetSubSurface(parent, 0, DDSCAPS2_MIPMAPSUBLEVEL);
  2) when is cube, loop all sites
  2b) loop mipmap for each side
  	mipmap = GetSubSurface(parent, 0, DDSCAPS2_MIPMAPSUBLEVEL);
	3) when texture stop
	4) loop flip chain and check for attached depth buffer
	
	JH: ... and is all wrong, and whole surface chain is stupid.
	Probably best strategy: loop all surfaces to one 1D array/list
	and do all operations on this.
*/

static BOOL surf_has_flags(LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD dwCapsOR, DWORD dwCaps2OR, DWORD dwCapsNAND, DWORD dwCaps2NAND)
{
	if((surf->ddsCaps.dwCaps & dwCapsOR) != 0 ||
		(surf->lpSurfMore->ddsCapsEx.dwCaps2 & dwCaps2OR) != 0)
	{
		if((surf->ddsCaps.dwCaps & dwCapsNAND) == 0 &&
			(surf->lpSurfMore->ddsCapsEx.dwCaps2 & dwCaps2NAND) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

typedef struct _surfaceitem_t
{
	LPDDRAWI_DDRAWSURFACE_LCL surf;
	int cmp_flags;
	int cmp_mip;
} surfaceitem_t;

typedef struct _surfaceitem_list_t
{
	surfaceitem_t *items;
	DWORD length;
} surfaceitem_list_t;

static void surface_chain_loop(hashtable_t *ht, LPDDRAWI_DDRAWSURFACE_LCL surf, LPDDRAWI_DDRAWSURFACE_LCL base)
{
	ht_replace(ht, (DWORD)surf, (void*)surf);

  LPATTACHLIST al;
  for(al = surf->lpAttachList; al != NULL; al = al->lpLink)
  {
		LPDDRAWI_DDRAWSURFACE_LCL attached = al->lpAttached;
		if(attached == surf) break;
		if(attached == base) break;

		if(attached != NULL)
		{
			surface_chain_loop(ht, attached, base);
		}
	}
}

static void surface_chain_swap(surfaceitem_t *a, surfaceitem_t *b)
{
	surfaceitem_t swap;
	
	memcpy(&swap, a,     sizeof(surfaceitem_t));
	memcpy(a,     b,     sizeof(surfaceitem_t));
	memcpy(b,     &swap, sizeof(surfaceitem_t));
}

static int surface_chain_cmp(surfaceitem_t *a, surfaceitem_t *b)
{
	if(a->cmp_flags == b->cmp_flags)
	{
		return a->cmp_mip - b->cmp_mip;
	}
	return b->cmp_flags - a->cmp_flags;
}

static void surface_chain_bubble(surfaceitem_t *items, DWORD length)
{
	/* TODO: bubble sort... but numbers of items are low and most are sorted */
	BOOL bubble;
	do
	{
		bubble = FALSE;
		DWORD i;
		for(i = 1; i < length; i++)
		{
			if(surface_chain_cmp(items+(i-1), items+i) < 0)
			{
				surface_chain_swap(items+(i-1), items+i);
				bubble = TRUE;
			}
		}
	} while(bubble);
}

#define CMP_PRIMARY 0
#define CMP_FLIP 1
#define CMP_TEXTURE 2
#define CMP_DEPTH   8
#define CMP_MISC    16

void surface_chain_fill(DWORD id, void *target, void *data)
{
	surfaceitem_list_t *items = data;
	LPDDRAWI_DDRAWSURFACE_LCL surf = target;
	
	surfaceitem_t *act = &items->items[items->length++];
	act->surf = target;
	act->cmp_flags = 0;
	act->cmp_mip = 0;
	
	if(surf_has_flags(surf, DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER, 0, 0, 0))
	{
		act->cmp_flags |= CMP_PRIMARY;
	}
	else if(surf_has_flags(surf, DDSCAPS_BACKBUFFER | DDSCAPS_FLIP, 0, 0, 0))
	{
		act->cmp_flags |= CMP_FLIP;
	}
	else if(surf_has_flags(surf, DDSCAPS_TEXTURE, 0, 0, 0))
	{
		act->cmp_flags |= CMP_TEXTURE;
		if(surf->lpGbl->wHeight > surf->lpGbl->wWidth)
		{
			act->cmp_mip = surf->lpGbl->wHeight;
		}
		else
		{
			act->cmp_mip = surf->lpGbl->wWidth;
		}
		
		if(surf_has_flags(surf, 0, DDSCAPS2_CUBEMAP, 0, 0))
		{
			act->cmp_mip |= (DDSURFACE_CUBE_SIDE_5 - conv_dxcube(surf->lpSurfMore->ddsCapsEx.dwCaps2)) << 16;
		}
	}
	else if(surf_has_flags(surf, DDSCAPS_ZBUFFER, 0, 0, 0))
	{
		act->cmp_flags |= CMP_DEPTH;
	}
	else
	{
		act->cmp_flags |= CMP_MISC;
	}
}

surfaceitem_list_t *surface_chain(LPDDRAWI_DDRAWSURFACE_LCL base)
{
	hashtable_t *ht = ht_init(HT_PRIME_SMALL);
	if(ht)
	{
		surface_chain_loop(ht, base, base);
		DWORD len = ht->length;
		if(len > 0)
		{
			surfaceitem_list_t *items = hal3d_malloc(sizeof(surfaceitem_list_t) + sizeof(surfaceitem_t)*len);
			if(items)
			{
				items->items = (surfaceitem_t*)(items+1);
				items->length = 0;
				ht_walk(ht, surface_chain_fill, items);
				ht_destroy(&ht);
				
				surface_chain_bubble(items->items, items->length);
				return items;
			}
			else
			{
				ht_destroy(&ht);
			}
		}
	}
	
	return NULL;
}

static ddsurface_t *ddsurf_create(mesa3d_entry_t *entry, void *flat, DWORD uid, DWORD ctxid, DWORD dxid, DWORD level)
{
	ddsurface_t *ddsurf = (ddsurface_t*)hal3d_calloc(sizeof(ddsurface_t));
	if(ddsurf != NULL)
	{
		ddsurf->flatptr = flat;
		ddsurf->uid     = uid;
		ddsurf->dxid    = dxid;
		ddsurf->level   = level;
		ddsurf->ctxid   = ctxid;
		ddsurf->up      = NULL;
		ddsurf->down    = NULL;
		ddsurf->usermem_pid = 0;
		ddsurf->dirty   = TRUE;
		ddsurf->lcl     = NULL;

		// replace older (FIXME: cleanup?)
		ht_delete(entry->ht_flat, DW_FLAT(flat));

		TOPIC("SURFEX", "HT_INSERT <- %X", flat);
		HT_INSERT_T(ddsurface_t, entry->ht_flat, DW_FLAT(flat), ddsurf);
		if(dxid)
		{
			ht_delete(entry->ht_dxid, dxid);
			HT_INSERT_T(ddsurface_t, entry->ht_dxid, dxid, ddsurf);
		}
	}
	return ddsurf;
}

static DWORD ddsurf_dxid(mesa3d_entry_t *entry, LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD ctxid)
{
	DWORD id = 0;
	if(ctxid)
	{
		id = surf->lpSurfMore->dwSurfaceHandle;
	}
	else
	{
		id = surf->dwReserved1;
	}
	
	if(id == 0 && ctxid)
	{
		DWORD id_local;
		if(entry->ids.id_assign(&id_local))
		{
			id_local |= DXID_OWN;
			id = surf->dwReserved1 = id_local;
		}
	}

	return id;
}

static void ddsurf_usermem(mesa3d_entry_t *entry, ddsurface_t *ddsurf, FBHDA_DD_surface_t *fbsurf)
{
	ddsurf->usermem_pid = entry->pid;
}

NUKED_LOCAL ddsurface_t *ddsurf_register(mesa3d_entry_t *entry, LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD ctxid)
{
	FBHDA_DD_surface_t fbsub;
	ddsurface_t *last = NULL;

	TOPIC("SURFEX", "Surface roll:");
	surfaceitem_list_t *list = surface_chain(surf);
	
	if(list)
	{
		DWORD i;
		DWORD last_level = 0;
		
		for(i = 0; i < list->length; i++)
		{
			LPDDRAWI_DDRAWSURFACE_LCL sub = list->items[i].surf;
			if(conv_fbsurf(sub, &fbsub, entry->hda))
			{
				BOOL surf_vram = FBHDA_DD_surface_set(fbsub.data, &fbsub);
				DWORD dxid_sub = ddsurf_dxid(entry, sub, ctxid);
				DWORD level = 0;
				
				if(surf_has_flags(sub, DDSCAPS_TEXTURE, 0, 0, 0))
				{
					/*if(surf_has_flags(sub, DDSCAPS_MIPMAP, 0, 0, 0))
					{
						last_level = 0;
					}*/
					DWORD cube_level = conv_dxcube(sub->lpSurfMore->ddsCapsEx.dwCaps2);
					
					if(cube_level != (level & DDSURFACE_CUBE_MASK))
					{
						last_level = 0;
					}
					
					level = last_level | cube_level;
					last_level++;
				}

				ddsurface_t *ddsub = ddsurf_create(entry, fbsub.data, fbsub.uid, ctxid, dxid_sub, level);
				if(!ddsub) break;

				if(!surf_vram)
				{
					ddsurf_usermem(entry, ddsub, &fbsub);
				}
				TOPIC("SURFEX", "  id=%d, level=%d, vram=%d, fourcc=%X", dxid_sub, level, surf_vram, fbsub.four_cc);
				
				if(last != NULL)
				{
					ddsurf_pair(last, ddsub);
				}
				last = ddsub;
			}
		}
		TOPIC("SURFEX", "-----");
		
		hal3d_free((void**)&list);
	}

	return last;
}

static BOOL conv_gldata(FBHDA_DD_surface_t *in, ddsurface_t *ddin, gldata_t *out)
{
	switch(in->four_cc)
	{
		case FOURCC_BUFFER:
			out->internalformat = GL_RED;
			out->format = GL_RED;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = FALSE;
			out->bpp = 8;
			break;
		case MAKEFOURCC('D', 'X', 'T', '1'):
			out->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			out->format = GL_RGBA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = TRUE;
			out->palette = FALSE;
			out->bpp = 32;
			break;
		case MAKEFOURCC('D', 'X', 'T', '2'):
		case MAKEFOURCC('D', 'X', 'T', '3'):
			out->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			out->format = GL_RGBA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = TRUE;
			out->palette = FALSE;
			out->bpp = 32;
			break;
		case MAKEFOURCC('D', 'X', 'T', '4'):
		case MAKEFOURCC('D', 'X', 'T', '5'):
			out->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			out->format = GL_RGBA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = TRUE;
			out->palette = FALSE;
			out->bpp = 32;
			break;
			/* DX8/DX9 */
		case D3DFMT_R5G6B5:
			out->bpp = 16;
			out->internalformat = GL_RGB;
			out->format = GL_RGB;
			out->type = GL_UNSIGNED_SHORT_5_6_5;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_X8R8G8B8:
			out->bpp = 32;
			out->internalformat = GL_RGB;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_INT_8_8_8_8_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A1R5G5B5:
			out->bpp = 16;
			out->internalformat = GL_RGBA;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_X1R5G5B5:
			out->bpp = 16;
			out->internalformat = GL_RGB;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A4R4G4B4:
			out->bpp = 16;
			out->internalformat = GL_RGBA;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A8R8G8B8:
			out->bpp = 32;
			out->internalformat = GL_RGBA;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_INT_8_8_8_8_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_X4R4G4B4:
			out->bpp = 16;
			out->internalformat = GL_RGB;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_R8G8B8:
			out->bpp = 24;
			out->internalformat = GL_RGB;
			out->format = GL_RGB;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A8:
			out->bpp = 8;
			out->internalformat = GL_ALPHA8;
			out->format = GL_ALPHA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_L8:
			out->bpp = 8;
			out->internalformat = GL_LUMINANCE8;
			out->format = GL_LUMINANCE;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A8L8:
			out->bpp = 16;
			out->internalformat = GL_LUMINANCE8_ALPHA8;
			out->format = GL_LUMINANCE_ALPHA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_P8:
		case D3DFMT_A8P8:
			out->bpp = 32;
			out->internalformat = GL_RGBA;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_BYTE;
			out->compressed = FALSE;
			out->palette = TRUE;
			break;
		case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D16:
			out->internalformat = GL_DEPTH_COMPONENT;
			out->format = GL_DEPTH_COMPONENT;
			out->type = GL_UNSIGNED_SHORT;
			out->bpp = 16;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_D32:
			out->internalformat = GL_DEPTH_COMPONENT;
			out->format = GL_DEPTH_COMPONENT;
			out->type = GL_UNSIGNED_INT;
			out->bpp = 32;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_S8D24:
			out->internalformat = GL_DEPTH_STENCIL;
			out->format = GL_DEPTH_STENCIL;
			out->type = GL_UNSIGNED_INT_24_8;
			out->bpp = 32;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_D24X8:
			out->internalformat = GL_DEPTH_COMPONENT;
			out->format = GL_DEPTH_STENCIL;
			out->type = GL_UNSIGNED_INT_24_8;
			out->bpp = 32;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A8B8G8R8:
			out->bpp = 32;
			out->internalformat = GL_RGBA;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_INT_8_8_8_8_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_X8B8G8R8:
			out->bpp = 32;
			out->internalformat = GL_RGB;
			out->format = GL_BGRA;
			out->type = GL_UNSIGNED_INT_8_8_8_8_REV;
			out->compressed = FALSE;
			out->palette = FALSE;
			break;
		case D3DFMT_A8R3G3B2:
		case D3DFMT_A16B16G16R16:
		case D3DFMT_R3G3B2:
		case D3DFMT_A4L4:
		case D3DFMT_L16:
		default:
			ERR("Unknown fourcc=%X", in->four_cc);
			memset(out, 0, sizeof(gldata_t)); /* shut up some warnings */
			return FALSE;
	}

	switch(ddin->level & DDSURFACE_CUBE_MASK)
	{
		case DDSURFACE_CUBE_SIDE_0: out->target = GL_TEXTURE_CUBE_MAP_POSITIVE_X; break;
		case DDSURFACE_CUBE_SIDE_1: out->target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; break;
		case DDSURFACE_CUBE_SIDE_2: out->target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y; break;
		case DDSURFACE_CUBE_SIDE_3: out->target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; break;
		case DDSURFACE_CUBE_SIDE_4: out->target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z; break;
		case DDSURFACE_CUBE_SIDE_5: out->target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; break;
		default: out->target = GL_TEXTURE_2D; break;
	}
	
	out->depth = -1;
	out->dirty = FALSE;
	ddsurface_t *scan = ddin;
	do
	{
		if(scan->dirty) out->dirty = TRUE;

		if((scan->level & DDSURFACE_CUBE_MASK) <= GL_TEXTURE_CUBE_MAP_POSITIVE_X)
		{
			out->depth++;
		}
		scan = scan->down;
	} while(scan != NULL);
	
	out->width = in->width;
	out->height = in->height;
	out->colorkey = ((in->attrs.flags & FBHDA_DD_FLAG_COLORKEY) == 0) ? FALSE : TRUE;
	if(out->colorkey)
	{
		out->ck_low =  in->attrs.colorkey_low;
		out->ck_high = in->attrs.colorkey_high;
	}

	/* FIXME: set palette */
	out->pal = NULL;
	out->pal_count = 0;

	out->need_convert = out->compressed || out->colorkey || out->palette;

	return TRUE;
}

NUKED_LOCAL BOOL ddsurf_glinfo(ddsurface_t *ddin, gldata_t *out, int inlevel, int *gl_level)
{
	FBHDA_DD_surface_t fb;
	if((ddin->level & DDSURFACE_LEVEL_MASK) <= inlevel) /* FIXME: sure? */
	{
		if(FBHDA_DD_surface_get(ddin->flatptr, &fb))
		{
			if(conv_gldata(&fb, ddin, out))
			{
				*gl_level = inlevel & DDSURFACE_LEVEL_MASK;
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

#define SURFEX_BUF_CNT 6
static surfaceex_t sufex_buff[SURFEX_BUF_CNT];
static gldata_t sufex_gl_buff[SURFEX_BUF_CNT];
static FBHDA_DD_surface_t sufex_fb_buff[SURFEX_BUF_CNT];
static int sufex_buff_sel = 0;

NUKED_LOCAL surfaceex_t *surfex_get(mesa3d_ctx_t *ctx, DWORD dxid, BOOL need_fb, BOOL need_gl)
{
	surfaceex_t *se = sufex_buff + sufex_buff_sel;
	ddsurface_t *dd = ddsurf_get_by_id(ctx, dxid);
	if(dd)
	{
		TRACE("dd = ..., need_fb=%d, need_gl=%d", need_fb, need_gl);
		se->dd = dd;
		se->fb = NULL;
		se->gl = NULL;

		if(dd->usermem_pid != 0 && dd->usermem_pid != ctx->entry->pid)
		{
			return NULL;
		}

		if(need_fb)
		{
			TRACE("dd->usermem_pid = %d", dd->usermem_pid);
			if(dd->usermem_pid == 0)
			{
				se->fb = sufex_fb_buff+sufex_buff_sel;
				if(!FBHDA_DD_surface_get(dd->flatptr, se->fb))
				{
					ERR("FBHDA_DD_surface_get(%X, ...) FAIL", dd->flatptr);
					return NULL;
				}
				TRACE("FBHDA_DD_surface_get");
	
				if(need_gl)
				{
					se->gl = sufex_gl_buff+sufex_buff_sel;
					if(!conv_gldata(se->fb, se->dd, se->gl))
					{
						ERR("conv_gldata(%X, ...) FAIL", se->fb->four_cc);
						return NULL;
					}
				}
				TRACE("conv_gldata");
			}
			else
			{
				ERR("surfex_get(..., %d, %d, %d) on usermem surface (ptr=0x%X, dxid=%d)",
					dxid, need_fb, need_gl, dd->flatptr, dd->dxid);
				return NULL;
			}
		}
		
		se->tex = HT_LOOKUP_T(gldata_tex_t, ctx->ht_tex, dxid);
		
		if(se->tex == NULL)
		{
			se->tex = (gldata_tex_t*)hal3d_calloc(sizeof(gldata_tex_t));
			HT_INSERT_T(gldata_tex_t, ctx->ht_tex, dxid, se->tex);
		}
		TRACE("se->tex = %X", se->tex);

		sufex_buff_sel = (sufex_buff_sel + 1) % SURFEX_BUF_CNT;
		return se;
	}
	ERR("ddsurf_get_by_id(..., %d, %d, %d) FAIL", dxid, need_gl, need_fb);
	return NULL;
}

NUKED_LOCAL void ddsurf_destroy(mesa3d_ctx_t *ctx, DWORD dxid)
{
	ddsurface_t *dd = HT_LOOKUP_T(ddsurface_t, ctx->entry->ht_dxid, dxid);
	if(dd)
	{
		ddsurface_t *dd_next = dd->down;
		ddsurf_unpair(dd);
		DWORD dd_dxid = dd->dxid;

		if(dd_dxid)
		{
			ht_delete_more(ctx->entry->ht_dxid, dd_dxid, dd);
			ctx->entry->ids.id_free(dd_dxid);
		}
		
		ht_delete_more(ctx->entry->ht_flat, DW_FLAT(dd->flatptr), dd);
		hal3d_free((void**)&dd);
		
		dd = dd_next;
	}
	
	gldata_tex_t *tex = HT_LOOKUP_T(gldata_tex_t, ctx->ht_tex, dxid);
	if(tex)
	{
		ht_delete(ctx->ht_tex, dxid);
		hal3d_free((void**)&tex);
	}
}

NUKED_LOCAL ddsurface_t *ddsurf_get_by_id(mesa3d_ctx_t *ctx, DWORD dxid)
{
	return HT_LOOKUP_T(ddsurface_t, ctx->entry->ht_dxid, dxid);
}

NUKED_LOCAL void ddsurf_unregister(mesa3d_entry_t *entry, DWORD ctxid, DWORD dxid)
{
	ddsurface_t *dd = HT_LOOKUP_T(ddsurface_t, entry->ht_dxid, dxid);

	while(dd)
	{
		if(dd->usermem_pid == 0)
		{
			FBHDA_DD_surface_delete(dd->flatptr);
		}
		dd = dd->down;
	}
}

NUKED_LOCAL BOOL ddsurf_swap(mesa3d_ctx_t *ctx, DWORD dxid_1, DWORD dxid_2)
{
	mesa3d_entry_t *entry = ctx->entry;
	if(((dxid_1 & dxid_2) & DXID_OWN) != 0)
	{
		ddsurface_t *dd_1 = HT_LOOKUP_T(ddsurface_t, entry->ht_dxid, dxid_1);
		ddsurface_t *dd_2 = HT_LOOKUP_T(ddsurface_t, entry->ht_dxid, dxid_2);
		
		if(dd_1 && dd_2)
		{
			gldata_tex_t *tex_1 = HT_LOOKUP_T(gldata_tex_t, ctx->ht_tex, dxid_1);
			gldata_tex_t *tex_2 = HT_LOOKUP_T(gldata_tex_t, ctx->ht_tex, dxid_2);
			
			ht_delete(entry->ht_dxid, dxid_1);
			ht_delete(ctx->ht_tex,    dxid_1);
			ht_delete(entry->ht_dxid, dxid_2);
			ht_delete(ctx->ht_tex,    dxid_2);

			dd_1->dxid = dxid_2;
			dd_2->dxid = dxid_1;

			HT_INSERT_T(ddsurface_t, entry->ht_dxid, dxid_2, dd_1);
			HT_INSERT_T(ddsurface_t, entry->ht_dxid, dxid_1, dd_2);
		
			if(tex_1)
			{
				HT_INSERT_T(gldata_tex_t, ctx->ht_tex, dxid_2, tex_1);
			}
			
			if(tex_2)
			{
				HT_INSERT_T(gldata_tex_t, ctx->ht_tex, dxid_1, tex_2);
			}
			
			return TRUE;
		}
	}
	return FALSE;
}

