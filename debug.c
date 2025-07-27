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

#if defined(D3DHAL) && defined(DEBUG)
#include "d3dhal_ddk.h"
#include "mesa3d.h"
#endif

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

#ifdef DEBUG

#define GL_STR(_e) case _e: return #_e;

const char *dbg_glenum_name(GLenum e)
{
	switch(e)
	{
		GL_STR(GL_NEVER)
		GL_STR(GL_LESS)
		GL_STR(GL_EQUAL)
		GL_STR(GL_LEQUAL)
		GL_STR(GL_GREATER)
		GL_STR(GL_NOTEQUAL)
		GL_STR(GL_GEQUAL)
		GL_STR(GL_ALWAYS)
		GL_STR(GL_ZERO)
		GL_STR(GL_ONE)
		GL_STR(GL_SRC_COLOR)
		GL_STR(GL_ONE_MINUS_SRC_COLOR)
		GL_STR(GL_SRC_ALPHA)
		GL_STR(GL_ONE_MINUS_SRC_ALPHA)
		GL_STR(GL_DST_ALPHA)
		GL_STR(GL_ONE_MINUS_DST_ALPHA)
		GL_STR(GL_DST_COLOR)
		GL_STR(GL_ONE_MINUS_DST_COLOR)
		GL_STR(GL_SRC_ALPHA_SATURATE)
		GL_STR(GL_TEXTURE_ENV_MODE)
		GL_STR(GL_TEXTURE_ENV_COLOR)
		GL_STR(GL_MODULATE)
		GL_STR(GL_DECAL)
		GL_STR(GL_TEXTURE_ENV)
		GL_STR(GL_EYE_LINEAR)
		GL_STR(GL_OBJECT_LINEAR)
		GL_STR(GL_SPHERE_MAP)
		GL_STR(GL_TEXTURE_GEN_MODE)
		GL_STR(GL_OBJECT_PLANE)
		GL_STR(GL_EYE_PLANE)
		GL_STR(GL_NEAREST)
		GL_STR(GL_LINEAR)
		GL_STR(GL_NEAREST_MIPMAP_NEAREST)
		GL_STR(GL_LINEAR_MIPMAP_NEAREST)
		GL_STR(GL_NEAREST_MIPMAP_LINEAR)
		GL_STR(GL_LINEAR_MIPMAP_LINEAR)
		GL_STR(GL_TEXTURE_MAG_FILTER)
		GL_STR(GL_TEXTURE_MIN_FILTER)
		GL_STR(GL_TEXTURE_WRAP_S)
		GL_STR(GL_TEXTURE_WRAP_T)
		GL_STR(GL_CLAMP)
		GL_STR(GL_REPEAT)
		GL_STR(GL_COMBINE)
		GL_STR(GL_COMBINE_RGB)
		GL_STR(GL_COMBINE_ALPHA)
		GL_STR(GL_SOURCE0_RGB)
		GL_STR(GL_SOURCE1_RGB)
		GL_STR(GL_SOURCE2_RGB)
		GL_STR(GL_SOURCE0_ALPHA)
		GL_STR(GL_SOURCE1_ALPHA)
		GL_STR(GL_SOURCE2_ALPHA)
		GL_STR(GL_OPERAND0_RGB)
		GL_STR(GL_OPERAND1_RGB)
		GL_STR(GL_OPERAND2_RGB)
		GL_STR(GL_OPERAND0_ALPHA)
		GL_STR(GL_OPERAND1_ALPHA)
		GL_STR(GL_OPERAND2_ALPHA)
		GL_STR(GL_RGB_SCALE)
		GL_STR(GL_ADD_SIGNED)
		GL_STR(GL_INTERPOLATE)
		GL_STR(GL_SUBTRACT)
		GL_STR(GL_CONSTANT)
		GL_STR(GL_PRIMARY_COLOR)
		GL_STR(GL_PREVIOUS)
		GL_STR(GL_DOT3_RGB)
		GL_STR(GL_DOT3_RGBA)
		GL_STR(GL_COMBINE4_NV)
		GL_STR(GL_SOURCE3_RGB_NV)
		GL_STR(GL_SOURCE3_ALPHA_NV)
		GL_STR(GL_OPERAND3_RGB_NV)
		GL_STR(GL_OPERAND3_ALPHA_NV)
		GL_STR(GL_ADD)
		default: return "UNKNOWN GLenum";
	}
}

#undef GL_STR

#endif
