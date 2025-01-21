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
#ifndef __MESA3D_ZCONV_H__INCLUDED__
#define __MESA3D_ZCONV_H__INCLUDED__

static void *convert_int_to_s24s8(mesa3d_ctx_t *ctx, const void *in, int bpp)
{
	DWORD dst_pitch = SurfacePitch(ctx->state.sw, 32);
	size_t size = dst_pitch * ctx->state.sh;

	DWORD *plane = MesaTempAlloc(ctx, ctx->state.sw, size);
	DWORD *dst = plane;
	
	DWORD x, y;
	
	if(plane)
	{
		switch(bpp)
		{
			case 15:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x] = (((DWORD)src[x]) << 9) & 0xFFFFFF;
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 16:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x] = ((DWORD)src[x]) << 8;
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 24:
			{
				BYTE *src = (BYTE*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 24);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x]  = *((DWORD*)&src[x*3]);
						dst[x] &= 0xFFFFFF;
					}
					src = src + src_pitch;
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 32:
			{
				DWORD *src = (DWORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 32);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x] = src[x];
						if(!ctx->depth_stencil)
						{
							dst[x] &= 0xFFFFFF;
						}
					}
					src = (DWORD*)(((BYTE*)src) + src_pitch);
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
		}
	}
	
	return plane;
}

#define FIXED_12_4_TO_FLOAT(_dw) (((float)((_dw) >> 4)) + (((float)((_dw) & 0xF)) * MESA_1OVER16))
#define FLOAT_24_TO_FLOAT(_dw) ((((_dw) & 0x3FFFFF) | 0x1FC00000))
#define COPY_DW_TO_FLOAT(_dw) (*((float*)(&_dw)))

static void *convert_float_to_s24s8(mesa3d_ctx_t *ctx, const void *in, int bpp)
{
	DWORD dst_pitch = SurfacePitch(ctx->state.sw, 32);
	size_t size = dst_pitch * ctx->state.sh;

	DWORD *plane = MesaTempAlloc(ctx, ctx->state.sw, size);
	DWORD *dst = plane;
	
	DWORD x, y;
	
	if(plane)
	{
		switch(bpp)
		{
			case 16:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						float tmp = FIXED_12_4_TO_FLOAT(src[x]);
						dst[x] = (DWORD)(tmp * 0x7FFFFF);
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 24:
			{
				BYTE *src = (BYTE*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 24);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						//DWORD dwtmp = *((DWORD*)&src[x*3]); = faster but break mem. allign
						DWORD dwtmp = ((DWORD)src[x*3]) | ((DWORD)src[x*3+1] << 8) | ((DWORD)src[x*3+2] << 16); // LE only
						DWORD ftmp_dw = FLOAT_24_TO_FLOAT(dwtmp);
						float ftmp = COPY_DW_TO_FLOAT(ftmp_dw);
						dst[x] = (DWORD)(ftmp * 0x7FFFFF);
						dst[x] &= 0xFFFFFF;						
					}
					src = src + src_pitch;
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 32:
			{
				DWORD *src = (DWORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 32);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x] = src[x];
						if(!ctx->depth_stencil)
						{
							DWORD tmp_dw = FLOAT_24_TO_FLOAT(src[x]);
							float tmp = COPY_DW_TO_FLOAT(tmp_dw);
							dst[x] = (DWORD)(tmp * 0x7FFFFF);
							dst[x] &= 0xFFFFFF;

							if(ctx->depth_stencil)
							{
								dst[x] |= src[x] << 24;
							}
						}
					}
					src = (DWORD*)(((BYTE*)src) + src_pitch);
					dst = (DWORD*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
		}
	}
	
	return plane;
}

#pragma pack(push)
#pragma pack(1)
typedef struct f32_s8
{
	float depth;
	DWORD stencil;
} f32_s8_t;
#pragma pack(pop)

static void *convert_int_to_f32s8(mesa3d_ctx_t *ctx, const void *in, int bpp)
{
	DWORD dst_pitch = SurfacePitch(ctx->state.sw, 64);
	size_t size = dst_pitch * ctx->state.sh;

	DWORD *plane = MesaTempAlloc(ctx, ctx->state.sw, size);
	f32_s8_t *dst = (f32_s8_t*)plane;
	
	DWORD x, y;
	
	if(plane)
	{
		switch(bpp)
		{
			case 15:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x].depth = (src[x] & 0x7FFF) * MESA_1OVER32767;
						dst[x].stencil = 0;
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 16:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x].depth = src[x] * MESA_1OVER65535;
						dst[x].stencil = 0;
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 24:
			{
				BYTE *src = (BYTE*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 24);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						DWORD dtmp = *((DWORD*)&src[x*3]);
						dst[x].depth = dtmp * MESA_1OVER16777215;
						dst[x].stencil = 0;
					}
					src = src + src_pitch;
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 32:
			{
				DWORD *src = (DWORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 32);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x].depth = (src[x] & 0xFFFFFF) * MESA_1OVER16777215;
						dst[x].stencil = src[x] >> 24;
					}
					src = (DWORD*)(((BYTE*)src) + src_pitch);
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
		}
	}
	
	return plane;
}

static void *convert_float_to_f32s8(mesa3d_ctx_t *ctx, const void *in, int bpp)
{
	DWORD dst_pitch = SurfacePitch(ctx->state.sw, 64);
	size_t size = dst_pitch * ctx->state.sh;

	DWORD *plane = MesaTempAlloc(ctx, ctx->state.sw, size);
	f32_s8_t *dst = (f32_s8_t*)plane;
	
	DWORD x, y;
	
	if(plane)
	{
		switch(bpp)
		{
			case 16:
			{
				WORD *src = (WORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 16);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						dst[x].depth = FIXED_12_4_TO_FLOAT(src[x]);
						dst[x].stencil = 0;
					}
					src = ( WORD*)(((BYTE*)src) + src_pitch);
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 24:
			{
				BYTE *src = (BYTE*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 24);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						DWORD tmp_dw = *((DWORD*)&src[x*3]);
						DWORD tmp_fdw = FLOAT_24_TO_FLOAT(tmp_dw);
						dst[x].depth = COPY_DW_TO_FLOAT(tmp_fdw);		
						dst[x].stencil = 0;
					}
					src = src + src_pitch;
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
			case 32:
			{
				DWORD *src = (DWORD*)in;
				DWORD src_pitch = SurfacePitch(ctx->state.sw, 32);
				for(y = 0; y < ctx->state.sh; y++)
				{
					for(x = 0; x < ctx->state.sw; x++)
					{
						DWORD tmp_fdw = FLOAT_24_TO_FLOAT(src[x]);
						dst[x].depth = COPY_DW_TO_FLOAT(tmp_fdw);	
						dst[x].stencil = src[x] >> 24;
					}
					src = (DWORD*)(((BYTE*)src) + src_pitch);
					dst = (f32_s8_t*)(((BYTE*)dst) + dst_pitch);
				}
				break;
			}
		}
	}
	
	return plane;
}

static void *convert_depth2GL(mesa3d_ctx_t *ctx, const void *in, int bpp)
{
	if(ctx->state.depth.wbuffer)
	{
		if(VMHALenv.zfloat)
		{
			return convert_float_to_f32s8(ctx, in, bpp);
		}
		else
		{
			return convert_float_to_s24s8(ctx, in, bpp);
		}
	}
	else
	{
		if(VMHALenv.zfloat)
		{
			return convert_int_to_f32s8(ctx, in, bpp);
		}
		else
		{
			return convert_int_to_s24s8(ctx, in, bpp);
		}
	}
	
	return NULL;
}

static void transform_int_to_float(mesa3d_ctx_t *ctx, void *plane, int bpp)
{
	DWORD pitch;
	DWORD x, y;
	switch(bpp)
	{
		case 16:
		{
			pitch = SurfacePitch(ctx->state.sw, 16);
			WORD *ptr = plane;
			for(y = 0; y < ctx->state.sh; y++)
			{
				for(x = 0; x < ctx->state.sw; x++)
				{
					float f = ptr[x] * MESA_1OVER65535;
					ptr[x] = f * 16;
				}
			}
			ptr = (WORD*)(((BYTE*)ptr) + pitch);
			break;
		}
		case 32:
		{
			pitch = SurfacePitch(ctx->state.sw, 32);
			DWORD *ptr = plane;
			for(y = 0; y < ctx->state.sh; y++)
			{
				float *fptr = (float*)ptr;
				for(x = 0; x < ctx->state.sw; x++)
				{
					float f = (ptr[x] & 0x00FFFFFF) * MESA_1OVER16777215;
					DWORD s =  ptr[x] & 0xFF000000;
					fptr[x] = f;
					ptr[x] &= 0x003FFFFF;
					ptr[x] |= s;
				}
			}
			ptr = (DWORD*)(((BYTE*)ptr) + pitch);
			break;
		}
	}
}

static void copy_int_to(mesa3d_ctx_t *ctx, const void *in, void *out, int bpp, BOOL f24)
{
	DWORD x, y;
	switch(bpp)
	{
		case 24:
		{
			DWORD src_pitch = SurfacePitch(ctx->state.sw, 32);
			DWORD dst_pitch = SurfacePitch(ctx->state.sw, 24);
			BYTE *dst = out;
			const DWORD *src = in;
			for(y = 0; y < ctx->state.sh; y++)
			{
				for(x = 0; x < ctx->state.sw; x++)
				{
					DWORD d;
					if(f24)
					{
						float f = (src[x] & 0x00FFFFFF) * MESA_1OVER16777215;
						d = *((DWORD*)&f);
						d &= 0x003FFFFF;
					}
					else
					{
						d = src[x];
					}

					dst[x*3 + 0] = d & 0xFF;
					dst[x*3 + 1] = (d >> 8) & 0xFF;
					dst[x*3 + 2] = (d >> 16) & 0xFF;
				}
			}
			src = (const DWORD*)(((const BYTE*)src) + src_pitch);
			dst += dst_pitch;
			break;
		}
	}
}

#endif /* __MESA3D_ZCONV_H__INCLUDED__ */
