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
#ifndef __VMHAL9X__HT_H__INCLUDED__
#define __VMHAL9X__HT_H__INCLUDED__

#define HT_PRIME_TINY 13
#define HT_PRIME_SMALL 113
#define HT_PRIME_DEF 991
#define HT_PRIME_LARGE 9973

#define HT_HASH(_tb, _id) ((_id) % ((_tb)->prime))

typedef struct hashtable_item
{
	DWORD id;
	void *target;
	struct hashtable_item *next;
} hashtable_item_t;

typedef struct hashtable
{
	DWORD prime;
	DWORD length;
	hashtable_item_t *items[1];
} hashtable_t;

typedef void (*hashtable_item_callback_f)(DWORD id, void *target, void *data);

hashtable_t *ht_init(DWORD prime);
void ht_insert(hashtable_t *ht, DWORD id, void *target);
void ht_replace(hashtable_t *ht, DWORD id, void *target);
void *ht_lookup(hashtable_t *ht, DWORD id);
void *ht_lookup_more(hashtable_t *ht, DWORD id, DWORD item_num);
BOOL ht_exists(hashtable_t *ht, DWORD id, void *target);
void ht_delete(hashtable_t *ht, DWORD id);
void ht_delete_more(hashtable_t *ht, DWORD id, void *target);
void ht_walk(hashtable_t *ht, hashtable_item_callback_f callback, void *data);
void ht_roll(hashtable_t *ht, hashtable_item_callback_f callback, void *data);
void ht_clean(hashtable_t *ht);
void ht_destroy(hashtable_t **ht);

#define HT_INSERT_T(_t, _ht, _id, _item) ht_insert((_ht), (_id), (void*)(_item))
#define HT_LOOKUP_T(_t, _ht, _id) (_t*)ht_lookup((_ht), (_id))

#endif /* __VMHAL9X__HT_H__INCLUDED__ */
