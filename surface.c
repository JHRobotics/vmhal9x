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

	//surf->lpSurfMore->dwSurfaceHandle = (DWORD)info;
	surf->dwReserved1 = (DWORD)info;
	
	//return surf->lpSurfMore->dwSurfaceHandle;
	return surf->dwReserved1;
}

static surface_info_t *SurfaceGetInfo(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf->dwReserved1 >= 0x80000000) /* hack: allocation from shared heap resides in shared memory */
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
			if(citem->ctx->front)
			{
				if(citem->ctx->front->lpGbl->fpVidMem == vidmem)
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
			if(citem->ctx->render.dirty && citem->ctx->front)
			{
				if(citem->ctx->front->lpGbl->fpVidMem == vidmem)
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

/*
	This is a bit headache, in time when DD Flip is called DX runtime switch
	primary pointer to the front buffer surface. So pointers are now:
		ctx->front->lpGbl->fpVidMem == targ->lpGbl->fpVidMem
		So we can copy GL color buffer to "targ" surface,

		but in same we should read back from "curr" back to GL
		but this seems to be pretty ineffective
*/
void SurfaceFlipMesa(LPDDRAWI_DDRAWSURFACE_LCL curr, LPDDRAWI_DDRAWSURFACE_LCL targ)
{
	TRACE_ENTRY
	
	context_attachment_t *item = contexts.first;
	
	DWORD pid = GetCurrentProcessId();
	
	while(item)
	{
		if(item->pid == pid)
		{
			GL_BLOCK_BEGIN(item->ctx)
				if(item->ctx->render.dirty)
				{
					MesaBufferDownloadColor(ctx, (void*)ctx->front->lpGbl->fpVidMem);
					ctx->render.dirty = FALSE;
				}
#if 0
				if(ctx->front->lpGbl->fpVidMem == targ->lpGbl->fpVidMem)
				{
					MesaBufferUploadColor(ctx, (void*)curr->lpGbl->fpVidMem);
					ctx->render.dirty = TRUE;
				}
#endif
			GL_BLOCK_END
		}
		
		item = item->next;
	}
}

void SurfaceDeattachTexture(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_tex)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_TEX && item->texture.tex == mesa_tex)
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
