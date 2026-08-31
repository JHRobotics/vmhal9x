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
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"
#include "d3dhal.h"

#include "nocrt.h"
#endif

#define D3DTOP_MODULATE_LEGACY 64

#ifndef D3DTSS_TCI_SPHEREMAP
#define D3DTSS_TCI_SPHEREMAP 0x40000
#endif

static const GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

#define MESA_LIB_SW_NAME "mesa3d.dll"
#define MESA_LIB_SVGA_NAME "vmwsgl32.dll"
#define MESA_LIB_QEMU3DFX_NAME "qmfxgl32.dll"

#define OS_WIDTH   320
#define OS_HEIGHT  240
#define OS_FORMAT  OSMESA_RGBA
#define OS_TYPE    GL_UNSIGNED_BYTE

static char *MesaLibName()
{
	VMHAL_enviroment_t *env = GlobalVMHALenv();

	if(!env->forceos)
	{
		FBHDA_t *hda = FBHDA_setup();
		if(hda)
		{
			if(hda->flags & FB_ACCEL_VMSVGA3D)
			{
				return MESA_LIB_SVGA_NAME;
			}

			if(hda->flags & FB_ACCEL_QEMU3DFX)
			{
				return MESA_LIB_QEMU3DFX_NAME;
			}
		}
	}

	return MESA_LIB_SW_NAME;
}

#define MESA_API(_n, _t, _p) \
	mesa->proc.p ## _n = (_n ## _h)mesa->GetProcAddress(#_n); \
	if(!mesa->proc.p ## _n){mesa->proc.p ## _n = (_n ## _h)GetProcAddress(mesa->lib, #_n);} \
	if(!mesa->proc.p ## _n){valid = FALSE; ERR("GetProcAddress fail for %s", #_n); break;}

#define MESA_API_OS(_n, _t, _p) \
	if(mesa->os){MESA_API(_n, _t, _p)}else{mesa->proc.p ## _n = NULL;}

#define MESA_API_DRV(_n, _t, _p) \
	if(!mesa->os){ \
		mesa->proc.p ## _n = (_n ## _h)GetProcAddress(mesa->lib, #_n); if(!mesa->proc.p ## _n){valid = FALSE; ERR("GetProcAddress fail for %s", #_n); break;} \
	}else{mesa->proc.p ## _n = NULL;}

static BOOL Mesa3DCreate(mesa3d_entry_t *mesa)
{
	TRACE_ENTRY

	memset(mesa, 0, sizeof(mesa3d_entry_t));

	BOOL valid = TRUE;

	do
	{
		mesa->hda = FBHDA_setup();
		if(mesa->hda == NULL)
		{
			valid = FALSE; break;
		}
		mesa->pid = GetCurrentProcessId();

		mesa->ht_ctx = ht_init(HT_PRIME_TINY);
		mesa->ht_flat = ht_init(HT_PRIME_DEF);
		mesa->ht_dxid = ht_init(HT_PRIME_DEF);
		if(mesa->ht_ctx == NULL || mesa->ht_flat == NULL || mesa->ht_dxid == NULL)
		{
			ERR("init hashstables fail");
			valid = FALSE; break;
		}
		
		if(ids_procs == NULL)
		{
			ERR("Invalid init");
			valid = FALSE; break;
		}
		else
		{
			memcpy(&(mesa->ids), ids_procs, sizeof(ids_proc_t));
		}

		TRACE("LoadLibraryA(%s)", MesaLibName());
		mesa->lib = LoadLibraryA(MesaLibName());
		if(!mesa->lib)
		{
			valid = FALSE; break;
		}

		//mesa->GetProcAddress = NULL;
		mesa->GetProcAddress = (OSMesaGetProcAddress_h)GetProcAddress(mesa->lib, "OSMesaGetProcAddress");
		if(!mesa->GetProcAddress)
		{
			mesa->GetProcAddress = (OSMesaGetProcAddress_h)GetProcAddress(mesa->lib, "DrvGetProcAddress");
			if(!mesa->GetProcAddress)
			{
				valid = FALSE;
			}
			else
			{
				mesa->os = FALSE;
			}
		}
		else
		{
			mesa->os = TRUE;
		}
		TOPIC("OSR", "Loaded, ?OS=%d", mesa->os);

		#include "mesa3d_api.h"

		GetVMHALenv(&mesa->env);
		UpdateVMHALenv(&mesa->env);
		//memcpy(&mesa->env, &VMHALenv, sizeof(VMHAL_enviroment_t));

	} while(0);

	if(!valid)
	{
		return FALSE;
	}

#ifdef TRACE_ON
	{
		char exename[MAX_PATH];
		GetModuleFileNameA(NULL, exename, MAX_PATH);
		TOPIC("EXE", "Exe loader: %s", exename);
	}
#endif

	return TRUE;
}

#undef MESA_API
#undef MESA_API_OS
#undef MESA_API_DRV

#define FBO_WND_CLASS_NAME "vmhal9x_fbo_win"

/* for HW opengl we need some DC to retrieve GL functions, create context,
 * and set FBO without touching real frame buffer. Probably safest way is
 * create hidden window. Alternatives are:
 *  1) use system window - e.g. GetDC(NULL), but could be problem if there
 *     are more contexts from multiple threads.
 *  2) Call CreateDC(...), but this is supported until 98/Me and I'm not
 *     sure how well. 
 */
static HWND MesaCreateWindow(int w, int h)
{
	WNDCLASS wc      = {0};
	wc.lpfnWndProc   = DefWindowProc;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = FBO_WND_CLASS_NAME;
	wc.style         = CS_OWNDC;
	wc.hInstance     = GetModuleHandle(NULL);
	
	RegisterClass(&wc);
	
	return CreateWindowA(FBO_WND_CLASS_NAME, "vmhal9x dummy", /*WS_OVERLAPPEDWINDOW|WS_VISIBLE*/0, 0,0, w,h, 0,0, NULL, 0);
}

static mesa3d_entry_t *mesalocal = NULL;

NUKED_LOCAL mesa3d_entry_t *Mesa3DGet(BOOL create)
{
	TRACE_ENTRY
	
	if(mesalocal != NULL)
	{
		return mesalocal;
	}
	
	if(create)
	{
		mesa3d_entry_t *mem = hal3d_calloc(sizeof(mesa3d_entry_t));

		if(Mesa3DCreate(mem))
		{
			mesalocal = mem;
			return mesalocal;
		}
	}
	
	return NULL;
}

NUKED_LOCAL void Mesa3DFree(BOOL unload)
{
	if(mesalocal != NULL)
	{
		MesaDestroyAllCtx(mesalocal);
		if(unload)
		{
			FreeLibrary(mesalocal->lib);
		}
		hal3d_free((void**)&mesalocal);
	}
}

NUKED_FAST BOOL MesaOldFlip(mesa3d_ctx_t *ctx)
{
/*	if(ctx->dxif <= MESA_CTX_IF_DX5)
	{
		return TRUE;
	}*/
	
	if(ctx->entry->runtime_ver < 7)
	{
		return TRUE;
	}
	
	return FALSE;
}

static GLenum DXSencilToGL(D3DSTENCILOP op)
{
	switch(op)
	{
    case D3DSTENCILOP_ZERO:    return GL_ZERO;
    case D3DSTENCILOP_REPLACE: return GL_REPLACE;
    case D3DSTENCILOP_INCRSAT: return GL_INCR_WRAP;
    case D3DSTENCILOP_DECRSAT: return GL_DECR_WRAP;
    case D3DSTENCILOP_INVERT:  return GL_INVERT;
    case D3DSTENCILOP_INCR:    return GL_INCR;
    case D3DSTENCILOP_DECR:    return GL_DECR;
    case D3DSTENCILOP_KEEP:
    default:
    	return GL_KEEP;
	} // switch
	
	return GL_KEEP;
}

/* only needs for DX6+ */
NUKED_LOCAL void MesaStencilApply(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	if(ctx->depth_stencil && ctx->state.stencil.enabled)
	{
		GL_CHECK(entry->proc.pglEnable(GL_STENCIL_TEST));
		GL_CHECK(entry->proc.pglStencilFunc(
			ctx->state.stencil.func,
			ctx->state.stencil.ref,
			ctx->state.stencil.mask
		));
		GL_CHECK(entry->proc.pglStencilOp(
			ctx->state.stencil.sfail,
 			ctx->state.stencil.dpfail,
 			ctx->state.stencil.dppass
 		));
		GL_CHECK(entry->proc.pglStencilMask(ctx->state.stencil.writemask));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_STENCIL_TEST));
	}
}

static void MesaDepthApply(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY

	mesa3d_entry_t *entry = ctx->entry;
	BOOL have_depth = FALSE;

	if(ctx->depth_bpp)
	{
		have_depth = TRUE;
	}

	if(have_depth && ctx->state.depth.enabled)
	{
		GL_CHECK(entry->proc.pglEnable(GL_DEPTH_TEST));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_DEPTH_TEST));
	}
	
	if(have_depth && ctx->state.depth.writable)
	{
		GL_CHECK(entry->proc.pglDepthMask(GL_TRUE));
	}
	else
	{
		GL_CHECK(entry->proc.pglDepthMask(GL_FALSE));
	}
}

NUKED_FAST void *MesaGetVidMem(mesa3d_ctx_t *ctx, DWORD sid, BOOL ddi6)
{
	void *ptr = NULL;
	surfaceex_t *se = surfex_get(ctx, sid, FALSE, FALSE);
	if(se)
	{
		ptr = se->dd->flatptr;
		if(ddi6 && se->dd->lcl)
		{
			ptr = (void *)(((LPDDRAWI_DDRAWSURFACE_LCL)(se->dd->lcl))->lpGbl->fpVidMem);
		}
	}
	return ptr;
}

NUKED_INLINE BOOL MesaBackbufferIsFront(mesa3d_ctx_t *ctx)
{
	DWORD addr = (DWORD)MesaGetVidMem(ctx, ctx->backbuffer, MesaOldFlip(ctx));
	TRACE("MesaBackbufferIsFront addr=0x%X", addr);
	
	FBHDA_t *hda = FBHDA_setup();
	DWORD visible_addr = ((DWORD)hda->vram_pm32) + hda->surface;
	
	if(addr == visible_addr)
	{
		return TRUE; 
	}
	else
	{
		return FALSE; 
	}
}

NUKED_LOCAL mesa3d_ctx_t *MesaCreateCtx(mesa3d_entry_t *entry, DWORD dds_sid, DWORD ddz_sid)
{
	TRACE_ENTRY
//	int i;
	DWORD ctx_id;
	mesa3d_ctx_t *ctx = NULL;
	
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if(!entry->ids.id_assign(&ctx_id))
		return NULL;

	BOOL valid = FALSE;
	do
	{
		ctx = hal3d_calloc(sizeof(mesa3d_ctx_t));
		if(ctx == NULL) break;

		ctx->id = ctx_id;
		ctx->fbo = &ctx->fbo_swap[0];

		ctx->ht_tex = ht_init(HT_PRIME_DEF);
		if(ctx->ht_tex == NULL) break;
			
		ctx->ht_tex_garbage = ht_init(HT_PRIME_DEF);
		if(ctx->ht_tex_garbage == NULL) break;

		ctx->fifo_top = entry->hda->dd_fifo_top;

/*
		if(SurfaceIsEmpty(dds_sid))
		{
			TOPIC("SURFACE", "empty surface as draw surface");
			SurfaceEmptyClear(dds_sid);
		}*/

		ctx->thread_lock = 0;
		ctx->entry = entry;

		if(entry->os)
		{
			TOPIC("RS", "OSMesaCreateContextExt(OSMESA_RGBA, 24, 8, 0, NULL)");

			/* create context every time with 24bit depth buffer and 8bit stencil buffer,
			 * because we can't dynamicaly change depth and stencil plane.
			 */
			ctx->osctx = entry->proc.pOSMesaCreateContextExt(OS_FORMAT, 24, 8, 0, NULL);
			if(ctx->osctx == NULL)
				break;

			ctx->ossize = SurfacePitch(OS_WIDTH, 4)*OS_HEIGHT;
			ctx->osbuf = hal3d_malloc(ctx->ossize);
			if(ctx->osbuf == NULL)
				break;

			if(!entry->proc.pOSMesaMakeCurrent(ctx->osctx, ctx->osbuf, OS_TYPE, OS_WIDTH, OS_HEIGHT))
				break;

			entry->proc.pOSMesaPixelStore(OSMESA_Y_UP, 1);
		}
		else
		{
			int ipixel;

			ctx->fbo_win = MesaCreateWindow(OS_WIDTH, OS_HEIGHT);
			if(ctx->fbo_win == NULL)
				break;

			ctx->dc = GetDC(ctx->fbo_win);
			if(ctx->dc == NULL)
				break;

			ipixel = entry->proc.pDrvDescribePixelFormat(ctx->dc, 0, 0, NULL);
			if(ipixel == 0)
			{
				ipixel = ChoosePixelFormat(ctx->dc, &pfd); 
			}
			entry->proc.pDrvSetPixelFormat(ctx->dc, ipixel);

			ctx->glrc = entry->proc.pDrvCreateLayerContext(ctx->dc, 0);
			if(ctx->glrc == NULL)
				break;

			entry->proc.pDrvSetContext(ctx->dc, ctx->glrc, NULL);
		}

		char *ver_minor = NULL;
		const char *gl_ver = (const char *)entry->proc.pglGetString(GL_VERSION);
		if(gl_ver == NULL)
			break;

		entry->gl_minor = 0;
		entry->gl_major = strtol(gl_ver, &ver_minor, 10);
		if(ver_minor)
		{
			if(ver_minor[0] == '.')
			{
				entry->gl_minor = strtol(ver_minor+1, NULL, 10);
			}
		}

		TOPIC("GLVER", "GL version %d.%d - %s", entry->gl_major, entry->gl_minor, gl_ver);

		//if(!entry->env.scanned)
		if(1)
		{
			if(entry->gl_major < 3)
			{
				entry->env.zfloat = FALSE;
			}
			GLint max_tex_size = 0;
			GLint max_clips = 0;

			entry->proc.pglGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
			entry->proc.pglGetIntegerv(GL_MAX_CLIP_PLANES, &max_clips);

			if(max_tex_size > 16384)
			{
				max_tex_size = 16384;
			}
			entry->env.texture_max_width  = max_tex_size;
			entry->env.texture_max_height = max_tex_size;
			entry->env.num_clips = max_clips;

			TOPIC("GLVER", "entry->env.texture_max_width=%d", entry->env.texture_max_width);
			TOPIC("GLVER", "entry->env.num_clips=%d", entry->env.num_clips);

//			entry->env.scanned = TRUE;
		}

		if(dds_sid != 0)
		{
			MesaSetTarget(ctx, dds_sid, ddz_sid, TRUE);
		}
		else
		{
			MesaSetEmptyTarget(ctx, TRUE);
		}

#ifdef TRACE_ON
		{
			const GLubyte *s;

			TRACE("Context ON, os = %d", entry->os);
			s = entry->proc.pglGetString(GL_VENDOR);
			TRACE("GL_VENDOR=%s", s);
			s = entry->proc.pglGetString(GL_RENDERER);
			TRACE("GL_RENDERER=%s", s);
			s = entry->proc.pglGetString(GL_VERSION);
			TRACE("GL_VERSION=%s", s);
		}
#endif
		ctx->thread_id = GetCurrentThreadId();

		MesaInitCtx(ctx);
		//UpdateScreenCoords(ctx, (GLfloat)width, (GLfloat)height);
		MesaApplyViewport(ctx, 0, 0, ctx->state.sw, ctx->state.sh, TRUE);

		valid = TRUE;
	} while(0);

	/* error, clean the garbage */
	if(!valid)
	{
		ERR("Context creation failure!");
				
		if(ctx)
		{
			if(entry->os)
			{
				if(ctx->osctx)
					entry->proc.pOSMesaDestroyContext(ctx->osctx);

				if(ctx->osbuf)
					hal3d_free(ctx->osbuf);
			}
			else
			{
				if(ctx->glrc)
					entry->proc.pDrvDeleteContext(ctx->glrc);

				if(ctx->fbo_win)
					DestroyWindow(ctx->fbo_win);
			}

			ht_destroy(&(ctx->ht_tex));
			ht_destroy(&(ctx->ht_tex_garbage));

			hal3d_free((void**)&ctx);

			ctx = NULL;
		}
	}
	else
	{
		HT_INSERT_T(mesa3d_ctx_t, entry->ht_ctx, ctx_id, ctx);
	}

#if DEBUG
	if(ctx)
	{
		MesaMemInfo(ctx->entry);
	}
#endif

	return ctx;
}

NUKED_LOCAL BOOL MesaSetTarget(mesa3d_ctx_t *ctx, DWORD dds_sid, DWORD ddz_sid, BOOL create)
{
	TOPIC("TARGET", "MesaSetTarget(ctx, %d, %d, %d)", dds_sid, ddz_sid, create);

	surfaceex_t *se = surfex_get(ctx, dds_sid, TRUE, TRUE);
	if(se == NULL)
	{
		ERR("dds_sid=%d => se=NULL", dds_sid);
		return FALSE;
	}

	int width  = se->gl->width;
	int height = se->gl->height;
	int bpp =    se->gl->bpp;
	int bpp_depth = 0;
	BOOL viewport_set = TRUE;
	//dds->fpVidMem = dds->lpLcl->lpGbl->fpVidMem;
	TOPIC("TARGET", "MesaSetTarget: target size(%d x %d x %d)", width, height, bpp);

	if(ddz_sid)
	{
		surfaceex_t *sez = surfex_get(ctx, ddz_sid, TRUE, TRUE);
		if(sez)
		{
			bpp_depth = sez->gl->bpp;
			TRACE("bpp_depth = %d", bpp_depth);

			if(sez->fb->four_cc == D3DFMT_D24S8 || sez->fb->four_cc == D3DFMT_D24X4S4)
			{
				ctx->depth_stencil = TRUE;
			}
		}
		else
		{
			ERR("ddz_sid=%d => ddz=NULL", ddz_sid);
		}
	}

	if(create || ctx->state.sw != width || ctx->state.sh != height/* || ctx->front_bpp != bpp*/)
	{
		MesaBufferFBOSetup(ctx, width, height, bpp);
	}
	else
	{
		viewport_set = FALSE;
	}

	ctx->front_bpp = bpp;
	ctx->backbuffer = dds_sid;
	ctx->depth_bpp = bpp_depth;
	ctx->depth = ddz_sid;

	ctx->state.sw = width;
	ctx->state.sh = height;

	//UpdateScreenCoords(ctx, (GLfloat)width, (GLfloat)height);
	ctx->state.textarget = (se->tex->loadedstate.flags & FBHDA_DD_FLAG_TEXTURE) ? TRUE : FALSE;

	if(viewport_set)
		MesaApplyViewport(ctx, 0, 0, width, height, FALSE);

	MesaDepthApply(ctx);
	MesaStencilApply(ctx);
	MesaDrawRefreshState(ctx);

	return TRUE;
}

NUKED_LOCAL BOOL MesaSetEmptyTarget(mesa3d_ctx_t *ctx, BOOL create)
{
	TRACE_ENTRY

	int width  = 640;
	int height = 480;
	int bpp =    32;
	int bpp_depth = 0;
	BOOL viewport_set = TRUE;

	if(create || ctx->state.sw != width || ctx->state.sh != height)
	{
		MesaBufferFBOSetup(ctx, width, height, bpp);
	}
	else
	{
		viewport_set = FALSE;
	}

	ctx->front_bpp = bpp;
	ctx->depth_bpp = bpp_depth;
	ctx->backbuffer = 0;
	ctx->depth = 0;

	ctx->state.sw = width;
	ctx->state.sh = height;

	ctx->state.textarget = FALSE;
	if(viewport_set)
		MesaApplyViewport(ctx, 0, 0, width, height, FALSE);

	MesaDepthApply(ctx);
	MesaStencilApply(ctx);
	MesaDrawRefreshState(ctx);

	return TRUE;
}

NUKED_LOCAL void MesaTextureRoll(DWORD id, void *target, void *data)
{
	mesa3d_ctx_t *ctx = data;
	gldata_tex_t *tex = target;
	
	//ht_delete_more(ctx->entry->ht_flat, DW_FLAT(dd->flatptr), target);
	
	//id_free(dd->dxid ^ DXID_LOCAL);
	
	//hal3d_free((void**)&dd);
	//ht_delete_more(ctx->entry->ht_flat, flatptr, target);	
}

NUKED_LOCAL void MesaDestroyCtx(mesa3d_ctx_t *ctx, BOOL roll)
{
	TRACE_ENTRY
	mesa3d_entry_t *entry = ctx->entry;
	
	TOPIC("GL", "OSMesaDestroyContext");

	if(ctx->osctx != NULL)
	{
		entry->proc.pOSMesaDestroyContext(ctx->osctx);
	}
	
	if(ctx->glrc)
	{
		entry->proc.pDrvDeleteContext(ctx->glrc);
	}
	
	if(ctx->fbo_win)
	{
		DestroyWindow(ctx->fbo_win);
	}

	if(ctx->osbuf != NULL)
	{
		hal3d_free(ctx->osbuf);
	}
	
	if(ctx->temp.buf)
	{
		hal3d_free(ctx->temp.buf);
		ctx->temp.size = ctx->temp.width = 0;
		ctx->temp.buf = NULL;
	}

	MesaLightDestroyAll(ctx);
	MesaFreePals(ctx);
	MesaVSDestroyAll(ctx);
	
	ht_roll(ctx->ht_tex, MesaTextureRoll, ctx);
	ht_destroy(&(ctx->ht_tex));

	if(!roll)
	{
		ht_delete(entry->ht_ctx, ctx->id);
	}

	entry->ids.id_free(ctx->id);
	hal3d_free((void**)&ctx);
}

NUKED_LOCAL void MesaCtxRoll(DWORD id, void *target, void *data)
{
	//mesa3d_entry_t *entry = data;
	MesaDestroyCtx(target, TRUE);
}

NUKED_LOCAL void MesaDestroyAllCtx(mesa3d_entry_t *entry)
{
	TRACE_ENTRY
	
	ht_roll(entry->ht_ctx, MesaCtxRoll, entry);
}

NUKED_LOCAL void ApplyBlend(mesa3d_ctx_t *ctx)
{
#if 1
	mesa3d_entry_t *entry = ctx->entry;
	if(ctx->state.blend.alphablend || ctx->state.blend.edgeantialias)
	{
		GL_CHECK(entry->proc.pglEnable(GL_BLEND));
		
		if(ctx->state.blend.edgeantialias && ctx->entry->env.lowdetail == 0)
		{
			GL_CHECK(entry->proc.pglEnable(GL_LINE_SMOOTH));
		}
		else
		{
			GL_CHECK(entry->proc.pglDisable(GL_LINE_SMOOTH));
		}
		
		if(!(ctx->state.blend.srcRGB == GL_SRC_COLOR && ctx->state.blend.dstRGB == GL_SRC_ALPHA)) /* from wine9x */
		{
			GL_CHECK(entry->proc.pglBlendFunc(
				ctx->state.blend.srcRGB,
				ctx->state.blend.dstRGB
			));
		}
		
		/*
		GL_CHECK(entry->proc.pglBlendFunc(
			GL_ONE,
			GL_ZERO
		));*/

		GL_CHECK(entry->proc.pglBlendEquation(ctx->state.blend.blendop));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_BLEND));
	}
#else
	mesa3d_entry_t *entry = ctx->entry;

	if(ctx->state.blend.edgeantialias && ctx->entry->env.lowdetail == 0)
	{
		GL_CHECK(entry->proc.pglEnable(GL_LINE_SMOOTH));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_LINE_SMOOTH));
	}

	if(ctx->state.blend.alphablend)
	{
		if(!(ctx->state.blend.srcRGB == GL_SRC_COLOR && ctx->state.blend.dstRGB == GL_SRC_ALPHA)) /* from wine9x */
		{
			GL_CHECK(entry->proc.pglBlendFunc(
				ctx->state.blend.srcRGB,
				ctx->state.blend.dstRGB
			));
		}
	}
	else
	{
		GL_CHECK(entry->proc.pglBlendFunc(GL_ONE, GL_ZERO));
	}

	GL_CHECK(entry->proc.pglBlendEquation(ctx->state.blend.blendop));
#endif
/*
	TOPIC("BLENDSTATE", "==========================");
	TOPIC("BLENDSTATE", "ctx->state.blend.alphablend = %d", ctx->state.blend.alphablend);
	TOPIC("BLENDSTATE", "ctx->state.blend.srcRGB = %d",     ctx->state.blend.srcRGB);
	TOPIC("BLENDSTATE", "ctx->state.blend.dstRGB = %d",     ctx->state.blend.dstRGB);
	TOPIC("BLENDSTATE", "ctx->state.blend.blendop = %d",    ctx->state.blend.blendop);
*/
}

NUKED_LOCAL void MesaInitCtx(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	int i = 0;
	GLint real_tus;
	
	GL_CHECK(entry->proc.pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
	
	entry->proc.pglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &real_tus);
	
	ctx->tmu_count = entry->env.texture_num_units > MESA_TMU_MAX ? MESA_TMU_MAX : entry->env.texture_num_units;
	
	if(real_tus > ctx->tmu_count)
	{
		ctx->fbo_tmu = ctx->tmu_count;
	}
	else
	{
		ctx->fbo_tmu = 0;
	}

	ctx->state.tmu[0].active = 1;

	GL_CHECK(entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN));
	GL_CHECK(entry->proc.pglPixelStorei(GL_PACK_ALIGNMENT, FBHDA_ROW_ALIGN));

	GL_CHECK(entry->proc.pglFrontFace(GL_CCW));

	GL_CHECK(entry->proc.pglMatrixMode(GL_MODELVIEW));
	GL_CHECK(entry->proc.pglLoadIdentity());
	GL_CHECK(entry->proc.pglMatrixMode(GL_PROJECTION));
	GL_CHECK(entry->proc.pglLoadIdentity());
	
	entry->proc.pglDepthRange(0.0f, 1.0f);

	ctx->matrix.wmax = GL_WRANGE_MAX;
	ctx->matrix.zscale = 1.0;

	MesaIdentity(ctx->matrix.world[0]);
	MesaIdentity(ctx->matrix.world[1]);
	MesaIdentity(ctx->matrix.world[2]);
	MesaIdentity(ctx->matrix.world[3]);

	MesaIdentity(ctx->matrix.view);
	MesaIdentity(ctx->matrix.proj);

	//MesaIdentity(ctx->matrix.modelview);
	//ctx->matrix.is_identity = TRUE;

	//GL_CHECK(entry->proc.pglDisable(GL_MULTISAMPLE));
	GL_CHECK(entry->proc.pglDisable(GL_LIGHTING));
	//GL_CHECK(entry->proc.pglLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE));

	GL_CHECK(entry->proc.pglLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR));
	//GL_CHECK(entry->proc.pglLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR));
	
	if(ctx->entry->env.lowdetail >= 1)
	{
		GL_CHECK(entry->proc.pglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
		GL_CHECK(entry->proc.pglHint(GL_LINE_SMOOTH_HINT, GL_FASTEST));
		GL_CHECK(entry->proc.pglHint( GL_POLYGON_SMOOTH_HINT, GL_FASTEST));
		GL_CHECK(entry->proc.pglDisable(GL_DITHER));
	}

	if(ctx->entry->env.lowdetail >= 3)
	{
		GL_CHECK(entry->proc.pglShadeModel(GL_FLAT));
	}

	// enable edge filtering on cubemap
	if(entry->gl_major >= 3)
	{
		if(ctx->entry->env.lowdetail <= 1)
		{
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
		}
	}

	// extends clamp over z-depth range
	//GL_CHECK(entry->proc.pglEnable(GL_DEPTH_CLAMP_NV));

	// needs ARB_vertex_blend
	//GL_CHECK(entry->proc.pglEnable(GL_WEIGHT_SUM_UNITY_ARB));

	for(i = 0; i < ctx->tmu_count; i++)
	{
		GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0 + i));
		
		if(ctx->state.tmu[i].active)
		{
			entry->proc.pglEnable(GL_TEXTURE_2D);
		}
		
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
		
		ctx->state.tmu[i].reload = TRUE;
		ctx->state.tmu[i].update = TRUE;
		
		/*
			defaults:
			https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms886612(v=msdn.10)
		*/
		ctx->state.tmu[i].dx_mip = D3DTFP_NONE;
		ctx->state.tmu[i].dx_mag = D3DTFG_POINT;
		ctx->state.tmu[i].dx_min = D3DTFN_POINT;
		
		/*
			defaults:
			https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtexturestagestatetype
		*/
		ctx->state.tmu[i].color_op   = D3DTOP_MODULATE;
		ctx->state.tmu[i].color_arg1 = D3DTA_TEXTURE;
		ctx->state.tmu[i].color_arg2 = D3DTA_CURRENT;
		ctx->state.tmu[i].alpha_op   = D3DTOP_SELECTARG1;
		ctx->state.tmu[i].alpha_arg1 = D3DTA_TEXTURE;
		ctx->state.tmu[i].alpha_arg2 = D3DTA_CURRENT;

		if(i > 0)
		{
			ctx->state.tmu[i].color_op = D3DTOP_DISABLE;
			ctx->state.tmu[i].alpha_op = D3DTOP_DISABLE;
		}
		
		MesaIdentity(ctx->state.tmu[i].matrix);
		ctx->state.tmu[i].matrix_idx = TRUE;
	}

	GL_CHECK(entry->proc.pglEnable(GL_BLEND));
//	entry->proc.pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ctx->state.blend.alphablend = FALSE;
	ctx->state.blend.srcRGB     = GL_SRC_ALPHA;
	ctx->state.blend.dstRGB     = GL_ONE_MINUS_SRC_ALPHA;
	ctx->state.blend.edgeantialias = FALSE;
	
	ctx->state.blend.srcAlpha   = GL_ONE;
	ctx->state.blend.dstAlpha   = GL_ZERO;
	ctx->state.blend.lineantialias = FALSE;

	ctx->state.blend.blendop = GL_FUNC_ADD;
	ctx->state.blend.blendop_alpha = GL_FUNC_ADD;

	ApplyBlend(ctx);

	ctx->state.texperspective  = TRUE;
	ctx->state.alpha.func = GL_NOTEQUAL;
	ctx->state.alpha.ref = 0.0f;

	entry->proc.pglDisable(GL_CULL_FACE);

	ctx->state.stencil.sfail     = GL_KEEP;
	ctx->state.stencil.dpfail    = GL_KEEP;
	ctx->state.stencil.dppass    = GL_KEEP;
	ctx->state.stencil.func      = GL_ALWAYS;
	ctx->state.stencil.ref       = 0;
	ctx->state.stencil.mask      = 0xFFFFFFFF;
	ctx->state.stencil.writemask = 0xFFFFFFFF;

	ctx->render.rop2 = R2_COPYPEN;
	ctx->render.planemask = 0xFFFFFFFF;

	MesaDepthApply(ctx);
	entry->proc.pglDepthFunc(GL_LESS);
	MesaStencilApply(ctx);

	//ctx->state.bind_vertices = 0;
	ctx->state.bind_indices = NULL;
	ctx->vstream[0].mem.ptr = NULL;

	ctx->shader.vs = NULL;

	MesaDrawRefreshState(ctx);

	MesaRecCaptureInit(ctx);
}

NUKED_LOCAL BOOL MesaSetCtx(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY

	if(ctx->entry->os)
	{
		if(ctx->entry->proc.pOSMesaMakeCurrent(
			ctx->osctx, ctx->osbuf, OS_TYPE, OS_WIDTH, OS_HEIGHT))
		{
			//UpdateScreenCoords(ctx, ctx->state.sw, ctx->state.sh);
			ctx->thread_id = GetCurrentThreadId();
			return TRUE;
		}
	}
	else
	{
		if(ctx->entry->proc.pDrvSetContext(ctx->dc, ctx->glrc, NULL))
		{
			//UpdateScreenCoords(ctx, ctx->state.sw, ctx->state.sh);
			ctx->thread_id = GetCurrentThreadId();
			return TRUE;
		}
	}

	return FALSE;
}

#define D3DBLEND_BLENDFACTOR 14
#define D3DBLEND_INVBLENDFACTOR 15

NUKED_INLINE GLenum GetBlendFactor(D3DBLEND dxfactor)
{
	switch(dxfactor)
	{
    case D3DBLEND_ZERO:
    	return GL_ZERO;
    case D3DBLEND_ONE:
    	return GL_ONE;
    case D3DBLEND_SRCCOLOR:
    	return GL_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
    	return GL_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
    	return GL_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
    	return GL_ONE_MINUS_SRC_ALPHA;
    case D3DBLEND_DESTALPHA:
    	return GL_DST_ALPHA;
    	//return GL_ONE;
    case D3DBLEND_INVDESTALPHA:
    	return GL_ONE_MINUS_DST_ALPHA;
    	//return GL_ZERO;
    case D3DBLEND_DESTCOLOR:
    	return GL_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
    	return GL_ONE_MINUS_DST_COLOR;
    case D3DBLEND_SRCALPHASAT:
    	return GL_SRC_ALPHA_SATURATE;
    default:
			switch((int)dxfactor)
			{
				case D3DBLEND_BLENDFACTOR:
					return GL_CONSTANT_COLOR;
    		case D3DBLEND_INVBLENDFACTOR:
    			return GL_ONE_MINUS_CONSTANT_COLOR;
    	}
    	break;
	}

	return 0;
}

NUKED_INLINE GLenum GetGLFogMode(D3DFOGMODE dxfog)
{
	switch(dxfog)
	{
		case D3DFOG_EXP: return GL_EXP;
		case D3DFOG_EXP2: return GL_EXP2;
		case D3DFOG_LINEAR: return GL_LINEAR;
		case D3DFOG_NONE:
		default:
			return 0;
	}
	return 0;
}

NUKED_INLINE GLenum GetGLCmpFunc(D3DCMPFUNC dxfn)
{
	switch(dxfn)
	{
		case D3DCMP_LESS:         return GL_LESS;
		case D3DCMP_EQUAL:        return GL_EQUAL;
		case D3DCMP_LESSEQUAL:    return GL_LEQUAL;
		case D3DCMP_GREATER:      return GL_GREATER;
		case D3DCMP_NOTEQUAL:     return GL_NOTEQUAL;
		case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
		case D3DCMP_ALWAYS:       return GL_ALWAYS;
		case D3DCMP_NEVER:
		default:                  return GL_NEVER;
	}
	return GL_NEVER;
}

static void MesaSetFog(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	GLenum func;

	if(ctx->state.fog.enabled)
	{
		func = ctx->state.fog.vmode;
		if(func == 0)
		{
			func = ctx->state.fog.tmode;
		}
		
		if(func != 0)
		{
			GL_CHECK(entry->proc.pglEnable(GL_FOG));
			GL_CHECK(entry->proc.pglFogi(GL_FOG_MODE, func));
			GL_CHECK(entry->proc.pglFogfv(GL_FOG_COLOR, &ctx->state.fog.color[0]));
			
			TOPIC("FOG", "glEnable(GL_FOG)");
			TOPIC("FOG", "glFogi(GL_FOG_MODE, %d)", func);

			if(func == GL_LINEAR)
			{
				GLfloat start = ctx->state.fog.start;
				GLfloat end = ctx->state.fog.end;

				GL_CHECK(entry->proc.pglFogf(GL_FOG_START, start));
				GL_CHECK(entry->proc.pglFogf(GL_FOG_END,   end));
				TOPIC("FOG", "glFogf(GL_FOG_START, %f)",   start);
				TOPIC("FOG", "glFogf(GL_FOG_END, %f)",     end);
			}
			else
			{
				GL_CHECK(entry->proc.pglFogf(GL_FOG_DENSITY, ctx->state.fog.density));
				TOPIC("FOG", "glFogf(GL_FOG_DENSITY, %f)", ctx->state.fog.density);
			}
			GL_CHECK(entry->proc.pglFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH));
			return;
		}
		else
		{
			GL_CHECK(entry->proc.pglDisable(GL_FOG));
		}
	}
	
	GL_CHECK(entry->proc.pglDisable(GL_FOG));
	TOPIC("FOG", "pglDisable(GL_FOG)");
}

static void MesaSetClipping(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;

	int i;
	int cnt = NOCRT_MIN(MESA_CLIPS_MAX, entry->env.num_clips);
	if(ctx->state.clipping.enabled)
	{
		for(i = 0; i < cnt; i++)
		{
			if(ctx->state.clipping.activeplane[i])
			{
				GL_CHECK(entry->proc.pglEnable(GL_CLIP_PLANE0 + i));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_CLIP_PLANE0 + i));
			}
		}
	}
	else
	{
		for(i = 0; i < cnt; i++)
		{
			GL_CHECK(entry->proc.pglDisable(GL_CLIP_PLANE0 + i));
		}
	}
}


NUKED_LOCAL void MesaApplyMaterial(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;

	GL_CHECK(entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &ctx->state.material.ambient[0]));
	GL_CHECK(entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &ctx->state.material.diffuse[0]));
	GL_CHECK(entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &ctx->state.material.emissive[0]));

	if(ctx->state.specular)
	{
		GL_CHECK(entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &ctx->state.material.specular[0]));
	}
	else
	{
		GL_CHECK(entry->proc.pglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]));
	}

	GL_CHECK(entry->proc.pglMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, ctx->state.material.shininess));

	TOPIC("LIGHT", "Material ambient=(%f %f %f %f), diffuse=(%f %f %f %f)",
		ctx->state.material.ambient[0], ctx->state.material.ambient[1],
		ctx->state.material.ambient[2], ctx->state.material.ambient[3],
		ctx->state.material.diffuse[0], ctx->state.material.diffuse[1],
		ctx->state.material.diffuse[2], ctx->state.material.diffuse[3]
	);

	TOPIC("LIGHT", "Material emissive=(%f %f %f %f), specular=(%f %f %f %f)",
		ctx->state.material.emissive[0], ctx->state.material.emissive[1],
		ctx->state.material.emissive[2], ctx->state.material.emissive[3],
		ctx->state.material.specular[0], ctx->state.material.specular[1],
		ctx->state.material.specular[2], ctx->state.material.specular[3]
	);

	TOPIC("LIGHT", "Material shininess=%f", ctx->state.material.shininess);
}

static void MesaApplyColorMaterial(mesa3d_ctx_t *ctx)
{
	ctx->state.material.untracked = 0;

	if(ctx->state.material.diffuse_source == D3DMCS_COLOR1)
		ctx->state.material.untracked |= MESA_MAT_DIFFUSE_C1;

	if(ctx->state.material.ambient_source == D3DMCS_COLOR1)
		ctx->state.material.untracked |= MESA_MAT_AMBIENT_C1;

	if(ctx->state.material.emissive_source == D3DMCS_COLOR1)
		ctx->state.material.untracked |= MESA_MAT_EMISSIVE_C1;

	if(ctx->state.specular && ctx->state.material.specular_source == D3DMCS_COLOR1)
		ctx->state.material.untracked |= MESA_MAT_SPECULAR_C1;

	if(ctx->state.material.diffuse_source == D3DMCS_COLOR2)
		ctx->state.material.untracked |= MESA_MAT_DIFFUSE_C2;

	if(ctx->state.material.ambient_source == D3DMCS_COLOR2)
		ctx->state.material.untracked |= MESA_MAT_AMBIENT_C2;

	if(ctx->state.material.emissive_source == D3DMCS_COLOR2)
		ctx->state.material.untracked |= MESA_MAT_EMISSIVE_C2;

	if(ctx->state.specular && ctx->state.material.specular_source == D3DMCS_COLOR2)
		ctx->state.specular_vertex = TRUE;
	else
		ctx->state.specular_vertex = FALSE;

	TOPIC("LIGHT", "Vertex color: primary=%d%d%d%d, secondary=%d%d%d%d",
		ctx->state.material.diffuse_source  == D3DMCS_COLOR1 ? 1 : 0,
		ctx->state.material.ambient_source  == D3DMCS_COLOR1 ? 1 : 0,
		ctx->state.material.emissive_source == D3DMCS_COLOR1 ? 1 : 0,
		ctx->state.material.specular_source == D3DMCS_COLOR1 ? 1 : 0,
		ctx->state.material.diffuse_source  == D3DMCS_COLOR2 ? 1 : 0,
		ctx->state.material.ambient_source  == D3DMCS_COLOR2 ? 1 : 0,
		ctx->state.material.emissive_source == D3DMCS_COLOR2 ? 1 : 0,
		ctx->state.material.specular_source == D3DMCS_COLOR2 ? 1 : 0
	);

	MesaApplyMaterial(ctx);
}

static void MesaApplyDX5TexBlend(mesa3d_ctx_t *ctx, int tmu, D3DTEXTUREBLEND blend)
{
	switch(blend)
	{
		case D3DTBLEND_MODULATEMASK:
			WARN("D3DTBLEND_MODULATEMASK -> D3DTBLEND_MODULATE");
			/* TRU */
		case D3DTBLEND_MODULATE:
		{
#if 0
			if(tex_alpha)
				ctx->state.tmu[tmu].alpha_op = D3DTOP_SELECTARG1;
			else
				ctx->state.tmu[tmu].alpha_op = D3DTOP_SELECTARG2;
#endif
			ctx->state.tmu[tmu].alpha_op = D3DTOP_MODULATE_LEGACY;

			ctx->state.tmu[tmu].alpha_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].alpha_arg2 = D3DTA_CURRENT;

			ctx->state.tmu[tmu].color_op   = D3DTOP_MODULATE;
			ctx->state.tmu[tmu].color_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].color_arg2 = D3DTA_CURRENT;
			break;
		}
		case D3DTBLEND_ADD:
		{
			ctx->state.tmu[tmu].alpha_op   = D3DTOP_SELECTARG2;
			ctx->state.tmu[tmu].alpha_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].alpha_arg2 = D3DTA_CURRENT;

			ctx->state.tmu[tmu].color_op   = D3DTOP_ADD;
			ctx->state.tmu[tmu].color_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].color_arg2 = D3DTA_CURRENT;
			break;
		}
		case D3DTBLEND_MODULATEALPHA:
		{
			ctx->state.tmu[tmu].alpha_op   = D3DTOP_MODULATE;
			ctx->state.tmu[tmu].alpha_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].alpha_arg2 = D3DTA_CURRENT;

			ctx->state.tmu[tmu].color_op   = D3DTOP_MODULATE;
			ctx->state.tmu[tmu].color_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].color_arg2 = D3DTA_CURRENT;
			break;
		}
		case D3DTBLEND_DECALMASK:
			WARN("D3DTBLEND_DECALMASK -> D3DTBLEND_DECAL");
			/* TRU */
		case D3DTBLEND_COPY:
		case D3DTBLEND_DECAL:
		{
			ctx->state.tmu[tmu].alpha_op   = D3DTOP_SELECTARG1;
			ctx->state.tmu[tmu].alpha_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].alpha_arg2 = D3DTA_CURRENT;

			ctx->state.tmu[tmu].color_op   = D3DTOP_SELECTARG1;
			ctx->state.tmu[tmu].color_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].color_arg2 = D3DTA_CURRENT;
			break;
		}
		case D3DTBLEND_DECALALPHA:
		{
			ctx->state.tmu[tmu].alpha_op   = D3DTOP_BLENDTEXTUREALPHA;
			ctx->state.tmu[tmu].alpha_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].alpha_arg2 = D3DTA_CURRENT;

			ctx->state.tmu[tmu].color_op   = D3DTOP_SELECTARG2;
			ctx->state.tmu[tmu].color_arg1 = D3DTA_TEXTURE;
			ctx->state.tmu[tmu].color_arg2 = D3DTA_CURRENT;
			break;
		}
		default:
			WARN("Unknown D3DTEXTUREBLEND state: %d", blend);
			break;
	}
	ctx->state.tmu[tmu].update = TRUE;
}


NUKED_LOCAL void MesaApplyLighting(mesa3d_ctx_t *ctx)
{
	/* JH: I spend lots of time debuging bad lighting situations,
	   but there is simple formula: no normal set, no lighting
	   
	   Update: no, only D3DFVF_XYZRHW means no lighting!
	 */
	if(ctx->state.material.lighting && !ctx->state.vertex.xyzrhw)
	{
		ctx->entry->proc.pglEnable(GL_LIGHTING);
	}
	else
	{
		ctx->entry->proc.pglDisable(GL_LIGHTING);
	}
}

#define TSS_DWORD (*((DWORD*)value))
#define TSS_FLOAT (*((D3DVALUE*)value))

#define RENDERSTATE(_c) case _c: TOPIC("RS", "%s:%02d=0x%X", #_c, tmu, TSS_DWORD);

NUKED_LOCAL void MesaSetTextureState(mesa3d_ctx_t *ctx, int tmu, DWORD state, void *value)
{
	if(tmu >= ctx->tmu_count)
	{
		return;
	}

	BOOL invalid_state = FALSE;
	struct mesa3d_tmustate *ts = &ctx->state.tmu[tmu];

	switch(state)
	{
		/* texture resource */
		RENDERSTATE(D3DTSS_TEXTUREMAP)
			if(ts->active_dxid != TSS_DWORD)
			{
				ts->active_dxid = TSS_DWORD;
				ts->reload = TRUE;
			}
			TOPIC("GLTEXENV", "texture TU%d = %d", tmu, ts->active_dxid);
			break;
		/* D3DTEXTUREOP - per-stage blending controls for color channels */
		RENDERSTATE(D3DTSS_COLOROP)
			ts->color_op = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_COLORARG1)
			ts->color_arg1 = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_COLORARG2)
			ts->color_arg2 = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTEXTUREOP - per-stage blending controls for alpha channel */
		RENDERSTATE(D3DTSS_ALPHAOP)
			ts->alpha_op = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_ALPHAARG1)
			ts->alpha_arg1 = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_ALPHAARG2)
			ts->alpha_arg2 = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DVALUE (bump mapping matrix) */
		RENDERSTATE(D3DTSS_BUMPENVMAT00)
			break;
		/* D3DVALUE (bump mapping matrix) */
		RENDERSTATE(D3DTSS_BUMPENVMAT01)
			break;
		/* D3DVALUE (bump mapping matrix) */
		RENDERSTATE(D3DTSS_BUMPENVMAT10)
			break;
		/* D3DVALUE (bump mapping matrix) */
		RENDERSTATE(D3DTSS_BUMPENVMAT11)
			break;
		/* identifies which set of texture coordinates index this texture */
		RENDERSTATE(D3DTSS_TEXCOORDINDEX)
		{
			// Values, used with D3DTSS_TEXCOORDINDEX, to specify that the vertex data(position
			// and normal in the camera space) should be taken as texture coordinates
			// Low 16 bits are used to specify texture coordinate index, to take the WRAP mode from
			// D3DTSS_TCI_*
			ts->coordindex = TSS_DWORD & 0xFFFF;
			if(ts->coordindex >= 8)
			{
				WARN("Too high TEXCOORDINDEX: %d", ts->coordindex);
				ts->coordindex = 0;
			}

			switch(TSS_DWORD & 0xFFFF0000)
			{
				case D3DTSS_TCI_PASSTHRU:
					/* Use the specified texture coordinates contained within the vertex format.
					   This value resolves to zero. */
					ts->mapping = D3DTSS_TCI_PASSTHRU;
					break;
				case D3DTSS_TCI_CAMERASPACENORMAL:
					/* Use the vertex normal, transformed to camera space, as the input texture
					   coordinates for this stage's texture transformation. */
					ts->mapping = D3DTSS_TCI_CAMERASPACENORMAL;
					break;
				case D3DTSS_TCI_CAMERASPACEPOSITION:
					/* Use the vertex position, transformed to camera space, as the input texture
					   coordinates for this stage's texture transformation. */
					ts->mapping = D3DTSS_TCI_CAMERASPACEPOSITION;
					break;
				case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
					/* Use the reflection vector, transformed to camera space, as the input
					   texture coordinate for this stage's texture transformation. The reflection
					   vector is computed from the input vertex position and normal vector. */
					ts->mapping = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;
					break;
				case D3DTSS_TCI_SPHEREMAP: /* DX8/9 */
					/* Use the specified texture coordinates for sphere mapping. */
					ts->mapping = D3DTSS_TCI_SPHEREMAP;
					break;
				default:
					WARN("Unknown D3DTSS_TCI mode: 0x%X", TSS_DWORD & 0xFFFF0000);
					break;
			}

			ts->update = TRUE;

			TOPIC("MAPPING", "TEXCOORDINDEX 0x%X for unit %d", TSS_DWORD, tmu);
			break;
		}
		/* D3DTEXTUREADDRESS for both coordinates */
		RENDERSTATE(D3DTSS_ADDRESS)
			ts->texaddr_u = (D3DTEXTUREADDRESS)TSS_DWORD;
			ts->texaddr_v = (D3DTEXTUREADDRESS)TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTEXTUREADDRESS for U coordinate */
		RENDERSTATE(D3DTSS_ADDRESSU)
			ts->texaddr_u = (D3DTEXTUREADDRESS)TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DTEXTUREADDRESS for V coordinate */
		RENDERSTATE(D3DTSS_ADDRESSV)
			ts->texaddr_v = (D3DTEXTUREADDRESS)TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DCOLOR */
		RENDERSTATE(D3DTSS_BORDERCOLOR)
		{
			D3DCOLOR c = TSS_DWORD;
			MESA_D3DCOLOR_TO_FV(c, ts->border);
			ts->update = TRUE;
			break;
		}
		/* D3DTEXTUREMAGFILTER filter to use for magnification */
		RENDERSTATE(D3DTSS_MAGFILTER)
			if(ctx->dxif >= MESA_CTX_IF_DX8)
			{
				switch(TSS_DWORD)
				{
					case D3DTEXF_POINT:          ts->dx_mag = D3DTFG_POINT;         break;
					case D3DTEXF_LINEAR:         ts->dx_mag = D3DTFG_LINEAR;        break;
					case D3DTEXF_ANISOTROPIC:    ts->dx_mag = D3DTFG_ANISOTROPIC;   break;
					case D3DTEXF_FLATCUBIC:      ts->dx_mag = D3DTFG_FLATCUBIC;     break;
					case D3DTEXF_GAUSSIANCUBIC:  ts->dx_mag = D3DTFG_GAUSSIANCUBIC; break;
				}
			}
			else
			{
				ts->dx_mag = (D3DTEXTUREMAGFILTER)TSS_DWORD;
			}
			TOPIC("MINMAG", "D3DTSS_MAGFILTER=%d", ts->dx_mag);
			ts->update = TRUE;
			break;
		/* D3DTEXTUREMINFILTER filter to use for minification */
		RENDERSTATE(D3DTSS_MINFILTER)
			if(ctx->dxif >= MESA_CTX_IF_DX8)
			{
				switch(TSS_DWORD)
				{
					case D3DTEXF_POINT:     ts->dx_min = D3DTFN_POINT;       break;
					case D3DTEXF_LINEAR:    ts->dx_min = D3DTFN_LINEAR;      break;
					case D3DTEXF_FLATCUBIC: ts->dx_min = D3DTFN_ANISOTROPIC; break;
				}
			}
			else
			{
				ts->dx_min = (D3DTEXTUREMINFILTER)TSS_DWORD;
			}
			TOPIC("MINMAG", "D3DTSS_MINFILTER=%d", ts->dx_min);
			ts->update = TRUE;
			break;
		/* D3DTEXTUREMIPFILTER filter to use between mipmaps during minification */
		RENDERSTATE(D3DTSS_MIPFILTER)
			if(ctx->dxif >= MESA_CTX_IF_DX8)
			{
				switch(TSS_DWORD)
				{
					case D3DTEXF_NONE:   ts->dx_mip = D3DTFP_NONE;   break;
					case D3DTEXF_POINT:  ts->dx_mip = D3DTFP_POINT;  break;
					case D3DTEXF_LINEAR: ts->dx_mip = D3DTFP_LINEAR; break;
				}
			}
			else
			{
				ts->dx_mip = (D3DTEXTUREMIPFILTER)TSS_DWORD;
			}
			TOPIC("MINMAG", "D3DTSS_MIPFILTER=%d", ts->dx_mip);
			ts->update = TRUE;
			break;
		/* D3DVALUE Mipmap LOD bias */
		RENDERSTATE(D3DTSS_MIPMAPLODBIAS)
			ts->miplodbias = TSS_FLOAT;
			ts->update = TRUE;
			break;
		/* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
		RENDERSTATE(D3DTSS_MAXMIPLEVEL)
			ts->mipmaxlevel = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* DWORD maximum anisotropy */
		RENDERSTATE(D3DTSS_MAXANISOTROPY)
			ts->anisotropy = TSS_DWORD;
			ts->update = TRUE;
			break;
		/* D3DVALUE scale for bump map luminance */
		RENDERSTATE(D3DTSS_BUMPENVLSCALE)
			break;
		/* D3DVALUE offset for bump map luminance */
		RENDERSTATE(D3DTSS_BUMPENVLOFFSET)
			break;
		/* D3DTEXTURETRANSFORMFLAGS controls texture transform (DX7) */
		RENDERSTATE(D3DTSS_TEXTURETRANSFORMFLAGS)
		{
			DWORD tflags = TSS_DWORD;
			TOPIC("MAPPING", "D3DTSS_TEXTURETRANSFORMFLAGS=0x%X", tflags);
			switch(tflags & (D3DTTFF_PROJECTED-1))
			{
				case D3DTTFF_DISABLE:// texture coordinates are passed directly
					ts->coordscalc = 0;
					break;
				case D3DTTFF_COUNT1: // rasterizer should expect 1-D texture coords
					ts->coordscalc = 1;
					break;
				case D3DTTFF_COUNT2: // rasterizer should expect 2-D texture coords
					ts->coordscalc = 2;
					break;
				case D3DTTFF_COUNT3: // rasterizer should expect 3-D texture coords
					ts->coordscalc = 3;
					break;
				case D3DTTFF_COUNT4: // rasterizer should expect 4-D texture coords
					ts->coordscalc = 4;
					break;
			}
			if(tflags & D3DTTFF_PROJECTED)
			{
				ts->projected = TRUE;
			}
			else
			{
				ts->projected = FALSE;
			}

			ts->update = TRUE;
			break;
		}
		RENDERSTATE(D3DTSS_ADDRESSW)
			ts->texaddr_w = (D3DTEXTUREADDRESS)TSS_DWORD;
			ts->update = TRUE;
			break;
		RENDERSTATE(D3DTSS_COLORARG0) /* D3DTA_* third arg for triadic ops */
			ts->color_arg3 = TSS_DWORD;
			ts->update = TRUE;
			break;
		RENDERSTATE(D3DTSS_ALPHAARG0) /* D3DTA_* third arg for triadic ops */
			ts->alpha_arg3 = TSS_DWORD;
			ts->update = TRUE;
			break;
		RENDERSTATE(D3DTSS_RESULTARG) /* D3DTA_* arg for result (CURRENT or TEMP) */
			if(TSS_DWORD == D3DTA_TEMP)
			{
				ts->result_temp = TRUE;
			}
			else
			{
				ts->result_temp = FALSE;
			}
			break;
		RENDERSTATE(D3DTSS_CONSTANT)
			break;
		default:
			WARN("Unknown D3DTSS: state=0x%X, tmu=%d", state, tmu);
			invalid_state = TRUE;
			break;
	} // switch
	
	if(!invalid_state && state < MESA_REC_MAX_TMU_STATE)
	{
		ctx->state.current.tmu[tmu].set[0] |= 1 << state;
		ctx->state.current.tmu[tmu].state[state] = TSS_DWORD;
	}
}

#undef RENDERSTATE

/* from DDK */
#define IS_OVERRIDE(type)       ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)      ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define SET_OVERRIDE(_type) if((_type) < 256){ \
		DWORD wi = (_type) / 32; \
		DWORD bi = (_type) % 32; \
		ctx->state.overrides[wi] |= 1 << bi;}

#define CLEAN_OVERRIDE(_type) if((_type) < 256){ \
		DWORD wi = (_type) / 32; \
		DWORD bi = (_type) % 32; \
		ctx->state.overrides[wi] &= ~(1 << bi);}

#define RETURN_IF_OVERRIDE(_t) do{ \
		DWORD dt = (DWORD)(_t); \
		if(dt < 256){ \
			DWORD wi = dt / 32; \
			DWORD bi = dt % 32; \
			if(ctx->state.overrides[wi] & (1 << bi)){return;} \
		}\
	}while(0)

#define D3DRENDERSTATE_EVICTMANAGEDTEXTURES   61
#define D3DRENDERSTATE_SCENECAPTURE           62
#define D3DRS_DELETERTPATCH                  169
#define D3DRS_MAXVERTEXSHADERINST            196
#define D3DRS_MAXPIXELSHADERINST             197
/*
 * I may miss somewere something but this (undocumented) state
 * is send just before DD flip ...
 * Update (after more investigation): it is part of permedia2 sample driver,
 * but not part of the D3DRENDERSTATE enum and all later documentations/headers.
 * Update II: part of DDK2k3 documentation
 */

#define RENDERSTATE(_c) case _c: TOPIC("RS", "%s=0x%X", #_c, state->dwState);

//NUKED_LOCAL void MesaSetRenderState(mesa3d_ctx_t *ctx, LPD3DSTATE state, LPDWORD RStates)
NUKED_LOCAL void MesaSetRenderState(mesa3d_ctx_t *ctx, LPD3DHAL_DP2RENDERSTATE state, LPDWORD RStates)
{
	D3DRENDERSTATETYPE type = state->RenderState;
	TOPIC("READBACK", "state = %d", type);
	BOOL wrong_state = FALSE;
	
	if(IS_OVERRIDE(type))
	{
		DWORD override = GET_OVERRIDE(type);
		if(state->dwState)
		{
			TOPIC("OVERRIDE", "override = %d SET", override);
			SET_OVERRIDE(override);
		}
		else
		{
			TOPIC("OVERRIDE", "override = %d CLEAN", override);
			CLEAN_OVERRIDE(override);
		}
		
		return;
	}
	
	RETURN_IF_OVERRIDE(type);
	
	if(RStates != NULL)
	{
		RStates[type] = state->dwState;
	}

	mesa3d_entry_t *entry = ctx->entry;

	switch(type)
	{
		RENDERSTATE(D3DRENDERSTATE_TEXTUREHANDLE) /* Texture handle */
		{
			if(ctx->state.tmu[0].active_dxid != state->dwState)
			{
				ctx->state.tmu[0].active_dxid = state->dwState;
				ctx->state.tmu[0].reload = TRUE;
			}
			TOPIC("GLTEXENV", "texture TU%d = %d", 0, ctx->state.tmu[0].active_dxid);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_ANTIALIAS) /* D3DANTIALIASMODE */
		{
			D3DANTIALIASMODE mode = (D3DANTIALIASMODE)state->dwState;
			switch(mode)
			{
				case D3DANTIALIAS_NONE:
					GL_CHECK(entry->proc.pglDisable(GL_MULTISAMPLE));
					break;
				case D3DANTIALIAS_SORTDEPENDENT:
				case D3DANTIALIAS_SORTINDEPENDENT:
				default:
					GL_CHECK(entry->proc.pglEnable(GL_MULTISAMPLE));
					break;
			}
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESS) /* D3DTEXTUREADDRESS	*/
			ctx->state.tmu[0].texaddr_u = state->dwState;
			ctx->state.tmu[0].texaddr_v = state->dwState;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREPERSPECTIVE) /* TRUE for perspective correction */
			ctx->state.texperspective = state->dwState == 0 ? FALSE : TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPU) /* TRUE for wrapping in u */
			if(state->dwState)
			{
				ctx->state.tmu[0].wrap |= D3DWRAPCOORD_0;
			}
			else
			{
				ctx->state.tmu[0].wrap &= ~D3DWRAPCOORD_0;
			}
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPV) /* TRUE for wrapping in v */
			if(state->dwState)
			{
				ctx->state.tmu[0].wrap |= D3DWRAPCOORD_1;
			}
			else
			{
				ctx->state.tmu[0].wrap &= ~D3DWRAPCOORD_1;
			}
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ZENABLE) /* TRUE to enable z test (DX7 = D3DZBUFFERTYPE) */
			TOPIC("DEPTHCONV", "D3DRENDERSTATE_ZENABLE=%d", state->dwState);
			switch(state->dwState)
			{
				case D3DZB_FALSE: /* disabled */
					ctx->state.depth.enabled = FALSE;
					ctx->state.depth.wbuffer = FALSE;
					break;
				case D3DZB_USEW: /* enabled W */
					ctx->state.depth.enabled = TRUE;
					ctx->state.depth.wbuffer = TRUE;
					break;
				case D3DZB_TRUE: /* enabled Z */
				default: /* != FALSE */
					ctx->state.depth.enabled = TRUE;
					ctx->state.depth.wbuffer = FALSE;
					break;
			}
			MesaDepthApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_FILLMODE) /* D3DFILL_MODE	*/
			switch((D3DFILLMODE)state->dwState)
			{
				case D3DFILL_POINT:
					GL_CHECK(entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_POINT));
					break;
				case D3DFILL_WIREFRAME:
					GL_CHECK(entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
					break;
				case D3DFILL_SOLID:
				default:
					GL_CHECK(entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
					break;
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_SHADEMODE) /* D3DSHADEMODE */
			if(ctx->entry->env.lowdetail >= 3)
			{
				GL_CHECK(entry->proc.pglShadeModel(GL_FLAT));
			}
			else
			{
				switch((D3DSHADEMODE)state->dwState)
				{
	        case D3DSHADE_FLAT:
						GL_CHECK(entry->proc.pglShadeModel(GL_FLAT));
						break;
					case D3DSHADE_GOURAUD:
					/* Note from WINE: D3DSHADE_PHONG in practice is the same as D3DSHADE_GOURAUD in D3D */
					case D3DSHADE_PHONG:
					default:
						GL_CHECK(entry->proc.pglShadeModel(GL_SMOOTH));
						break;
				}
			}
			TOPIC("LIGHT", "SHADEMODE=%d", state->dwState);
			break;
		RENDERSTATE(D3DRENDERSTATE_LINEPATTERN) /* D3DLINEPATTERN */
		{
			WORD *pattern = (WORD*)&state->dwState;
			// 0 - wRepeatFactor;
			// 1 - wLinePattern;
			if(pattern[0])
			{
				GL_CHECK(entry->proc.pglEnable(GL_LINE_STIPPLE));
				GL_CHECK(entry->proc.pglLineStipple(pattern[0], pattern[1]));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_LINE_STIPPLE));
			}
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_MONOENABLE) /* TRUE to enable mono rasterization */
			WARN("D3DRENDERSTATE_MONOENABLE=0x%X", state->dwState);
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ROP2) /* ROP2 */
			WARN("D3DRENDERSTATE_ROP2=0x%X", state->dwState);
			ctx->render.rop2 = state->dwState & 0xF;
			break;
		RENDERSTATE(D3DRENDERSTATE_PLANEMASK) /* DWORD physical plane mask */
			WARN("D3DRENDERSTATE_PLANEMASK=0x%X", state->dwState);
			ctx->render.planemask = state->dwState;
			break;
		RENDERSTATE(D3DRENDERSTATE_ZWRITEENABLE) /* TRUE to enable z writes */
			TOPIC("DEPTHCONV", "D3DRENDERSTATE_ZWRITEENABLE=%d", state->dwState);
			if(state->dwState != 0)
			{
				ctx->state.depth.writable = TRUE;
			}
			else
			{
				ctx->state.depth.writable = FALSE;
			}
			MesaDepthApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHATESTENABLE) /* TRUE to enable alpha tests */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHATESTENABLE = 0x%X", state->dwState);
			ctx->state.alpha.enabled = state->dwState != 0 ? TRUE : FALSE;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_LASTPIXEL) /* TRUE for last-pixel on lines */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAG) /* D3DTEXTUREFILTER */
			switch(state->dwState)
			{
		    case D3DFILTER_LINEAR:
		    case D3DFILTER_MIPLINEAR:
		    case D3DFILTER_LINEARMIPNEAREST:
		    case D3DFILTER_LINEARMIPLINEAR:
		    	ctx->state.tmu[0].dx_mag = D3DTFG_LINEAR;
		    	break;
		    case D3DFILTER_NEAREST:
		    case D3DFILTER_MIPNEAREST:
		    default:
		    	ctx->state.tmu[0].dx_mag = D3DTFG_POINT;
		    	break;
			}
			TOPIC("MINMAG", "D3DRENDERSTATE_TEXTUREMAG=%d", state->dwState);
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMIN) /* D3DTEXTUREFILTER */
			switch(state->dwState)
			{
		    case D3DFILTER_LINEAR:
		    	ctx->state.tmu[0].dx_min = D3DTFN_LINEAR;
		    	ctx->state.tmu[0].dx_mip = D3DTFP_NONE;
		    	break;
		    case D3DFILTER_MIPLINEAR:
		    case D3DFILTER_LINEARMIPLINEAR:
		    	ctx->state.tmu[0].dx_min = D3DTFN_LINEAR;
		    	ctx->state.tmu[0].dx_mip = D3DTFP_LINEAR;
		    	break;
		    case D3DFILTER_LINEARMIPNEAREST:
		    	ctx->state.tmu[0].dx_min = D3DTFN_LINEAR;
		    	ctx->state.tmu[0].dx_mip = D3DTFP_POINT;
		    	break;
		    case D3DFILTER_MIPNEAREST:
		    	ctx->state.tmu[0].dx_min = D3DTFN_POINT;
		    	ctx->state.tmu[0].dx_mip = D3DTFP_POINT;
		    case D3DFILTER_NEAREST:
		    	ctx->state.tmu[0].dx_min = D3DTFN_POINT;
		    	ctx->state.tmu[0].dx_mip = D3DTFP_NONE;
		    	break;
			}
			TOPIC("MINMAG", "D3DRENDERSTATE_TEXTUREMIN=%d", state->dwState);
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_SRCBLEND) /* D3DBLEND */
		{
			TOPIC("BLEND", "D3DRENDERSTATE_SRCBLEND = 0x%X", state->dwState);
			D3DBLEND dxblend = (D3DBLEND)state->dwState;
			/* WINE: BLEND_BOTHSRCALPHA and BLEND_BOTHINVSRCALPHA are legacy
			 * source blending values which are still valid up to d3d9. They should
			 * not occur as dest blend values. */
			if(dxblend == D3DBLEND_BOTHSRCALPHA)
			{
				ctx->state.blend.srcRGB = GL_SRC_ALPHA;
				ctx->state.blend.dstRGB = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if(dxblend == D3DBLEND_BOTHINVSRCALPHA)
			{
				ctx->state.blend.srcRGB = GL_ONE_MINUS_SRC_ALPHA;
				ctx->state.blend.dstRGB = GL_SRC_ALPHA;
			}
			else
			{
				ctx->state.blend.srcRGB = GetBlendFactor(dxblend);
			}
			ApplyBlend(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_DESTBLEND) /* D3DBLEND */
			TOPIC("BLEND", "D3DRENDERSTATE_DESTBLEND = 0x%X", state->dwState);
			ctx->state.blend.dstRGB = GetBlendFactor((D3DBLEND)state->dwState);
			ApplyBlend(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAPBLEND) /* D3DTEXTUREBLEND */
			MesaApplyDX5TexBlend(ctx, 0, (D3DTEXTUREBLEND)state->dwState);
			break;
		RENDERSTATE(D3DRENDERSTATE_CULLMODE) /* D3DCULL */
			switch(state->dwState)
			{
				case D3DCULL_NONE:
					ctx->state.cull = GL_NONE;
					break;
				case D3DCULL_CW:
					ctx->state.cull = GL_FRONT;//GL_BACK;
					break;
				case D3DCULL_CCW:
					ctx->state.cull = GL_BACK;//GL_FRONT;
					break;
			}
			MesaSetCull(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_ZFUNC) /* D3DCMPFUNC */
			GL_CHECK(entry->proc.pglDepthFunc(GetGLCmpFunc(state->dwState)));
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAREF) /* D3DFIXED */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAREF = 0x%X", state->dwState);
			ctx->state.alpha.ref = (GLclampf)(state->dwState * MESA_1OVER255); 
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAFUNC) /* D3DCMPFUNC */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAFUNC = 0x%X", state->dwState);
			ctx->state.alpha.func = GetGLCmpFunc(state->dwState);
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_DITHERENABLE) /* TRUE to enable dithering */
			if(state->dwState && ctx->entry->env.lowdetail == 0)
			{
				GL_CHECK(entry->proc.pglEnable(GL_DITHER));
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_DITHER);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHABLENDENABLE) /* TRUE to enable alpha blending */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHABLENDENABLE = 0x%X", state->dwState);
			ctx->state.blend.alphablend = (state->dwState != 0) ? TRUE : FALSE;
			ApplyBlend(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_FOGENABLE) /* TRUE to enable fog */
			ctx->state.fog.enabled = state->dwState == 0 ? FALSE : TRUE;
			TOPIC("FOG", "D3DRENDERSTATE_FOGENABLE=%X", state->dwState);
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_SPECULARENABLE) /* TRUE to enable specular */
			// need EXT_SECONDARY_COLOR
			if(state->dwState)
			{
				ctx->state.specular = TRUE;
			}
			else
			{
				ctx->state.specular = FALSE;
			}
			MesaApplyColorMaterial(ctx);
			TOPIC("LIGHT", "SPECULARENABLE=%d", state->dwState);
			break;
		RENDERSTATE(D3DRENDERSTATE_ZVISIBLE) /* TRUE to enable z checking */
			ctx->state.zvisible = state->dwState;
			break;
		RENDERSTATE(D3DRENDERSTATE_SUBPIXEL) /* TRUE to enable subpixel correction (<= d3d6) */
			break;
		RENDERSTATE(D3DRENDERSTATE_SUBPIXELX) /* TRUE to enable correction in X only (<= d3d6) */
			break;
		RENDERSTATE(D3DRENDERSTATE_STIPPLEDALPHA) /* TRUE to enable stippled alpha */
			break;
		RENDERSTATE(D3DRENDERSTATE_FOGCOLOR) /* D3DCOLOR */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwState;
			MESA_D3DCOLOR_TO_FV(c, ctx->state.fog.color);
			
			TOPIC("FOG", "D3DRENDERSTATE_FOGCOLOR=(%f, %f, %f, %f)",
				ctx->state.fog.color[0],
				ctx->state.fog.color[1],
				ctx->state.fog.color[2],
				ctx->state.fog.color[3]
			);
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLEMODE) /* D3DFOGMODE */
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEMODE=%X", state->dwState);
  		ctx->state.fog.tmode = GetGLFogMode(state->dwState);
  		if(ctx->state.fog.tmode != 0)
  		{
  			ctx->state.fog.vmode = 0;
  		}
  		MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLESTART) /* Fog table start (float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLESTART=%f", state->dvState);
			ctx->state.fog.start = state->dvState;
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLEEND) /* Fog table end (float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEEND=%f", state->dvState);
			ctx->state.fog.end = state->dvState;
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLEDENSITY) /* Fog table density (probably float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEDENSITY=%f", state->dvState);
			ctx->state.fog.density = state->dvState;
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_STIPPLEENABLE) /* TRUE to enable stippling (<= d3d6) */
			if(state->dwState)
			{
				GL_CHECK(entry->proc.pglEnable(GL_POLYGON_STIPPLE));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_POLYGON_STIPPLE));
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_EDGEANTIALIAS) /* TRUE to enable edge antialiasing */
			ctx->state.blend.edgeantialias = (state->dwState == 0) ? FALSE : TRUE;
			ApplyBlend(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORKEYENABLE) /* TRUE to enable source colorkeyed textures */
			ctx->state.tmu[0].colorkey = (state->dwState == 0) ? FALSE : TRUE;
			ctx->state.tmu[0].reload = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_BORDERCOLOR) /* Border color for texturing w/border */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwState;
			MESA_D3DCOLOR_TO_FV(c, ctx->state.tmu[0].border);
			ctx->state.tmu[0].update = TRUE;
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESSU) /* Texture addressing mode for U coordinate */
			ctx->state.tmu[0].texaddr_u = state->dwState;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESSV) /* Texture addressing mode for V coordinate */
			ctx->state.tmu[0].texaddr_v = state->dwState;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_MIPMAPLODBIAS) /* D3DVALUE Mipmap LOD bias */
			ctx->state.tmu[0].miplodbias = state->dwState;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ZBIAS) /* LONG Z bias */
		{
			LONG bias = (LONG)state->dwState;
			if(bias)
			{
				GLfloat fbias = bias;
				GL_CHECK(entry->proc.pglEnable(GL_POLYGON_OFFSET_FILL));
				GL_CHECK(entry->proc.pglPolygonOffset(fbias, fbias));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_POLYGON_OFFSET_FILL));
			}
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_RANGEFOGENABLE) /* Enables range-based fog */
			TOPIC("FOG", "D3DRENDERSTATE_RANGEFOGENABLE=%X", state->dwState);
			ctx->state.fog.range = state->dwState == 0 ? FALSE : TRUE;
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_ANISOTROPY) /* Max. anisotropy. 1 = no anisotropy */
			ctx->state.tmu[0].anisotropy = state->dwState;
			break;
		RENDERSTATE(D3DRENDERSTATE_FLUSHBATCH) /* Explicit flush for DP batching (DX5 Only) */
			/* nop */
			break;
		/* d3d6 */
		RENDERSTATE(D3DRENDERSTATE_STENCILENABLE) /* BOOL enable/disable stenciling */
			ctx->state.stencil.enabled = state->dwState == 0 ? FALSE : TRUE;
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFAIL) /* D3DSTENCILOP to do if stencil test fails */
			ctx->state.stencil.sfail = DXSencilToGL(state->dwState);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILZFAIL) /* D3DSTENCILOP to do if stencil test passes and Z test fails */
			ctx->state.stencil.dpfail = DXSencilToGL(state->dwState);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILPASS) /* D3DSTENCILOP to do if both stencil and Z tests pass */
			ctx->state.stencil.dppass = DXSencilToGL(state->dwState);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFUNC) /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
			ctx->state.stencil.func = GetGLCmpFunc(state->dwState);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILREF) /* Reference value used in stencil test */
			ctx->state.stencil.ref = state->dwState;
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILMASK) /* Mask value used in stencil test */
			ctx->state.stencil.mask = state->dwState;
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILWRITEMASK) /* Write mask applied to values written to stencil buffer */
			ctx->state.stencil.writemask = state->dwState;
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREFACTOR) /* D3DCOLOR used for multi-texture blend */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwState;
			MESA_D3DCOLOR_TO_FV(c, ctx->state.tfactor);
			int i;
			for(i = 0; i < ctx->tmu_count; i++)
			{
				ctx->state.tmu[i].update = TRUE;
			}
			break;
		}
		case D3DRENDERSTATE_STIPPLEPATTERN00 ... D3DRENDERSTATE_STIPPLEPATTERN31:
		{
			ctx->state.stipple[type - D3DRENDERSTATE_STIPPLEPATTERN00] = state->dwState;
			GL_CHECK(entry->proc.pglPolygonStipple((GLubyte*)&ctx->state.stipple[0]));
			break;
		}
		case D3DRENDERSTATE_WRAP0 ... D3DRENDERSTATE_WRAP7: /* wrap for 1-8 texture coord. set */
		{
			ctx->state.tmu[type - D3DRENDERSTATE_WRAP0].wrap = state->dwState;
			ctx->state.tmu[type - D3DRENDERSTATE_WRAP0].update = TRUE;
			break;
		}
		/* d3d7 */
		RENDERSTATE(D3DRENDERSTATE_CLIPPING)
			ctx->state.clipping.enabled = (state->dwState == 0) ? FALSE : TRUE;
			MesaSetClipping(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_LIGHTING)
			if(state->dwState)
			{
				ctx->state.material.lighting = TRUE;
			}
			else
			{
				ctx->state.material.lighting = FALSE;
			}
			MesaApplyLighting(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_EXTENTS)
			WARN("D3DRENDERSTATE_EXTENTS=0x%X", state->dwState);
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENT)
		{
			GLfloat v[4];
			MESA_D3DCOLOR_TO_FV(state->dwState, v);
			TOPIC("LIGHT", "global ambient = (%f %f %f %f)", v[0], v[1], v[2], v[3]);
			GL_CHECK(entry->proc.pglLightModelfv(GL_LIGHT_MODEL_AMBIENT, &v[0]));
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGVERTEXMODE)
			TOPIC("FOG", "D3DRENDERSTATE_FOGVERTEXMODE=0x%X", state->dwState);
			ctx->state.fog.vmode = GetGLFogMode(state->dwState);
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORVERTEX)
			ctx->state.material.color_vertex = state->dwState != 0 ? TRUE : FALSE;
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_LOCALVIEWER)
			GL_CHECK(entry->proc.pglLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, state->dwState == 0 ? 0 : 1));
			break;
		RENDERSTATE(D3DRENDERSTATE_NORMALIZENORMALS)
			if(state->dwState != 0)
			{
				GL_CHECK(entry->proc.pglEnable(GL_NORMALIZE));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_NORMALIZE));
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORKEYBLENDENABLE)
			WARN("D3DRENDERSTATE_COLORKEYBLENDENABLE=0x%X", state->dwState);
			break;
		RENDERSTATE(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE)
			ctx->state.material.diffuse_source = state->dwState;
			TOPIC("DIFFUSE", "ctx->state.material.diffuse_source=%d", state->dwState);
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_SPECULARMATERIALSOURCE)
			ctx->state.material.specular_source = state->dwState;
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENTMATERIALSOURCE)
			ctx->state.material.ambient_source = state->dwState;
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE)
			ctx->state.material.emissive_source = state->dwState;
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_VERTEXBLEND)
			switch(state->dwState)
			{
				case D3DVBLEND_DISABLE: // Disable vertex blending
					ctx->matrix.weight = 0;
					break;
				case D3DVBLEND_1WEIGHT: // blend between 2 matrices
					ctx->matrix.weight = 1;
					break;
				case D3DVBLEND_2WEIGHTS: // blend between 3 matrices
					ctx->matrix.weight = 2;
					break;
				case D3DVBLEND_3WEIGHTS: // blend between 4 matrices
					ctx->matrix.weight = 3;
					break;
			}
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		RENDERSTATE(D3DRENDERSTATE_CLIPPLANEENABLE)
		{
			int i;
			int cnt = NOCRT_MIN(MESA_CLIPS_MAX, ctx->entry->env.num_clips);
			DWORD b = state->dwState;
			for(i = 0; i < cnt; i++)
			{
				ctx->state.clipping.activeplane[i] = ((b & 0x1) == 0) ? FALSE : TRUE;
				b >>= 1;
			}
			MesaSetClipping(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT)
			WARN("D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=0x%X", state->dwState);
			break;
		default:
			switch((DWORD)type)
			{
				RENDERSTATE(D3DRENDERSTATE_SCENECAPTURE)
					TOPIC("READBACK", "D3DRENDERSTATE_SCENECAPTURE %d", state->dwState);
					switch(state->dwState)
					{
						case 0: /* just before flip (end scene) */
							MesaSceneEnd(ctx);
							break;
						case 1: /* after clear (start scene) */
							MesaSceneBegin(ctx);
							break;
					}
					break;
				RENDERSTATE(D3DRS_SOFTWAREVERTEXPROCESSING)
					TRACE("D3DRS_SOFTWAREVERTEXPROCESSING=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_POINTSIZE)
					break;
				RENDERSTATE(D3DRS_POINTSIZE_MIN)
					break;
				RENDERSTATE(D3DRS_POINTSPRITEENABLE)
					break;
				RENDERSTATE(D3DRS_POINTSCALEENABLE)
					break;
				RENDERSTATE(D3DRS_POINTSCALE_A)
					break;
				RENDERSTATE(D3DRS_POINTSCALE_B)
					break;
				RENDERSTATE(D3DRS_POINTSCALE_C)
					break;
				RENDERSTATE(D3DRS_MULTISAMPLEANTIALIAS)
					break;
				RENDERSTATE(D3DRS_MULTISAMPLEMASK)
					break;
				RENDERSTATE(D3DRS_PATCHEDGESTYLE)
					WARN("D3DRS_PATCHEDGESTYLE=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_PATCHSEGMENTS)
					WARN("D3DRS_PATCHSEGMENTS=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_DEBUGMONITORTOKEN)
					TRACE("D3DRS_DEBUGMONITORTOKEN=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_POINTSIZE_MAX)
					break;
				RENDERSTATE(D3DRS_INDEXEDVERTEXBLENDENABLE)
					TRACE("D3DRS_INDEXEDVERTEXBLENDENABLE=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_COLORWRITEENABLE)
					TRACE("D3DRS_COLORWRITEENABLE=0x%X", state->dwState);
					break;
				RENDERSTATE(D3DRS_TWEENFACTOR)
					break;
				RENDERSTATE(D3DRS_BLENDOP)
					TRACE("D3DRS_BLENDOP=0x%X", state->dwState);
					switch(state->dwState)
					{
						case D3DBLENDOP_ADD:
							ctx->state.blend.blendop = GL_FUNC_ADD;
							break;
						case D3DBLENDOP_SUBTRACT:
							ctx->state.blend.blendop = GL_FUNC_SUBTRACT;
							break;
						case D3DBLENDOP_REVSUBTRACT:
							 ctx->state.blend.blendop = GL_FUNC_REVERSE_SUBTRACT;
							break;
						case D3DBLENDOP_MIN:
							ctx->state.blend.blendop = GL_MIN;
							break;
						case D3DBLENDOP_MAX:
							ctx->state.blend.blendop = GL_MAX;
							break;
					}
					ApplyBlend(ctx);
					break;
				RENDERSTATE(D3DRS_POSITIONORDER)
					break;
				RENDERSTATE(D3DRS_NORMALORDER)
					break;
				default:
					wrong_state = TRUE;
					WARN("Unknown render state: %d (0x%X)", type, type);
					break;
			}
			/* NOP */
			break;
	}

	if(!wrong_state && type <= MESA_REC_MAX_STATE)
	{
		DWORD mask_byte = type >> 5; // div 32
		DWORD mask_bit  = 1 << (type & 31);

		ctx->state.current.state[type] = state->dwState;
		ctx->state.current.stateset[mask_byte] |= mask_bit;
	}
}

#undef RENDERSTATE

NUKED_FAST GLenum MesaConvPrimType(D3DPRIMITIVETYPE dx_type)
{
	GLenum gl_type = GL_NOOP; /* GL_POINTS=0, so using some invalid value */
	
	switch(dx_type)
	{
		case D3DPT_POINTLIST:
			gl_type = GL_POINTS;
			break;
		case D3DPT_LINELIST:
			gl_type = GL_LINES;
			break;
		case D3DPT_LINESTRIP:
			gl_type = GL_LINE_STRIP;
			break;
		case D3DPT_TRIANGLELIST:
			gl_type = GL_TRIANGLES;
			break;
		case D3DPT_TRIANGLESTRIP:
			gl_type = GL_TRIANGLE_STRIP;
			break;
		case D3DPT_TRIANGLEFAN:
			gl_type = GL_TRIANGLE_FAN;
			break;
		default:
			/* NOP */
			break;
	}

	return gl_type;
}

NUKED_FAST DWORD MesaConvPrimVertex(D3DPRIMITIVETYPE dx_type, DWORD prim_count)
{
	switch(dx_type)
	{
		case D3DPT_POINTLIST:     return prim_count;
		case D3DPT_LINELIST:      return prim_count*2;
		case D3DPT_LINESTRIP:     return prim_count+1;
		case D3DPT_TRIANGLELIST:  return prim_count*3;
		case D3DPT_TRIANGLESTRIP: return prim_count+2;
		case D3DPT_TRIANGLEFAN:   return prim_count+2;
		default:
			break;
	}

	return 0;
}

#ifdef DEBUG

#define C2S(_e) case _e: return #_e;
static const char *debug_dxtextureop_str(DWORD e)
{
	switch(e)
	{
		C2S(D3DTOP_DISABLE)
		C2S(D3DTOP_SELECTARG1)
		C2S(D3DTOP_SELECTARG2)
		C2S(D3DTOP_MODULATE)
		C2S(D3DTOP_MODULATE_LEGACY)
		C2S(D3DTOP_MODULATE2X)
		C2S(D3DTOP_MODULATE4X)
		C2S(D3DTOP_ADD)
		C2S(D3DTOP_ADDSIGNED)
		C2S(D3DTOP_ADDSIGNED2X)
		C2S(D3DTOP_SUBTRACT)
		C2S(D3DTOP_ADDSMOOTH)
		C2S(D3DTOP_BLENDDIFFUSEALPHA)
		C2S(D3DTOP_BLENDTEXTUREALPHA)
		C2S(D3DTOP_BLENDFACTORALPHA)
		C2S(D3DTOP_BLENDTEXTUREALPHAPM)
		C2S(D3DTOP_BLENDCURRENTALPHA)
		C2S(D3DTOP_PREMODULATE)
		C2S(D3DTOP_MODULATEALPHA_ADDCOLOR)
		C2S(D3DTOP_MODULATECOLOR_ADDALPHA)
		C2S(D3DTOP_MODULATEINVALPHA_ADDCOLOR)
		C2S(D3DTOP_MODULATEINVCOLOR_ADDALPHA)
		C2S(D3DTOP_BUMPENVMAP)
		C2S(D3DTOP_BUMPENVMAPLUMINANCE)
		C2S(D3DTOP_DOTPRODUCT3)
		C2S(D3DTOP_MULTIPLYADD)
		C2S(D3DTOP_LERP)
		default: break;
	}
	return "unknown";
}

#undef C2S

#define C2S(_e) case _e: \
	if(d & D3DTA_ALPHAREPLICATE){ \
		if(d & D3DTA_COMPLEMENT){ \
			return "alpha(1 - " #_e ")"; \
		}else{ \
			return "alpha(" #_e ")"; \
		} \
	}else{ \
		if(d & D3DTA_COMPLEMENT){ \
			return "color(1 - " #_e ")"; \
		}else{ \
			return "color(" #_e ")"; \
		} \
	} break;

static const char *debug_dxcolorarg_str(DWORD d)
{
	switch(d & D3DTA_SELECTMASK)
	{
		C2S(D3DTA_DIFFUSE)
		C2S(D3DTA_CURRENT)
		C2S(D3DTA_TEXTURE)
		C2S(D3DTA_TFACTOR)
		C2S(D3DTA_SPECULAR)
		C2S(D3DTA_TEMP)
//		C2S(D3DTA_CONSTANT)
	}

	return "unknown";
}

#undef C2S

#endif /* DEBUG */

NUKED_INLINE void D3DTA2GL(DWORD dxarg, GLint *gl_src, GLint *gl_op, BOOL active_image, int tmu)
{
	switch(dxarg & D3DTA_SELECTMASK)
	{
		case D3DTA_TEMP:
			WARN("D3DTA_TEMP");
			/* TRU */
		case D3DTA_CURRENT:
			if(tmu == 0)
			{
				*gl_src = GL_PRIMARY_COLOR;
			}
			else
			{
				*gl_src = GL_PREVIOUS;
			}
			break;
		case D3DTA_DIFFUSE:
			*gl_src = GL_PRIMARY_COLOR;
			break;
		case D3DTA_TFACTOR:
		//case D3DTA_CONSTANT:
			*gl_src = GL_CONSTANT_EXT;
			break;
		case D3DTA_SPECULAR: /* not possible, fail to texture */
			WARN("D3DTA_SPECULAR");
			/* TRU */
		case D3DTA_TEXTURE:
		default:
			/* DDK: D3DTA_TEXTURE is a value for a D3DTSS_COLORARG1 or D3DTSS_ALPHAARG1
       * state of any stage, or defaults to 0x0 if no texture is bound to this stage.
       */
			if(active_image)
			{
				*gl_src = GL_TEXTURE;
			}
			else
			{
				//*gl_src = GL_PREVIOUS;
				*gl_src = GL_PRIMARY_COLOR;
			}
			break;
	}
	
	if((dxarg & (D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE)) ==
		(D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE))
	{
		 *gl_op = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if(dxarg & D3DTA_ALPHAREPLICATE)
	{
		*gl_op = GL_SRC_ALPHA;
	}
	else if(dxarg & D3DTA_COMPLEMENT)
	{
		*gl_op = GL_ONE_MINUS_SRC_COLOR;
	}
	else
	{
		*gl_op = GL_SRC_COLOR;
	}
}

NUKED_INLINE void setTexGen(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, int num_coords, BOOL projected)
{
/*	if(projected && num_coords > 0)
	{
		num_coords--;
	}*/

	switch(num_coords)
	{
		case 4:
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_GEN_Q));
			/* thru */
		case 3:
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_GEN_R));
			/* thru */
		case 2:
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_GEN_T));
			/* thru */
		case 1:
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_GEN_S));
			/* thru */
		default:
			break;
	}

	switch(4 - num_coords)
	{
		case 4:
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_GEN_S));
			/* thru */
		case 3:
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_GEN_T));
			/* thru */
		case 2:
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_GEN_R));
			/* thru */
		case 1:
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_GEN_Q));
			/* thru */
		default:
			break;
	}

/*	if(projected)
	{
		if(num_coords > 0)
		{
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_GEN_Q));
		}
		else
		{
			GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_GEN_Q));
		}
	}*/
}

static void ApplyTextureState(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, int tmu)
{
	static const GLfloat s_plane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat t_plane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	static const GLfloat r_plane[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	static const GLfloat q_plane[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	TRACE("ApplyTextureState(..., ..., %d)", tmu);
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));

	struct mesa3d_tmustate *ts = &ctx->state.tmu[tmu];
	BOOL color_key = FALSE;
	GLenum target = GL_TEXTURE_2D;

	switch(ts->mapping)
	{
		case D3DTSS_TCI_CAMERASPACENORMAL:
		case D3DTSS_TCI_CAMERASPACEPOSITION:
		case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
			ts->coordscalc_used = ts->coordscalc;
			break;
		case D3DTSS_TCI_SPHEREMAP:
			ts->coordscalc_used = 2;
			break;
		case D3DTSS_TCI_PASSTHRU:
		default:
			ts->coordscalc_used = 0;
			break;
	}

	ts->active = FALSE;
	
	surfaceex_t *image = NULL;
	
	if(ts->active_dxid)
	{
		image = surfex_get(ctx, ts->active_dxid, TRUE, FALSE);
	}
	
	if(image/* && ctx->state.tmu[0].image*/) /* texturing active only when unit 0 is active */
	{
		if((image->dd->level & DDSURFACE_CUBE_MASK) == 0) /* likely */
		{
			BOOL vertex_coords = ctx->state.vertex.type.texcoords[ts->coordindex] != MESA_VDT_NONE;

			TRACE("vertex_coords = %d (ts->coordindex=%d)", vertex_coords, ts->coordindex);

			if(vertex_coords || ts->coordscalc_used > 0)
			{
				GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
				GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP));
				GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, image->tex->gltex));
				GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, 0));	
				ts->active = TRUE;
			}
		}
		else
		{
			TRACE("vertex_coords = cubemap");
			BOOL vertex_coords = ctx->state.vertex.type.texcoords[ts->coordindex] != MESA_VDT_NONE;

			if(vertex_coords || ts->coordscalc_used > 0)
			{
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
			GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, 0));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, image->tex->gltex));
			target = GL_TEXTURE_CUBE_MAP;
			ts->active = TRUE;
			}
		}

		if(ts->color_op == D3DTOP_DISABLE && ts->alpha_op == D3DTOP_DISABLE)
		{
			ts->active = FALSE;
		}
	}

	if(ts->active)
	{
		if((image->fb->attrs.flags & FBHDA_DD_FLAG_COLORKEY) != 0 && tmu == 0)
		{
			color_key = TRUE;
		}

		/*
		 * texture filtering
		 */
		BOOL use_anisotropic = FALSE;
		GLenum filter_min = GL_NEAREST;
		GLenum filter_mag = GL_NEAREST;
		TRACE("image->mipmap=%d, ts->dx_mip=%d", image->tex->mipmaps, ts->dx_mip);

		if(image->tex->mipmaps && ts->dx_mip != D3DTFP_NONE) /* mipmap */
		{
			GLint maxlevel = image->tex->mipmaps;
			GLint minlevel = ts->mipmaxlevel;
			if(minlevel > maxlevel)
			{
				minlevel = maxlevel;
			}

			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_BASE_LEVEL, minlevel));
			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxlevel));
			GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, ts->miplodbias));

			switch(ts->dx_min)
			{
				case D3DTFN_ANISOTROPIC:
					filter_min = GL_LINEAR_MIPMAP_LINEAR;
					use_anisotropic = TRUE;
					break;
				case D3DTFN_LINEAR:
					switch(ts->dx_mip)
					{
						case D3DTFP_LINEAR:
							filter_min = GL_LINEAR_MIPMAP_LINEAR;
							break;
						case D3DTFP_POINT:
							filter_min = GL_LINEAR_MIPMAP_NEAREST;
							break;
						default:
							WARN("unknown ts->dx_mip = %d", ts->dx_mip);
							break;
					}
					break;
				case D3DTFN_POINT:
					switch(ts->dx_mip)
					{
						case D3DTFP_LINEAR:
							filter_min = GL_NEAREST_MIPMAP_LINEAR;
							break;
						case D3DTFP_POINT:
							filter_min = GL_NEAREST_MIPMAP_NEAREST;
							break;
						default:
							WARN("unknown ts->dx_mip = %d", ts->dx_mip);
							break;
					}
					break;
				default:
					WARN("unknown ts->dx_min = %d (using mipmap)", ts->dx_min);
					break;
			}
		}
		else /* non mipmap */
		{
			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));
			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0));

			switch(ts->dx_min)
			{
				case D3DTFN_ANISOTROPIC:
					use_anisotropic = TRUE;
					filter_min = GL_LINEAR;
					break;
				case D3DTFN_LINEAR:
					filter_min = GL_LINEAR;
					break;
				case D3DTFN_POINT:
					filter_min = GL_NEAREST;
					break;
				default:
					WARN("unknown ts->dx_min = %d (using mipmap)", ts->dx_min);
					break;
			}
		} /* non mipaps */

		switch(ts->dx_mag)
		{
			case D3DTFG_ANISOTROPIC:
				use_anisotropic = TRUE;
				filter_mag = GL_LINEAR;
				break;
			case D3DTFG_LINEAR:
				filter_mag = GL_LINEAR;
				break;
			case D3DTFG_POINT:
				filter_mag = GL_NEAREST;
				break;
			default:
				WARN("unknown ts->dx_mag = %d (using mipmap)", ts->dx_mag);
				break;
		}

		if(entry->env.filter_bug)
		{
			/* There is bug (Mesa 23.1.x) causing that
				GL_LINEAR and GL_NEAREST are ignored for GL_TEXTURE_MIN_FILTER,
				but since GL_TEXTURE_MAX_LEVEL is 0, we can use mipmap filter on
				single level texture.
			 */
			switch(filter_min)
			{
				case GL_LINEAR:
					filter_min = GL_LINEAR_MIPMAP_LINEAR;
					break;
				case GL_NEAREST:
					filter_min = GL_NEAREST_MIPMAP_NEAREST;
					break;
			}
		}

		if(ctx->entry->env.lowdetail >= 2)
		{
			if(filter_min == GL_LINEAR || filter_min == GL_NEAREST)
			{
				filter_min = GL_NEAREST;
			}
			else
			{
				filter_min = GL_NEAREST_MIPMAP_NEAREST;
			}

			if(filter_mag == GL_LINEAR || filter_mag == GL_NEAREST)
			{
				filter_mag = GL_NEAREST;
			}
			else
			{
				filter_mag = GL_NEAREST_MIPMAP_NEAREST;
			}

			use_anisotropic = FALSE;
		}

		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter_min));
		GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter_mag));
		
		//TOPIC("SURFEX", "GLFilter target=%X min=%X mag=%X", target, filter_min, filter_mag);

		/* needs EXT_texture_filter_anisotropic */
		if(entry->env.max_anisotropy > 1)
		{
			GLfloat fanisotropy = 1.0;
			if(use_anisotropic)
				fanisotropy = ts->anisotropy;

			if(fanisotropy < 1.0)
				fanisotropy = 1.0;

			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, fanisotropy));
		}

		switch(ts->mapping)
		{
			case D3DTSS_TCI_CAMERASPACENORMAL:
				TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACENORMAL: %d", tmu, ts->coordscalc);

				MesaSpaceModelviewSet(ctx);
				GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
				MesaSpaceModelviewReset(ctx);

				GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));
				GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));
				GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));

				setTexGen(entry, ctx, 3/*ts->coordscalc*/, ts->projected);
				break;
			case D3DTSS_TCI_CAMERASPACEPOSITION:
				TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACEPOSITION: %d", tmu, ts->coordscalc);

				MesaSpaceModelviewSet(ctx);
				GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
				MesaSpaceModelviewReset(ctx);

				GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
				GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
				GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));

				setTexGen(entry, ctx, 3/*ts->coordscalc*/, ts->projected);
				break;
			case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
				TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR: %d", tmu, ts->coordscalc);

				MesaSpaceModelviewSet(ctx);
				GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
				GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
				MesaSpaceModelviewReset(ctx);

				GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));
				GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));
				GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));

				setTexGen(entry, ctx, 3/*ts->coordscalc*/, ts->projected);
				break;
			case D3DTSS_TCI_SPHEREMAP:
				TOPIC("MAPPING", "(%d)D3DTSS_TCI_SPHEREMAP: %d", tmu, ts->coordscalc);

				GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP));
				GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP));

				setTexGen(entry, ctx, 2, ts->projected);
				break;
			case D3DTSS_TCI_PASSTHRU:
			default:
				TOPIC("MAPPING", "(%d)D3DTSS_TCI_PASSTHRU: %d", tmu, ts->coordscalc);
				setTexGen(entry, ctx, 0, FALSE);
				break;
		}

		if(!ctx->matrix.identity_mode)
		{
			//MesaSpaceModelviewSet(ctx);

			GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
			MesaTMUApplyMatrix(ctx, tmu);

			//MesaSpaceModelviewReset(ctx);
		}
	}
	else /* !image */
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		//GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, 0));
	}
	
	/*
	 * Texture blend
	 */
	GLint color_fn = GL_REPLACE;
	GLint color_arg1_source = GL_TEXTURE;
	GLint color_arg1_op = GL_SRC_COLOR;
	GLint color_arg2_source = GL_PREVIOUS;
	GLint color_arg2_op = GL_SRC_COLOR;

	GLint alpha_fn = GL_REPLACE;
	GLint alpha_arg1_source = GL_TEXTURE;
	GLint alpha_arg1_op = GL_SRC_ALPHA;
	GLint alpha_arg2_source = GL_PREVIOUS;
	GLint alpha_arg2_op = GL_SRC_ALPHA;

	// for GL_INTERPOLATE
	GLint color_arg3_source = GL_CONSTANT;
	GLint color_arg3_op = GL_SRC_COLOR;
	GLint alpha_arg3_source = GL_CONSTANT;
	GLint alpha_arg3_op = GL_SRC_ALPHA;
	
	// for GL_COMBINE4
	GLint color_arg4_source = GL_ZERO;
	GLint color_arg4_op = GL_ONE_MINUS_SRC_COLOR;
	GLint alpha_arg4_source = GL_ZERO;
	GLint alpha_arg4_op = GL_ONE_MINUS_SRC_ALPHA;

	D3DTA2GL(ts->color_arg1,                      &color_arg1_source, &color_arg1_op, ts->active, tmu);
	D3DTA2GL(ts->color_arg2,                      &color_arg2_source, &color_arg2_op, ts->active, tmu);
	D3DTA2GL(ts->color_arg3,                      &color_arg3_source, &color_arg3_op, ts->active, tmu);
	D3DTA2GL(ts->alpha_arg1|D3DTA_ALPHAREPLICATE, &alpha_arg1_source, &alpha_arg1_op, ts->active, tmu);
	D3DTA2GL(ts->alpha_arg2|D3DTA_ALPHAREPLICATE, &alpha_arg2_source, &alpha_arg2_op, ts->active, tmu);
	D3DTA2GL(ts->alpha_arg3|D3DTA_ALPHAREPLICATE, &alpha_arg3_source, &alpha_arg3_op, ts->active, tmu);

	GLfloat color_mult = 1.0f;
	GLfloat alpha_mult = 1.0f;
	
	BOOL use_nv4 = TRUE;

	switch(ts->color_op)
	{
		case D3DTOP_DISABLE:
			TOPIC("TEXENV", "color D3DTOP_DISABLE");
			if(!use_nv4)
			{
				color_fn = GL_REPLACE;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
			}
			color_arg1_source = GL_PREVIOUS; //GL_CONSTANT;
			color_arg1_op = GL_SRC_COLOR;
			break;
		case D3DTOP_SELECTARG1:
			TOPIC("TEXENV", "color D3DTOP_SELECTARG1");
			if(!use_nv4)
			{
				color_fn = GL_REPLACE;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
			}
			break;
		case D3DTOP_SELECTARG2:
			TOPIC("TEXENV", "color D3DTOP_SELECTARG2");
			if(!use_nv4)
			{
				color_fn = GL_REPLACE;
				color_arg1_source = color_arg2_source;
				color_arg1_op = color_arg2_op;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg1_source = GL_ZERO;
				color_arg1_op     = GL_ONE_MINUS_SRC_COLOR;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
			}
			break;
		case D3DTOP_MODULATE_LEGACY:
		case D3DTOP_MODULATE:
			TOPIC("TEXENV", "color D3DTOP_MODULATE");
			if(!use_nv4)
			{
				color_fn = GL_MODULATE;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
				
				if(tmu == 0 && color_arg1_source == GL_PRIMARY_COLOR && color_arg2_source == GL_PRIMARY_COLOR)
				{
					color_arg2_source = GL_ZERO;
					color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
				}
			}
			break;
		case D3DTOP_MODULATE2X:
			TOPIC("TEXENV", "color D3DTOP_MODULATE2X");
			if(!use_nv4)
			{
				color_fn = GL_MODULATE;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
			}
			color_mult = 2.0f;
			break;
		case D3DTOP_MODULATE4X:
			TOPIC("TEXENV", "color D3DTOP_MODULATE4X");
			if(!use_nv4)
			{
				color_fn = GL_MODULATE;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = GL_ZERO;
				color_arg3_op     = GL_SRC_COLOR;
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_SRC_COLOR;
			}
			color_mult = 4.0f;
			break;
		case D3DTOP_ADD:
			TOPIC("TEXENV", "color D3DTOP_ADD");
			color_fn = GL_ADD;
			if(use_nv4)
			{
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
			}
			break;
		case D3DTOP_ADDSIGNED:
			TOPIC("TEXENV", "color D3DTOP_ADDSIGNED");
			color_fn = GL_ADD_SIGNED;
			if(use_nv4)
			{
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
			}
			break;
		case D3DTOP_ADDSIGNED2X:
			TOPIC("TEXENV", "color D3DTOP_ADDSIGNED2X");
			color_fn = GL_ADD_SIGNED;
			if(use_nv4)
			{
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
			}
			color_mult = 2.0f;
			break;
		case D3DTOP_SUBTRACT:
			TOPIC("TEXENV", "color D3DTOP_SUBTRACT");
			color_fn = GL_SUBTRACT;
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			TOPIC("TEXENV", "color D3DTOP_BLENDDIFFUSEALPHA");
			if(!use_nv4)
			{
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_PRIMARY_COLOR;
				color_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_PRIMARY_COLOR;
				color_arg2_op     = GL_SRC_ALPHA;
				color_arg4_source = GL_PRIMARY_COLOR;
				color_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDTEXTUREALPHA: // Arg1 x (alpha) + Arg2 (1-alpha)
			TOPIC("TEXENV", "color D3DTOP_BLENDTEXTUREALPHA");
			if(!use_nv4)
			{
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_TEXTURE;
				color_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_TEXTURE;
				color_arg2_op     = GL_SRC_ALPHA;
				color_arg4_source = GL_TEXTURE;
				color_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDFACTORALPHA: // Arg1 x (alpha) + Arg2 x (1-alpha)
			TOPIC("TEXENV", "color D3DTOP_BLENDFACTORALPHA");
			if(!use_nv4)
			{
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_CONSTANT;
				color_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_CONSTANT;
				color_arg2_op     = GL_SRC_ALPHA;
				color_arg4_source = GL_CONSTANT;
				color_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			TOPIC("TEXENV", "color D3DTOP_BLENDCURRENTALPHA");
			if(!use_nv4)
			{
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_PREVIOUS;
				color_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_PREVIOUS;
				color_arg2_op     = GL_SRC_ALPHA;
				color_arg4_source = GL_PREVIOUS;
				color_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_DOTPRODUCT3:
			TOPIC("TEXENV", "color D3DTOP_DOTPRODUCT3");
			color_fn = GL_DOT3_RGBA;
			break;
		case D3DTOP_LERP:
			TOPIC("TEXENV", "color D3DTOP_LERP");
			color_fn = GL_INTERPOLATE;
			break;
		/*
				NOTE: for another states we can use this:
					https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_env_combine4.txt
		*/
		case D3DTOP_MODULATEALPHA_ADDCOLOR: // Arg1.RGB + Arg1.A*Arg2.RGB
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_MODULATEALPHA_ADDCOLOR");
				color_fn = GL_ADD;
				color_arg4_source = color_arg2_source;
				color_arg4_op     = color_arg2_op;
				color_arg3_source = color_arg1_source;
				color_arg3_op     = color_arg1_op;
				switch(color_arg1_op)
				{
					case GL_SRC_COLOR:           color_arg3_op = GL_SRC_ALPHA;           break;
					case GL_ONE_MINUS_SRC_COLOR: color_arg3_op = GL_ONE_MINUS_SRC_ALPHA; break;
				}
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
				break;
			}
			/* TRU */
		case D3DTOP_MODULATEINVALPHA_ADDCOLOR: // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_MODULATEINVALPHA_ADDCOLOR");
				color_fn = GL_ADD;
				color_arg3_source = color_arg1_source;
				color_arg3_op     = color_arg1_op;
				switch(color_arg1_op)
				{
					case GL_SRC_COLOR:
						color_arg1_op = GL_ONE_MINUS_SRC_COLOR;
						color_arg3_op = GL_SRC_ALPHA;
						break;
					case GL_ONE_MINUS_SRC_COLOR:
						color_arg1_op = GL_SRC_COLOR;
						color_arg3_op = GL_ONE_MINUS_SRC_ALPHA;
						break;
					case GL_SRC_ALPHA:
						color_arg1_op = GL_ONE_MINUS_SRC_ALPHA;
						break;
					case GL_ONE_MINUS_SRC_ALPHA:
						color_arg1_op = GL_SRC_ALPHA;
						break;
				}
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_ONE_MINUS_SRC_COLOR;
				break;
			}
			/* TRU */
		case D3DTOP_MODULATECOLOR_ADDALPHA: // Arg1.RGB*Arg2.RGB + Arg1.A
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_MODULATECOLOR_ADDALPHA");
				color_fn          = GL_ADD;
				color_arg3_source = color_arg1_source;
				color_arg3_op     = color_arg1_op;
				switch(color_arg3_op)
				{
					case GL_SRC_COLOR:           color_arg3_op = GL_SRC_ALPHA;           break;
					case GL_ONE_MINUS_SRC_COLOR: color_arg3_op = GL_ONE_MINUS_SRC_ALPHA; break;
				}
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_ONE_MINUS_SRC_COLOR;
				break;
			}
			/* TRU */
		case D3DTOP_MODULATEINVCOLOR_ADDALPHA: // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_MODULATEINVCOLOR_ADDALPHA");
				color_fn          = GL_ADD;
				color_arg3_source = color_arg1_source;
				color_arg3_op     = color_arg1_op;
				switch(color_arg1_op)
				{
					case GL_SRC_COLOR:
						color_arg1_op = GL_ONE_MINUS_SRC_COLOR;
						color_arg3_op = GL_SRC_ALPHA;
						break;
					case GL_ONE_MINUS_SRC_COLOR:
						color_arg1_op = GL_SRC_COLOR;
						color_arg3_op = GL_ONE_MINUS_SRC_ALPHA;
						break;
					case GL_SRC_ALPHA:
						color_arg1_op = GL_ONE_MINUS_SRC_ALPHA;
						break;
					case GL_ONE_MINUS_SRC_ALPHA:
						color_arg1_op = GL_SRC_ALPHA;
						break;
				}
				color_arg4_source = GL_ZERO;
				color_arg4_op     = GL_ONE_MINUS_SRC_COLOR;
				break;
			}
			/* TRU */
		case D3DTOP_ADDSMOOTH: // = Arg1 + (1-Arg1)*Arg2
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_ADDSMOOTH");
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = color_arg1_source;
				switch(color_arg1_op)
				{
					case GL_SRC_COLOR:           color_arg2_op = GL_ONE_MINUS_SRC_COLOR; break;
					case GL_ONE_MINUS_SRC_COLOR: color_arg2_op = GL_SRC_COLOR;           break;
					case GL_SRC_ALPHA:           color_arg2_op = GL_ONE_MINUS_SRC_ALPHA; break;
					case GL_ONE_MINUS_SRC_ALPHA: color_arg2_op = GL_SRC_ALPHA;           break;
				}
				break;
			}
			/* TRU */
		case D3DTOP_PREMODULATE:
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_PREMODULATE");
				color_fn = GL_MODULATE;
				color_arg1_source = GL_TEXTURE0 + tmu;
				color_arg1_op = GL_SRC_COLOR;
				color_arg2_source = GL_TEXTURE0 + tmu + 1;
				color_arg2_op = GL_SRC_COLOR;
				break;
			}
			/* TRU */
		case D3DTOP_BLENDTEXTUREALPHAPM: // Arg1 + Arg2 x (1-Alpha)
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_BLENDTEXTUREALPHAPM");
				color_fn = GL_ADD;
				color_arg3_source = color_arg2_source;
				color_arg3_op     = color_arg2_op;
				color_arg2_source = GL_ZERO;
				color_arg2_op     = GL_ONE_MINUS_SRC_COLOR;
				color_arg4_source = GL_TEXTURE;
				color_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			/* TRU */
		case D3DTOP_MULTIPLYADD: // (Arg1) * Arg2 + (1-Arg1) * Arg3.
			if(use_nv4)
			{
				TOPIC("TEXENV", "color D3DTOP_MULTIPLYADD");
				color_fn = GL_ADD;
				color_arg4_source = color_arg1_source;
				switch(color_arg1_op)
				{
					case GL_SRC_COLOR:           color_arg4_op = GL_ONE_MINUS_SRC_COLOR; break;
					case GL_ONE_MINUS_SRC_COLOR: color_arg4_op = GL_SRC_COLOR;           break;
					case GL_SRC_ALPHA:           color_arg4_op = GL_ONE_MINUS_SRC_ALPHA; break;
					case GL_ONE_MINUS_SRC_ALPHA: color_arg4_op = GL_SRC_ALPHA;           break;
				}
				break;
			}
			/* TRU */
		default:
			WARN("Unknown color texture blend operation: %d, TMU: %d", ts->color_op, tmu);
			break;
	}

	switch(ts->alpha_op)
	{
		case D3DTOP_DISABLE:
			TOPIC("TEXENV", "alpha D3DTOP_DISABLE");
			if(!use_nv4)
			{
				alpha_fn = GL_REPLACE;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
			}
			alpha_arg1_source = GL_PREVIOUS;
			alpha_arg1_op = GL_SRC_ALPHA;
			break;
		case D3DTOP_SELECTARG1:
			TOPIC("TEXENV", "alpha D3DTOP_SELECTARG1");
			if(!use_nv4)
			{
				alpha_fn = GL_REPLACE;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
			}
			break;
		case D3DTOP_SELECTARG2:
			TOPIC("TEXENV", "alpha D3DTOP_SELECTARG2");
			if(!use_nv4)
			{
				alpha_fn = GL_REPLACE;
				alpha_arg1_source = alpha_arg2_source;
				alpha_arg1_op = alpha_arg2_op;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg1_source = GL_ZERO;
				alpha_arg1_op     = GL_ONE_MINUS_SRC_ALPHA;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
			}
			break;
		case D3DTOP_MODULATE_LEGACY:
		case D3DTOP_MODULATE:
			TOPIC("TEXENV", "alpha D3DTOP_MODULATE");
			if(!use_nv4)
			{
				alpha_fn = GL_MODULATE;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
				
				if(tmu == 0 && alpha_arg1_source == GL_PRIMARY_COLOR && alpha_arg2_source == GL_PRIMARY_COLOR)
				{
					alpha_arg2_source = GL_ZERO;
					alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
				}
			}
			break;
		case D3DTOP_MODULATE2X:
			TOPIC("TEXENV", "alpha D3DTOP_MODULATE2X");
			if(!use_nv4)
			{
				alpha_fn = GL_MODULATE;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
			}
			alpha_mult = 2.0f;
			break;
		case D3DTOP_MODULATE4X:
			TOPIC("TEXENV", "alpha D3DTOP_MODULATE4X");
			if(!use_nv4)
			{
				alpha_fn = GL_MODULATE;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = GL_ZERO;
				alpha_arg3_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_ZERO;
				alpha_arg4_op     = GL_SRC_ALPHA;
			}
			alpha_mult = 4.0f;
			break;
		case D3DTOP_ADD:
			TOPIC("TEXENV", "alpha D3DTOP_ADD");
			alpha_fn = GL_ADD;
			if(use_nv4)
			{
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_ADDSIGNED:
			TOPIC("TEXENV", "alpha D3DTOP_ADDSIGNED");
			alpha_fn = GL_ADD_SIGNED;
			if(use_nv4)
			{
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_ADDSIGNED2X:
			TOPIC("TEXENV", "alpha D3DTOP_ADDSIGNED2X");
			alpha_fn = GL_ADD_SIGNED;
			if(use_nv4)
			{
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			alpha_mult = 2.0f;
			break;
		case D3DTOP_SUBTRACT:
			TOPIC("TEXENV", "alpha D3DTOP_SUBTRACT");
			alpha_fn = GL_SUBTRACT;
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			TOPIC("TEXENV", "alpha D3DTOP_BLENDDIFFUSEALPHA");
			if(!use_nv4)
			{
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_PRIMARY_COLOR;
				alpha_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_PRIMARY_COLOR;
				alpha_arg2_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_PRIMARY_COLOR;
				alpha_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			TOPIC("TEXENV", "alpha D3DTOP_BLENDTEXTUREALPHA");
			if(use_nv4)
			{
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_TEXTURE;
				alpha_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_TEXTURE;
				alpha_arg2_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_TEXTURE;
				alpha_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDFACTORALPHA:
			TOPIC("TEXENV", "alpha D3DTOP_BLENDFACTORALPHA");
			if(!use_nv4)
			{
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_CONSTANT;
				alpha_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_CONSTANT;
				alpha_arg2_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_CONSTANT;
				alpha_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			TOPIC("TEXENV", "alpha D3DTOP_BLENDCURRENTALPHA");
			if(!use_nv4)
			{
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_PREVIOUS;
				alpha_arg3_op = GL_SRC_ALPHA;
			}
			else
			{
				alpha_fn = GL_ADD;
				alpha_arg3_source = color_arg2_source;
				alpha_arg3_op     = color_arg2_op;
				alpha_arg2_source = GL_PREVIOUS;
				alpha_arg2_op     = GL_SRC_ALPHA;
				alpha_arg4_source = GL_PREVIOUS;
				alpha_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
			}
			break;
		case D3DTOP_DOTPRODUCT3:
			TOPIC("TEXENV", "alpha D3DTOP_DOTPRODUCT3");
			alpha_fn = GL_DOT3_RGBA;
			break;
		case D3DTOP_LERP:
			TOPIC("TEXENV", "alpha D3DTOP_LERP");
			alpha_fn = GL_INTERPOLATE;
			break;
		case D3DTOP_ADDSMOOTH: // = Arg1 + (1-Arg1)*Arg2
			if(use_nv4)
			{
				TOPIC("TEXENV", "alpha D3DTOP_ADDSMOOTH");
				alpha_fn = GL_ADD;
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = alpha_arg1_source;
				switch(alpha_arg1_op)
				{
					case GL_SRC_ALPHA:           alpha_arg2_op = GL_ONE_MINUS_SRC_ALPHA; break;
					case GL_ONE_MINUS_SRC_ALPHA: alpha_arg2_op = GL_SRC_ALPHA;           break;
				}
				break;
			}
			/* TRU */
		case D3DTOP_PREMODULATE:
			if(use_nv4)
			{
				TOPIC("TEXENV", "alpha D3DTOP_PREMODULATE");
				alpha_fn = GL_MODULATE;
				alpha_arg1_source = GL_TEXTURE0 + tmu;
				alpha_arg1_op = GL_SRC_ALPHA;
				alpha_arg2_source = GL_TEXTURE0 + tmu + 1;
				alpha_arg2_op = GL_SRC_ALPHA;
				break;
			}
			/* TRU */
		case D3DTOP_BLENDTEXTUREALPHAPM: // Arg1 + Arg2 x (1-Alpha)
			if(use_nv4)
			{
				TOPIC("TEXENV", "alpha D3DTOP_BLENDTEXTUREALPHAPM");
				alpha_fn = GL_ADD;
				alpha_arg3_source = alpha_arg2_source;
				alpha_arg3_op     = alpha_arg2_op;
				alpha_arg2_source = GL_ZERO;
				alpha_arg2_op     = GL_ONE_MINUS_SRC_ALPHA;
				alpha_arg4_source = GL_PREVIOUS;
				alpha_arg4_op     = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			/* TRU */
		case D3DTOP_MULTIPLYADD: // (Arg1) * Arg2 + (1-Arg1) * Arg3.
			if(use_nv4)
			{
				TOPIC("TEXENV", "alpha D3DTOP_MULTIPLYADD");
				alpha_fn = GL_ADD;
				alpha_arg4_source = alpha_arg1_source;
				switch(alpha_arg1_op)
				{
					case GL_SRC_ALPHA:           alpha_arg4_op = GL_ONE_MINUS_SRC_ALPHA; break;
					case GL_ONE_MINUS_SRC_ALPHA: alpha_arg4_op = GL_SRC_ALPHA;           break;
				}
				break;
			}
			/* TRU */
		default:
			WARN("Unknown alpha texture blend operation: %d, TMU: %d", ts->color_op, tmu);
			break;
	}

	TOPIC("GLTEXENV", "env setting for TU: %d", tmu);
	
	if(use_nv4)
	{
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, %s)", dbg_glenum_name(GL_COMBINE4_NV));
	}
	else
	{
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, %s)", dbg_glenum_name(GL_COMBINE));
	}
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, color_fn));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, alpha_fn));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, %s)", dbg_glenum_name(color_fn));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, %s)", dbg_glenum_name(alpha_fn));

	GL_CHECK(entry->proc.pglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &ctx->state.tfactor[0]));
	TOPIC("GLTEXENV", "glTexEnfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, [%f %f %f %f])",
		ctx->state.tfactor[0], ctx->state.tfactor[1], ctx->state.tfactor[2], ctx->state.tfactor[3]);

	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, color_arg1_source));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, color_arg1_op));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, color_arg2_source));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, color_arg2_op));

	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, %s)", dbg_glenum_name(color_arg1_source));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, %s)", dbg_glenum_name(color_arg1_op));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, %s)", dbg_glenum_name(color_arg2_source));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, %s)", dbg_glenum_name(color_arg2_op));

	if(color_fn == GL_INTERPOLATE || use_nv4)
	{
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, color_arg3_source));
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, color_arg3_op));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, %s)", dbg_glenum_name(color_arg3_source));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, %s)", dbg_glenum_name(color_arg3_op));
	}

	if(use_nv4)
	{
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, color_arg4_source));
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, color_arg4_op));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_RGB_NV, %s)", dbg_glenum_name(color_arg4_source));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_RGB_NV, %s)", dbg_glenum_name(color_arg4_op));
	}

	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, alpha_arg1_source));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, alpha_arg1_op));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, alpha_arg2_source));
	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, alpha_arg2_op));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, %s)", dbg_glenum_name(alpha_arg1_source));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, %s)", dbg_glenum_name(alpha_arg1_op));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, %s)", dbg_glenum_name(alpha_arg2_source));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, %s)", dbg_glenum_name(alpha_arg2_op));

	if(alpha_fn == GL_INTERPOLATE || use_nv4)
	{
	 	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_ALPHA, alpha_arg3_source));
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, alpha_arg3_op));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_ALPHA, %s)", dbg_glenum_name(alpha_arg3_source));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, %s)", dbg_glenum_name(alpha_arg3_op));
	}

	if(use_nv4)
	{
	 	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, alpha_arg4_source));
		GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, alpha_arg4_op));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE3_ALPHA_NV, %s)", dbg_glenum_name(alpha_arg4_source));
		TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND3_ALPHA_NV, %s)", dbg_glenum_name(alpha_arg4_op));
	}

	GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,   color_mult));
	GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, alpha_mult));
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, %d)",   color_mult);
	TOPIC("GLTEXENV", "glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, %d)", alpha_mult);

	TOPIC("TEXENV2", "#%d color: %s (%s, %s, %s)", tmu,
		debug_dxtextureop_str(ts->color_op),
		debug_dxcolorarg_str(ts->color_arg1),
		debug_dxcolorarg_str(ts->color_arg2),
		debug_dxcolorarg_str(ts->color_arg3));
	TOPIC("TEXENV2", "#%d alpha: %s (%s, %s, %s)", tmu,
		debug_dxtextureop_str(ts->alpha_op),
		debug_dxcolorarg_str(ts->alpha_arg1),
		debug_dxcolorarg_str(ts->alpha_arg2),
		debug_dxcolorarg_str(ts->alpha_arg3));
	/*
	 * Texture addressing
	 */
	if(ts->active_dxid)
	{
		if(ts->texaddr_u == D3DTADDRESS_BORDER ||
			ts->texaddr_v == D3DTADDRESS_BORDER ||
			ts->texaddr_w == D3DTADDRESS_BORDER)
		{
			GL_CHECK(entry->proc.pglTexParameterfv(target,
				GL_TEXTURE_BORDER_COLOR, &(ts->border[0])));
		}
		
		switch((DWORD)ts->texaddr_u)
		{
			case D3DTADDRESS_MIRROR:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
				break;
			case D3DTADDRESS_CLAMP:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_BORDER:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
				break;
			case D3DTADDRESS_MIRRORONCE:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_WRAP:
			default:
				if(ts->wrap & D3DWRAPCOORD_0)
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
				}
				else
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
				}
				break;
		}
	
		switch((DWORD)ts->texaddr_v)
		{
			case D3DTADDRESS_MIRROR:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
				break;
			case D3DTADDRESS_CLAMP:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_BORDER:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
				break;
			case D3DTADDRESS_MIRRORONCE:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_WRAP:
			default:
				if(ts->wrap & D3DWRAPCOORD_1)
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
				}
				else
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
				}
				break;
		}

		switch((DWORD)ts->texaddr_w)
		{
			case D3DTADDRESS_MIRROR:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT));
				break;
			case D3DTADDRESS_CLAMP:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_BORDER:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER));
				break;
			case D3DTADDRESS_MIRRORONCE:
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_MIRROR_CLAMP_TO_EDGE));
				break;
			case D3DTADDRESS_WRAP:
			default:
				if(ts->wrap & D3DWRAPCOORD_2)
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
				}
				else
				{
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT));
				}
				break;
		}
	}
	
	if(tmu == 0)
	{
		if(ctx->state.alpha.enabled || color_key)
		{
			ctx->entry->proc.pglEnable(GL_ALPHA_TEST);
			
			if(color_key)
			{
				ctx->entry->proc.pglAlphaFunc(GL_NOTEQUAL, 0.0f);
			}
			else
			{
				ctx->entry->proc.pglAlphaFunc(ctx->state.alpha.func, ctx->state.alpha.ref);
			}
		}
		else
		{
			ctx->entry->proc.pglDisable(GL_ALPHA_TEST);
		}
	}
}

NUKED_LOCAL void MesaDrawRefreshState(mesa3d_ctx_t *ctx)
{
	int i;
	for(i = 0; i < ctx->tmu_count; i++)
	{
		if(ctx->state.tmu[i].reload)
		{
			MesaBufferTextureLoad(ctx, ctx->state.tmu[i].active_dxid, i, FALSE);
			
			ctx->state.tmu[i].update = TRUE;
			//ctx->state.tmu[i].move   = TRUE;
			ctx->state.tmu[i].reload = FALSE;
		}

		if(ctx->state.tmu[i].update)
		{
			ApplyTextureState(ctx->entry, ctx, i);
			ctx->state.tmu[i].update = FALSE;
		}
		
		/*if(ctx->state.tmu[i].move)
		{
			ctx->state.tmu[i].move = FALSE;
		}*/
	}
}

NUKED_LOCAL void MesaDraw5(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype, LPVOID vertices, DWORD verticesCnt)
{
	mesa3d_entry_t *entry = ctx->entry;

//	MesaDrawSetSurfaces(ctx);

	GLenum gl_ptype = MesaConvPrimType(dx_ptype);

	if(gl_ptype != GL_NOOP)
	{
		TOPIC("GL", "glBegin(%d)", gl_ptype);
		entry->proc.pglBegin(gl_ptype);
		switch(vtype)
		{
			case D3DVT_VERTEX:
			{
				LPD3DVERTEX vptr = vertices;
				while(verticesCnt--)
				{
					MesaDrawVertex(ctx, vptr);
					vptr++;
				}
				break;
			}
    	case D3DVT_LVERTEX:
    	{
    		LPD3DLVERTEX vptr = vertices;
				while(verticesCnt--)
				{
					MesaDrawLVertex(ctx, vptr);
					vptr++;
				}
    		break;
    	}
    	case D3DVT_TLVERTEX:
    	default:
    	{
				LPD3DTLVERTEX vptr = vertices;
				while(verticesCnt--)
				{
					MesaDrawTLVertex(ctx, vptr);
					vptr++;
				}
				break;
			}
		}
		GL_CHECK(entry->proc.pglEnd());
		
		ctx->render.dirty = TRUE;
		ctx->render.zdirty = TRUE;
	}
}

NUKED_LOCAL void MesaDraw3(mesa3d_ctx_t *ctx, DWORD op, void *prim, LPBYTE vertices)
{
	mesa3d_entry_t *entry = ctx->entry;
	LPD3DTLVERTEX vertex = (LPD3DTLVERTEX)vertices;
	int i;

	switch(op)
	{
		case D3DOP_POINT:
			LPD3DPOINT point = (LPD3DPOINT)prim;
			entry->proc.pglBegin(GL_POINTS);
			for(i = 0; i < point->wCount; i++)
			{
				MesaDrawTLVertex(ctx, &vertex[point->wFirst+i]);
			}
			entry->proc.pglEnd();
			break;
		case D3DOP_SPAN:
			break;
		case D3DOP_LINE:
			LPD3DSPAN span = (LPD3DSPAN)prim;
			entry->proc.pglBegin(GL_LINES);
			for(i = 0; i < span->wCount; i++)
			{
				MesaDrawTLVertex(ctx, &vertex[span->wFirst+i]);
			}
			entry->proc.pglEnd();
			break;
		case D3DOP_TRIANGLE:
			LPD3DTRIANGLE triPtr = (LPD3DTRIANGLE)prim;
			entry->proc.pglBegin(GL_TRIANGLES);
			MesaDrawTLVertex(ctx, &vertex[triPtr->v1]);
			MesaDrawTLVertex(ctx, &vertex[triPtr->v2]);
			MesaDrawTLVertex(ctx, &vertex[triPtr->v3]);
			entry->proc.pglEnd();
			break;
	}
	
	ctx->render.dirty = TRUE;
	ctx->render.zdirty = TRUE;
}

NUKED_LOCAL void MesaDraw5Index(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype,
	LPVOID vertices, DWORD verticesCnt,
	LPWORD indices, DWORD indicesCnt)
{
	mesa3d_entry_t *entry = ctx->entry;
	DWORD i;
	
//	MesaDrawSetSurfaces(ctx);

	GLenum gl_ptype = MesaConvPrimType(dx_ptype);

	if(gl_ptype != GL_NOOP)
	{
		TOPIC("GL", "glBegin(%d)", gl_ptype);
		entry->proc.pglBegin(gl_ptype);

		switch(vtype)
		{
			case D3DVT_VERTEX:
				for(i = 0; i < indicesCnt; i++)
				{
					LPD3DVERTEX vptr = ((LPD3DVERTEX)vertices) + indices[i];
					MesaDrawVertex(ctx, vptr);
				}
				break;
    	case D3DVT_LVERTEX:
				for(i = 0; i < indicesCnt; i++)
				{
					LPD3DLVERTEX vptr = ((LPD3DLVERTEX)vertices) + indices[i];
					MesaDrawLVertex(ctx, vptr);
				}
    		break;
    	case D3DVT_TLVERTEX:
    	default:
				for(i = 0; i < indicesCnt; i++)
				{
					LPD3DTLVERTEX vptr = ((LPD3DTLVERTEX)vertices) + indices[i];
					MesaDrawTLVertex(ctx, vptr);
				}
    		break;
		} // switch
		entry->proc.pglEnd();
		
		ctx->render.dirty = TRUE;
		ctx->render.zdirty = TRUE;
	}
}

NUKED_INLINE BOOL IsFullSurface(mesa3d_ctx_t *ctx, RECT *rect)
{
	return
		rect->left == 0 &&
		rect->top == 0 &&
		rect->right == ctx->state.sw &&
		rect->bottom == ctx->state.sh;
}

NUKED_LOCAL void MesaClear(mesa3d_ctx_t *ctx, DWORD flags, D3DCOLOR color, D3DVALUE depth, DWORD stencil, int rects_cnt, RECT *rects)
{
	GLfloat cv[4];
	int i;
	mesa3d_entry_t *entry = ctx->entry;

	TOPIC("DEPTHCONV", "Clear=%X", flags);

	GLbitfield mask = 0;
	if(flags & D3DCLEAR_TARGET)
	{
		MESA_D3DCOLOR_TO_FV(color, cv);
		entry->proc.pglClearColor(cv[0], cv[1], cv[2], cv[3]);
		mask |= GL_COLOR_BUFFER_BIT;
	}

	if(flags & D3DCLEAR_ZBUFFER)
	{
		mask |= GL_DEPTH_BUFFER_BIT;
		entry->proc.pglEnable(GL_DEPTH_TEST);
		entry->proc.pglDepthMask(GL_TRUE);
		entry->proc.pglClearDepth(depth);
	}
	
	if(flags & D3DCLEAR_STENCIL)
	{
		mask |= GL_STENCIL_BUFFER_BIT;
		entry->proc.pglEnable(GL_STENCIL_TEST);
		entry->proc.pglStencilMask(0xFF);
		entry->proc.pglClearStencil(stencil);
	}

	if(rects_cnt == 0 || IsFullSurface(ctx, &rects[0])) // full surface
	{
		TOPIC("DEPTHCONV", "full clear");
		entry->proc.pglClear(mask);
		//entry->proc.pglFlush();
	}
	else
	{
		TOPIC("CLEAR", "partly clean");
#if 0
		if(flags & D3DCLEAR_TARGET)
		{
			void *ptr = MesaGetVidMem(ctx, ctx->backbuffer, MesaOldFlip(ctx));
			if(ptr)
			{
				MesaBufferUploadColor(ctx, ptr);
			}
		}
		
		if(ctx->depth_bpp && (flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)))
		{
			void *ptr = MesaGetVidMem(ctx, ctx->depth, MesaOldFlip(ctx));
			MesaBufferUploadDepth(ctx, ptr);
		}
#endif
		TOPIC("DEPTHCONV", "Clear %X => %X, %f, %X", flags, color, depth, stencil);
		for(i = 0; i < rects_cnt; i++)
		{
			entry->proc.pglEnable(GL_SCISSOR_TEST);
			entry->proc.pglScissor(
				rects[i].left, rects[i].top,
				rects[i].right - rects[i].left,
				rects[i].bottom - rects[i].top);
	
			entry->proc.pglClear(mask);
			entry->proc.pglDisable(GL_SCISSOR_TEST);
		}
	}

	if(flags & D3DCLEAR_TARGET)
	{
		ctx->render.dirty = TRUE;
	}

	if(flags & D3DCLEAR_ZBUFFER)
	{
		if(ctx->state.depth.enabled)
			entry->proc.pglEnable(GL_DEPTH_TEST);
		else
			entry->proc.pglDisable(GL_DEPTH_TEST);

		entry->proc.pglDepthMask(ctx->state.depth.writable ? GL_TRUE : GL_FALSE);

		ctx->render.zdirty = TRUE;
	}

	if(flags & D3DCLEAR_STENCIL)
	{
		if(ctx->state.stencil.enabled)
			entry->proc.pglEnable(GL_STENCIL_TEST);
		else
			entry->proc.pglDisable(GL_STENCIL_TEST);

		entry->proc.pglStencilMask(ctx->state.stencil.writemask);

		ctx->render.zdirty = TRUE;
	}

	MesaDrawRefreshState(ctx);
}

NUKED_LOCAL void MesaBlockLock(mesa3d_ctx_t *ctx)
{
#ifdef OPENGL_BLOCK_LOCK
	LONG tmp;
	do
	{
		tmp = InterlockedExchange(&ctx->thread_lock, 1);
		if(tmp == 1)
		{
			Sleep(10);
		}
	} while(tmp == 1);
#endif
}

NUKED_LOCAL void MesaBlockUnlock(mesa3d_ctx_t *ctx)
{
#ifdef OPENGL_BLOCK_LOCK
	InterlockedExchange(&ctx->thread_lock, 0);
#endif
}

NUKED_LOCAL void MesaSceneBegin(mesa3d_ctx_t *ctx)
{
	if(!ctx->render.dirty)
	{
		void *ptr = MesaGetVidMem(ctx, ctx->backbuffer, MesaOldFlip(ctx));
		if(ptr)
		{
			MesaBufferUploadColor(ctx, ptr);
		}
	}
}

NUKED_LOCAL void MesaSceneEnd(mesa3d_ctx_t *ctx)
{
	BOOL is_visible = MesaBackbufferIsFront(ctx);
	void *ptr = MesaGetVidMem(ctx, ctx->backbuffer, MesaOldFlip(ctx));
	if(ptr)
	{
#if 1
		if(is_visible) /* fixme: check for DDSCAPS_PRIMARYSURFACE */
			FBHDA_access_begin(0);

		TOPIC("TARGET", "MesaBufferDownloadColor(ctx, 0x%X)", ptr);
		if(ctx->render.dirty)
		{
			MesaBufferDownloadColor(ctx, ptr);
		}
		
		if(ctx->state.textarget)
		{
			TOPIC("TEXTARGET", "Render to texture");
			ddsurface_t *dd = ddsurf_get_by_id(ctx, ctx->backbuffer);
			if(dd)
			{
				dd->dirty = TRUE;
			}
			//DDSURF *surf = SurfaceGetSURF(ctx->backbuffer);
			//SurfaceToMesa(surf->lpLcl, TRUE);
		}

		if(is_visible)
			FBHDA_access_end(0);
#else
		if(is_visible) /* fixme: check for DDSCAPS_PRIMARYSURFACE */
		{
			FBHDA_access_begin(0);

			TOPIC("TARGET", "MesaBufferDownloadColor(ctx, 0x%X)", ptr);
			if(ctx->render.dirty)
			{
				MesaBufferDownloadColor(ctx, ptr);
			}
			FBHDA_access_end(0);
		}
		
		if(ctx->state.textarget)
		{
			TOPIC("TEXTARGET", "Render to texture");
			SurfaceToMesaTex(ctx->backbuffer);
		}
#endif
	}

	//ctx->entry->proc.pglFinish();

#ifdef DEBUG
	if(mesa_dump_key() == MESA_KEY_DUMP)
	{
		mesa_dump(ctx);
		mesa_dump_inc();
	}
#endif
}

NUKED_LOCAL void *MesaTempAlloc(mesa3d_ctx_t *ctx, DWORD w, DWORD size)
{
	if(ctx->temp.width == 0)
	{
		DWORD cmp_w = w;
		DWORD dst_w = 0;
		DWORD i;
		
		FBHDA_t *hda = FBHDA_setup();
		if(hda)
		{
			if(hda->width > cmp_w)
			{
				cmp_w = hda->width;
			}
		}
		
		for(i = 0; i < sizeof(DWORD)*8; i++)
		{
			dst_w = 1 << i;
			if(dst_w >= cmp_w)
			{
				break;
			}
		}
		
		ctx->temp.width = dst_w;
		ctx->temp.size = dst_w*dst_w*4;
		ctx->temp.buf = hal3d_malloc(ctx->temp.size);
	}

	if(ctx->temp.buf == NULL ||
		w > ctx->temp.width ||
		size > ctx->temp.size ||
		ctx->temp.lock != 0
		)
	{
		return hal3d_malloc(size);
	}
	
	ctx->temp.lock = 1;
	return ctx->temp.buf;
}

NUKED_LOCAL void MesaTempFree(mesa3d_ctx_t *ctx, void *ptr)
{
	if(ptr != NULL)
	{
		if(ptr == ctx->temp.buf)
		{
			ctx->temp.lock = 0;
		}
		else
		{
			hal3d_free(ptr);
		}
	}
}

NUKED_FAST void MesaSetCull(mesa3d_ctx_t *ctx)
{
	switch(ctx->state.cull)
	{
		case GL_NONE:
			ctx->entry->proc.pglDisable(GL_CULL_FACE);
			break;
		case GL_BACK:
			ctx->entry->proc.pglEnable(GL_CULL_FACE);
			ctx->entry->proc.pglCullFace(GL_BACK);
			break;
		case GL_FRONT:
			ctx->entry->proc.pglEnable(GL_CULL_FACE);
			ctx->entry->proc.pglCullFace(GL_FRONT);
			break;
	}
}

NUKED_FAST void MesaReverseCull(mesa3d_ctx_t *ctx)
{
	switch(ctx->state.cull)
	{
		case GL_NONE:
			ctx->entry->proc.pglDisable(GL_CULL_FACE);
			break;
		case GL_BACK:
			ctx->entry->proc.pglEnable(GL_CULL_FACE);
			ctx->entry->proc.pglCullFace(GL_FRONT);
			break;
		case GL_FRONT:
			ctx->entry->proc.pglEnable(GL_CULL_FACE);
			ctx->entry->proc.pglCullFace(GL_BACK);
			break;
	}
}

NUKED_LOCAL mesa_pal8_t *MesaGetPal(mesa3d_ctx_t *ctx, DWORD palette_handle)
{
	mesa_pal8_t **ppal = &ctx->first_pal;
	while((*ppal) != NULL)
	{
		if((*ppal)->palette_handle == palette_handle)
		{
			return *ppal;
		}
		
		ppal = &((*ppal)->next);
	}
	
	*ppal = hal3d_calloc(sizeof(mesa_pal8_t));
	if(*ppal != NULL)
	{
		(*ppal)->next = NULL;
		(*ppal)->stamp = 0;
		(*ppal)->palette_handle = palette_handle;
		return *ppal;
	}
	
	return NULL;
}

NUKED_LOCAL void MesaFreePals(mesa3d_ctx_t *ctx)
{
	mesa_pal8_t *ptr;
	while(ctx->first_pal != NULL)
	{
		ptr = ctx->first_pal;
		ctx->first_pal = ptr->next;
		hal3d_free((void**)&ptr);
	}
}

NUKED_LOCAL void MesaApplyMaterialSet(mesa3d_ctx_t *ctx, D3DHAL_DP2SETMATERIAL *material)
{
	MESA_D3DCOLORVALUE_TO_FV(material->dcvDiffuse,  ctx->state.material.diffuse);
	MESA_D3DCOLORVALUE_TO_FV(material->dcvAmbient,  ctx->state.material.ambient);
	MESA_D3DCOLORVALUE_TO_FV(material->dcvEmissive, ctx->state.material.emissive);
	MESA_D3DCOLORVALUE_TO_FV(material->dcvSpecular, ctx->state.material.specular);

	if(material->dvPower < 0)
		ctx->state.material.shininess = 0;
	else if(material->dvPower > 128)
		ctx->state.material.shininess = 128;
	else
		ctx->state.material.shininess = material->dvPower;

	ctx->state.current.material = *material;
	ctx->state.current.extraset[0] |= 1 << MESA_REC_EXTRA_MATERIAL;

	MesaApplyMaterial(ctx);
}

#if 0
NUKED_LOCAL void MesaGC(mesa3d_ctx_t *ctx, BOOL oom)
{
	unsigned int i, j;
	static unsigned int runs = 0;

	if(!oom && (runs % 100 == 0))
	{
		FBHDA_t *hda = ctx->entry->hda;
		if(hda)
		{
			if(hda->flags & FB_ACCEL_GPUMEM)
			{
				DWORD left = hda->gpu_mem_total - hda->gpu_mem_used;
				if(left < 34*1024*1024)
				{
					TOPIC("GC", "oom = %u %u", hda->gpu_mem_total, hda->gpu_mem_used);
					oom = TRUE;
				}
			}
		}
	}

	if(oom)
	{
		int tmus = ctx->tmu_count;
		for(i = 0; i < MESA3D_MAX_TEXS; i++)
		{
			if(ctx->tex[i] != NULL)
			{
				/* check if image isn't attached */
				for(j = 0; j < tmus; j++)
				{
					if(ctx->state.tmu[j].image == ctx->tex[i])
					{
						break;
					}
				}

				if(j == tmus)
				{
					TOPIC("GC", "Cleaning texture: id=%d, glid=%d", i, ctx->tex[i]->gltex);
					MesaDestroyTexture(ctx->tex[i], FALSE, 0);
					ctx->tex[i] = NULL;
				}
			}
		}
		ctx->entry->proc.pglFinish();
	}

	runs++;
}
#endif

#ifdef DEBUG

NUKED_LOCAL void MesaMemInfo(mesa3d_entry_t *entry)
{
	MEMORYSTATUS ms;
	memset(&ms, 0, sizeof(MEMORYSTATUS));
	ms.dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus(&ms);

	TOPIC("GC", "Memory dwMemoryLoad=%u dwTotalPhys=%u dwAvailPhys=%u dwTotalPageFile=%u dwAvailPageFile=%u dwTotalVirtual=%u dwAvailVirtual=%u",
		ms.dwMemoryLoad, ms.dwTotalPhys, ms.dwAvailPhys, ms.dwTotalPageFile, ms.dwAvailPageFile, ms.dwTotalVirtual, ms.dwAvailVirtual
	);
}

#endif
