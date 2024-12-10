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

void MesaBufferUploadColor(mesa3d_ctx_t *ctx, const void *src)
{
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
	
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo.tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.color_fb));
	GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.color_tex));
	GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx->state.sw, ctx->state.sh, 0, format, type, src));
	GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbo.color_tex, 0));
	
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo.color_fb));
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
	GL_CHECK(entry->proc.pglBlitFramebuffer(
		0, 0, ctx->state.sw, ctx->state.sh,
		0, 0, ctx->state.sw, ctx->state.sh,
  	GL_COLOR_BUFFER_BIT, GL_NEAREST));
	
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, 0));
	//ctx->state.last_tex = ctx->fbo.color_tex;
	
	if(ctx->fbo.tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo.tmu].update = TRUE;
	}
	
	TOPIC("READBACK", "upload color!");
}

void MesaBufferDownloadColor(mesa3d_ctx_t *ctx, void *dst)
{
//	size_t s = ctx->state.sw * ctx->state.sh * ((ctx->front_bpp + 7)/8);
//	memcpy(dst, ctx->osbuf, s);
	//size_t s = ctx->state.sw * ctx->state.sh * ((ctx->front_bpp + 7)/8);
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

	ctx->entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, dst);
	
	if(front_surface)
		FBHDA_access_end(0);
		
	TOPIC("READBACK", "download color!");
}

void MesaBufferUploadDepth(mesa3d_ctx_t *ctx, const void *src)
{
	mesa3d_entry_t *entry = ctx->entry;
	GLenum type;
	GLenum format;

	switch(ctx->depth_bpp)
	{
		case 16:
			type   = GL_UNSIGNED_SHORT;
			format = GL_DEPTH_COMPONENT;
			break;
		case 24:
			type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			format = GL_DEPTH_STENCIL;
			break;
		case 32:
		default:
			if(ctx->depth_stencil)
			{
				type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
				format = GL_DEPTH_STENCIL;
			}
			else
			{
				type = GL_FLOAT;
				format = GL_DEPTH_COMPONENT;
			}
			break;
	}
	
	TRACE("depth_bpp=%d, type=0x%X, format=0x%X, ?stencil = %d", ctx->depth_bpp, type, format, ctx->depth_stencil);
	
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo.tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));

	GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.depth_fb));
	GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.depth_tex));
	GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, ctx->state.sw, ctx->state.sh, 0, format, type, src));
	GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ctx->fbo.depth_tex, 0));

	GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo.depth_fb));
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));

#ifdef TRACE_ON
	GLenum test = entry->proc.pglCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
	TOPIC("GL", "(depth) glCheckFramebufferStatus = 0x%X", test);
#endif
	
	GL_CHECK(entry->proc.pglBlitFramebuffer(
		0, 0, ctx->state.sw, ctx->state.sh,
		0, 0, ctx->state.sw, ctx->state.sh,
  	GL_DEPTH_BUFFER_BIT, GL_NEAREST));
	
	if(ctx->depth_stencil)
	{
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo.stencil_fb));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo.stencil_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX, ctx->state.sw, ctx->state.sh, 0, format, type, src));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ctx->fbo.stencil_tex, 0));
	
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo.stencil_fb));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
		
#ifdef TRACE_ON
		GLenum test = entry->proc.pglCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
		TOPIC("GL", "(stencil) glCheckFramebufferStatus = 0x%X", test);
#endif
		
		GL_CHECK(entry->proc.pglBlitFramebuffer(
			0, 0, ctx->state.sw, ctx->state.sh,
			0, 0, ctx->state.sw, ctx->state.sh, GL_STENCIL_BUFFER_BIT, GL_NEAREST));
	}
	
	// default buffer binding
	GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, 0));
	
	if(ctx->fbo.tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo.tmu].update = TRUE;
	}
	
	TOPIC("READBACK", "upload depth!");
}

void MesaBufferDownloadDepth(mesa3d_ctx_t *ctx, void *dst)
{
	//size_t s = ctx->state.sw * ctx->state.sh * ((ctx->front_bpp + 7)/8);
	GLenum type;
	GLenum format = GL_DEPTH_COMPONENT;
	
	switch(ctx->depth_bpp)
	{
		case 16:
			type = GL_UNSIGNED_SHORT;
			break;
		case 24:
			 /* we silently increase ZBUFF 24 -> 32 bpp */
			type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			break;
		case 32:
		default:
			if(ctx->depth_stencil)
			{
				type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
				format = GL_DEPTH_STENCIL;
			}
			else
			{
				type = GL_FLOAT;
			}
			break;
	}

	ctx->entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, dst);
	
	TOPIC("READBACK", "download depth!");
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

void MesaBufferUploadTexture(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level, int tmu)
{
	mesa3d_entry_t *entry = ctx->entry;
	GLuint w = tex->width;
	GLuint h = tex->height;
	
	w /= (1 << level);
	h /= (1 << level);

	TOPIC("GL", "glTexImage2D(GL_TEXTURE_2D, %d, %X, %d, %d, 0, %X, %X, %X)",
		level, tex->internalformat, w, h, tex->format, tex->type, tex->data_ptr[level]
	);
	
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
	GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, tex->gltex));
	
	if(tex->compressed)
	{
		entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		GL_CHECK(entry->proc.pglCompressedTexImage2D(GL_TEXTURE_2D, level, tex->internalformat,
			w, h, 0, compressed_size(tex->internalformat, w, h), (void*)tex->data_ptr[level]));
		
		entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN);
	}
	else
	{
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, level, tex->internalformat,
			w, h, 0, tex->format, tex->type, (void*)tex->data_ptr[level]));
	}
	
	ctx->state.tmu[tmu].update = TRUE;
}

static void *chroma_convert(
	int width, int height, int bpp, GLenum type, void *ptr,
	DWORD chroma_lw, DWORD chroma_hi)
{
	void *data = NULL;
	
	switch(bpp)
	{
		case 12:
			data = MesaChroma12(ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 15:
			data = MesaChroma15(ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 16:
			if(type == GL_UNSIGNED_SHORT_4_4_4_4_REV || type == GL_UNSIGNED_SHORT_4_4_4_4)
			{
				data = MesaChroma12(ptr, width, height, chroma_lw, chroma_hi);
			}
			else if(type == GL_UNSIGNED_SHORT_5_5_5_1 || type == GL_UNSIGNED_SHORT_1_5_5_5_REV)
			{
				data = MesaChroma15(ptr, width, height, chroma_lw, chroma_hi);
			}
			else
			{
				data = MesaChroma16(ptr, width, height, chroma_lw, chroma_hi);
			}
			break;
		case 24:
			data = MesaChroma24(ptr, width, height, chroma_lw, chroma_hi);
			break;
		case 32:
			data = MesaChroma32(ptr, width, height, chroma_lw, chroma_hi);
			break;
		default:
			TOPIC("FORMAT", "wrong bpp: %d", bpp);
			break;
	}
	
	return data;
}

void MesaBufferUploadTextureChroma(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level, int tmu, DWORD chroma_lw, DWORD chroma_hi)
{
	TRACE_ENTRY
	
	GLuint w = tex->width;
	GLuint h = tex->height;
	
	w /= (1 << level);
	h /= (1 << level);
	
	mesa3d_entry_t *entry = ctx->entry;

	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
	GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
	GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, tex->gltex));

	TRACE("FORMAT", "Chroma key (0x%08X 0x%08X)", chroma_lw, chroma_hi);

	void *data = chroma_convert(w, h, tex->bpp, tex->type, (void*)tex->data_ptr[level], chroma_lw, chroma_hi);
	TOPIC("TEX", "downloaded chroma bpp: %d, success: %X", tex->bpp, data);
	
	if(data)
	{
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data));
		
		MesaChromaFree(data);
	}
	ctx->state.tmu[tmu].update = TRUE;
}
