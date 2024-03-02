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
#ifndef __FILL_H__INCLUDED__
#define __FILL_H__INCLUDED__

void memcpy16(void *dst, const void *src, size_t size);
void memcpy8(void *dst, const void *src, size_t size);

void fill4(void *dst, size_t size, uint32_t bpp, uint32_t color);
void fill8(void *dst, size_t size, uint32_t bpp, uint32_t color);
void fill16(void *dst, size_t size, uint32_t bpp, uint32_t color);

#if defined(__GNUC__) && defined(__SSE__)
# define fill_memcpy memcpy16
#elif defined(__GNUC__) && defined(__MMX__)
# define fill_memcpy memcpy8
#else
# define fill_memcpy memcpy
#endif

#if defined(__GNUC__) && defined(__SSE__)
# define fill fill16
#elif defined(__GNUC__) && defined(__MMX__)
# define fill fill8
#else
# define fill fill4
#endif

#endif
