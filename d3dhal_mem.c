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
#include <windows.h>
#include <stdint.h>

#include "d3dhal_mem.h"

static HANDLE hal3d_heap = NULL;

static HANDLE hal3d_create_heap()
{
	return HeapCreate(0, 0, 0);
}

void *hal3d_malloc(size_t size)
{
	if(hal3d_heap == NULL) hal3d_heap = hal3d_create_heap();

	if(hal3d_heap != NULL)
	{
		return HeapAlloc(hal3d_heap, 0, size);
	}
	
	return NULL;
}

void *hal3d_calloc(size_t size)
{
	if(hal3d_heap == NULL) hal3d_heap = hal3d_create_heap();

	if(hal3d_heap != NULL)
	{
		return HeapAlloc(hal3d_heap, HEAP_ZERO_MEMORY, size);
	}
	
	return NULL;
}

BOOL hal3d_realloc(void **mem, size_t newsize)
{
	if(hal3d_heap == NULL) hal3d_heap = hal3d_create_heap();

	if(*mem == NULL)
	{
		*mem = HeapAlloc(hal3d_heap, HEAP_ZERO_MEMORY, newsize);
		return *mem == NULL ? FALSE : TRUE;
	}
	else
	{
		void *ptr = HeapReAlloc(hal3d_heap, HEAP_ZERO_MEMORY, *mem, newsize);
		if(ptr != NULL)
		{
			*mem = ptr;
			return TRUE;
		}
	}
	
	return FALSE;
}

void hal3d_free(void **ptr)
{
	if(hal3d_heap != NULL)
	{
		HeapFree(hal3d_heap, 0, *ptr);
		*ptr = NULL;
	}
}
