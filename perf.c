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
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include <x86intrin.h>

#include "vmhal9x.h"

#include "nocrt.h"

#define PERF_INDEX_SIZE 977 	
#define PERF_FILE_MAX 64
#define PERF_LOG "C:\\halperf.log"

typedef struct _perf_item_t
{
	char fname[PERF_FILE_MAX];
	int line;
	uint64_t wasted;
	struct _perf_item_t *next;
} perf_item_t;

static perf_item_t *perf_table[PERF_INDEX_SIZE];
static uint64_t perf_point_cnt = 0;
static uint64_t perf_start_cnt = 0;

static uint32_t perf_hash(const char *file, int line_no)
{
	uint32_t h = 0;
	const char *p = file;
	
	while(*p != '\0')
	{
		h += (uint32_t)(*p);
		p++;
	}
	
	h += (uint32_t)line_no;
	return h % PERF_INDEX_SIZE;
}

static perf_item_t *perf_get_item(const char *file, int line_no)
{
	uint32_t h = perf_hash(file, line_no);
	perf_item_t **pitem = &perf_table[h];
	
	while(*pitem != NULL)
	{
		if(strncmp(file, (*pitem)->fname, PERF_FILE_MAX-1) == 0)
		{
			if((*pitem)->line == line_no)
			{
				return *pitem;
			}
		}
		pitem = &((*pitem)->next);
	}
	
	*pitem = hal_calloc(HEAP_NORMAL, sizeof(perf_item_t), 0);
	if(*pitem != NULL)
	{
		strncpy((*pitem)->fname, file, PERF_FILE_MAX-1);
		(*pitem)->fname[PERF_FILE_MAX-1] = '\0';
		(*pitem)->line = line_no;
		(*pitem)->wasted = 0;
		(*pitem)->next   = NULL;
	}
	return *pitem;
}

void perf_init()
{
	perf_start_cnt = __rdtsc();
}

void perf_destroy()
{
	uint32_t h = 0;
	for(h = 0; h < PERF_INDEX_SIZE; h++)
	{
		perf_item_t *garbage;
		perf_item_t **pitem = &perf_table[h];
		while(*pitem != NULL)
		{
			garbage = *pitem;
			*pitem = garbage->next;
			hal_free(HEAP_NORMAL, garbage);
		}
	}
}

void perf_dump()
{
	uint32_t h = 0;
	uint64_t final = __rdtsc();
	uint64_t total = final - perf_start_cnt;

	FILE *log = fopen(PERF_LOG, "wb");
	if(log)
	{
		for(h = 0; h < PERF_INDEX_SIZE; h++)
		{
			perf_item_t *item = perf_table[h];
			while(item != NULL)
			{
				double d = ((double)item->wasted / (double)total) * 100.0f;
				
				fprintf(log, "%16s:%04d: %02.4f %%\r\n", item->fname, item->line, d);
				item = item->next;
			}
		}
		fclose(log);
	}
}

void perf_mark()
{
	perf_point_cnt = __rdtsc();
}

void perf_point(const char *file, int line_no)
{
	uint64_t e = __rdtsc();
	uint64_t spent = e - perf_point_cnt;
	
	perf_item_t *item = perf_get_item(file, line_no);
	if(item)
	{
		item->wasted += spent;
	}
}
