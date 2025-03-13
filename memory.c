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
#include "memory.h"

#ifdef DEBUG_MEMORY
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include "vmdahal32.h"
#include "vmhal9x.h"
#endif

#include "nocrt.h"

#define MEM_ALIGN 16

#ifndef HEAP_SHARED
/* undocumented heap behaviour for shared DLL (from D3_DD32.C) */
#define HEAP_SHARED      0x04000000
#endif

/* heaps */
HANDLE hSharedHeap = NULL;
HANDLE hSharedLargeHeap = NULL;

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
		item = item->next;
	}
	TOPIC("MEMORY", "--- leak table end ---");
}

#endif /* DEBUG_MEMORY */
