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
#ifndef NUKED_SKIP
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include "ddrawi_ddk.h"
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"
#endif

#define RESET_COLOR 1
//#define RESET_DEPTH 2
//#define RESET_SWAP  4

static void FBOConvReset(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, DWORD flags)
{
	if(flags & RESET_COLOR)
	{
		if(ctx->fbo->color16_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", ctx->fbo->color16_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &ctx->fbo->color16_fb));
			ctx->fbo->color16_fb = 0;
		}

		if(ctx->fbo->color16_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", ctx->fbo->color16_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &ctx->fbo->color16_tex));
			ctx->fbo->color16_tex = 0;
		}
		
		ctx->fbo->color16_format = 0;
	}
}

NUKED_LOCAL void MesaBufferUploadColor(mesa3d_ctx_t *ctx, const void *src)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	GLenum type;
	GLint format;
	BOOL create = FALSE;
	
	switch(ctx->front_bpp)
	{
		case 8:
			return; /* nope, ignore it */
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

		GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
		GL_CHECK(entry->proc.pglPushMatrix());
		GL_CHECK(entry->proc.pglLoadIdentity());

		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo->plane_color_tex));
		//GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ctx->state.sw, ctx->state.sh, 0, format, type, src));
		GL_CHECK(entry->proc.pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ctx->state.sw, ctx->state.sh, format, type, src));

		GL_CHECK(entry->proc.pglPopMatrix());
		
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
	}
	else
	{
		if(ctx->fbo->color16_format != format)
		{
			TOPIC("FRAMEBUFFER", "different color - %d vs %d", ctx->fbo->color16_format, format);
			FBOConvReset(entry, ctx, RESET_COLOR);
			
			GL_CHECK(entry->proc.pglGenTextures(1, &ctx->fbo->color16_tex));
			TOPIC("FRAMEBUFFER", "new texture: %d", ctx->fbo->color16_tex);
			
			GL_CHECK(entry->proc.pglGenFramebuffers(1, &ctx->fbo->color16_fb));
			TOPIC("FRAMEBUFFER", "new frambuffer: %d", ctx->fbo->color16_fb);
			
			ctx->fbo->color16_format = format;
			create = TRUE;
		}
		
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));

		GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
		GL_CHECK(entry->proc.pglPushMatrix());
		GL_CHECK(entry->proc.pglLoadIdentity());

		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo->color16_fb));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo->color16_tex));
		//GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx->state.sw, ctx->state.sh, 0, format, type, src));
		
		if(create)
		{
			GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx->fbo->width, ctx->fbo->height, 0, format, type, NULL));
		}
		
		GL_CHECK(entry->proc.pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ctx->state.sw, ctx->state.sh, format, type, src));
		
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->fbo->color16_tex, 0));
		
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->fbo->color16_fb));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->fbo->plane_fb));
		GL_CHECK(entry->proc.pglBlitFramebuffer(
			0, 0, ctx->state.sw, ctx->state.sh,
			0, 0, ctx->state.sw, ctx->state.sh,
	  	GL_COLOR_BUFFER_BIT, GL_NEAREST));
		
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, ctx->fbo->plane_fb));
		
		GL_CHECK(entry->proc.pglPopMatrix());
		
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
	}
	
	if(ctx->fbo_tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo_tmu].update = TRUE;
		MesaDrawRefreshState(ctx);
	}
	
	TOPIC("READBACK", "%X -> upload color!", src);
}

NUKED_LOCAL void MesaBufferDownloadColor(mesa3d_ctx_t *ctx, void *dst)
{
	TRACE_ENTRY

	GLenum type;
	GLenum format;
	mesa3d_entry_t *entry = entry = ctx->entry;

	BOOL front_surface = ((BYTE*)entry->hda->vram_pm32 + entry->hda->surface) == (BYTE*)dst;

	switch(ctx->front_bpp)
	{
		case 8:
			type = GL_UNSIGNED_BYTE_3_3_2;
			format = GL_RGB;
			break;
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

	GL_CHECK(entry->proc.pglReadPixels(0, 0, ctx->state.sw, ctx->state.sh, format, type, dst));
	
	if(front_surface)
		FBHDA_access_end(0);
	else
		FBHDA_DD_surface_modify(dst);

	TOPIC("READBACK", "%X <- download color (%d x %d)!", dst, ctx->state.sw, ctx->state.sh);
}

#include "mesa3d_zconv.h"

#define DS_NATIVE 0
//#define DS_CONVERT_GPU 1
#define DS_CONVERT_CPU 2

NUKED_LOCAL void MesaBufferUploadDepth(mesa3d_ctx_t *ctx, const void *src)
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
				if(!entry->env.zfloat)
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
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo->plane_depth_tex));
		GL_CHECK(entry->proc.pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ctx->state.sw, ctx->state.sh, format, type, src));
	}
	else if(convert_type == DS_CONVERT_CPU) /* DX depth buffer can not be load to GL directly */
	{
		void *native_src = convert_depth2GL(ctx, src, ctx->depth_bpp);
		if(native_src)
		{
			TOPIC("DEPTHCONV", "convert_depth2GL - %d %d", ctx->state.sw, ctx->state.sh);
			GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+ctx->fbo_tmu));
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ctx->fbo->plane_depth_tex));
			GL_CHECK(entry->proc.pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
				ctx->state.sw, ctx->state.sh, GL_DEPTH_STENCIL,
				entry->env.zfloat ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_UNSIGNED_INT_24_8,
				native_src));
			
			MesaTempFree(ctx, native_src);
		}
	}
		
	if(ctx->fbo_tmu < ctx->tmu_count)
	{
		ctx->state.tmu[ctx->fbo_tmu].update = TRUE;
		MesaDrawRefreshState(ctx);
	}
	
	TOPIC("READBACK", "%X -> upload depth!", src);
}

NUKED_LOCAL void MesaBufferDownloadDepth(mesa3d_ctx_t *ctx, void *dst)
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

	TOPIC("DEPTHCONV", "Z GL->DX, bpp=%d", ctx->depth_bpp);
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

#if 0
static DWORD GLType2bpp(GLenum format, GLenum type)
{
	switch(type)
	{
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		case GL_UNSIGNED_SHORT_5_6_5:
			return 16;
		case GL_UNSIGNED_INT_8_8_8_8_REV:
			return 32;
		case GL_ALPHA:
			return 8;
		case GL_UNSIGNED_BYTE:
			switch(format)
			{
				case GL_ALPHA:
				case GL_LUMINANCE:
					return 8;
				case GL_LUMINANCE8_ALPHA8:
					return 16;
				case GL_RGBA:
				case GL_BGRA:
					return 32;
				case GL_RGB:
				case GL_BGR:
					return 24;
			}
			break;
	}
	
	WARN("Uknown format size format=0x%X type=0x%X",
		format, type);

	return 32;
}
#endif


static mesa_fbo_t *fbo_find_empty(mesa3d_ctx_t *ctx)
{
	unsigned int i;
	for(i = 0; i < FBO_COUNT; i++)
	{
		if(!ctx->fbo_swap[i].allocated)
		{
			ctx->fbo_swap[i].allocated = TRUE;
			TOPIC("FBSWAP", "new empty FBO - %d", i);
			return &ctx->fbo_swap[i];
		}
	}

	mesa_fbo_t *fbo_min = &ctx->fbo_swap[i];
	for(i = 1; i < FBO_COUNT; i++)
	{
		if(ctx->fbo_swap[i].width < fbo_min->width)
		{
			fbo_min = &ctx->fbo_swap[i];
		}
	}

	TOPIC("FBSWAP", "reusing FBO - %d", (fbo_min - &ctx->fbo_swap[0]));

	return fbo_min;
}

static mesa_fbo_t *fbo_find_match(mesa3d_ctx_t *ctx, int width, int height)
{
	unsigned int i;
	for(i = 0; i < FBO_COUNT; i++)
	{
		if(ctx->fbo_swap[i].allocated)
		{
			if(ctx->fbo_swap[i].width == width && ctx->fbo_swap[i].height == height)
			{
				TOPIC("FBSWAP", "match exists FBO - %d", i);
				return &ctx->fbo_swap[i];
			}
		}
	}
	return NULL;
}

NUKED_LOCAL BOOL MesaBufferFBOSetup(mesa3d_ctx_t *ctx, int width, int height, int bpp)
{
	mesa3d_entry_t *entry = ctx->entry;
	BOOL need_create = FALSE;
	TRACE("MesaBufferFBOSetup(ctx, %d, %d)", width, height);

	TRACE("active FBO (%d, %d, %d)", ctx->fbo->width, ctx->fbo->height, ctx->fbo->bpp);
	//if(ctx->fbo->width >= width && ctx->fbo->height >= height)
	if(ctx->fbo->width != width || ctx->fbo->height != height)
	{
		mesa_fbo_t *fbo = fbo_find_match(ctx, width, height);
		if(fbo != NULL)
		{
			ctx->fbo = fbo;
			GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0 + ctx->fbo_tmu));
			GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, fbo->plane_fb));

			if(fbo->bpp != bpp)
			{
				FBOConvReset(entry, ctx, RESET_COLOR);
				fbo->bpp = bpp;
			}
		}
		else
		{
			need_create = TRUE;
		}
	}
	
	if(need_create)
	{
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, 0));

		mesa_fbo_t *fbo = fbo_find_empty(ctx);

		if(fbo->plane_fb)
		{
			TOPIC("FRAMEBUFFER", "delete frambuffer: %d", fbo->plane_fb);
			GL_CHECK(entry->proc.pglDeleteFramebuffers(1, &fbo->plane_fb));
			TOPIC("FBSWAP", "delete fbo_swap");
		}

		if(fbo->plane_color_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", fbo->plane_color_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &fbo->plane_color_tex));
		}

		if(fbo->plane_depth_tex)
		{
			TOPIC("FRAMEBUFFER", "delete texture: %d", fbo->plane_depth_tex);
			GL_CHECK(entry->proc.pglDeleteTextures(1, &fbo->plane_depth_tex));
		}

		GL_CHECK(entry->proc.pglGenFramebuffers(1, &fbo->plane_fb));
		TOPIC("FRAMEBUFFER", "new frambuffer: %d", fbo->plane_fb);
		GL_CHECK(entry->proc.pglGenTextures(1, &fbo->plane_color_tex));
		TOPIC("FRAMEBUFFER", "new texture: %d", fbo->plane_color_tex);
		GL_CHECK(entry->proc.pglGenTextures(1, &fbo->plane_depth_tex));
		TOPIC("FRAMEBUFFER", "new texture: %d", fbo->plane_depth_tex);

		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0 + ctx->fbo_tmu));
		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, fbo->plane_fb));
	
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, fbo->plane_color_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));

		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->plane_color_tex, 0));

		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, fbo->plane_depth_tex));
		GL_CHECK(entry->proc.pglTexImage2D(GL_TEXTURE_2D, 0,
			entry->env.zfloat ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8,
			width, height, 0, GL_DEPTH_STENCIL,
			entry->env.zfloat ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_UNSIGNED_INT_24_8,
			NULL));

		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,	GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo->plane_depth_tex, 0));
		GL_CHECK(entry->proc.pglFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,	GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbo->plane_depth_tex, 0));

		fbo->width = width;
		fbo->height = height;
		fbo->bpp = bpp;

		ctx->fbo = fbo;

		FBOConvReset(entry, ctx, RESET_COLOR);

		GL_CHECK(entry->proc.pglBindFramebuffer(GL_FRAMEBUFFER, fbo->plane_fb));
		//GL_CHECK(entry->proc.pglViewport(0, 0, width, height));

		TOPIC("FBSWAP", "new fbo");
	}
	
	return TRUE;
}

/**
 * NOTE: call this only from MesaBufferTexturesChanges
 */
static void MesaBufferTextureDestroy(mesa3d_ctx_t *ctx, DWORD dxid)
{
	TOPIC("surfex", "Destroy texture: %d", dxid);
	surfaceex_t *se = surfex_get(ctx, dxid, FALSE, FALSE);
	if(se)
	{
		if(se->tex->gltex)
		{
			int tmu;
			for(tmu = 0; tmu < ctx->tmu_count; tmu++)
			{
				if(ctx->state.tmu[tmu].active_dxid == dxid)
				{
					MesaBufferTextureLoad(ctx, 0, tmu, FALSE);
				}
			}
			ctx->entry->proc.pglDeleteTextures(1, &(se->tex->gltex));
			se->tex->gltex = 0;
		}
	}
	ddsurf_destroy(ctx, dxid);
}

NUKED_LOCAL void MesaBufferTexturesChanges(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	FBHDA_t *hda = entry->hda;
	
	while(ctx->fifo_top != hda->dd_fifo_top)
	{
		FBHDA_DD_fifo_item_t *item = &hda->dd_fifo[ctx->fifo_top];
		DWORD tmu;
		DWORD i = 0;
		ddsurface_t *ddsurf;
		TOPIC("SURFEX", "fifo: %X %X %d", item->surface_flat, item->uid, item->action);
		while((ddsurf = ht_lookup_more(entry->ht_flat, DW_FLAT(item->surface_flat), i++)) != NULL)
		{
			TOPIC("SURFEX", "ddsurf->uid: %X", ddsurf->uid);
			if(ddsurf->uid == item->uid)
			{
				switch(item->action)
				{
					case FBHDA_DD_CREATE:
					case FBHDA_DD_MODIFY:
						ddsurf->dirty = TRUE;
						TOPIC("SURFEX", "modified: %d", ddsurf->dxid);
						if(ddsurf->dxid)
						{
							for(tmu = 0; tmu < ctx->tmu_count; tmu++)
							{
								if(ddsurf->dxid == ctx->state.tmu[i].active_dxid)
								{
									ctx->state.tmu[i].reload = TRUE;
								}
							}
						} // loop ddsurf = ddsurf->up
						break;
					case FBHDA_DD_DELETE:
						MesaBufferTextureDestroy(ctx, ddsurf->dxid);
						break;
				}
				break;
			}
		}
		
		ctx->fifo_top = (ctx->fifo_top + 1) % hda->dd_fifo_length;
	}
}

NUKED_LOCAL void MesaBufferTextureLoad(mesa3d_ctx_t *ctx, DWORD dxid, int tmu, BOOL force)
{
	TOPIC("SURFEX", "MesaBufferTextureLoad(..., %d, %d, %d)", dxid, tmu, force);
	mesa3d_entry_t *entry = ctx->entry;
	
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));	
	surfaceex_t *se = NULL;
	ddsurface_t *dd;
	if(dxid != 0)
	{
		se = surfex_get(ctx, dxid, TRUE, TRUE);
	}

	if(se == NULL)
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
		ctx->state.tmu[tmu].active_dxid = 0;
		return;
	}

	if(se->dd->level & DDSURFACE_CUBE_MASK)
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
	}

	if(se->tex->gltex == 0)
	{
		GL_CHECK(entry->proc.pglGenTextures(1, &se->tex->gltex));
		force = TRUE;
	}
	
	int max_level = 0;
	GLenum target;

	if(se->dd->level & DDSURFACE_CUBE_MASK)
	{
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D,       0));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, se->tex->gltex));
		for(dd = se->dd; dd != NULL; dd = dd->down)
		{
			if((dd->level & DDSURFACE_CUBE_MASK) == DDSURFACE_CUBE_SIDE_0)
			{
				max_level++;
			}
		}
		target = GL_TEXTURE_CUBE_MAP;
	}
	else
	{
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D,       se->tex->gltex));
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, 0));
		for(dd = se->dd; dd != NULL; dd = dd->down)
		{
			max_level++;
		}
		target = GL_TEXTURE_2D;
	}
	se->tex->mipmaps = max_level-1;
	
	GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));
	GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, max_level-1));
	
	if(max_level > 1)
	{
		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	}
	else
	{
		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	}
	
	// filter!
	TOPIC("SURFEX", "  target=%X, max_level=%d", target, max_level);
	if(se->gl->dirty || force)
	{
		dd = se->dd;
		int dd_level = 0;
		DWORD last_level = 0;
		
		while(dd)
		{
			gldata_t glinfo;
			int level;
			if((last_level & DDSURFACE_CUBE_MASK) != (dd->level & DDSURFACE_CUBE_MASK))
			{
				dd_level = 0;
			}
			
			if(dd->dirty || force)
			{
				//if(ddsurf_glinfo(dd, &glinfo, se->dd->level & DDSURFACE_LEVEL_MASK, &level))
				//if(ddsurf_glinfo(dd, &glinfo, dd_level, &level))
				if(ddsurf_glinfo(dd, &glinfo, dd->level & DDSURFACE_LEVEL_MASK, &level))
				{
					TOPIC("SURFEX", "  glinfo.need_convert=%d", glinfo.need_convert);
					if(!glinfo.need_convert)
					{
						TOPIC("SURFEX", "  glTexImage2D(%d, %d, %d, %d, %d, 0, %d, %d, %X)",
							glinfo.target,
							level,
							glinfo.internalformat,
							glinfo.width,
							glinfo.height,
							glinfo.format, glinfo.type, dd->flatptr
						);
						GL_CHECK(entry->proc.pglTexImage2D(
							glinfo.target,
							level,
							glinfo.internalformat,
							glinfo.width,
							glinfo.height,
							0,
							glinfo.format, glinfo.type, dd->flatptr));
					}
					else
					{
						void *data = NULL;
						if(glinfo.compressed)
						{
							switch(glinfo.internalformat)
							{
								case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
									data = MesaDXT1(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.colorkey, glinfo.ck_low, glinfo.ck_high);
									break;
								case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
									data = MesaDXT3(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.colorkey, glinfo.ck_low, glinfo.ck_high);
									break;
								case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
									data = MesaDXT5(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.colorkey, glinfo.ck_low, glinfo.ck_high);
									break;
							}
						}
						else if(glinfo.palette)
						{
							// FIXME: convert palette to RGBA
						}
						else if(glinfo.colorkey)
						{
							switch(glinfo.type)
							{
								case GL_UNSIGNED_SHORT_4_4_4_4_REV:
									data = MesaChroma12(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.ck_low, glinfo.ck_high);
									break;
								case GL_UNSIGNED_SHORT_1_5_5_5_REV:
									data = MesaChroma15(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.ck_low, glinfo.ck_high);
									break;
								case GL_UNSIGNED_SHORT_5_6_5:
									data = MesaChroma16(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.ck_low, glinfo.ck_high);
									break;
								case GL_UNSIGNED_BYTE:
									if(glinfo.bpp == 32)
									{
										data = MesaChroma32(ctx, dd->flatptr, glinfo.width, glinfo.height,
											glinfo.ck_low, glinfo.ck_high);
									}
									else if(glinfo.bpp == 24)
									{
										data = MesaChroma24(ctx, dd->flatptr, glinfo.width, glinfo.height,
											glinfo.ck_low, glinfo.ck_high);
									}
									break;
								case GL_UNSIGNED_INT_8_8_8_8_REV:
									data = MesaChroma32(ctx, dd->flatptr, glinfo.width, glinfo.height,
										glinfo.ck_low, glinfo.ck_high);
									break;
							}
						}
						
						if(data != NULL)
						{
							GL_CHECK(entry->proc.pglTexImage2D(
								glinfo.target,
								level,
								GL_RGBA,
								glinfo.width,
								glinfo.height,
								0,
								GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data));
							MesaTempFree(ctx, data);
							data = NULL;
						}
					}
						
					dd->dirty = FALSE;
				}
			}
			last_level = dd->level;
			
			dd = dd->down;
			dd_level++;
			
		}
	}
	
	ctx->state.tmu[tmu].active_dxid = dxid;
	ctx->state.tmu[tmu].reload = FALSE;
	ctx->state.tmu[tmu].update = TRUE;
}
