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
#ifndef NUKED_SKIP
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
#include "d3dhal_ddk.h"

#include "nocrt.h"
#endif

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
	DWORD count;
} surface_info_table_t;

static context_info_t contexts = {NULL};
static surface_info_table_t infos = {NULL, 0, 1};

static DWORD SurfaceNextId()
{
	if(infos.next_id >= infos.tablesize)
	{
		DWORD new_size = ((infos.next_id + 1024) / 1024) * 1024;
		
		if(hal_realloc(HEAP_NORMAL, (void**)&infos.table, new_size*sizeof(surface_id), TRUE))
		{
			infos.tablesize = new_size;
		}
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
			if(surf->dwReserved1 < infos.tablesize && infos.table[surf->dwReserved1] != NULL)
			{
				return infos.table[surf->dwReserved1];
			}
		}
	}

	if((DWORD)surf->lpGbl < 0x80000000 && surf->lpGbl->fpVidMem < 0x80000000)
	{
		ERR("ono global lpGbl=0x%X", surf->lpGbl);
		return NULL;
	}
	
	surface_info_t *info = hal_calloc(HEAP_NORMAL, sizeof(surface_info_t), 0);
	if(info)
	{
		info->magic = SURFACE_TABLE_MAGIC;
		info->first = NULL;
		info->flags = 0;

		SurfaceCopyLCL(surf, &info->surf, recursion);

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
			TOPIC("MEMORY", "created surface info for sid = %d", id);

			infos.table[id] = info;
			infos.count++;
			surf->dwReserved1 = id;
#ifdef DEBUG_MEMORY
			info->id = id;
#endif
	
			return info;
		}
		else
		{
			hal_free(HEAP_NORMAL, info);
		}
	}
	
	return NULL;
}

static void SurfaceCopyLCLLoop(LPDDRAWI_DDRAWSURFACE_LCL base, LPDDRAWI_DDRAWSURFACE_LCL target, DDSURF *ddsurf)
{
	LPATTACHLIST item = target->lpAttachList;
	while(item != NULL)
	{
		if(item->lpAttached == base)
			break;

		if(item->lpAttached)
		{
			int k = ddsurf->attachments_cnt;
			if(k < DDSURF_ATTACH_MAX)
			{
				if(item->lpAttached->dwReserved1 == 0)
				{
					if(SurfaceCreateInfo(item->lpAttached, FALSE) == NULL)
					{
						continue;
					}
				}
				
				if(item->lpAttached->dwReserved1 != 0)
				{
					ddsurf->attachments[k] = item->lpAttached->dwReserved1;
					ddsurf->attachments_cnt++;
				}
			}

			SurfaceCopyLCLLoop(base, item->lpAttached, ddsurf);
		}

		item = item->lpLink;
	}
}

static void SurfaceCopyLCL(LPDDRAWI_DDRAWSURFACE_LCL surf, DDSURF *dest, BOOL recursion)
{
	dest->lpLclDX7        = surf;
	dest->lpGbl           = surf->lpGbl;
	dest->fpVidMem        = surf->lpGbl->fpVidMem;
	dest->dwFlags         = surf->dwFlags;
	dest->attachments_cnt = 0;
	dest->dwCaps          = surf->ddsCaps.dwCaps;
	dest->dwCaps2         = 0;
	dest->dwSurfaceHandle = 0;
	dest->dwColorKeyLow   = surf->ddckCKSrcBlt.dwColorSpaceLowValue;
	dest->dwColorKeyHigh  = surf->ddckCKSrcBlt.dwColorSpaceHighValue;

	if(surf->lpSurfMore != NULL)
	{
		dest->dwCaps2 = surf->lpSurfMore->ddsCapsEx.dwCaps2;
		dest->dwSurfaceHandle = surf->lpSurfMore->dwSurfaceHandle;
	}

	if(recursion)
	{
		SurfaceCopyLCLLoop(surf, surf, dest);
#if 0
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
#endif
	}
}

DWORD SurfaceCreate(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(SurfaceCreateInfo(surf, TRUE) == NULL)
	{
		return 0;
	}

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

LPDDRAWI_DDRAWSURFACE_LCL SurfaceGetLCL_DX7(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		return (void*)info->surf.lpLclDX7;
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
		
		item = hal_calloc(HEAP_NORMAL, sizeof(surface_attachment_t), 0);
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
	
	item = hal_calloc(HEAP_NORMAL, sizeof(context_attachment_t), 0);
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

void SurfaceToMesaTex(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	if(info)
	{
		//if(info->lock)
		//	return;

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
}

void SurfaceToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL texonly)
{
	TRACE_ENTRY
	
	SurfaceToMesaTex(surf->dwReserved1);

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
				TOPIC("DEPTHCONV", "Color to mesa");
				GL_BLOCK_BEGIN(citem->ctx)
					MesaBufferUploadColor(ctx, vidmem);
					ctx->render.dirty = FALSE;
				GL_BLOCK_END
			}

			if(VMHALenv.touchdepth && citem->ctx->depth_bpp)
			{
				if(SurfaceGetVidMem(citem->ctx->depth) == vidmem)
				{
					TOPIC("DEPTHCONV", "Depth to mesa");
					GL_BLOCK_BEGIN(citem->ctx)
						//entry->proc.pglDepthMask(GL_TRUE);
						//entry->proc.pglClear(GL_DEPTH_BUFFER_BIT);
						MesaBufferUploadDepth(ctx, vidmem);
						ctx->render.zdirty = FALSE;
						//if(!ctx->state.depth.writable)
						//{
						//	entry->proc.pglDepthMask(GL_FALSE);
						//}
					GL_BLOCK_END
				}
			}
		}
		
		citem = citem->next;
	}
}

void SurfaceZToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD color)
{
	TRACE_ENTRY

	if(VMHALenv.touchdepth)
		return;

	void *vidmem = (void*)surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		if(citem->pid == pid)
		{
			if(SurfaceGetVidMem(citem->ctx->depth) == vidmem)
			{
				TOPIC("DEPTHCONV", "Clear Z, color=0x%X", color);
				GL_BLOCK_BEGIN(citem->ctx)
					D3DVALUE v = 0.0;
					if(color > 0)
					{
						v = 1.0;
					}
					MesaClear(ctx, D3DCLEAR_ZBUFFER, 0, v, 0, 0, NULL);
				GL_BLOCK_END
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
		//if(info->lock)
		//	return;

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

static void SurfaceTableReduce()
{
	if(infos.count == 0)
	{
		infos.next_id = 1;
		infos.tablesize = 0;
		hal_free(HEAP_NORMAL, infos.table);
		infos.table = NULL;
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
		TOPIC("MEMORY", "Deleting surface sid=%d", sid);
		
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
							TOPIC("TEXMEM", "NON deleted: %d", item->texture.tex->gltex);
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
			hal_free(HEAP_NORMAL, item);
		}

		infos.table[sid] = NULL;
		infos.count--;
		if(info->surf.cache != NULL)
		{
			hal_free(HEAP_LARGE, info->surf.cache);
			info->surf.cache = NULL;
		}
		
		SurfaceTableReduce();
		
		hal_free(HEAP_NORMAL, info);
//		surf->dwReserved1 = 0;
	}
	return TRUE;
}

static BOOL SurfaceDeleteGarbage(surface_id sid)
{
	TRACE_ENTRY

	surface_info_t *info = NULL;
	if(sid > 0 && sid < infos.tablesize)
	{
		info = infos.table[sid];
	}

	if(info)
	{
		while(info->first)
		{
			surface_attachment_t *item = info->first;
			info->first = item->next;
			hal_free(HEAP_NORMAL, item);
		}

		infos.table[sid] = NULL;
		infos.count--;
		if(info->surf.cache != NULL)
		{
			hal_free(HEAP_LARGE, info->surf.cache);
			info->surf.cache = NULL;
		}
		
		SurfaceTableReduce();
		
		hal_free(HEAP_NORMAL, info);
	}
	return TRUE;
}

NUKED_LOCAL void SurfaceFree(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, LPDDRAWI_DDRAWSURFACE_LCL surface)
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
				is_sys = (info->flags & SURF_FLAG_COPY) ? TRUE : FALSE;
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
					LPDDRAWI_DDRAWSURFACE_LCL lcl = SurfaceGetLCL_DX7(garbage[i]); /* own managed mem */
					if(lcl)
					{
						SurfaceDelete(garbage[i]);
						hal_free(HEAP_LARGE, lcl);
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
				hal_free(HEAP_NORMAL, ptr);
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
			hal_free(HEAP_NORMAL, ptr);
		}
		else
		{
			last = item;
			item = item->next;
		}
	}
}

NUKED_LOCAL mesa3d_texture_t *SurfaceGetTexture(surface_id sid, void *ctx, int level, int side)
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
					surface_id sid = SurfaceCreate(dup);
					if(sid)
					{
						info->surf.attachments[k] = sid;
						TOPIC("CUBE", "cube subimage dwCaps=0x%X", item->lpAttached->ddsCaps.dwCaps);
						info->surf.attachments_cnt++;
					}
				}
			}

			SurfaceLoopDuplicate(base, item->lpAttached, info);
		}

		item = item->lpLink;
	}
}

NUKED_LOCAL BOOL SurfaceExInsert(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, LPDDRAWI_DDRAWSURFACE_LCL surface)
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
	if(((surface->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE)) == 
		(DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE)) || surface->lpGbl->fpVidMem < 0x80000000)
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
				TOPIC("MEMORY", "Duplicate handle=%d, sid=%d (original)dwCaps=0x%X",
					scopy->lpSurfMore->dwSurfaceHandle,
					scopy->dwReserved1,
					surface->ddsCaps.dwCaps
				);

				MesaSurfacesTableInsertHandle(entry, lpDDLcl, scopy->lpSurfMore->dwSurfaceHandle, scopy->dwReserved1);

				DWORD i;
				for(i = 0; i < info->surf.attachments_cnt; i++)
				{
					surface_id sub_sid = info->surf.attachments[i];
					if(sub_sid)
					{
						DDSURF *sub_surf = SurfaceGetSURF(sub_sid);
					
						if(sub_surf)
						{
							if((sub_surf->dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
							{
								if(sub_surf->dwSurfaceHandle)
								{
									MesaSurfacesTableInsertHandle(entry, lpDDLcl, sub_surf->dwSurfaceHandle, sub_sid);
								}
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

	/* video memory */
	surface_info_t *info = SurfaceGetInfoFromLcl(surface);
	if(info)
	{
		info->surf.dwSurfaceHandle = surface->lpSurfMore->dwSurfaceHandle; /* update handle */
		
		MesaSurfacesTableInsertHandle(entry, lpDDLcl, surface->lpSurfMore->dwSurfaceHandle, surface->dwReserved1);
		TOPIC("TEXTARGET", "sid=%d (handle=%d) has %d atachments", surface->dwReserved1, surface->lpSurfMore->dwSurfaceHandle,  info->surf.attachments_cnt);

		DWORD i;
		for(i = 0; i < info->surf.attachments_cnt; i++)
		{
			//LPDDRAWI_DDRAWSURFACE_LCL sub_lcl = SurfaceGetLCL(info->surf.attachments[i]);
			DDSURF *sub_surf = SurfaceGetSURF(info->surf.attachments[i]);
			if(sub_surf)
			{
				if((sub_surf->dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) == 0)
				{
					TOPIC("TEXTARGET", "Insert sid=%d handle=%d", info->surf.attachments[i], sub_surf->dwSurfaceHandle);
					if(sub_surf->dwSurfaceHandle)
					{
						MesaSurfacesTableInsertHandle(entry, lpDDLcl, sub_surf->dwSurfaceHandle, info->surf.attachments[i]);
					}
				}
			}
		}

		return TRUE;
	}
	else
	{
		WARN("Failed get info for surface handle=%d, type=0x%X, surface id=%d", surface->lpSurfMore->dwSurfaceHandle, 
			surface->ddsCaps.dwCaps, surface->dwReserved1
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

DWORD SurfaceDataSize(LPDDRAWI_DDRAWSURFACE_GBL gbl, DWORD *outPitch)
{
	if(gbl)
	{
		if(gbl->ddpfSurface.dwFlags & DDPF_FOURCC)
		{
			DWORD blksize = 0;
			BOOL align = TRUE;
			DWORD dx = gbl->wWidth;
			DWORD dy = gbl->wHeight;
			
			switch(gbl->ddpfSurface.dwFourCC)
			{
				case MAKEFOURCC('D', 'X', 'T', '1'):
					blksize = 8;
					dx = (gbl->wWidth  + 3) >> 2;
					dy = (gbl->wHeight + 3) >> 2;
					align = FALSE;
					break;
				case MAKEFOURCC('D', 'X', 'T', '2'):
				case MAKEFOURCC('D', 'X', 'T', '3'):
					blksize = 16;
					dx = (gbl->wWidth  + 3) >> 2;
					dy = (gbl->wHeight + 3) >> 2;
					align = FALSE;
					break;
				case MAKEFOURCC('D', 'X', 'T', '4'):
				case MAKEFOURCC('D', 'X', 'T', '5'):
					blksize = 16;
					dx = (gbl->wWidth  + 3) >> 2;
					dy = (gbl->wHeight + 3) >> 2;
					align = FALSE;
					break;
				case D3DFMT_R5G6B5:
				case D3DFMT_A1R5G5B5:
				case D3DFMT_X1R5G5B5:
				case D3DFMT_A4R4G4B4:
				case D3DFMT_D16_LOCKABLE:
				case D3DFMT_D16:
					blksize = 2;
					break;
				case D3DFMT_X8R8G8B8:
				case D3DFMT_A8R8G8B8:
				case D3DFMT_D32:
				case D3DFMT_S8D24:
					blksize = 4;
					break;
				default:
					return 0;
					break;
			}
			
			if(align)
			{
				DWORD pitch = ((dx * blksize) + FBHDA_ROW_ALIGN - 1) & (~(FBHDA_ROW_ALIGN-1));
				if(outPitch)
				{
					*outPitch = pitch;
				}
				return dy * pitch;
			}
			
			if(outPitch)
			{
			 *outPitch = dx * blksize;
			}
			return dx * dy *  blksize;
		}
		
		if(outPitch)
		{
			*outPitch = gbl->lPitch;
		}
		return gbl->wHeight * gbl->lPitch;
	}
	
	return 0;
}

void SurfaceClearData(surface_id sid)
{
	surface_info_t *info = SurfaceGetInfo(sid);
	
	if(info && info->surf.fpVidMem != 0)
	{
		DWORD s = SurfaceDataSize(info->surf.lpGbl, NULL);
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

	DWORD datasize = SurfaceDataSize(original->lpGbl, NULL);
	size += datasize;

	TOPIC("CUBE", "alloc, size=%d from this datasize=%d, original->lpGbl=0x%X",
		size, datasize, original->lpGbl);

	LPDDRAWI_DDRAWSURFACE_LCL copy = hal_alloc(HEAP_LARGE, size, 0);
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

void SurfaceApplyColorKey(surface_id sid, DWORD low, DWORD hi, DWORD low_pal, DWORD hi_pal)
{
	TRACE("SurfaceApplyColorKey: sid=%d, low=%X, hi=%X", sid, low, hi);
	if(sid)
	{
		surface_info_t *info = SurfaceGetInfo(sid);
		if(info)
		{
			info->surf.dwFlags |= DDRAWISURF_HASCKEYSRCBLT;
			info->surf.dwColorKeyLow = low;
			info->surf.dwColorKeyHigh = hi;
			info->surf.dwColorKeyLowPal = low_pal;
			info->surf.dwColorKeyHighPal = hi_pal;
		}
	}
}

void SurfaceDeleteAll()
{
	TRACE_ENTRY

	DWORD sid = 1;
	while(sid < infos.tablesize)
	{
		if(infos.table[sid] != NULL)
		{
			TOPIC("MEMORY", "Clean %d=>%X", sid, infos.table[sid]);
			TOPIC("MEMORY", "Abandoned surface: %u", sid);
			SurfaceDeleteGarbage(sid);
		}
		sid++;
	}
}

void SurfaceLock(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	surface_info_t *info = SurfaceGetInfoFromLcl(surf);
	if(info)
	{
		info->lock++;
	}
}

void SurfaceUnlock(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	surface_info_t *info = SurfaceGetInfoFromLcl(surf);
	if(info)
	{
		if(info->lock > 0)
		{
			info->lock--;
		}
	}
}
