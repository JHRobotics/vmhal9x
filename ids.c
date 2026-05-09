/******************************************************************************
 * Copyright (c) 2026 Jaroslav Hensl                                          *
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
#include "memory.h"
#include "ids.h"
#include "nocrt.h"

static DWORD ids_bf_length = 0; /* in DWORDs */
static DWORD *ids_bf = NULL;

#define BF_BITS 32
#define BF_FULL (~((DWORD)0))
#define BF_LENGTH_INC 128
#define BF_SHIFT 5
#define BF_MASK 0x1F

#define BF_ID_START 1

BOOL __stdcall id_assign(DWORD *new_id)
{
	DWORD i, j;
	for(i = 0; i < ids_bf_length; i++)
	{
		DWORD *ptr = &ids_bf[i];
		if(*ptr != BF_FULL)
		{
			for(j = 0; j < BF_BITS; j++)
			{
				if((((*ptr) >> j) & 0x1) == 0)
				{
					*ptr |= 1 << j;
					*new_id = (i * BF_BITS + j) + BF_ID_START;
					return TRUE;
				}
			}
		}
	}

	/* resize bit field */
	DWORD new_size = ids_bf_length + BF_LENGTH_INC;
	BOOL valid;

	if(ids_bf == NULL)
	{
		ids_bf = hal_calloc(HEAP_NORMAL, new_size*sizeof(DWORD), 0);
		valid = ids_bf != NULL;
	}
	else
	{
		valid = hal_realloc(HEAP_NORMAL, (void**)&ids_bf, new_size*sizeof(DWORD), TRUE);
	}

	if(valid)
	{
		ids_bf[ids_bf_length] = 0x1;
		*new_id = (ids_bf_length * BF_BITS) + BF_ID_START;
		ids_bf_length = new_size;
		return TRUE;
	}
	return FALSE;
}

void __stdcall id_free(DWORD id)
{
	if(id >= BF_ID_START)
	{
		DWORD idd = id - BF_ID_START;
		DWORD dw = idd >> BF_SHIFT;
		if(dw < ids_bf_length)
		{
			ids_bf[dw] &= ~(idd & BF_MASK);
		}
	}
}

void __stdcall id_destroy()
{
	if(ids_bf_length)
	{
		hal_free(HEAP_NORMAL, ids_bf);
		ids_bf = NULL;
		ids_bf_length = 0;
	}
}
