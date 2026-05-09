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
#ifndef NUKED_SKIP
#include <windows.h>
#include <stdint.h>
#include "d3dhal_mem.h"
#include "ht.h"
#include "nocrt.h"
#endif

hashtable_t *ht_init(DWORD prime)
{
	hashtable_t *ht;
	if(prime == 0) prime = HT_PRIME_DEF;
	ht = (hashtable_t*)hal3d_calloc(sizeof(hashtable_t)+sizeof(hashtable_item_t*)*(prime-1));
	if(ht != NULL)
	{
		ht->length = 0;
		ht->prime = prime;
	}
	return ht;
}

void ht_insert(hashtable_t *ht, DWORD id, void *target)
{
	hashtable_item_t **ptr;
	for(ptr = &ht->items[HT_HASH(ht, id)]; *ptr != NULL; ptr = &((*ptr)->next));
	*ptr = (hashtable_item_t*)hal3d_malloc(sizeof(hashtable_item_t));
	if(*ptr != NULL)
	{
		(*ptr)->next = NULL;
		(*ptr)->target = target;
		(*ptr)->id = id;
		ht->length++;
	}
}

void ht_replace(hashtable_t *ht, DWORD id, void *target)
{
	hashtable_item_t *item;
	for(item = ht->items[HT_HASH(ht, id)]; item != NULL; item = item->next)
	{
		if(item->id == id)
		{
			item->target = target;
			return;
		}
	}
	ht_insert(ht, id, target);
}

void *ht_lookup_more(hashtable_t *ht, DWORD id, DWORD item_num)
{
	hashtable_item_t *item;
	for(item = ht->items[HT_HASH(ht, id)]; item != NULL; item = item->next)
	{
		if(item->id == id)
		{
			if(item_num-- == 0)
			{
				return item->target;
			}
		}
	}
	return NULL;
}

void *ht_lookup(hashtable_t *ht, DWORD id)
{
	return ht_lookup_more(ht, id, 0);
}

BOOL ht_exists(hashtable_t *ht, DWORD id, void *target)
{
	hashtable_item_t *item;
	for(item = ht->items[HT_HASH(ht, id)]; item != NULL; item = item->next)
	{
		if(item->id == id && item->target == target)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void ht_delete(hashtable_t *ht, DWORD id)
{
	hashtable_item_t **ptr = &ht->items[HT_HASH(ht, id)];
	while(*ptr != NULL)
	{
		hashtable_item_t *item = *ptr;
		if(item->id == id)
		{
			*ptr = item->next;
			ht->length--;
			hal3d_free((void**)&item);
		}
		else
		{
			ptr = &(item->next);
		}
	}
}

void ht_delete_more(hashtable_t *ht, DWORD id, void *target)
{
	hashtable_item_t **ptr = &ht->items[HT_HASH(ht, id)];
	while(*ptr != NULL)
	{
		hashtable_item_t *item = *ptr;
		if(item->id == id && item->target == target)
		{
			*ptr = item->next;
			ht->length--;
			hal3d_free((void**)&item);
		}
		else
		{
			ptr = &(item->next);
		}
	}
}

void ht_walk(hashtable_t *ht, hashtable_item_callback_f callback, void *data)
{
	int i;
	for(i = 0; i < ht->prime; i++)
	{
		hashtable_item_t *item;
		for(item = ht->items[i]; item != NULL; item = item->next)
		{
			callback(item->id, item->target, data);
		}
	}
}

void ht_roll(hashtable_t *ht, hashtable_item_callback_f callback, void *data)
{
	int i;
	for(i = 0; i < ht->prime; i++)
	{
		hashtable_item_t **ptr = &ht->items[i];
		while(*ptr != NULL)
		{
			hashtable_item_t *item = *ptr;
			if(callback != NULL)
			{
				callback(item->id, item->target, data);
			}
			*ptr = item->next;
			ht->length--;
			hal3d_free((void**)&item);
		}
	}
}

void ht_clean(hashtable_t *ht)
{
	ht_roll(ht, NULL, NULL);
}

void ht_destroy(hashtable_t **ht)
{
	if(*ht != NULL)
	{
		ht_clean(*ht);
		hal3d_free((void**)ht);
	}
}
