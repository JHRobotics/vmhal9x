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

extern HANDLE hSharedHeap;

#define SURF_TYPE_NONE 1
#define SURF_TYPE_TEX  2
//#define SURF_TYPE_CTX  3

typedef struct surface_attachment
{
	struct surface_attachment *next;
	DWORD type;
	union
	{
		struct {
			mesa3d_texture_t *tex;
			int level;
		} texture;
		//mesa3d_ctx_t *ctx;
	};
} surface_attachment_t;

typedef struct surface_info
{
	DWORD magic;
	surface_attachment_t *first;
	LPDDRAWI_DDRAWSURFACE_LCL surf;
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

DWORD SurfaceTableCreate(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	TRACE_ENTRY
	
	surface_info_t *info = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_info_t));
	if(info)
	{
		info->magic = SURFACE_TABLE_MAGIC;
		info->surf  = surf;
		info->first = NULL;
	}

	surf->dwReserved1 = (DWORD)info;

	return surf->dwReserved1;
}

static surface_info_t *SurfaceGetInfo(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf != NULL && surf->dwReserved1 >= 0x80000000) /* hack: allocation from shared heap resides in shared memory */
	{
		surface_info_t *info = (surface_info_t*)surf->dwReserved1;
		if(info)
		{
			if(info->magic == SURFACE_TABLE_MAGIC)
			{
				return info;
			}
		}
	}
	
	return NULL;
}

void SurfaceAttachTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *tex, int level)
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
				item->texture.tex == tex &&
				item->texture.level == level
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

void SurfaceToMesa(LPDDRAWI_DDRAWSURFACE_LCL surf)
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
					if(item->texture.level <= item->texture.tex->mipmap_level)
					{
						item->texture.tex->data_dirty[item->texture.level] = TRUE;
						item->texture.tex->dirty = TRUE;
					}
					break;
			}
			
			item = item->next;
		}
	}
	
	FLATPTR vidmem = surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		if(citem->pid == pid)
		{
			int i;
			for(i = 0; i < citem->ctx->flips_cnt; i++)
			{
				if(citem->ctx->flips[i]->lpGbl->fpVidMem == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferUploadColor(ctx, (void*)vidmem);
						ctx->render.dirty = TRUE;
					GL_BLOCK_END
				}
			}
			
			if(citem->ctx->depth)
			{
				if(citem->ctx->depth->lpGbl->fpVidMem == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferUploadDepth(ctx, (void*)vidmem);
						ctx->render.zdirty = TRUE;
					GL_BLOCK_END
				}
			}
		}
		
		citem = citem->next;
	}
}

void SurfaceFromMesa(LPDDRAWI_DDRAWSURFACE_LCL surf)
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
	
	FLATPTR vidmem = surf->lpGbl->fpVidMem;
	context_attachment_t *citem = contexts.first;	
	DWORD pid = GetCurrentProcessId();

	while(citem)
	{
		if(citem->pid == pid)
		{
			int i;
			for(i = 0; i < citem->ctx->flips_cnt; i++)
			{
				if(citem->ctx->flips[i]->lpGbl->fpVidMem == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferDownloadColor(ctx, (void*)vidmem);
						ctx->render.dirty = FALSE;
					GL_BLOCK_END
				}
			}
			
			if(citem->ctx->render.zdirty && citem->ctx->depth)
			{
				if(citem->ctx->depth->lpGbl->fpVidMem == vidmem)
				{
					GL_BLOCK_BEGIN(citem->ctx)
						MesaBufferDownloadDepth(ctx, (void*)vidmem);
						ctx->render.zdirty = FALSE;
					GL_BLOCK_END
				}
			}
		}
		
		citem = citem->next;
	}
}

void SurfaceTableDestroy(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		while(info->first)
		{
			surface_attachment_t *item = info->first;
			info->first = item->next;
			HeapFree(hSharedHeap, 0, item);
		}
	}
	
	HeapFree(hSharedHeap, 0, info);
	surf->dwReserved1 = 0;
}

void SurfaceDeattachTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_tex, int level)
{
	TRACE_ENTRY

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_TEX && item->texture.tex == mesa_tex && item->texture.level == level)
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

typedef struct surface_nest_item
{
	struct surface_nest_item *next;
	mesa3d_ctx_t *ctx;
	mesa3d_texture_t *tex;
} surface_nest_item_t;

typedef struct surface_nest
{
	LPDDRAWI_DDRAWSURFACE_LCL surf;
	void *ddlcl;
	surface_nest_item_t *first;
} surface_nest_t;

#define NESTS_TABLE_POOL 1024

typedef struct surface_nests_table
{
	DWORD size;
	surface_nest_t **nests;
} surface_nests_table_t;

static surface_nests_table_t nests = {0};

static BOOL SurfaceNestAlloc(surface_nest_t **info, DWORD id)
{
	DWORD index = id - 1;
	
	surface_nest_t **oldptr = nests.nests;
	
	if(index >= nests.size)
	{
		nests.size = ((index + NESTS_TABLE_POOL)/NESTS_TABLE_POOL) * NESTS_TABLE_POOL;
		
		if(nests.nests == NULL)
		{
			nests.nests = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_nest_t*)*nests.size);
		}
		else
		{
			nests.nests = HeapReAlloc(hSharedHeap, HEAP_ZERO_MEMORY, 
				nests.nests, sizeof(surface_nest_t*)*nests.size);
		}
	}
	
	if(nests.nests == NULL)
	{
		nests.nests = oldptr;
		return FALSE;
	}
	
	if(nests.nests[index] != NULL)
	{
		/* someone is already here, clear this first */
		SurfaceNestDestroy(id, FALSE);
	}
	
	surface_nest_t *nest = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_nest_t));
	if(nest)
	{
		nest->first = NULL;
		nests.nests[index] = nest;
		*info = nest;
		
		return TRUE;
	}
	
	return FALSE;
}

static surface_nest_t *get_nest(DWORD id)
{
	if(id > 0)
	{
		return nests.nests[id-1];
	}
	
	return NULL;
}

DWORD SurfaceNestCreate(LPDDRAWI_DDRAWSURFACE_LCL surf, void *ddlcl)
{
	TRACE_ENTRY

	surface_nest_t *info;

	if(surf->lpSurfMore->dwSurfaceHandle == 0)
	{
		return 0;
	}
	
	if(SurfaceNestAlloc(&info, surf->lpSurfMore->dwSurfaceHandle))
	{
		info->first = NULL;
		info->surf  = surf;
		info->ddlcl = ddlcl;
		TRACE("SurfaceNestCreate: nest=%d", surf->lpSurfMore->dwSurfaceHandle);
		
		return surf->lpSurfMore->dwSurfaceHandle;
	}
	
	return 0;
}

void SurfaceNestDestroy(DWORD nest, BOOL call_destructor)
{
	TRACE_ENTRY
	
	TRACE("SurfaceNestDestroy: nest=%d", nest);

	DWORD pid = GetCurrentProcessId();
	
	surface_nest_t *info = get_nest(nest);
	if(info)
	{
		while(info->first)
		{
			surface_nest_item_t *item = info->first;
			info->first = item->next;
			
			if(call_destructor &&
				item->ctx->entry->pid == pid)
			{
				GL_BLOCK_BEGIN(item->ctx)
					MesaDestroyTexture(item->tex);
				GL_BLOCK_END
			}

			HeapFree(hSharedHeap, 0, item);
		}
		
		nests.nests[nest-1] = NULL;
		HeapFree(hSharedHeap, 0, info);
	}
}

void *SurfaceNestTexture(DWORD nest, void *mesa_ctx)
{
	TRACE_ENTRY

	surface_nest_t *info = get_nest(nest);
	mesa3d_texture_t *tex = NULL;
	
	if(info)
	{
		surface_nest_item_t *item = info->first;
		surface_nest_item_t *last = NULL;
		while(item)
		{
			if(item->ctx == mesa_ctx)
			{
				return item->tex;
			}
			
			last = item;
			item = item->next;
		}
		
		item = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_nest_item_t));
		
		if(item)
		{
			//GL_BLOCK_BEGIN(mesa_ctx)
			 	tex = MesaCreateTexture(mesa_ctx, info->surf);
			//GL_BLOCK_END
			
			if(tex)
			{
				item->ctx = mesa_ctx;
				item->tex = tex;
				item->next = NULL;
				
				if(last)
					last->next = item;
				else
					info->first = item;
			}
			else
			{
				HeapFree(hSharedHeap, 0, item);
			}
		}
	}
	
#ifdef WARN_ON
	if(tex == NULL && nest != 0)
	{
		WARN("Cannot load texture %d", nest);
	}
#endif
	
	return tex;
}

LPDDRAWI_DDRAWSURFACE_LCL SurfaceNestSurface(DWORD nest)
{
	TRACE_ENTRY

	surface_nest_t *info = get_nest(nest);
	
	if(info)
	{
		return info->surf;
	}
	
	return NULL;
}

void SurfaceNestCleanupCtx(void *mesa_ctx)
{
	TRACE_ENTRY

	int i;
	for(i = 0; i < nests.size; i++)
	{
		if(nests.nests[i] != NULL)
		{
			surface_nest_t *info = nests.nests[i];
			surface_nest_item_t *item = info->first;
			surface_nest_item_t *last = NULL;
			
			while(item)
			{
				if(item->ctx == mesa_ctx)
				{
					if(last)
					{
						last->next = item->next;
					}
					else
					{
						info->first = item->next;
					}
					
					surface_nest_item_t *ptr = item;
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
}

void SurfaceNestCleanupAll(void *ddlcl)
{
	TRACE_ENTRY

	int i;
	for(i = 0; i < nests.size; i++)
	{
		if(nests.nests[i] != NULL)
		{
			if(nests.nests[i]->ddlcl == ddlcl)
			{
				SurfaceNestDestroy(i+1, FALSE);
			}
		}
	}
}
