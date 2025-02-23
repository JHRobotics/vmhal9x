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
#ifndef __VMHAL9X__MEMORY_H__INCLUDED__
#define __VMHAL9X__MEMORY_H__INCLUDED__

#define HEAP_NORMAL 0
#define HEAP_LARGE  1
#define HEAP_VRAM   2 /* don't use yet! */

BOOL  hal_memory_init();
void hal_memory_destroy();

void *hal_alloc(int heap, size_t size, DWORD width);
void *hal_calloc(int heap, size_t size, DWORD width);
BOOL hal_realloc(int heap, void **block, size_t new_size, BOOL zero);
void  hal_free(int heap, void *block);

#endif /* __VMHAL9X__MEMORY_H__INCLUDED__ */
