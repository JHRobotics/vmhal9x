/******************************************************************************
 * Copyright (c) 2023 Jaroslav Hensl                                          *
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

#include "vmhal9x.h"

#include "nocrt.h"

void dbg_prefix_printf(const char *topic, const char *prefix, const char *file, int line, const char *fmt, ...)
{
  va_list vl;
  FILE *fa;  
  
#ifdef DEBUG_TOPIC
	if(topic == NULL)
	{
		if(prefix == NULL)
		{
			return;
		}

		if((strcmp(prefix, "T|") == 0) || (strcmp(prefix, "D|") == 0))
		{
			return;
		}
	}
	else if(strcmp(topic, VMHAL_DSTR(DEBUG_TOPIC)) != 0)
	{
		return;
	}
#endif

  fa = fopen("C:\\vmhal9x.log", "ab");
  if(!fa) return;
  fputs(prefix, fa);
  
#ifdef DEBUG_THREAD
	fprintf(fa, "%08X|", GetCurrentThreadId());
#endif
  
  va_start(vl, fmt);
  vfprintf(fa, fmt, vl);
  va_end(vl);
  
  fprintf(fa, "|%s:%d\r\n", file, line);
  fclose(fa);
}
