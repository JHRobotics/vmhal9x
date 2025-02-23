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
#include "nocrt.h"

#define MEM_ALIGN 16

#ifndef HEAP_SHARED
/* undocumented heap behaviour for shared DLL (from D3_DD32.C) */
#define HEAP_SHARED      0x04000000
#endif

/* heaps */
HANDLE hSharedHeap = NULL;
HANDLE hSharedLargeHeap = NULL;

BOOL hal_memory_init()
{
	hSharedHeap = HeapCreate(HEAP_SHARED, 0x2000, 0);
	hSharedLargeHeap = HeapCreate(HEAP_SHARED, 0x10000, 0);
	
	return hSharedHeap != NULL && hSharedLargeHeap != NULL;
}

void hal_memory_destroy()
{
	HeapDestroy(hSharedHeap);
	HeapDestroy(hSharedLargeHeap);
	hSharedHeap = NULL;
	hSharedLargeHeap = NULL;
}

static HANDLE get_heap(int heap)
{
	switch(heap)
	{
		case HEAP_NORMAL: return hSharedHeap;
		case HEAP_LARGE: return hSharedLargeHeap;
	}
	
	return NULL;
}

#define ALIGN_SIZE(_s) (((_s) + ((MEM_ALIGN)-1)) & (~((MEM_ALIGN)-1)))

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
