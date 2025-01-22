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
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"

#define RESET_COLOR 1
#define RESET_DEPTH 2
#define RESET_SWAP  4

static void FBOConvReset(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD flags)
{
	if(flags & RESET_SWAP)
		return;

	if(flags & RESET_COLOR)
	{
		if(ctx->fbo.color_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", ctx->fbo.color_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &ctx->fbo.color_fb));
			ctx->fbo.color_fb = 0;
		}

		if(ctx->fbo.color_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo.color_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo.color_tex));
			ctx->fbo.color_tex = 0;
		}
		
		ctx->fbo.color_format = 0;
	}
	
	if(flags & RESET_DEPTH)
	{
		if(ctx->fbo.depth_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", ctx->fbo.depth_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &ctx->fbo.depth_fb));
			ctx->fbo.depth_fb = 0;
		}

		if(ctx->fbo.stencil_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", ctx->fbo.stencil_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &ctx->fbo.stencil_fb));
			ctx->fbo.stencil_fb = 0;
		}

		if(ctx->fbo.depth_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo.depth_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo.depth_tex));
			ctx->fbo.depth_tex = 0;
		}
		
		if(ctx->fbo.stencil_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo.stencil_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo.stencil_tex));
			ctx->fbo.stencil_tex = 0;
		}
		
		ctx->fbo.depth_type = 0;
	}
}

void MesaBufferUploadColor(mesa3d_ctx_t *ctx, const void *src)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	GLenum type;
	GLint format;
	
	switch(ctx->front_bpp)
	{
		case 16:
			type = GL_UNSIGNED_SHORT_5_6_5;
			format = GL_RGB;
			break;
		case 24:
			type = GL_UNSIGNED_BYTE;
			format = GL_BGR;
			break;
		case 32:
		default:
			type = GL_UNSIGNED_BYTE;
			format = GL_BGRA;
			break;
	}

	GL_CHECK(entry->proc.pglFinish());

	if(ctx->front_bpp == 32)
	{
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.plane_color_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ctx->state.sw, ctx->state.sh, 0, format, type, src));
	}
	else
	{
		if(ctx->fbo.color_format != format)
		{
			TOPIC("FRAMEBUFFER", "different color - %d vs %d", ctx->fbo.color_format, format);
			FBOConvReset(entry, ctx, RESET_COLOR);
			
			GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo.color_tex));
			TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo.color_tex);
			if(ctx->fbo.color_fb == 0)
			{
				GL_CHECK(entry->proc.pglGenFramebuffers(1, &ctx->fbo.color_fb));
				TOPIC("FRAMEBUFFER", "new frambuffer: %d", ctx->fbo.color_fb);
			}
			
			ctx->fbo.color_format = format;
		}
		
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.color_fb));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.color_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx->state.sw, ctx->state.sh, 0, format, type, src));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbo.color_tex, 0));
		
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo.color_fb));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->fbo.plane_fb));
		GL_CHECK(entry->proc.pglBlitFramebuffer(
			0, 0, ctx->state.sw, ctx->state.sh,
			0, 0, ctx->state.sw, ctx->state.sh,
	  	GL_COLOR_BUFFER_BIT, GL_NEAREST));
		
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.plane_fb));
	}
	
	if(ctx->fbo_tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo_tmu].update = TRUE;
	}
	
	TOPIC("READBACK", "%X -> upload color!", src);
}

void MesaBufferDownloadColor(mesa3d_ctx_t *ctx, void *dst)
{
	TRACE_ENTRY

	GLenum type;
	GLenum format;

	BOOL front_surface = IsInFront(GetHAL(ctx->dd), dst);

	switch(ctx->front_bpp)
	{
		case 15:
			type = GL_UNSIGNED_SHORT_5_5_5_1;
			format = GL_RGBA;
			break;
		case 16:
			type = GL_UNSIGNED_SHORT_5_6_5;
			format = GL_RGB;
			break;
		case 24:
			type = GL_UNSIGNED_BYTE;
			format = GL_BGR;
			break;
		case 32:
		default:
			type = GL_UNSIGNED_BYTE;
			format = GL_BGRA;
			break;
	}

	if(front_surface)
		FBHDA_access_begin(0);
	
	mesa3d_entry_t *entry = entry = ctx->entry;

	GL_CHECK(entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, dst));
	
	if(front_surface)
		FBHDA_access_end(0);
		
	TOPIC("READBACK", "%X <- download color!", dst);
}

#include "mesa3d_zconv.h"

#define DS_NATIVE 0
#define DS_CONVERT_GPU 1
#define DS_CONVERT_CPU 2

void MesaBufferUploadDepth(mesa3d_ctx_t *ctx, const void *src)
{
	TRACE_ENTRY
	
	mesa3d_entry_t *entry = ctx->entry;
	GLenum type;
	GLenum format;
	int convert_type = DS_CONVERT_CPU;

	switch(ctx->depth_bpp)
	{
		case 15:
			type   = GL_UNSIGNED_SHORT;
			format = GL_DEPTH_COMPONENT;
			break;
		case 16:
			type   = GL_UNSIGNED_SHORT;
			format = GL_DEPTH_COMPONENT;
			if(!ctx->state.depth.wbuffer)
			{
				convert_type = DS_CONVERT_GPU;
			}
			break;
		case 24:
			type = GL_UNSIGNED_INT_24_8;
			format = GL_DEPTH_STENCIL;
			break;
		case 32:
			type = GL_UNSIGNED_INT_24_8;
			format = GL_DEPTH_STENCIL;
			if(!ctx->state.depth.wbuffer)
			{
				if(VMHALenv.zfloat)
					convert_type = DS_CONVERT_GPU;
				else
					convert_type = DS_NATIVE;
			}
			break;
		default:
			ERR("Unknown depth buffer depth: %d", ctx->depth_bpp);
			return;
			break;
	}
	
	TRACE("depth_bpp=%d, type=0x%X, format=0x%X, ?stencil = %d", ctx->depth_bpp, type, format, ctx->depth_stencil);
	GL_CHECK(entry->proc.pglFinish());
	
	if(convert_type == DS_NATIVE) /* DX depth buffer is in GL native format */
	{
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.plane_depth_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, ctx->state.sw, ctx->state.sh, 0, format, type, src));
	}
	else if(convert_type == DS_CONVERT_CPU) /* DX depth buffer can not be load to GL directly */
	{
		void *native_src = convert_depth2GL(ctx, src, ctx->depth_bpp);
		if(native_src)
		{
			GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.plane_depth_tex));
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0,
				VMHALenv.zfloat ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8,
				ctx->state.sw, ctx->state.sh, 0, GL_DEPTH_STENCIL,
				VMHALenv.zfloat ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_UNSIGNED_INT_24_8,
				native_src));
			
			MesaTempFree(ctx, native_src);
		}
	}
	else /* = DS_CONVERT_GPU, DX depth buffer is in GL compatible format */
	{
		if(ctx->fbo.depth_type != type)
		{
			TOPIC("FRAMEBUFFER", "different fb type %d vs %d, FBOConvReset()", ctx->fbo.depth_type, type);
			FBOConvReset(entry, ctx, RESET_DEPTH);

			GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo.depth_tex));
			TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo.depth_tex);

			if(ctx->fbo.depth_fb == 0)
			{
				GL_CHECK(entry->proc.pglGenFramebuffers(1, &ctx->fbo.depth_fb));
				TOPIC("FRAMEBUFFER", "new frambuffer: %d",  ctx->fbo.depth_fb);
			}

			GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo.stencil_tex));
			TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo.stencil_tex);

			if(ctx->fbo.stencil_fb == 0)
			{
				GL_CHECK(entry->proc.pglGenFramebuffers(1, &ctx->fbo.stencil_fb));
				TOPIC("FRAMEBUFFER", "new frambuffer: %d",  ctx->fbo.stencil_fb);
			}

			ctx->fbo.depth_type = type;
		}
		
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
	
		// GL_FLOAT_32_UNSIGNED_INT_24_8_REV
		// GL_DEPTH32F_STENCIL8
	
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.depth_fb));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.depth_tex));
		
		TRACE("w: %d h: %d; format=%X type=%X src=%X", ctx->state.sw, ctx->state.sh, format, type, src);
		
		if(ctx->depth_bpp != 32)
		{
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, VMHALenv.zfloat ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT24, ctx->state.sw, ctx->state.sh, 0, format, type, src));
			GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ctx->fbo.depth_tex, 0));
		}
		else
		{
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, VMHALenv.zfloat ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8, ctx->state.sw, ctx->state.sh, 0, format, type, src));
			GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ctx->fbo.depth_tex, 0));
		}
	
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo.depth_fb));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->fbo.plane_fb));
	
	#ifdef TRACE_ON
		GLenum test = entry->proc.pglCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
		TOPIC("GL", "(depth) glCheckFramebufferStatus = 0x%X", test);
	#endif
		
		GL_CHECK(entry->proc.pglBlitFramebuffer(
			0, 0, ctx->state.sw, ctx->state.sh,
			0, 0, ctx->state.sw, ctx->state.sh,
	  	GL_DEPTH_BUFFER_BIT, GL_NEAREST));

		// default buffer binding
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.plane_fb));
	}
		
	if(ctx->fbo_tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo_tmu].update = TRUE;
	}
	
	TOPIC("READBACK", "%X -> upload depth!", src);
}

void MesaBufferDownloadDepth(mesa3d_ctx_t *ctx, void *dst)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	GLenum type;
	GLenum format = GL_DEPTH_COMPONENT;
	
	switch(ctx->depth_bpp)
	{
		case 15:
		case 16:
			type = GL_UNSIGNED_SHORT;
			break;
		case 24:
			 /* we silently increase ZBUFF 24 -> 32 bpp */
			type = GL_UNSIGNED_INT_24_8;
			format = GL_DEPTH_STENCIL;
			break;
		case 32:
			type = GL_UNSIGNED_INT_24_8;
			format = GL_DEPTH_STENCIL;
			break;
		default:
			ERR("Unknown depth depth %d", ctx->depth_bpp);
			return;
	}

	if(ctx->depth_bpp != 24)
	{
		GL_CHECK(entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, dst));
		if(ctx->state.depth.wbuffer)
		{
			transform_int_to_float(ctx, dst, ctx->depth_bpp);
		}
	}
	else
	{
		size_t s = SurfacePitch(ctx->state.sw, 32) * ctx->state.sh;
		void *buf = MesaTempAlloc(ctx, ctx->state.sw, s);
		if(buf)
		{
			GL_CHECK(entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, buf));
			copy_int_to(ctx, buf, dst, ctx->depth_bpp, ctx->state.depth.wbuffer);
			MesaTempFree(ctx, buf);
		}
	}
	
	TOPIC("READBACK", "%X -> download depth!", dst);
}

static DWORD compressed_size(GLenum internal_format, GLuint w, GLuint h)
{
	DWORD s = 0;
	
	switch(internal_format)
	{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			s = ((w + 3) >> 2) * ((h + 3) >> 2) * 8;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			s = ((w + 3) >> 2) * ((h + 3) >> 2) * 16;
			break;
	} // switch
	
	return s;
}

static const int Mesa2GLSide[MESA3D_CUBE_SIDES] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

#include "mesa3d_flip.h"

void MesaBufferUploadTexture(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level, int side, int tmu)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	GLuint w = tex->width;
	GLuint h = tex->height;

	w >>= level;
	h >>= level;

	if(w == 0) w = 1;
	if(h == 0) h = 1;

	TOPIC("CHROMA", "MesaBufferUploadTexture - level=%d", level);

	TOPIC("GL", "MesaBufferUploadTexture: is_cube=%d level=%d, side=%d internalformat=%X, w=%d, h=%d, format=%X, type=%X, sid=%X)",
		tex->cube, level, side, tex->internalformat, w, h, tex->format, tex->type, tex->data_sid[side][level]);

	surface_id sid = tex->data_sid[side][level];

	if(sid == 0)
	{
		ERR("Zero sid on side %d, level %d", side, level);
		return;
	}

	DDSURF *surf = SurfaceGetSURF(sid);
	if(surf == NULL)
	{
		ERR("NULL on side %d, level %d", side, level);
		return;
	}

	void *ptr = (void*)surf->fpVidMem;
	if(ptr == NULL)
	{
		ERR("NULL vram on side %d, level %d", side, level);
		return;
	}

	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));

	if(tex->cube)
	{
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, 0));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, tex->gltex));

		void *flipped = NULL;
		void *dst = ptr;

		switch(side)
		{
			case MESA_POSITIVEX:
				dst = flipped = flip_xy(ctx, ptr, w, h, tex->bpp);
				break;
			case MESA_NEGATIVEX:
				dst = flipped = flip_y(ctx, ptr, w, h, tex->bpp);
				break;
			case MESA_POSITIVEY:
				// nop
				break;
			case MESA_NEGATIVEY:
				dst = flipped = flip_y(ctx, ptr, w, h, tex->bpp);
				break;
			case MESA_POSITIVEZ:
				dst = flipped = flip_y(ctx, ptr, w, h, tex->bpp);
				break;
			case MESA_NEGATIVEZ:
				dst = flipped = flip_xy(ctx, ptr, w, h, tex->bpp);
				break;
		}

		if(dst)
		{
			TOPIC("CUBE", "glTexImage2D(0x%X, %d, ...)", Mesa2GLSide[side], level);
			if(!tex->compressed)
			{
				GL_CHECK(entry->proc.pglTexImage2D(Mesa2GLSide[side], level, tex->internalformat,
					w, h, 0, tex->format, tex->type, dst));
			}
			else
			{
				GL_CHECK(entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, 1));
				GL_CHECK(entry->proc.pglCompressedTexImage2D(Mesa2GLSide[side], level, tex->internalformat,
					w, h, 0, compressed_size(tex->internalformat, w, h), dst));
				GL_CHECK(entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN));
			}

			if(flipped)
			{
				MesaTempFree(ctx, flipped);
			}
		}
	}
	else
	{
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, tex->gltex));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, 0));

		TOPIC("CUBE", "glTexImage2D(GL_TEXTURE_2D, %d, ...)", level);
		if(!tex->compressed)
		{
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, level, tex->internalformat,
				w, h, 0, tex->format, tex->type, ptr));
		}
		else
		{
			GL_CHECK(entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, 1));
			GL_CHECK(entry->proc.pglCompressedTexImage2D(GL_TEXTURE_2D, level, tex->internalformat,
				w, h, 0, compressed_size(tex->internalformat, w, h), ptr));
			GL_CHECK(entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN));
		}
	}

	ctx->state.tmu[tmu].update = TRUE;
}

static void *chroma_convert(mesa3d_ctx_t *ctx,
	int width, int height, int bpp, GLenum type, void *ptr,
	DWORD chroma_lw, DWORD chroma_hi)
{
	void *data = NULL;
	
	TOPIC("CHROMA", "chroma - bpp: %d, chroma_lw=0x%X, chroma_hi=0x%X", bpp, chroma_lw, chroma_hi);
	
	switch(bpp)
	{
		case 12:
			data = MesaChroma12(ctx, ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 15:
			data = MesaChroma15(ctx, ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 16:
			if(type == GL_UNSIGNED_SHORT_4_4_4_4_REV || type == GL_UNSIGNED_SHORT_4_4_4_4)
			{
				data = MesaChroma12(ctx, ptr, width, height, chroma_lw, chroma_hi);
			}
			else if(type == GL_UNSIGNED_SHORT_5_5_5_1 || type == GL_UNSIGNED_SHORT_1_5_5_5_REV)
			{
				data = MesaChroma15(ctx, ptr, width, height, chroma_lw, chroma_hi);
			}
			else
			{
				data = MesaChroma16(ctx, ptr, width, height, chroma_lw, chroma_hi);
			}
			break;
		case 24:
			data = MesaChroma24(ctx, ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 32:
			data = MesaChroma32(ctx, ptr, width, height, chroma_lw, chroma_hi);
			break;
		default:
			TOPIC("FORMAT", "wrong bpp: %d", bpp);
			break;
	}
	
	return data;
}

void MesaBufferUploadTextureChroma(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level, int side, int tmu, DWORD chroma_lw, DWORD chroma_hi)
{
	TRACE_ENTRY
	
	GLuint w = tex->width;
	GLuint h = tex->height;
	
	w >>= level;
	h >>= level;
	
	if(w == 0) w = 1;
	if(h == 0) h = 1;

	mesa3d_entry_t *entry = ctx->entry;

	surface_id sid = tex->data_sid[side][level];
	if(sid == 0)
	{
		ERR("sid == 0");
		return;
	}

	void *vidmem = SurfaceGetVidMem(sid);
	if(vidmem == NULL)
	{
		ERR("vidmem == NULL");
		return;
	}

	// TODO: cube map!

	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
	GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, tex->gltex));

	TOPIC("CHROMA", "MesaBufferUploadTextureChroma - level=%d", level);

	void *data = chroma_convert(ctx, w, h, tex->bpp, tex->type, vidmem, chroma_lw, chroma_hi);
	TOPIC("TEX", "downloaded chroma bpp: %d, success: %X", tex->bpp, data);
	
	if(data)
	{
		if(tex->cube)
		{
			// ...
		}
		else
		{
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, level, GL_RGBA,
				w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data));
		}
		
		MesaChromaFree(ctx, data);
	}
	ctx->state.tmu[tmu].update = TRUE;
}

BOOL MesaBufferFBOSetup(mesa3d_ctx_t *ctx, int width, int height)
{
	mesa3d_entry_t *entry = ctx->entry;
	
	if(ctx->fbo.width == width && ctx->fbo.height == height)
	{
		// nop
	}
	else if(ctx->fbo_swap.width == width && ctx->fbo_swap.height == height)
	{
		mesa_fbo_t tmp;
		memcpy(&tmp,           &ctx->fbo,      sizeof(mesa_fbo_t));
		memcpy(&ctx->fbo,      &ctx->fbo_swap, sizeof(mesa_fbo_t));
		memcpy(&ctx->fbo_swap, &tmp,           sizeof(mesa_fbo_t));

		FBOConvReset(entry, ctx, RESET_SWAP);
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.plane_fb));
		GL_CHECK(entry->proc.pglViewport(0, 0, width, height));
		
		TOPIC("FBSWAP", "fbo <-> fbo_swap");
	}
	else
	{
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, 0));

		if(ctx->fbo_swap.plane_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", ctx->fbo_swap.plane_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &ctx->fbo_swap.plane_fb));
			TOPIC("FBSWAP", "delete fbo_swap");
		}

		if(ctx->fbo_swap.plane_color_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo_swap.plane_color_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo_swap.plane_color_tex));
		}

		if(ctx->fbo_swap.plane_depth_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo_swap.plane_depth_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo_swap.plane_depth_tex));
		}

		memcpy(&ctx->fbo_swap, &ctx->fbo, sizeof(mesa_fbo_t));

		GL_CHECK(entry->proc.pglGenFramebuffers(1, &ctx->fbo.plane_fb));
		TOPIC("FRAMEBUFFER", "new frambuffer: %d", ctx->fbo.plane_fb);
		GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo.plane_color_tex));
		TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo.plane_color_tex);
		GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo.plane_depth_tex));
		TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo.plane_depth_tex);

		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0 + ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.plane_fb));
	
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.plane_color_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));

		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbo.plane_color_tex, 0));
		
		GLenum depth_format = GL_DEPTH24_STENCIL8;
		if(VMHALenv.zfloat)
		{
			depth_format = GL_DEPTH32F_STENCIL8;
		}

		GL_CHECK(entry->proc.pglBindRenderbuffer(GL_RENDERBUFFER, ctx->fbo.plane_depth_tex));
		GL_CHECK(entry->proc.pglRenderbufferStorage(GL_RENDERBUFFER, depth_format, width, height));
		GL_CHECK(entry->proc.pglFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ctx->fbo.plane_depth_tex));

		ctx->fbo.width = width;
		ctx->fbo.height = height;

		FBOConvReset(entry, ctx, RESET_COLOR|RESET_DEPTH);
		
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.plane_fb));

		GL_CHECK(entry->proc.pglViewport(0, 0, width, height));
		
		TOPIC("FBSWAP", "fbo -> fbo_swap");
		TOPIC("FBSWAP", "new fbo");
	}
	
	return TRUE;
}
