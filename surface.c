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
#include "surface.h"

#include "nocrt.h"

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

#define SURF_TYPE_NONE 1
#define SURF_TYPE_TEX  2
//#define SURF_TYPE_CTX  3

#define SURF_FLAG_EMPTY 1
#define SURF_FLAG_COPY 2
#define SURF_FLAG_DELETED 4

typedef struct surface_attachment
{
	struct surface_attachment *next;
	DWORD type;
	DWORD pid;
	struct {
		mesa3d_texture_t *tex;
		int level;
		int side;
	} texture;
} surface_attachment_t;

typedef struct surface_info
{
	DWORD magic;
	surface_attachment_t *first;
	DDSURF surf;
	DWORD flags;
} surface_info_t;

typedef struct context_attachment
{
	struct context_attachment *next;
	DWORD pid;
	mesa3d_ctx_t *ctx;
} context_attachment_t;

typedef struct context_info
{
	context_attachment_t *first;
} context_info_t;

static context_info_t contexts = {NULL};

#define SURFACE_TABLE_MAGIC 0xD3D01234

static surface_info_t *SurfaceCreateInfo(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	TRACE_ENTRY
	
	surface_info_t *info = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_info_t));
	if(info)
	{
		info->magic = SURFACE_TABLE_MAGIC;
		info->first = NULL;
		info->flags = SURF_FLAG_EMPTY;
		
		SurfaceCopyLCL(surf, &info->surf);
	}
	
	surf->dwReserved1 = (DWORD)info;

	return info;
}

DWORD SurfaceCreate(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	SurfaceCreateInfo(surf);

	return surf->dwReserved1;
}

static surface_info_t *SurfaceGetInfo(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	/* hack: allocation from shared heap resides in shared memory */
	if(surf != NULL && surf->dwReserved1 >= 0x80000000)
	{
		surface_info_t *info = (surface_info_t*)surf->dwReserved1;
		if(info)
		{
			if(info->magic == SURFACE_TABLE_MAGIC)
			{
				if((info->flags & SURF_FLAG_DELETED) == 0)
				{
					return info;
				}
			}
		}
	}
	
	return NULL;
}

static void SurfaceAttachTexture_int(surface_info_t *info, void *tex, int level, int side)
{
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_TEX && 
				item->texture.tex == tex &&
				item->texture.level == level &&
				item->texture.side == side
			){
				/* texture already attached */
				WARN("SurfaceAttachTexture already exists, level %d", level);
				return;
			}
			
			last = item;
			item = item->next;
		}
		
		item = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_attachment_t));
		if(item)
		{
			item->type = SURF_TYPE_TEX;
			item->texture.tex = tex;
			item->texture.level = level;
			item->texture.side = side;
			item->pid = GetCurrentProcessId();
			item->next = NULL;
			if(last)
			{
				last->next = item;
			}
			else
			{
				info->first = item;
			}
		}
	} // info
}

void SurfaceAttachTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_tex, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		SurfaceAttachTexture_int(info, mesa_tex, level, side);
	}
}

void SurfaceAttachCtx(void *mesa_ctx)
{
	TRACE_ENTRY
	
	context_attachment_t *item = contexts.first;
	context_attachment_t *last = NULL;
	while(item)
	{
		if(item->ctx == mesa_ctx)
		{
			return;
		}
		
		last = item;
		item = item->next;
	}
	
	item = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(context_attachment_t));
	if(item)
	{
		item->ctx = mesa_ctx;
		item->next = NULL;
		item->pid = GetCurrentProcessId();
		
		if(last)
		{
			last->next = item;
		}
		else
		{
			contexts.first = item;
		}
	}
}

void SurfaceToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL texonly)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		// something was write to surface, remove empty flag
		info->flags &= ~SURF_FLAG_EMPTY;

		surface_attachment_t *item = info->first;
		while(item)
		{
			switch(item->type)
			{
				case SURF_TYPE_TEX:
					if(item->texture.level <= item->texture.tex->mipmap_level &&
						item->texture.side < MESA3D_CUBE_SIDES)
					{
						item->texture.tex->data_dirty[item->texture.side][item->texture.level] = TRUE;
						item->texture.tex->dirty = TRUE;
					}
					break;
			}
			
			item = item->next;
		}
	}

	if(texonly)
		return;

	void *vidmem = (void*)surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		if(citem->pid == pid)
		{
			if(citem->ctx->backbuffer_vidmem == vidmem)
			{
				GL_BLOCK_BEGIN(citem->ctx)
					MesaBufferUploadColor(ctx, (void*)vidmem);
					ctx->render.dirty = FALSE;
				GL_BLOCK_END
			}
			
			if(citem->ctx->depth_bpp)
			{
				if(citem->ctx->depth_vidmem == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferUploadDepth(ctx, (void*)vidmem);
						ctx->render.zdirty = FALSE;
					GL_BLOCK_END
				}
			}
		}
		
		citem = citem->next;
	}
}

void SurfaceFromMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL texonly)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		while(item)
		{
			switch(item->type)
			{
				case SURF_TYPE_TEX:
					// don't downloading textures back to surfaces
					break;
			}

			item = item->next;
		}
	}

	if(texonly)
		return;

	void *vidmem = (void*)surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;	
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		TRACE("SurfaceFromMesa - citem->pid = %X, pid = %X", citem->pid, pid);
		if(citem->pid == pid)
		{
			TOPIC("TARGET", "SurfaceFromMesa - citem->ctx->backbuffer.fpVidMem = %X, vidmem = %X",
				citem->ctx->backbuffer_vidmem, vidmem);
			if(citem->ctx->backbuffer_vidmem == vidmem && citem->ctx->render.dirty)
			{
				GL_BLOCK_BEGIN(citem->ctx)
					MesaBufferDownloadColor(ctx, vidmem);
					ctx->render.dirty = FALSE;
				GL_BLOCK_END
			}
			
			if(citem->ctx->render.zdirty && citem->ctx->depth_bpp)
			{
				if(citem->ctx->depth_vidmem == vidmem && citem->ctx->render.zdirty)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferDownloadDepth(ctx, vidmem);
						ctx->render.zdirty = FALSE;
					GL_BLOCK_END
				}
			}
		}
		
		citem = citem->next;
	}
}

BOOL SurfaceDelete(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(surf);
	DWORD pid = GetCurrentProcessId();
	
	TOPIC("GARBAGE", "SurfaceDelete: pid=%X, info=%p", pid, info);
	
	if(info)
	{
		surface_attachment_t *item = info->first;
		while(item)
		{
			switch(item->type)
			{
				case SURF_TYPE_TEX:
					if(item->pid == pid)
					{
						GL_BLOCK_BEGIN(item->texture.tex->ctx)
							MesaDestroyTexture(item->texture.tex, FALSE, surf);
						GL_BLOCK_END
					}
					else
					{
						info->flags |= SURF_FLAG_DELETED;
						TOPIC("GARBAGE", "wrong pid=%X", item->pid);
					}
					break;
			}
			
			item = item->next;
		}
		
		mesa3d_entry_t *entry = Mesa3DGet(pid, FALSE);
		if(entry)
		{
			MesaSurfacesTableRemoveSurface(entry, &info->surf);
		}
		
		if(info->flags & SURF_FLAG_DELETED)
		{
			// FIXME: there will be memory leaks!
			return FALSE;
		}

		while(info->first)
		{
			surface_attachment_t *item = info->first;
			info->first = item->next;
			HeapFree(hSharedHeap, 0, item);
		}
	}

	HeapFree(hSharedHeap, 0, info);
	surf->dwReserved1 = 0;
	return TRUE;
}

void SurfaceFree(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, LPDDRAWI_DDRAWSURFACE_LCL surface)
{
	DWORD id = surface->lpSurfMore->dwSurfaceHandle;
	
	mesa_surfaces_table_t *t = MesaSurfacesTableGet(entry, lpDDLcl, id);
	if(t)
	{
		DDSURF *s = t->table[id];

		if(s)
		{
			BOOL is_sys = FALSE;
			surface_info_t *info = SurfaceGetInfo(s->lpLcl);
			if(info)
			{
				is_sys = (info->flags & SURF_FLAG_COPY) == 0 ? TRUE : FALSE;
			}
			info = NULL;
			
			if(is_sys)
			{
				DWORD i;
				LPDDRAWI_DDRAWSURFACE_LCL garbage[DDSURF_ATTACH_MAX];
				DWORD garbage_cnt = s->dwAttachments;
				memcpy(garbage, s->lpAttachmentsLcl, garbage_cnt*sizeof(LPDDRAWI_DDRAWSURFACE_LCL));
				garbage[garbage_cnt++] = s->lpLcl;

				for(i = 0; i < garbage_cnt; i++)
				{
					SurfaceDelete(garbage[i]);
				}

				for(i = 0; i < garbage_cnt; i++)
				{
					TOPIC("FBSWAP", "free system %d", garbage[i]->lpSurfMore->dwSurfaceHandle);
					HeapFree(hSharedLargeHeap, 0, garbage[i]);
				}
			}
		}
	}
}

void SurfaceDeattachTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_tex, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_TEX &&
				item->texture.tex == mesa_tex &&
				item->texture.level == level &&
				item->texture.side == side)
			{
				if(!last)
				{
					info->first = item->next;
				}
				else
				{
					last->next = item->next;
				}
				
				surface_attachment_t *ptr = item;
				item = item->next;
				HeapFree(hSharedHeap, 0, ptr);
			}
			else
			{
				last = item;
				item = item->next;
			}
		}
	}
}

void SurfaceDeattachCtx(void *mesa_ctx)
{
	TRACE_ENTRY
	
	context_attachment_t *item = contexts.first;
	context_attachment_t *last = NULL;

	while(item)
	{
		if(item->ctx == mesa_ctx)
		{
			if(!last)
			{
				contexts.first = item->next;
			}
			else
			{
				last->next = item->next;
			}
			
			context_attachment_t *ptr = item;
			item = item->next;
			HeapFree(hSharedHeap, 0, ptr);
		}
		else
		{
			last = item;
			item = item->next;
		}
	}
}

mesa3d_texture_t *SurfaceGetTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *ctx, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		while(item)
		{
			switch(item->type)
			{
				case SURF_TYPE_TEX:
					if(item->texture.tex->ctx == ctx
						&& item->texture.level == level
						&& item->texture.side == side)
					{
						return item->texture.tex;
					}
					break;
			}
			
			item = item->next;
		}
	}
	
	return NULL;
}

void SurfaceCopyLCL(LPDDRAWI_DDRAWSURFACE_LCL surf, DDSURF *dest)
{
	dest->lpLcl    = surf;
	dest->lpGbl    = surf->lpGbl;
	dest->fpVidMem = surf->lpGbl->fpVidMem;
	dest->dwFlags  = surf->dwFlags;
	dest->dwAttachments = 0;
	dest->dwCaps   = surf->ddsCaps.dwCaps;

	LPATTACHLIST item = surf->lpAttachList;
	for(;;)
	{
		if(!item) break;
		if(!item->lpAttached) break;
		if(item->lpAttached == surf) break;

		if(dest->dwAttachments < DDSURF_ATTACH_MAX)
		{
			dest->lpAttachmentsLcl[dest->dwAttachments] = item->lpAttached;
			dest->lpAttachmentsGbl[dest->dwAttachments] = item->lpAttached->lpGbl;
			dest->dwAttachments++;
		}
			
		item = item->lpAttached->lpAttachList;
	}
}

static void SurfaceLoopDuplicate(LPDDRAWI_DDRAWSURFACE_LCL base, LPDDRAWI_DDRAWSURFACE_LCL target, surface_info_t *info)
{
	LPATTACHLIST item = target->lpAttachList;
	while(item != NULL)
	{
		if(item->lpAttached == base)
			break;

		if(item->lpAttached)
		{
			if(info->surf.dwAttachments < DDSURF_ATTACH_MAX)
			{
				int k = info->surf.dwAttachments;
				info->surf.lpAttachmentsLcl[k] = SurfaceDuplicate(item->lpAttached);
				if(info->surf.lpAttachmentsLcl[k])
				{
					info->surf.lpAttachmentsGbl[k] = info->surf.lpAttachmentsLcl[k]->lpGbl;
				}
				TOPIC("CUBE", "cube subimage dwCaps=0x%X", item->lpAttached->ddsCaps.dwCaps);
				info->surf.dwAttachments++;
			}

			SurfaceLoopDuplicate(base, item->lpAttached, info);
		}

		item = item->lpLink;
	}
}

BOOL SurfaceExInsert(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, LPDDRAWI_DDRAWSURFACE_LCL surface)
{
	TOPIC("CUBE", "dwSurfaceHandle=%d, vram=0x%X, dwCaps=0x%X",
		surface->lpSurfMore->dwSurfaceHandle,
		surface->lpGbl->fpVidMem,
		surface->ddsCaps.dwCaps
	);

	if(surface->lpSurfMore->dwSurfaceHandle == 0)
	{
		return FALSE;
	}

	/* system memory */
	if((surface->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE)) == 
		(DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE))
	{
		LPDDRAWI_DDRAWSURFACE_LCL scopy = SurfaceDuplicate(surface);
		if(scopy)
		{
			surface_info_t *info = SurfaceCreateInfo(scopy);
			scopy->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
			if(info)
			{
				info->flags &= ~SURF_FLAG_EMPTY;
				info->flags |=  SURF_FLAG_COPY;
				SurfaceLoopDuplicate(surface, surface, info);
				
				MesaSurfacesTableInsertSurface(entry, lpDDLcl, scopy->lpSurfMore->dwSurfaceHandle, &info->surf);

				DWORD i;
				for(i = 0; i < info->surf.dwAttachments; i++)
				{
					LPDDRAWI_DDRAWSURFACE_LCL sub_lcl = info->surf.lpAttachmentsLcl[i];
					
					if((sub_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
					{
						surface_info_t *sub = SurfaceCreateInfo(sub_lcl);
						if(sub)
						{
							DWORD sub_id = sub->surf.lpLcl->lpSurfMore->dwSurfaceHandle;
							MesaSurfacesTableInsertSurface(entry, lpDDLcl, sub_id, &sub->surf);
						}
						else
						{
							WARN("SurfaceGetInfo == NULL");
						}
					}
				}

				TOPIC("CUBE", "surface duplicate success");
				return TRUE;
			}
		}
		TOPIC("CUBE", "surface duplicate failure");
		return FALSE;
	}

	surface_info_t *info = SurfaceGetInfo(surface);
	if(info)
	{
		/* update vram ptr */
		info->surf.fpVidMem = info->surf.lpGbl->fpVidMem;
		TOPIC("TARGET", "SurfaceExInsert(%d), info->surf.fpVidMem = 0x%X",
			surface->lpSurfMore->dwSurfaceHandle,
			info->surf.lpGbl->fpVidMem
		);

		MesaSurfacesTableInsertSurface(entry, lpDDLcl, surface->lpSurfMore->dwSurfaceHandle, &info->surf);

		DWORD i;
		for(i = 0; i < info->surf.dwAttachments; i++)
		{
			LPDDRAWI_DDRAWSURFACE_LCL sub_lcl = info->surf.lpAttachmentsLcl[i];

			if((sub_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
			{
				surface_info_t *sub = SurfaceCreateInfo(sub_lcl);
				if(sub)
				{
					DWORD sub_id = sub->surf.lpLcl->lpSurfMore->dwSurfaceHandle;
					MesaSurfacesTableInsertSurface(entry, lpDDLcl, sub_id, &sub->surf);
				}
			}
		}

		return TRUE;
	}
	else
	{
		WARN("Failed get info for surface id=%d, type=0x%X", surface->lpSurfMore->dwSurfaceHandle, 
			surface->ddsCaps.dwCaps
		);
	}

	return FALSE;
}

BOOL SurfaceIsEmpty(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		if(info->flags & SURF_FLAG_EMPTY)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void SurfaceClearEmpty(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		info->flags &= ~SURF_FLAG_EMPTY;
	}
}

static DWORD SurfaceDataSize(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf->lpGbl->ddpfSurface.dwFlags & DDPF_FOURCC)
	{
		DWORD blksize = 0;

		switch(surf->lpGbl->ddpfSurface.dwFourCC)
		{
			case MAKEFOURCC('D', 'X', 'T', '1'):
				blksize = 8;
				break;
			case MAKEFOURCC('D', 'X', 'T', '2'):
			case MAKEFOURCC('D', 'X', 'T', '3'):
				blksize = 16;
				break;
			case MAKEFOURCC('D', 'X', 'T', '4'):
			case MAKEFOURCC('D', 'X', 'T', '5'):
				blksize = 16;
				break;
			default:
				return 0;
				break;
		}

		DWORD dx = (surf->lpGbl->wWidth  + 3) >> 2;
		DWORD dy = (surf->lpGbl->wHeight + 3) >> 2;
		
		return dx * dy *  blksize;
	}
	
	return surf->lpGbl->wHeight * surf->lpGbl->lPitch;
}

void SurfaceClearData(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf->lpGbl->fpVidMem != 0)
	{
		DWORD s = SurfaceDataSize(surf);
		if(s)
		{
			memset((void*)surf->lpGbl->fpVidMem, 0, s);
		}
		TOPIC("CLEAR", "clear surface 0x%X", surf->lpGbl->fpVidMem);
		
		SurfaceClearEmpty(surf);
	}
}

LPDDRAWI_DDRAWSURFACE_LCL SurfaceDuplicate(LPDDRAWI_DDRAWSURFACE_LCL original)
{
	DWORD size =
		sizeof(DDRAWI_DDRAWSURFACE_LCL) +
		sizeof(DDRAWI_DDRAWSURFACE_MORE) +
		sizeof(DDRAWI_DDRAWSURFACE_GBL);

	DWORD datasize = SurfaceDataSize(original);
	size += datasize;

	TOPIC("CUBE", "alloc, size=%d from this datasize=%d, original->lpGbl=0x%X",
		size, datasize, original->lpGbl);

	LPDDRAWI_DDRAWSURFACE_LCL copy = HeapAlloc(hSharedLargeHeap, 0, size);
	if(copy)
	{
		memcpy(copy, original, sizeof(DDRAWI_DDRAWSURFACE_LCL));
		copy->lpSurfMore = (DDRAWI_DDRAWSURFACE_MORE*)(copy+1);
		copy->dwReserved1 = 0;

		if(original->lpSurfMore != NULL)
		{
			memcpy(copy->lpSurfMore, original->lpSurfMore, sizeof(DDRAWI_DDRAWSURFACE_MORE));
			copy->lpGbl = (DDRAWI_DDRAWSURFACE_GBL*)(copy->lpSurfMore+1);
		}
		else
		{
			copy->lpGbl = (DDRAWI_DDRAWSURFACE_GBL*)(copy+1);
		}

		memcpy(copy->lpGbl, original->lpGbl, sizeof(DDRAWI_DDRAWSURFACE_GBL));
		copy->lpGbl->fpVidMem = (FLATPTR)(copy->lpGbl+1);
		copy->lpAttachList = NULL;
		copy->lpAttachListFrom = NULL;
		copy->dwLocalRefCnt = 0;

		memcpy(
			(void*)copy->lpGbl->fpVidMem,
			(void*)original->lpGbl->fpVidMem,
			datasize);

		TOPIC("CUBE", "SurfaceDuplicate datasize=%d", datasize);

		return copy;
	}

	return NULL;
}

LPDDRAWI_DDRAWSURFACE_LCL SurfaceSubimage(DDSURF *surf, DWORD id)
{
	if(id)
	{
		DWORD i;
		for(i = 0; i < surf->dwAttachments; i++)
		{
			if(surf->lpAttachmentsLcl[i]->lpSurfMore->dwSurfaceHandle == id)
			{
				return surf->lpAttachmentsLcl[i];
			}
		}
	}
	
	return surf->lpLcl;
}
