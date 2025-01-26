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
#include "ddrawi_ddk.h"

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
#define SURF_FLAG_NO_VIDMEM 4

#define MAX_SURFACES 65535

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

typedef struct surface_info_table
{
	surface_info_t **table;
	DWORD tablesize;
	DWORD next_id;
} surface_info_table_t;

static context_info_t contexts = {NULL};
static surface_info_table_t infos = {NULL, 0, 1};

static DWORD SurfaceNextId()
{
	if(infos.next_id >= infos.tablesize)
	{
		DWORD new_size = ((infos.next_id + 1024) / 1024) * 1024;
		
		if(infos.table == NULL)
		{
			infos.table = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, new_size*sizeof(surface_info_table_t*));
			infos.tablesize = new_size;
		}
		else
		{
			infos.table = HeapReAlloc(hSharedHeap, HEAP_ZERO_MEMORY, infos.table, new_size*sizeof(surface_info_table_t*));
			infos.tablesize = new_size;
		}
		
		infos.tablesize = new_size;
	}
	
	DWORD ret_id = infos.next_id++;
	
	if(infos.next_id >= MAX_SURFACES)
	{
		infos.next_id = 1;
	}
	
	while(infos.table[infos.next_id] != NULL)
	{
		infos.next_id++;
		if(infos.next_id >= infos.tablesize)
		{
			break;
		}
	}

	return ret_id;
}

#define SURFACE_TABLE_MAGIC 0xD3D01234

static void SurfaceCopyLCL(LPDDRAWI_DDRAWSURFACE_LCL surf, DDSURF *dest, BOOL recursion);

static surface_info_t *SurfaceCreateInfo(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL recursion)
{
	TRACE_ENTRY
	
	if(surf->dwReserved1 != 0)
	{
		if(surf->dwReserved1 < infos.tablesize)
		{
			if(infos.table[surf->dwReserved1] != NULL)
			{
				return infos.table[surf->dwReserved1];
			}
		}
	}
	
	surface_info_t *info = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_info_t));
	if(info)
	{
		info->magic = SURFACE_TABLE_MAGIC;
		info->first = NULL;
		info->flags = 0;
		
		SurfaceCopyLCL(surf, &info->surf, recursion);
	}
	
	if(info->surf.fpVidMem <= DDHAL_PLEASEALLOC_LINEARSIZE)
	{
		info->flags |= SURF_FLAG_NO_VIDMEM;
		//info->flags |= SURF_FLAG_EMPTY;
		info->surf.fpVidMem = 0;
	}
	else
	{
		info->surf.fpVidMem = info->surf.lpGbl->fpVidMem;
	}

	DWORD id = SurfaceNextId();
	if(id)
	{
		infos.table[id] = info;
		surf->dwReserved1 = id;

		return info;
	}
	else
	{
		HeapFree(hSharedHeap, 0, info);
	}
	
	return NULL;
}

static void SurfaceCopyLCL(LPDDRAWI_DDRAWSURFACE_LCL surf, DDSURF *dest, BOOL recursion)
{
	dest->lpLcl    = surf;
	dest->lpGbl    = surf->lpGbl;
	dest->fpVidMem = surf->lpGbl->fpVidMem;
	dest->dwFlags  = surf->dwFlags;
	dest->attachments_cnt = 0;
	dest->dwCaps   = surf->ddsCaps.dwCaps;

	if(recursion)
	{
		LPATTACHLIST item = surf->lpAttachList;
		for(;;)
		{
			if(!item) break;
			if(!item->lpAttached) break;
			if(item->lpAttached == surf) break;
	
			if(dest->attachments_cnt < DDSURF_ATTACH_MAX)
			{
				if(item->lpAttached->dwReserved1 == 0)
				{
					SurfaceCreateInfo(item->lpAttached, FALSE);
				}
				
				dest->attachments[dest->attachments_cnt] = item->lpAttached->dwReserved1;
				dest->attachments_cnt++;
			}
	
			item = item->lpAttached->lpAttachList;
		}
	}
}

DWORD SurfaceCreate(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	SurfaceCreateInfo(surf, TRUE);

	return surf->dwReserved1;
}

static surface_info_t *SurfaceGetInfo(surface_id sid)
{
	if(sid > 0 && sid < infos.tablesize)
	{
		surface_info_t *info = infos.table[sid];
		if(info)
		{
			if(info->magic == SURFACE_TABLE_MAGIC)
			{
				if(info->flags & SURF_FLAG_NO_VIDMEM)
				{
					if(info->surf.lpGbl->fpVidMem > DDHAL_PLEASEALLOC_LINEARSIZE)
					{
						info->surf.fpVidMem = info->surf.lpGbl->fpVidMem;
						info->flags &= ~SURF_FLAG_NO_VIDMEM;
					}
				}
				
				return info;
			}
		}
	}

	return NULL;
}

static surface_info_t *SurfaceGetInfoFromLcl(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf != NULL)
	{
		surface_info_t *info = SurfaceGetInfo(surf->dwReserved1);
		return info;
	}

	return NULL;
}

DDSURF *SurfaceGetSURF(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		return &info->surf;
	}
	
	return NULL;
}

void *SurfaceGetVidMem(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		return (void*)info->surf.fpVidMem;
	}

	return NULL;
}

LPDDRAWI_DDRAWSURFACE_LCL SurfaceGetLCL(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		return (void*)info->surf.lpLcl;
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

void SurfaceAttachTexture(surface_id sid, void *mesa_tex, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(sid);
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
	
	surface_info_t *info = SurfaceGetInfoFromLcl(surf);
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
	
	if(!VMHALenv.readback)
		return;

	void *vidmem = (void*)surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		if(citem->pid == pid)
		{
			if(SurfaceGetVidMem(citem->ctx->backbuffer) == vidmem)
			{
				GL_BLOCK_BEGIN(citem->ctx)
					MesaBufferUploadColor(ctx, vidmem);
					ctx->render.dirty = FALSE;
				GL_BLOCK_END
			}

			if(VMHALenv.touchdepth && citem->ctx->depth_bpp)
			{
				if(SurfaceGetVidMem(citem->ctx->depth) == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferUploadDepth(ctx, vidmem);
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

	surface_info_t *info = SurfaceGetInfoFromLcl(surf);
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
			if(SurfaceGetVidMem(citem->ctx->backbuffer) == vidmem && citem->ctx->render.dirty)
			{
				GL_BLOCK_BEGIN(citem->ctx)
					MesaBufferDownloadColor(ctx, vidmem);
					ctx->render.dirty = FALSE;
				GL_BLOCK_END
			}

			if(VMHALenv.touchdepth)
			{
				if(citem->ctx->render.zdirty && citem->ctx->depth_bpp)
				{
					if(SurfaceGetVidMem(citem->ctx->depth) == vidmem && citem->ctx->render.zdirty)
					{
						GL_BLOCK_BEGIN(citem->ctx)
							MesaBufferDownloadDepth(ctx, vidmem);
							ctx->render.zdirty = FALSE;
						GL_BLOCK_END
					}
				}
			}
		}
		
		citem = citem->next;
	}
}

BOOL SurfaceDelete(surface_id sid)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(sid);
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
							TOPIC("GARBAGE", "Destroy texture %d", item->texture.tex->id);
							MesaDestroyTexture(item->texture.tex, FALSE, sid);
						GL_BLOCK_END
					}
#if 1
					else
					{
						TOPIC("GARBAGE", "wrong pid=%X", item->pid);
						if(ProcessExists(item->pid))
						{
							MesaDestroyTexture(item->texture.tex, TRUE, sid);
						}
					}
#endif
					break;
			}
			
			item = item->next;
		}
		
		mesa3d_entry_t *entry = Mesa3DGet(pid, FALSE);
		if(entry)
		{
			MesaSurfacesTableRemoveSurface(entry, sid);
		}
		
		while(info->first)
		{
			surface_attachment_t *item = info->first;
			info->first = item->next;
			HeapFree(hSharedHeap, 0, item);
		}

		infos.table[sid] = NULL;
		HeapFree(hSharedHeap, 0, info);
//		surf->dwReserved1 = 0;
	}
	return TRUE;
}

void SurfaceFree(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, LPDDRAWI_DDRAWSURFACE_LCL surface)
{
	DWORD handle = surface->lpSurfMore->dwSurfaceHandle;
	
	mesa_surfaces_table_t *t = MesaSurfacesTableGet(entry, lpDDLcl, handle);
	if(t)
	{
		surface_id sid = t->table[handle];

		if(sid)
		{
			BOOL is_sys = FALSE;
			surface_info_t *info = SurfaceGetInfo(sid);
			if(info)
			{
				is_sys = (info->flags & SURF_FLAG_COPY) == 0 ? TRUE : FALSE;
			}

			if(is_sys)
			{
				DWORD i;
				surface_id garbage[DDSURF_ATTACH_MAX];
				DWORD garbage_cnt = info->surf.attachments_cnt;
				memcpy(garbage, info->surf.attachments, garbage_cnt*sizeof(surface_id));
				garbage[garbage_cnt++] = sid;

				for(i = 0; i < garbage_cnt; i++)
				{
					LPDDRAWI_DDRAWSURFACE_LCL lcl = SurfaceGetLCL(garbage[i]);
					if(lcl)
					{
						SurfaceDelete(garbage[i]);
						HeapFree(hSharedLargeHeap, 0, lcl);
					}
				}
			}
		}
	}
}

void SurfaceDeattachTexture(surface_id sid, void *mesa_tex, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(sid);
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

mesa3d_texture_t *SurfaceGetTexture(surface_id sid, void *ctx, int level, int side)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(sid);
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

static void SurfaceLoopDuplicate(LPDDRAWI_DDRAWSURFACE_LCL base, LPDDRAWI_DDRAWSURFACE_LCL target, surface_info_t *info)
{
	LPATTACHLIST item = target->lpAttachList;
	while(item != NULL)
	{
		if(item->lpAttached == base)
			break;

		if(item->lpAttached)
		{
			if(info->surf.attachments_cnt < DDSURF_ATTACH_MAX)
			{
				int k = info->surf.attachments_cnt;
				LPDDRAWI_DDRAWSURFACE_LCL dup = SurfaceDuplicate(item->lpAttached);
				if(dup)
				{
					info->surf.attachments[k] = SurfaceCreate(dup);
					TOPIC("CUBE", "cube subimage dwCaps=0x%X", item->lpAttached->ddsCaps.dwCaps);
				}
				info->surf.attachments_cnt++;
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
			surface_info_t *info = SurfaceCreateInfo(scopy, TRUE);
			scopy->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
			if(info)
			{
				info->flags &= ~SURF_FLAG_EMPTY;
				info->flags |=  SURF_FLAG_COPY;
				SurfaceLoopDuplicate(surface, surface, info);
				
				MesaSurfacesTableInsertHandle(entry, lpDDLcl, scopy->lpSurfMore->dwSurfaceHandle, scopy->dwReserved1);

				DWORD i;
				for(i = 0; i < info->surf.attachments_cnt; i++)
				{
					surface_id sub_sid = info->surf.attachments[i];
					if(sub_sid)
					{
						LPDDRAWI_DDRAWSURFACE_LCL sub_lcl = SurfaceGetLCL(sub_sid);
					
						if(sub_lcl)
						{
							if((sub_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
							{
								DWORD sub_handle = sub_lcl->lpSurfMore->dwSurfaceHandle;
								MesaSurfacesTableInsertHandle(entry, lpDDLcl, sub_handle, sub_sid);
							}
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

	surface_info_t *info = SurfaceGetInfoFromLcl(surface);
	if(info)
	{
		MesaSurfacesTableInsertHandle(entry, lpDDLcl, surface->lpSurfMore->dwSurfaceHandle, surface->dwReserved1);

		DWORD i;
		for(i = 0; i < info->surf.attachments_cnt; i++)
		{
			LPDDRAWI_DDRAWSURFACE_LCL sub_lcl = SurfaceGetLCL(info->surf.attachments[i]);
			if(sub_lcl)
			{
				if((sub_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
				{
					DWORD sub_handle = sub_lcl->lpSurfMore->dwSurfaceHandle;
					MesaSurfacesTableInsertHandle(entry, lpDDLcl, sub_handle, info->surf.attachments[i]);
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

BOOL SurfaceIsEmpty(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		if(info->flags & SURF_FLAG_EMPTY)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void SurfaceClearEmpty(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		info->flags &= ~SURF_FLAG_EMPTY;
	}
}

static DWORD SurfaceDataSize(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf)
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
	
	return 0;
}

void SurfaceClearData(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	
	if(info && info->surf.fpVidMem != 0)
	{
		DWORD s = SurfaceDataSize(info->surf.lpLcl);
		if(s)
		{
			memset((void*)info->surf.fpVidMem, 0, s);
		}
		TOPIC("CLEAR", "clear surface 0x%X", info->surf.fpVidMem);
		
		SurfaceClearEmpty(sid);
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
		}
		else
		{
			memset(copy->lpSurfMore, 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
		}

		copy->lpGbl = (DDRAWI_DDRAWSURFACE_GBL*)(copy->lpSurfMore+1);
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
