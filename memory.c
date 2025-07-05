/******************************************************************************
 * Copyright (c) 2025 Jaroslav Hensl                                          *
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
#include <Windows.h>


#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include "ddrawi_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"

#ifdef DEBUG_MEMORY
#include "nuke.h"
#include "surface.h"
#endif

#include "memory.h"

#include "nocrt.h"

//#define NODDRAW

#define MEM_ALIGN 16

#define VID_HEAP_COUNT 4
#define VID_HEAP_SYSTEM VID_HEAP_COUNT

/* DDRAW memory functions, if someone has documentation for this, please tell me */
typedef FLATPTR (WINAPI *DDHAL32_VidMemAlloc_h)(LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, DWORD dwWidth, DWORD dwHeight);
typedef void (WINAPI *DDHAL32_VidMemFree_h)(LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, FLATPTR ptr);

// init heap
typedef LPVMEMHEAP (WINAPI *VidMemInit_h)(DWORD flags, FLATPTR start, FLATPTR end_or_width, DWORD height, DWORD pitch);
// destroy heap
typedef void (WINAPI *VidMemFini_h)(LPVMEMHEAP pvmh);
// get remain memory on heap
typedef DWORD (WINAPI *VidMemAmountFree_h)(LPVMEMHEAP pvmh);
// not present on W98
//typedef DWORD (WINAPI *VidMemAmountAllocated_h)(LPVMEMHEAP pvmh);
// ?
typedef DWORD (WINAPI *VidMemLargestFree_h)(LPVMEMHEAP pvmh);
// allocate on heap
typedef FLATPTR (WINAPI *VidMemAlloc_h)(LPVMEMHEAP pvmh, DWORD x, DWORD y);
// free on heap
typedef void (WINAPI *VidMemFree_h)(LPVMEMHEAP pvmh, FLATPTR ptr);

#ifndef HEAP_SHARED
/* undocumented heap behaviour for shared DLL (from D3_DD32.C) */
#define HEAP_SHARED      0x04000000
#endif

/* heaps */
HANDLE hSharedHeap = NULL;
HANDLE hSharedLargeHeap = NULL;

static DWORD mem_stat_vram_used = 0;
static DWORD mem_stat_vram_blocked = 0;

#ifdef DEBUG_MEMORY
CRITICAL_SECTION mem_cs;
#endif

BOOL hal_memory_init()
{
	hSharedHeap = HeapCreate(HEAP_SHARED, 0x2000, 0);
	hSharedLargeHeap = HeapCreate(HEAP_SHARED, 0x10000, 0);

#ifdef DEBUG_MEMORY
	InitializeCriticalSection(&mem_cs);
#endif

	return hSharedHeap != NULL && hSharedLargeHeap != NULL;
}

void hal_memory_destroy()
{
	HeapDestroy(hSharedHeap);
	HeapDestroy(hSharedLargeHeap);
	hSharedHeap = NULL;
	hSharedLargeHeap = NULL;
}

#define ALIGN_SIZE(_s) (((_s) + ((MEM_ALIGN)-1)) & (~((MEM_ALIGN)-1)))

static HANDLE get_heap(int heap)
{
	switch(heap)
	{
		case HEAP_NORMAL: return hSharedHeap;
		case HEAP_LARGE: return hSharedLargeHeap;
	}
	
	return NULL;
}

#define BLOCK_EMPTY (~0)

static int hal_find_gap(FBHDA_t *hda, int count, BOOL rowalign)
{
	int y, x;

	for(y = 0; y < hda->heap_length; y++)
	{
		if(rowalign)
		{
			DWORD offset = (BYTE*)hda->vram_pm32 - (hda->heap_end - (y+count)*FB_VRAM_HEAP_GRANULARITY);
			if((offset % hda->pitch) != 0)
			{
				continue;
			}
		}

		if(hda->heap_info[y] == BLOCK_EMPTY)
		{
			for(x = y+1; x < y + count; x++)
			{
				if(x >= hda->heap_length)
				{
					return -1;
				}
				if(hda->heap_info[x] != BLOCK_EMPTY)
				{
					y = x;
					break;
				}
			}
			if(x == y + count)
			{
				return y;
			}
		}
	}

	return -1;
}

BOOL hal_vinit()
{
	FBHDA_t *hda = FBHDA_setup();
	if(hda)
	{
		int x;
		for(x = 0; x < hda->heap_count; x++)
		{
			hda->heap_info[x] = BLOCK_EMPTY;
		}
		return TRUE;
	}
	return FALSE;
}

void hal_vblock_add(LPDDRAWI_DIRECTDRAW_GBL lpDD, LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	// TODO: lower vram heap start
	mem_stat_vram_blocked += surf->lpGbl->dwBlockSizeX;
}

void hal_vblock_reset()
{
	mem_stat_vram_blocked = 0;
}


BOOL hal_valloc(LPDDRAWI_DIRECTDRAW_GBL lpDD, LPDDRAWI_DDRAWSURFACE_LCL surf, BOOL systemram, BOOL rowalign)
{
	TRACE_ENTRY

	if(systemram == FALSE)
	{
		FBHDA_t *hda = FBHDA_setup();
		DWORD bs = (surf->lpGbl->dwBlockSizeX + FB_VRAM_HEAP_GRANULARITY-1) / FB_VRAM_HEAP_GRANULARITY;
		TOPIC("MEMORY", "vram alloc size=%d, bs=%d", surf->lpGbl->dwBlockSizeX, bs);
		if(bs == 0)
		{
			/* special size for zero allocation */
			surf->lpGbl->lpVidMemHeap = NULL;
			surf->lpGbl->fpVidMem = (FLATPTR)hda->heap_end;
			return TRUE;
		}
		else
		{
			int gap = hal_find_gap(hda, bs, rowalign);
			if(gap >= 0)
			{
				int id = gap + bs;
				TOPIC("MEMORY", "gap at %d, id=%d", id, gap);

				int x;
				for(x = gap; x < id; x++)
				{
					hda->heap_info[x] = id;
				}
				surf->lpGbl->lpVidMemHeap = NULL;
				surf->lpGbl->fpVidMem = (FLATPTR)(hda->heap_end - id*FB_VRAM_HEAP_GRANULARITY);
				TOPIC("MEMORY", "new vram ptr=%p", surf->lpGbl->fpVidMem);

				mem_stat_vram_used += bs * FB_VRAM_HEAP_GRANULARITY;

				return TRUE;
			}
			else
			{
				TOPIC("MEMORY", "vram alloc fail: heap_length=%d, first block=0x%X", hda->heap_length, hda->heap_info[0]);
			}
		}
		return FALSE;
	}
	else
	{
		DWORD size = surf->lpGbl->dwBlockSizeX;
		void *mem = hal_alloc(HEAP_LARGE, size, surf->lpGbl->lPitch);
		if(mem != NULL)
		{
#ifdef DEBUG
			memset(mem, 0xCC, size);
#endif
			surf->lpGbl->lpVidMemHeap = NULL;
			surf->lpGbl->fpVidMem = (FLATPTR)(mem);
			return TRUE;
		}

		return FALSE;
	}

	return TRUE;
}

void hal_vfree(LPDDRAWI_DIRECTDRAW_GBL lpDD, LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	TOPIC("VMALLOC", "free ptr=%08X", surf->lpGbl->fpVidMem);
	TOPIC("ALLOCTRACE", "hal_vfree");

	if(surf->lpGbl->fpVidMem > DDHAL_PLEASEALLOC_BLOCKSIZE)
	{
		FBHDA_t *hda = FBHDA_setup();
		BYTE *ptr = (BYTE*)surf->lpGbl->fpVidMem;
		BYTE *vram_end = (BYTE*)hda->vram_pm32 + hda->vram_size;

		if(ptr >= (BYTE*)hda->vram_pm32 && ptr < vram_end)
		{
			/* VRAM */
			if(ptr == hda->heap_end) /* zero vram allocation */
			{
				/* ... */
			}
			else if(ptr >= hda->heap_start && ptr < hda->heap_end)
			{
				TOPIC("MEMORY", "vram free ptr=%p (%p %p)", ptr, hda->heap_start, hda->heap_end);
				DWORD start = (hda->heap_end - ptr);
				DWORD id = start/FB_VRAM_HEAP_GRANULARITY;
				DWORD size = 0;
				TOPIC("MEMORY", "vram freeing id=%d, start=%d", id, start);

				if(hda->heap_info[id-1] == BLOCK_EMPTY)
				{
					ERR("double free on %p", ptr);
					return;
				}

				int x;
				for(x = id-1; x >= 0; x--)
				{
					if(hda->heap_info[x] != id)
					{
						break;
					}
					hda->heap_info[x] = BLOCK_EMPTY;
					size += FB_VRAM_HEAP_GRANULARITY;
				}

				mem_stat_vram_used -= size;
				TOPIC("MEMORY", "free success size=%d", size);
			}
		}
		else
		{
			hal_free(HEAP_LARGE, ptr);
		}
	}
	else
	{
		ERR("invalid vram ptr=%p for sid=%d", surf->lpGbl->fpVidMem, surf->dwReserved1);
	}

	surf->lpGbl->fpVidMem = 0;
}

#ifndef DEBUG_MEMORY
/* 
 * release version of allocators
 */

void *hal_alloc(int heap, size_t size, DWORD width)
{
	void *mem = NULL;
	HANDLE h = get_heap(heap);
	if(h != NULL)
	{
		size = ALIGN_SIZE(size);
		mem = HeapAlloc(h, 0, size);
	}
	
	return mem;
}

void *hal_calloc(int heap, size_t size, DWORD width)
{
	void *mem = NULL;
	HANDLE h = get_heap(heap);
	if(h != NULL)
	{
		size = ALIGN_SIZE(size);
		mem = HeapAlloc(h, HEAP_ZERO_MEMORY, size);
	}
	return mem;
}

BOOL hal_realloc(int heap, void **block, size_t new_size, BOOL zero)
{
	BOOL rc = FALSE;
	HANDLE h = get_heap(heap);
	if(h != NULL && block != NULL)
	{
		new_size = ALIGN_SIZE(new_size);
		if(*block == NULL)
		{
			*block = HeapAlloc(h, (zero == FALSE) ? 0 : HEAP_ZERO_MEMORY, new_size);
			if(*block != NULL)
				rc = TRUE;
		}
		else
		{
			void *mem = HeapReAlloc(h, (zero == FALSE) ? 0 : HEAP_ZERO_MEMORY, *block, new_size);
			if(mem != NULL)
			{
				*block = mem;
				rc = TRUE;
			}
		}
	}
	
	return rc;
}

void hal_free(int heap, void *mem)
{
	HANDLE h = get_heap(heap);
	if(h != NULL && mem != NULL)
	{
		HeapFree(h, 0, mem);
	}
}

#else
/* 
 * debuf version of allocators
 */

#define DEBUG_INFO_MAGIC 0xFCFC1234

typedef struct debug_meminfo
{
	DWORD magic;
	int heap;
	size_t size;
	const char *src_file;
	int src_line;
	struct debug_meminfo *prev;
	struct debug_meminfo *next;
	DWORD pad;
} debug_meminfo_t;

static debug_meminfo_t *first = NULL;
static debug_meminfo_t *last  = NULL;

static void *hal_xalloc(const char *file, int line, int heap, size_t size, DWORD width, BOOL zero)
{
	TRACE_ENTRY

	debug_meminfo_t *mem = NULL;
	HANDLE h = get_heap(heap);
	if(h != NULL)
	{
		size = ALIGN_SIZE(size+sizeof(debug_meminfo_t));
		mem = HeapAlloc(h, (zero == FALSE) ? 0 : HEAP_ZERO_MEMORY, size);
		if(mem)
		{
			mem->magic = DEBUG_INFO_MAGIC;
			mem->heap = heap;
			mem->size = size;
			mem->src_file = file;
			mem->src_line = line;
			if(last)
			{
				mem->prev = last;
				mem->next = NULL;
				last->next = mem;
				last = mem;
			}
			else
			{
				mem->prev = NULL;
				mem->next = NULL;
				first = mem;
				last  = mem;
			}
		}
	}

	if(mem != NULL)
	{
		return mem+1;
	}
	
	return NULL;
}

void *hal_alloc_debug(const char *file, int line, int heap, size_t size, DWORD width)
{
	void *ptr;
	EnterCriticalSection(&mem_cs);
	ptr = hal_xalloc(file, line, heap, size, width, FALSE);
	LeaveCriticalSection(&mem_cs);

	return ptr;
}

void *hal_calloc_debug(const char *file, int line, int heap, size_t size, DWORD width)
{
	void *ptr;
	EnterCriticalSection(&mem_cs);
	ptr = hal_xalloc(file, line, heap, size, width, TRUE);
	LeaveCriticalSection(&mem_cs);

	return ptr;
}

BOOL hal_realloc_debug(const char *file, int line, int heap, void **block, size_t new_size, BOOL zero)
{
	BOOL rc = FALSE;
	EnterCriticalSection(&mem_cs);
	HANDLE h = get_heap(heap);
	if(h != NULL && block != NULL)
	{
		if(*block == NULL)
		{
			*block = hal_xalloc(file, line, heap, new_size, 0, zero);
			if(*block != NULL)
				rc = TRUE;
		}
		else
		{
			new_size = ALIGN_SIZE(new_size + sizeof(debug_meminfo_t));

			debug_meminfo_t *base_block = ((debug_meminfo_t*)*block) - 1;
			debug_meminfo_t *mem = HeapReAlloc(h, (zero == FALSE) ? 0 : HEAP_ZERO_MEMORY, base_block, new_size);
			if(mem != NULL)
			{
				mem->size = new_size;
				if(mem->prev)
				{
					mem->prev->next = mem;
				}
				else
				{
					first = mem;
				}

				if(mem->next)
				{
					mem->next->prev = mem;
				}
				else
				{
					last = mem;
				}

				*block = mem+1;
				rc = TRUE;
			}
		}
	}

	LeaveCriticalSection(&mem_cs);
	return rc;
}

void hal_free_debug(const char *file, int line, int heap, void *mem)
{
	HANDLE h = get_heap(heap);
	if(h != NULL && mem != NULL)
	{
		debug_meminfo_t *base_block = ((debug_meminfo_t*)mem) - 1;
		if(base_block->magic != DEBUG_INFO_MAGIC)
		{
			if(base_block->magic == ~DEBUG_INFO_MAGIC)
			{
				ERR("double free on %s:%d", file, line);
			}
			else
			{
				ERR("invalid memory %s:%d", file, line);
			}
			return;
		}
		
		if(base_block->heap != heap)
		{
			ERR("Invalid heap - allocated in %d on %s:%d, tried to free as %d on %s:%d",
				base_block->heap, base_block->src_file, base_block->src_line,
				heap, file, line
			);
			return;
		}
		

		EnterCriticalSection(&mem_cs);
		base_block->magic = ~DEBUG_INFO_MAGIC;
		if(base_block->prev)
		{
			base_block->prev->next = base_block->next;
			if(base_block->next == NULL)
			{
				last = base_block->prev;
			}
		}
		else
		{
			first = base_block->next;
			if(first)
			{
				first->prev = NULL;
			}
		}

		if(base_block->next)
		{
			base_block->next->prev = base_block->prev;
			if(base_block->prev == NULL)
			{
				first = base_block->next;
			}
		}
		else
		{
			last = base_block->prev;
			if(last)
			{
				last->next = NULL;
			}
		}

		if(first == NULL || last == NULL)
		{
			first = NULL;
			last  = NULL;
		}

		HeapFree(h, 0, base_block);
		LeaveCriticalSection(&mem_cs);
	}
}

void hal_dump_allocs()
{
	debug_meminfo_t *item = first;
	
	TOPIC("MEMORY", "--- leak table ---");
	while(item != NULL)
	{
		TOPIC("MEMORY", "%s:%d = %d", item->src_file, item->src_line, item->size);
#if 0
		/* WARN: before use this, make sure, that files and lines match! */
		if(strcmp(item->src_file, "surface.c") == 0 && item->src_line == 152)
		{
			surface_info_t *info = (surface_info_t *)(item+1);
			TOPIC("MEMORY", "Lost surface sid=%d", info->id);
		}
#endif
		item = item->next;
	}

	TOPIC("MEMORY", "--- leak table end ---");
}

void hal_alloc_info()
{
	debug_meminfo_t *item = first;
	DWORD s = 0;
	while(item != NULL)
	{
		s += item->size;
		item = item->next;
	}
	TOPIC("GC", "HAL total memory allocation: %u", s);
}

#endif /* DEBUG_MEMORY */

/* public */

BOOL __stdcall VidMemInfo(DWORD *pused, DWORD *pfree)
{
	TRACE_ENTRY
	
	FBHDA_t *hda = FBHDA_setup();
	if(hda)
	{
		*pused = mem_stat_vram_used + mem_stat_vram_blocked;
		*pfree = hda->vram_size - (hda->system_surface + mem_stat_vram_used + mem_stat_vram_blocked + hda->overlays_size + hda->stride);
		return TRUE;
	}

	return FALSE;
}
