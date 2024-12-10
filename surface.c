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
#define SURF_TYPE_CTX  3

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
		mesa3d_ctx_t *ctx;
	};
} surface_attachment_t;

typedef struct surface_info
{
	DWORD magic;
	surface_attachment_t *first;
	LPDDRAWI_DDRAWSURFACE_LCL surf;
} surface_info_t;

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

void SurfaceAttachCtx(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_ctx)
{
	TRACE_ENTRY
	
	if(!surf)
		return;

	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_CTX && 
				item->ctx == mesa_ctx
			){
				/* context already attached */
				return;
			}
			
			item = item->next;
		}
		
		item = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_attachment_t));
		if(item)
		{
			item->type = SURF_TYPE_CTX;
			item->ctx = mesa_ctx;
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
				case SURF_TYPE_CTX:
					if(item->ctx->front)
					{
						if(SurfaceGetInfo(item->ctx->front->lpLcl) == info)
						{
							GL_BLOCK_BEGIN(item->ctx)
								MesaBufferUploadColor(ctx, (void*)ctx->front->lpLcl->lpGbl->fpVidMem);
								item->ctx->render.dirty = FALSE;
							GL_BLOCK_END
						}
					}
					if(item->ctx->depth)
					{
						if(SurfaceGetInfo(item->ctx->depth->lpLcl) == info)
						{
							GL_BLOCK_BEGIN(item->ctx)
								MesaBufferUploadDepth(ctx, (void*)ctx->depth->lpLcl->lpGbl->fpVidMem);
								item->ctx->render.zdirty = FALSE;
							GL_BLOCK_END
						}
					}
					break;
			}
			
			item = item->next;
		}
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
				case SURF_TYPE_CTX:
					if(item->ctx->render.dirty && item->ctx->front)
					{
						if(SurfaceGetInfo(item->ctx->front->lpLcl) == info)
						{
							GL_BLOCK_BEGIN(item->ctx)
								MesaBufferDownloadColor(ctx, (void*)ctx->front->lpLcl->lpGbl->fpVidMem);
								item->ctx->render.dirty = FALSE;
							GL_BLOCK_END
						}
					}
					if(item->ctx->render.zdirty && item->ctx->depth)
					{
						if(SurfaceGetInfo(item->ctx->depth->lpLcl) == info)
						{
							GL_BLOCK_BEGIN(item->ctx)
								MesaBufferDownloadDepth(ctx, (void*)ctx->depth->lpLcl->lpGbl->fpVidMem);
								item->ctx->render.zdirty = FALSE;
							GL_BLOCK_END
						}
					}
					break;
			}
			
			item = item->next;
		}
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

void SurfaceFlipMesa()
{
	TRACE_ENTRY
	
	mesa3d_entry_t *entry = Mesa3DGet(GetCurrentProcessId(), FALSE);
	
	if(entry)
	{
		int i;
		for(i = 0; i < MESA3D_MAX_CTXS; i++)
		{
			if(entry->ctx[i] != NULL)
			{
				if(entry->ctx[i]->render.dirty)
				{
					GL_BLOCK_BEGIN(entry->ctx[i])
						MesaBufferDownloadColor(ctx, (void*)ctx->front->lpLcl->lpGbl->fpVidMem);
						ctx->render.dirty = FALSE;
					GL_BLOCK_END
				}
			}
		}
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

void SurfaceDeattachCtx(LPDDRAWI_DDRAWSURFACE_LCL surf, void *mesa_ctx)
{
	TRACE_ENTRY
	
	surface_info_t *info = SurfaceGetInfo(surf);
	if(info)
	{
		surface_attachment_t *item = info->first;
		surface_attachment_t *last = NULL;
		
		while(item)
		{
			if(item->type == SURF_TYPE_CTX && item->ctx == mesa_ctx)
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
