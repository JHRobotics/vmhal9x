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
#include "d3dhal_ddk.h"
#include "vmdahal32.h"
#include "vmhal9x.h"
#include "mesa3d.h"
#include "osmesa.h"

#include "nocrt.h"

#ifndef D3DTSS_TCI_SPHEREMAP
#define D3DTSS_TCI_SPHEREMAP 0x40000
#endif

static const GLfloat black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

#define MESA_HT_MOD 113

#define MESA_LIB_SW_NAME "mesa3d.dll"
#define MESA_LIB_SVGA_NAME "vmwsgl32.dll"

#define OS_WIDTH   320
#define OS_HEIGHT  240
#define OS_FORMAT  OSMESA_RGBA
#define OS_TYPE    GL_UNSIGNED_BYTE

static char *MesaLibName()
{
	FBHDA_t *hda = FBHDA_setup();
	if(hda)
	{
		if(hda->flags & FB_ACCEL_VMSVGA3D)
		{
			return MESA_LIB_SVGA_NAME;
		}
	}

	return MESA_LIB_SW_NAME;
}

static mesa3d_entry_t *mesa_entry_ht[MESA_HT_MOD] = {};

#define MESA_API(_n, _t, _p) \
	mesa->proc.p ## _n = (_n ## _h)mesa->GetProcAddress(#_n); if(!mesa->proc.p ## _n){valid = FALSE; ERR("GetProcAddress fail for %s", #_n); break;}

#define MESA_API_OS(_n, _t, _p) \
	if(mesa->os){MESA_API(_n, _t, _p)}else{mesa->proc.p ## _n = NULL;}

#define MESA_API_DRV(_n, _t, _p) \
	if(!mesa->os){ \
		mesa->proc.p ## _n = (_n ## _h)GetProcAddress(mesa->lib, #_n); if(!mesa->proc.p ## _n){valid = FALSE; ERR("GetProcAddress fail for %s", #_n); break;} \
	}else{mesa->proc.p ## _n = NULL;}

static mesa3d_entry_t *Mesa3DCreate(DWORD pid, mesa3d_entry_t *mesa)
{
	mesa3d_entry_t *orig_next = mesa->next;
	memset(mesa, 0, sizeof(mesa3d_entry_t));
	
	mesa->pid = pid;
	mesa->next = orig_next;

	BOOL valid = TRUE;

	do
	{
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
			
		#include "mesa3d_api.h"
			
	} while(0);

	if(!valid)
	{
		ERR("Can't load library %s", MesaLibName());

		mesa = NULL;
	}
	
	return mesa;
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

mesa3d_entry_t *Mesa3DGet(DWORD pid, BOOL create)
{
	TRACE_ENTRY
	
	//TOPIC("RENDER", "Created entry for pid=%d", pid);
	
	mesa3d_entry_t *entry = mesa_entry_ht[pid % MESA_HT_MOD];
	
	while(entry != NULL)
	{
		if(entry->pid == pid)
		{
			HMODULE h_test = GetModuleHandleA(MesaLibName());
			if(h_test)
			{
				return entry;
			}
			
			ERR("Reloading mesa3d.dll!");
			return Mesa3DCreate(pid, entry);
		}

		entry = entry->next;
	}
	
	if(create)
	{
		mesa3d_entry_t *mem = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(mesa3d_entry_t));
		mesa3d_entry_t *new_entry = Mesa3DCreate(pid, mem);
		
		if(new_entry)
		{
			new_entry->next = mesa_entry_ht[pid % MESA_HT_MOD];
			mesa_entry_ht[pid % MESA_HT_MOD] = new_entry;
			return new_entry;
		}
		else
		{
			HeapFree(hSharedHeap, 0, mem);
		}
	}
	
	return NULL;
}

void Mesa3DFree(DWORD pid)
{
	mesa3d_entry_t *entry = mesa_entry_ht[pid % MESA_HT_MOD];
	mesa3d_entry_t *prev = NULL;
	
	while(entry != NULL)
	{
		if(entry->pid == pid)
		{
			if(prev)
			{
				prev->next = entry->next;
			}
			else
			{
				mesa_entry_ht[pid % MESA_HT_MOD] = entry->next;
			}

			mesa3d_entry_t *clean_ptr = entry;
			entry = entry->next;
			
			MesaDestroyAllCtx(clean_ptr);
			FreeLibrary(clean_ptr->lib);
			HeapFree(hSharedHeap, 0, clean_ptr);
		}
		else
		{
			prev = entry;
			entry = entry->next;
		}
	}
}

void Mesa3DCleanProc()
{
	TRACE_ENTRY
	
	DWORD pid = GetCurrentProcessId();
	Mesa3DFree(pid);
}

static BOOL DDSurfaceToGL(DDSURF *surf, GLuint *bpp, GLint *internalformat, GLenum *format, GLenum *type, BOOL *compressed)
{
	if((surf->dwFlags & DDRAWISURF_HASPIXELFORMAT) != 0)
	{
		DDPIXELFORMAT fmt = surf->lpGbl->ddpfSurface;
		TOPIC("BLEND", "DDPIXELFORMAT.dwFlags = 0x%X", fmt.dwFlags);
		
		if(fmt.dwFlags & DDPF_FOURCC)
		{
			*bpp = 32;
			*compressed = TRUE;

			switch(fmt.dwFourCC)
			{
				case MAKEFOURCC('D', 'X', 'T', '1'):
					*internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					*format = GL_RGBA;
					*type = GL_UNSIGNED_BYTE;
					return TRUE;
					break;
				case MAKEFOURCC('D', 'X', 'T', '2'):
				case MAKEFOURCC('D', 'X', 'T', '3'):
					*internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					*format = GL_RGBA;
					*type = GL_UNSIGNED_BYTE;
					return TRUE;
					break;
				case MAKEFOURCC('D', 'X', 'T', '4'):
				case MAKEFOURCC('D', 'X', 'T', '5'):
					*internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					*format = GL_RGBA;
					*type = GL_UNSIGNED_BYTE;
					return TRUE;
					break;
			}
		}
		else if(fmt.dwFlags & DDPF_ALPHA)
		{
			*internalformat = GL_RED;
			*format = GL_RED;
			*bpp = fmt.dwAlphaBitDepth;
			*compressed = FALSE;
			switch(fmt.dwAlphaBitDepth)
			{
				case 8:
					*type = GL_UNSIGNED_BYTE;
					break;
				case 15:
				case 16:
					*type =  GL_UNSIGNED_SHORT;
					break;
				case 24:
				default:
					 *type = GL_UNSIGNED_INT;
					break;
			}
			return TRUE;
		}
		else if(fmt.dwFlags & DDPF_RGB)
		{
			*compressed = FALSE;
			if(fmt.dwFlags & DDPF_ALPHAPIXELS)
			{
				TOPIC("FORMAT", "case DDPF_ALPHAPIXELS (RGBA(%d): %08X %08X %08X %08X)",
					fmt.dwRGBBitCount, fmt.dwRBitMask, fmt.dwGBitMask, fmt.dwBBitMask, fmt.dwRGBAlphaBitMask
				);
				
				*internalformat = GL_RGBA;
				*format = GL_BGRA;
				
				switch(fmt.dwRGBBitCount)
				{
					case 16:
						*bpp = 16;
						if(fmt.dwRGBAlphaBitMask == 0xF000)
						{
							*type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
						}
						else if(fmt.dwRGBAlphaBitMask == 0x000F)
						{
							*type = GL_UNSIGNED_SHORT_4_4_4_4;
						}
						else if(fmt.dwRGBAlphaBitMask == 0x0001)
						{
							*type = GL_UNSIGNED_SHORT_5_5_5_1;
						}
						else
						{
							*type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
						}
						break;
					default:
						*bpp = 32;
						if(fmt.dwRGBAlphaBitMask == 0xFF000000)
						{
							*type = GL_UNSIGNED_INT_8_8_8_8_REV;
						}
						else
						{
							*type = GL_BGRA;
						}
						break;
				}
			}
			else // no alpha bits
			{
				*internalformat = GL_RGB;
				*bpp = fmt.dwRGBBitCount;
				*format = GL_BGR;
				
				TOPIC("FORMAT", "case !DDPF_ALPHAPIXELS (RGBA(%d): %08X %08X %08X %08X)",
					fmt.dwRGBBitCount, fmt.dwRBitMask, fmt.dwGBitMask, fmt.dwBBitMask, fmt.dwRGBAlphaBitMask
				);
				
				TOPIC("GL", "fmt.dwFlags & DDPF_RGB + fmt.dwRGBBitCount = %d", fmt.dwRGBBitCount);
				
				switch(fmt.dwRGBBitCount)
				{
					case 8:
						*type = GL_UNSIGNED_BYTE_3_3_2;
						*format = GL_RGB;
						break;
					case 12:
						*type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
						*format = GL_RGBA;
						break;
					case 15:
						*type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
						*format = GL_BGRA;
						break;
					case 16:
						if(fmt.dwRBitMask == 0x7C00)
						{
							*bpp = 15;
							*type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
							*format = GL_BGRA;
						}
						else if(fmt.dwRBitMask == 0x0F00)
						{
							*bpp = 12;
							*type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
							*format = GL_BGRA;
						}
						else
						{
							*type = GL_UNSIGNED_SHORT_5_6_5;
							*format = GL_RGB;
						}
						break;
					case 24:
						*type = GL_UNSIGNED_BYTE;
						if(fmt.dwRBitMask == 0x0000FF)
						{
							*format = GL_RGB;
						}
						break;
					case 32:
					default:
						*type = GL_UNSIGNED_BYTE;
						if(fmt.dwRBitMask == 0x0000FF)
						{
							*format = GL_RGBA;
						}
						else
						{
							*format = GL_BGRA;
						}
						break;
				}
			}
			return TRUE;
		}
		else if(fmt.dwFlags & DDPF_ZBUFFER)
		{
			*internalformat = GL_DEPTH_COMPONENT;
			*format = GL_DEPTH_COMPONENT;
			*bpp = fmt.dwAlphaBitDepth;
			*compressed = 0;
			switch(fmt.dwAlphaBitDepth)
			{
				case 8:
					*type = GL_UNSIGNED_BYTE;
					break;
				case 15:
				case 16:
					*type = GL_UNSIGNED_SHORT;
					break;
				case 24:
				default:
					 *type = GL_UNSIGNED_INT;
					break;
			}
			return TRUE;
		}
	
		return FALSE;
	} // HASPIXELFORMAT
	else
	{
		FBHDA_t *hda = FBHDA_setup();

		*bpp = hda->bpp;
		*internalformat = GL_RGB;
		
		TOPIC("FORMAT", "case SURFACE format");
		
		switch(hda->bpp)
		{
			case 8:
				*type = GL_UNSIGNED_BYTE_3_3_2;
				*format = GL_RGB;
				break;
			case 15:
				*type = GL_UNSIGNED_SHORT_5_5_5_1;
				*format = GL_RGBA;
				break;
			case 16:
				*type = GL_UNSIGNED_SHORT_5_6_5;
				*format = GL_RGB;
				break;
			case 24:
				*type = GL_UNSIGNED_BYTE;
				*format = GL_BGR;
				break;
			case 32:
			default:
				*type = GL_UNSIGNED_BYTE;
				*format = GL_BGRA;
				break;
		}
		return TRUE;
	}
	
	return FALSE; /* not reached */
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
void MesaStencilApply(mesa3d_ctx_t *ctx)
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

static void MesaDepthReeval(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY
	
	mesa3d_entry_t *entry = ctx->entry;
	ctx->depth_stencil = FALSE;
	ctx->state.stencil.enabled = FALSE;
	
	if(ctx->depth_bpp)
	{
		DDSURF *surf = SurfaceGetSURF(ctx->depth);
		GL_CHECK(entry->proc.pglEnable(GL_DEPTH_TEST));
		GL_CHECK(entry->proc.pglDepthMask(GL_TRUE));
		ctx->state.depth.enabled = TRUE;
		ctx->state.depth.writable = TRUE;

		
		if(surf->lpGbl->ddpfSurface.dwFlags & DDPF_STENCILBUFFER)
		{
			ctx->depth_stencil = TRUE;
		}
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_DEPTH_TEST));
		GL_CHECK(entry->proc.pglDepthMask(GL_FALSE));
		ctx->state.depth.enabled = FALSE;
		ctx->state.depth.writable = FALSE;
	}
	
	MesaStencilApply(ctx);
}

static BOOL MesaBackbufferIfFront(mesa3d_ctx_t *ctx)
{
	DWORD addr = (DWORD)SurfaceGetVidMem(ctx->backbuffer);
	TRACE("MesaBackbufferIfFront addr=0x%X", addr);
	
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


mesa3d_ctx_t *MesaCreateCtx(mesa3d_entry_t *entry, DWORD dds_sid, DWORD ddz_sid)
{
	TRACE_ENTRY
	int i;
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

	for(i = 0; i < MESA3D_MAX_CTXS; i++)
	{
		if(entry->ctx[i] == NULL)
		{
			BOOL valid = FALSE;
			do
			{
				ctx = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(mesa3d_ctx_t));
				if(ctx == NULL)
					break;

				if(SurfaceIsEmpty(dds_sid))
				{
					SurfaceClearEmpty(dds_sid);
				}

				ctx->thread_lock = 0;
				ctx->entry = entry;
				ctx->id = i;
	
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
					ctx->osbuf = HeapAlloc(hSharedLargeHeap, 0, ctx->ossize);
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

				MesaSetTarget(ctx, dds_sid, ddz_sid, TRUE);
//				MesaBufferFBOSetup(ctx, width, height);

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
				MesaApplyViewport(ctx, 0, 0, ctx->state.sw, ctx->state.sh);

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
							HeapFree(hSharedLargeHeap, 0, ctx->osbuf);
					}
					else
					{
						if(ctx->glrc)
							entry->proc.pDrvDeleteContext(ctx->glrc);
						
						if(ctx->fbo_win)
							DestroyWindow(ctx->fbo_win);
					}

					HeapFree(hSharedHeap, 0, ctx);

					ctx = NULL;
				}
			}
			else
			{
				entry->ctx[i] = ctx;
			}
			
			break;
		} // entry->ctx[i] == NULL
	} // for

	return ctx;
}

BOOL MesaSetTarget(mesa3d_ctx_t *ctx, surface_id dds_sid, surface_id ddz_sid, BOOL create)
{
	TOPIC("TARGET", "MesaSetTarget(ctx, %d, %d, %d)", dds_sid, ddz_sid, create);

	DDSURF *dds = SurfaceGetSURF(dds_sid);

	int width  = dds->lpGbl->wWidth;
	int height = dds->lpGbl->wHeight;
	int bpp = DDSurf_GetBPP(dds);
	int bpp_depth = 0;
	BOOL depth_reeval = TRUE;
	BOOL viewport_set = TRUE;
	//dds->fpVidMem = dds->lpLcl->lpGbl->fpVidMem;
	TOPIC("TARGET", "MesaSetTarget: target size(%d x %d x %d)", width, height, bpp);

	if(ddz_sid)
	{
		DDSURF *ddz = SurfaceGetSURF(ddz_sid);
		bpp_depth = DDSurf_GetBPP(ddz);
		if(ctx->depth_bpp != 0 && bpp_depth != 0)
		{
			depth_reeval = FALSE;
		}
		//ddz->fpVidMem = ddz->lpLcl->lpGbl->fpVidMem;
	}
	else
	{
		if(ctx->depth_bpp == 0)
		{
			depth_reeval = FALSE;
		}
	}

	if(create || ctx->state.sw != width || ctx->state.sh != height/* || ctx->front_bpp != bpp*/)
	{
		MesaBufferFBOSetup(ctx, width, height);
	}
	else
	{
		viewport_set = FALSE;
	}

	ctx->front_bpp = bpp;
	ctx->depth_bpp = bpp_depth;
	ctx->backbuffer = dds_sid;
	ctx->depth = ddz_sid;

	ctx->state.sw = width;
	ctx->state.sh = height;

	//UpdateScreenCoords(ctx, (GLfloat)width, (GLfloat)height);
	ctx->state.textarget = (dds->lpLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) ? TRUE : FALSE;

	if(viewport_set)
		MesaApplyViewport(ctx, 0, 0, width, height);

	if(depth_reeval)
		MesaDepthReeval(ctx);

	return TRUE;
}

void MesaDestroyCtx(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY
	int id = ctx->id;
	mesa3d_entry_t *entry = ctx->entry;
	int i;
	
	TOPIC("GL", "OSMesaDestroyContext");

	if(entry->pid == GetCurrentProcessId())
	{
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
		
	}
#ifdef WARN_ON
	else
	{
		WARN("Alien cleaning! Don't call destructor!");
	}
#endif
	
	entry->ctx[id] = NULL;
	
	for(i = 0; i < MESA3D_MAX_TEXS; i++)
	{
		if(ctx->tex[i] != NULL)
		{
			MesaDestroyTexture(ctx->tex[i], TRUE, 0);
			ctx->tex[i] = NULL;
		}
	}

	if(ctx->osbuf != NULL)
	{
		HeapFree(hSharedLargeHeap, 0, ctx->osbuf);
	}
	
	if(ctx->temp.buf)
	{
		HeapFree(hSharedLargeHeap, 0, ctx->temp.buf);
		ctx->temp.size = ctx->temp.width = 0;
		ctx->temp.buf = NULL;
	}
	
	MesaLightDestroyAll(ctx);
	HeapFree(hSharedHeap, 0, ctx);
}

void MesaDestroyAllCtx(mesa3d_entry_t *entry)
{
	TRACE_ENTRY
	int i;
	for(i = 0; i < MESA3D_MAX_CTXS; i++)
	{
		if(entry->ctx[i] != NULL)
		{
			MesaDestroyCtx(entry->ctx[i]);
		}
	}
}

void MesaInitCtx(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	int i = 0;
	
	entry->proc.pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	ctx->tmu_count = MESA_TMU_CNT();
	if(VMHALenv.texture_num_units > MESA_TMU_MAX)
	{
		ctx->fbo_tmu = MESA_TMU_MAX;
	}
	else
	{
		ctx->fbo_tmu = 0;
	}

	ctx->state.tmu[0].active = 1;

	entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN);
	entry->proc.pglPixelStorei(GL_PACK_ALIGNMENT, FBHDA_ROW_ALIGN);

	entry->proc.pglFrontFace(GL_CCW);

	entry->proc.pglMatrixMode(GL_MODELVIEW);
	entry->proc.pglLoadIdentity();
	entry->proc.pglMatrixMode(GL_PROJECTION);
	entry->proc.pglLoadIdentity();
	
	entry->proc.pglDepthRange(0.0f, 1.0f);

	ctx->matrix.wmax = GL_WRANGE_MAX;
	MesaIdentity(ctx->matrix.zscale);
	//ctx->matrix.zscale[10] = 0.9;

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
	GL_CHECK(entry->proc.pglEnable(GL_DEPTH_CLAMP_NV));

	// needs ARB_vertex_blend
	//GL_CHECK(entry->proc.pglEnable(GL_WEIGHT_SUM_UNITY_ARB));

	for(i = 0; i < ctx->tmu_count; i++)
	{
		entry->proc.pglActiveTexture(GL_TEXTURE0 + i);
		
		if(ctx->state.tmu[i].active)
		{
			entry->proc.pglEnable(GL_TEXTURE_2D);
		}
		
		entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		
		ctx->state.tmu[i].reload = TRUE;
		ctx->state.tmu[i].update = TRUE;
		ctx->state.tmu[i].texblend = D3DTBLEND_DECAL;
		
		/*
			defaults:
			https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms886612(v=msdn.10)
		*/
		ctx->state.tmu[i].dx6_filter = TRUE;
		ctx->state.tmu[i].dx_mip = D3DTFP_NONE;
		ctx->state.tmu[i].dx_mag = D3DTFG_POINT;
		ctx->state.tmu[i].dx_min = D3DTFN_POINT;
		
		/*
			defaults:
			https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtexturestagestatetype
		*/
		ctx->state.tmu[i].dx6_blend  = TRUE;
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
	}

	entry->proc.pglEnable(GL_BLEND);
	entry->proc.pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ctx->state.blend = TRUE;
	ctx->state.blend_sfactor = GL_SRC_ALPHA;
	ctx->state.blend_dfactor = GL_ONE_MINUS_SRC_ALPHA;
	ctx->state.texperspective = TRUE;
	ctx->state.alpha.func = GL_NOTEQUAL;
	ctx->state.alpha.ref = 0.0f;

//	entry->proc.pglEnable(GL_CULL_FACE);
//	entry->proc.pglCullFace(GL_FRONT_AND_BACK);
	
	ctx->state.stencil.sfail     = GL_KEEP;
	ctx->state.stencil.dpfail    = GL_KEEP;
	ctx->state.stencil.dppass    = GL_KEEP;
	ctx->state.stencil.func      = GL_ALWAYS;
	ctx->state.stencil.ref       = 0;
	ctx->state.stencil.mask      = 0xFF;
	ctx->state.stencil.writemask = 0xFF;

	ctx->render.rop2 = R2_COPYPEN;
	ctx->render.planemask = 0xFFFFFFFF;

	MesaDepthReeval(ctx);
	entry->proc.pglDepthFunc(GL_LESS);
}

BOOL MesaSetCtx(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY
	
	if(ctx->entry->pid == GetCurrentProcessId())
	{
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
	}
#ifdef WARN_ON
	else
	{
		WARN("Alien process! %X vs %X, ignoring!", ctx->entry->pid, GetCurrentProcessId());
	}
#endif

	return FALSE;
}

/* convert functions (TODO: move to special file) */
DWORD DDSurf_GetBPP(DDSURF *surf)
{
	if(surf == NULL)
	{
		return 0;
	}

	DWORD bpp;

	if((surf->dwFlags & DDRAWISURF_HASPIXELFORMAT) == 0)
	{
		FBHDA_t *hda = FBHDA_setup();
		bpp = hda->bpp;
	}
	else
	{
		bpp = surf->lpGbl->ddpfSurface.dwRGBBitCount;
	}

	return bpp;
}

#if 0
#define STRCASE(_c) case _c: return #_c;
static char *glname(GLenum e)
{
	switch(e)
	{
		STRCASE(GL_RGB)
		STRCASE(GL_RGBA)
		STRCASE(GL_ALPHA)
		STRCASE(GL_BGR)
		STRCASE(GL_BGRA)
		STRCASE(GL_UNSIGNED_BYTE)
		STRCASE(GL_BYTE)
		STRCASE(GL_UNSIGNED_SHORT)
		STRCASE(GL_SHORT)
		STRCASE(GL_UNSIGNED_INT)
		STRCASE(GL_INT)
		STRCASE(GL_UNSIGNED_BYTE_3_3_2)
		STRCASE(GL_UNSIGNED_BYTE_2_3_3_REV)
		STRCASE(GL_UNSIGNED_SHORT_5_6_5)
		STRCASE(GL_UNSIGNED_SHORT_5_6_5_REV)
		STRCASE(GL_UNSIGNED_SHORT_4_4_4_4)
		STRCASE(GL_UNSIGNED_SHORT_4_4_4_4_REV)
		STRCASE(GL_UNSIGNED_SHORT_5_5_5_1)
		STRCASE(GL_UNSIGNED_SHORT_1_5_5_5_REV)
		STRCASE(GL_UNSIGNED_INT_8_8_8_8)
		STRCASE(GL_UNSIGNED_INT_8_8_8_8_REV)
		STRCASE(GL_DEPTH_COMPONENT)
		STRCASE(GL_DEPTH_STENCIL)
		default: return "unknown enum";
	}
}
#undef STRCASE
#endif

static int GetCubeSide(DWORD dwCaps2)
{
	if(dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX)
		return MESA_POSITIVEX;
	else if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX)
		return MESA_NEGATIVEX;
	else if(dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY)
		return MESA_POSITIVEY;
	else if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY)
		return MESA_NEGATIVEY;
	else if(dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ)
		return MESA_POSITIVEZ;
	else if(dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ)
		return MESA_NEGATIVEZ;

	return 0;
}

mesa3d_texture_t *MesaCreateTexture(mesa3d_ctx_t *ctx, surface_id sid)
{
	mesa3d_texture_t *tex = NULL;
	DDSURF *surf = SurfaceGetSURF(sid);
	int i;

	if(surf == NULL)
		return NULL;

	TOPIC("GL", "new texture");

	for(i = 0; i < MESA3D_MAX_TEXS; i++)
	{
		if(ctx->tex[i] == NULL)
		{
			mesa3d_entry_t *entry = ctx->entry;
			TOPIC("RELOAD", "New texture - position: %d", i);

			tex = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(mesa3d_texture_t));
			if(tex)
			{
				int cube_levels[MESA3D_CUBE_SIDES] = {0, 0, 0, 0, 0, 0};
				int cube_index = 0;
				BOOL is_cube =
					(surf->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP) == 0 ? FALSE : TRUE;

				if(is_cube)
				{
					cube_index = GetCubeSide(surf->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2);
				}

				tex->id               = i;
				tex->ctx              = ctx;
				tex->sides            = 1;
				tex->cube             = is_cube;
				tex->data_sid[cube_index][0]   = sid;
				tex->data_dirty[cube_index][0] = TRUE;
				if(is_cube)
				{
					cube_levels[cube_index]++;
					tex->sides = MESA3D_CUBE_SIDES;
				}

				if(DDSurfaceToGL(surf, &tex->bpp, &tex->internalformat, &tex->format, &tex->type, &tex->compressed))
				{
					tex->width  = surf->lpGbl->wWidth;
					tex->height = surf->lpGbl->wHeight;

					GL_CHECK(entry->proc.pglGenTextures(1, &tex->gltex));
					TOPIC("FRAMEBUFFER", "new texture: %d", tex->gltex);

					if((surf->lpLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) && ctx->state.tmu[0].colorkey)
					{
						if(!tex->compressed)
						{
							tex->colorkey = TRUE;
						}
					}

					if(SurfaceIsEmpty(sid))
						SurfaceClearData(sid);

					SurfaceAttachTexture(sid, tex, 0, 0);
					if(is_cube)
					{
						int i;
						for(i = 0; i < surf->attachments_cnt; i++)
						{
							surface_id subsid = surf->attachments[i];
							DDSURF *subsurf = SurfaceGetSURF(subsid);

							cube_index = GetCubeSide(subsurf->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2);

							int cube_pos = cube_levels[cube_index];
							tex->data_dirty[cube_index][cube_pos] = TRUE;
							tex->data_sid[cube_index][cube_pos]   = subsid;

							if(SurfaceIsEmpty(subsid))
								SurfaceClearData(subsid);

							SurfaceAttachTexture(subsid, tex, cube_pos, cube_index);

							TOPIC("CUBE", "added size[%d][%d], dwCaps2=0x%X", cube_index, cube_pos,
								surf->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2
							);

							cube_levels[cube_index]++;
						}

						if(cube_levels[MESA_POSITIVEX] > 1)
						{
							tex->mipmap_level = cube_levels[MESA_POSITIVEX]-1;
							tex->mipmap = TRUE;
						}
						else
						{
							tex->mipmap_level = 0;
							tex->mipmap = FALSE;
						}
						tex->cube = TRUE;
					}
					else if(surf->dwCaps & DDSCAPS_MIPMAP) /* mipmaps */
					{
						int level;
						for(level = 0; level < surf->attachments_cnt; level++)
						{
							surface_id subsid = surf->attachments[level];

							tex->data_dirty[0][level+1] = TRUE;
							tex->data_sid[0][level+1] = subsid;

							if(SurfaceIsEmpty(subsid))
								SurfaceClearData(subsid);

							SurfaceAttachTexture(subsid, tex, level+1, 0);
						}
						tex->mipmap_level = surf->attachments_cnt;
						tex->mipmap = (tex->mipmap_level > 0) ? TRUE : FALSE;
						TOPIC("MIPMAP", "Created %d with mipmap, at level: %d", tex->gltex, tex->mipmap_level);
					} // mipmaps
					else
					{
						tex->mipmap_level = 0;
						tex->mipmap = FALSE;
						TOPIC("RELOAD", "Created %d without mipmap", tex->gltex);
					}

					//ctx->state.reload_tex_par = TRUE;
					tex->dirty = TRUE;
					ctx->tex[i] = tex;
				}
				else
				{
					TOPIC("RELOAD", "Failed to identify image type!");
					tex = NULL;
				}
			}

			break;
		}
	}

	return tex;
}

void MesaReloadTexture(mesa3d_texture_t *tex, int tmu)
{
	BOOL reload = tex->dirty;
	BOOL reload_done = FALSE;
	int level;
	int side;

	if(tex->tmu[tmu] == FALSE)
	{
		reload = TRUE;
		TOPIC("RELOAD", "Load texture %d to TMU %d", tex->gltex, tmu);
	}

	if(tex->ctx->state.tmu[tmu].colorkey != tex->colorkey)
	{
		reload = TRUE;
	}

	TOPIC("GL", "reload tex? %d", reload);

	for(side = 0; side < tex->sides; side++)
	{
		for(level = 0; level <= tex->mipmap_level; level++)
		{
			/* TODO: now we're realoading all levels, even there is change only on one */
			if(reload || tex->data_dirty[side][level])
			{
				DDSURF *primary = SurfaceGetSURF(tex->data_sid[0][0]);
				if(tex->ctx->state.tmu[tmu].colorkey && (primary->lpLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT))
				{
					if(tex->compressed)
					{
						WARN("Chroma in compressed format (opengl): 0x%X", tex->internalformat);
					}
					else
					{
						MesaBufferUploadTextureChroma(tex->ctx, tex, level, side, tmu,
								primary->lpLcl->ddckCKSrcBlt.dwColorSpaceLowValue,
								primary->lpLcl->ddckCKSrcBlt.dwColorSpaceHighValue);
					}
				}
				else
				{
					MesaBufferUploadTexture(tex->ctx, tex, level, side, tmu);
				}

				tex->data_dirty[side][level] = FALSE;
				reload_done = TRUE;

				TOPIC("RELOAD", "Reloading: %d, level %d", tex->gltex, level);
			}
			tex->dirty = FALSE;
		}
	}

	if(reload_done)
	{
		tex->colorkey = tex->ctx->state.tmu[tmu].colorkey;
		tex->tmu[tmu] = TRUE;
	}
}

void MesaDestroyTexture(mesa3d_texture_t *tex, BOOL ctx_cleanup, surface_id surface_delete)
{
	if(tex)
	{
		int i, j;
		mesa3d_entry_t *entry = tex->ctx->entry;

		for(i = 0; i < tex->ctx->tmu_count; i++)
		{
			if(tex->ctx->state.tmu[i].image == tex)
			{
				tex->ctx->state.tmu[i].image  = NULL;
				tex->ctx->state.tmu[i].reload = TRUE;
			}
		}

		if(!ctx_cleanup)
		{
			GL_CHECK(entry->proc.pglDeleteTextures(1, &tex->gltex));
		}

		for(j = 0; j < tex->sides; j++)
		{
			for(i = tex->mipmap_level; i >= 0; i--)
			{
				surface_id sid = tex->data_sid[j][i];
				TOPIC("GARBAGE", "level=%d, side=%d, sid=%d", i, j, sid);
				if(surface_delete != sid)
				{
					SurfaceDeattachTexture(sid, tex, i, j);
				}
			}
		}

		if(!ctx_cleanup)
		{
			tex->ctx->tex[tex->id] = NULL;
		}
		HeapFree(hSharedHeap, 0, tex);
	}
}

static GLenum GetBlendFactor(D3DBLEND dxfactor)
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
    case D3DBLEND_INVDESTALPHA:
    	return GL_ONE_MINUS_DST_ALPHA;
    case D3DBLEND_DESTCOLOR:
    	return GL_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
    	return GL_ONE_MINUS_DST_COLOR;
    case D3DBLEND_SRCALPHASAT:
    	return GL_SRC_ALPHA_SATURATE;
    default:
    	return 0;
	}

	return 0;
}

static GLenum GetGLTexFilter(D3DTEXTUREFILTER dxfilter)
{
	switch(dxfilter)
	{
    case D3DFILTER_LINEAR:
    	return GL_LINEAR;
    case D3DFILTER_MIPNEAREST:
    	return GL_NEAREST_MIPMAP_NEAREST;
    case D3DFILTER_MIPLINEAR:
    	return GL_LINEAR_MIPMAP_LINEAR;
    case D3DFILTER_LINEARMIPNEAREST:
    	return GL_LINEAR_MIPMAP_NEAREST;
    case D3DFILTER_LINEARMIPLINEAR:
    	return GL_NEAREST_MIPMAP_LINEAR;
    case D3DFILTER_NEAREST:
    default:
    	return GL_NEAREST;
	}

	return GL_NEAREST;
}

static GLenum GetGLFogMode(D3DFOGMODE dxfog)
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

static GLenum GetGLCmpFunc(D3DCMPFUNC dxfn)
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
	}
	
	GL_CHECK(entry->proc.pglDisable(GL_FOG));
	TOPIC("FOG", "pglDisable(GL_FOG)");
}

static void MesaSetClipping(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;

	int i;
	int cnt = NOCRT_MIN(MESA_CLIPS_MAX, VMHALenv.num_clips);
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

static void ApplyBlend(mesa3d_ctx_t *ctx)
{
	mesa3d_entry_t *entry = ctx->entry;
	if(ctx->state.blend)
	{
		GL_CHECK(entry->proc.pglEnable(GL_BLEND));
		GL_CHECK(entry->proc.pglBlendFunc(ctx->state.blend_sfactor, ctx->state.blend_dfactor));
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_BLEND));
		//entry->proc.pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	//GL_CHECK(entry->proc.pglDisable(GL_BLEND));
}

void MesaApplyMaterial(mesa3d_ctx_t *ctx)
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

void MesaApplyLighting(mesa3d_ctx_t *ctx)
{
	/* JH: I spend lots of time debuging bad lighting situations,
	   but there is simple formula: no normal set, no lighting
	 */
	if(ctx->state.material.lighting && ctx->state.fvf.pos_normal)
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

void MesaSetTextureState(mesa3d_ctx_t *ctx, int tmu, DWORD state, void *value)
{
	if(tmu >= ctx->tmu_count)
	{
		return;
	}

	struct mesa3d_tmustate *ts = &ctx->state.tmu[tmu];

	switch(state)
	{
		/* texture resource */
		RENDERSTATE(D3DTSS_TEXTUREMAP)
			if(ctx->entry->runtime_ver >= 7)
			{
				ts->image = MesaTextureFromSurfaceHandle(ctx, TSS_DWORD);
			}
			else
			{
				ts->image = MESA_HANDLE_TO_TEX(TSS_DWORD);
			}
			ts->reload = TRUE;
			break;
		/* D3DTEXTUREOP - per-stage blending controls for color channels */
		RENDERSTATE(D3DTSS_COLOROP)
			ts->color_op = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_COLORARG1)
			ts->color_arg1 = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_COLORARG2)
			ts->color_arg2 = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
			break;
		/* D3DTEXTUREOP - per-stage blending controls for alpha channel */
		RENDERSTATE(D3DTSS_ALPHAOP)
			ts->alpha_op = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_ALPHAARG1)
			ts->alpha_arg1 = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
			break;
		/* D3DTA_* (texture arg) */
		RENDERSTATE(D3DTSS_ALPHAARG2)
			ts->alpha_arg2 = TSS_DWORD;
			ts->dx6_blend = TRUE;
			ts->reload = TRUE;
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

			ts->move = TRUE;
			MesaFVFRecalc(ctx);

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
			ts->dx_mag = (D3DTEXTUREMAGFILTER)TSS_DWORD;
			ts->dx6_filter = TRUE;
			ts->update = TRUE;
			break;
		/* D3DTEXTUREMINFILTER filter to use for minification */
		RENDERSTATE(D3DTSS_MINFILTER)
			ts->dx_min = (D3DTEXTUREMINFILTER)TSS_DWORD;
			ts->dx6_filter = TRUE;
			ts->update = TRUE;
			break;
		/* D3DTEXTUREMIPFILTER filter to use between mipmaps during minification */
		RENDERSTATE(D3DTSS_MIPFILTER)
			ts->dx_mip = (D3DTEXTUREMIPFILTER)TSS_DWORD;
			ts->dx6_filter = TRUE;
			ts->update = TRUE;
			break;
		/* D3DVALUE Mipmap LOD bias */
		RENDERSTATE(D3DTSS_MIPMAPLODBIAS)
			ts->miplodbias = TSS_FLOAT;
			ts->dx6_filter = TRUE;
			ts->update = TRUE;
			break;
		/* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
		RENDERSTATE(D3DTSS_MAXMIPLEVEL)
			ts->mipmaxlevel = TSS_DWORD;
			ts->dx6_filter = TRUE;
			ts->update = TRUE;
			break;
		/* DWORD maximum anisotropy */
		RENDERSTATE(D3DTSS_MAXANISOTROPY)
			ts->anisotropy = TSS_DWORD;
			ts->dx6_filter = TRUE;
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
			
			MesaFVFRecalc(ctx);
			
			break;
		}
	} // switch
}

#undef RENDERSTATE

/* from DDK */
#define IS_OVERRIDE(type)       ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)      ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define SET_OVERRIDE(_type) if((_type) < 128){ \
		DWORD wi = (_type) / 32; \
		DWORD bi = (_type) % 32; \
		ctx->state.overrides[wi] |= 1 << bi;}

#define CLEAN_OVERRIDE(_type) if((_type) < 128){ \
		DWORD wi = (_type) / 32; \
		DWORD bi = (_type) % 32; \
		ctx->state.overrides[wi] &= ~(1 << bi);}

#define RETURN_IF_OVERRIDE(_t) do{ \
		DWORD dt = (DWORD)(_t); \
		if(dt < 128){ \
			DWORD wi = dt / 32; \
			DWORD bi = dt % 32; \
			if(ctx->state.overrides[wi] & (1 << bi)){return;} \
		}\
	}while(0)

#define D3DRENDERSTATE_SCENECAPTURE 62
/*
 * I may miss somewere something but this (undocumented) state
 * is send just before DD flip ...
 * Update (after more investigation): it is part of permedia2 sample driver,
 * but not part of the D3DRENDERSTATE enum and all later documentations/headers.
 */

#define RENDERSTATE(_c) case _c: TOPIC("RS", "%s=0x%X", #_c, state->dwArg[0]);

void MesaSetRenderState(mesa3d_ctx_t *ctx, LPD3DSTATE state, LPDWORD RStates)
{
	D3DRENDERSTATETYPE type = state->drstRenderStateType;
	TOPIC("READBACK", "state = %d", type);
	
	if(IS_OVERRIDE(type))
	{
		DWORD override = GET_OVERRIDE(type);
		if(state->dwArg[0])
		{
			SET_OVERRIDE(override);
		}
		else
		{
			CLEAN_OVERRIDE(override);
		}
		
		return;
	}
	
	RETURN_IF_OVERRIDE(type);
	
	if(RStates != NULL)
	{
		RStates[type] = state->dwArg[0];
	}

	mesa3d_entry_t *entry = ctx->entry;

	switch(type)
	{
		RENDERSTATE(D3DRENDERSTATE_TEXTUREHANDLE) /* Texture handle */
			if(state->dwArg[0] != 0)
			{
				if(ctx->entry->runtime_ver >= 7)
				{
					ctx->state.tmu[0].image = MesaTextureFromSurfaceHandle(ctx, state->dwArg[0]);
				}
				else
				{
					ctx->state.tmu[0].image = MESA_HANDLE_TO_TEX(state->dwArg[0]);
				}
			}
			else
			{
				ctx->state.tmu[0].image = NULL;
			}
			TRACE("D3DRENDERSTATE_TEXTUREHANDLE = %X", state->dwArg[0]);
			ctx->state.tmu[0].reload = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ANTIALIAS) /* D3DANTIALIASMODE */
		{
			D3DANTIALIASMODE mode = (D3DANTIALIASMODE)state->dwArg[0];
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
			ctx->state.tmu[0].texaddr_u = state->dwArg[0];
			ctx->state.tmu[0].texaddr_v = state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREPERSPECTIVE) /* TRUE for perspective correction */
			ctx->state.texperspective = state->dwArg[0] == 0 ? FALSE : TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPU) /* TRUE for wrapping in u */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPV) /* TRUE for wrapping in v */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ZENABLE) /* TRUE to enable z test (DX7 = D3DZBUFFERTYPE) */
			if(ctx->depth_bpp)
			{
				switch(state->dwArg[0])
				{
					case D3DZB_FALSE: /* disabled */
						GL_CHECK(entry->proc.pglDisable(GL_DEPTH_TEST));
						ctx->state.depth.enabled = FALSE;
						ctx->state.depth.wbuffer = FALSE;
						break;
					case D3DZB_USEW: /* enabled W */
						GL_CHECK(entry->proc.pglEnable(GL_DEPTH_TEST));
						ctx->state.depth.enabled = TRUE;
						ctx->state.depth.wbuffer = TRUE;
						break;
					case D3DZB_TRUE: /* enabled Z */
					default: /* != FALSE */
						GL_CHECK(entry->proc.pglEnable(GL_DEPTH_TEST));
						ctx->state.depth.enabled = TRUE;
						ctx->state.depth.wbuffer = FALSE;
						break;
				}
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_FILLMODE) /* D3DFILL_MODE	*/
			switch((D3DFILLMODE)state->dwArg[0])
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
			switch((D3DSHADEMODE)state->dwArg[0])
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
			TOPIC("LIGHT", "SHADEMODE=%d", state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_LINEPATTERN) /* D3DLINEPATTERN */
		{
			WORD *pattern = (WORD*)state->dwArg;
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
			WARN("D3DRENDERSTATE_MONOENABLE=0x%X", state->dwArg[0]);
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ROP2) /* ROP2 */
			WARN("D3DRENDERSTATE_ROP2=0x%X", state->dwArg[0]);
			ctx->render.rop2 = state->dwArg[0] & 0xF;
			break;
		RENDERSTATE(D3DRENDERSTATE_PLANEMASK) /* DWORD physical plane mask */
			WARN("D3DRENDERSTATE_PLANEMASK=0x%X", state->dwArg[0]);
			ctx->render.planemask = state->dwArg[0];
			break;
		RENDERSTATE(D3DRENDERSTATE_ZWRITEENABLE) /* TRUE to enable z writes */
			if(ctx->depth_bpp)
			{
				if(state->dwArg[0] != 0)
				{
					GL_CHECK(entry->proc.pglDepthMask(GL_TRUE));
					ctx->state.depth.writable = TRUE;
				}
				else
				{
					GL_CHECK(entry->proc.pglDepthMask(GL_FALSE));
					ctx->state.depth.writable = FALSE;
				}
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHATESTENABLE) /* TRUE to enable alpha tests */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHATESTENABLE = 0x%X", state->dwArg[0]);
			ctx->state.alpha.enabled = state->dwArg[0] != 0 ? TRUE : FALSE;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_LASTPIXEL) /* TRUE for last-pixel on lines */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAG) /* D3DTEXTUREFILTER */
			ctx->state.tmu[0].texmag = GetGLTexFilter((D3DTEXTUREFILTER)state->dwArg[0]);
			ctx->state.tmu[0].dx6_filter = FALSE;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMIN) /* D3DTEXTUREFILTER */
			ctx->state.tmu[0].texmin = GetGLTexFilter((D3DTEXTUREFILTER)state->dwArg[0]);
			ctx->state.tmu[0].dx6_filter = FALSE;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_SRCBLEND) /* D3DBLEND */
		{
			TOPIC("BLEND", "D3DRENDERSTATE_SRCBLEND = 0x%X", state->dwArg[0]);
			D3DBLEND dxblend = (D3DBLEND)state->dwArg[0];
			/* WINE: BLEND_BOTHSRCALPHA and BLEND_BOTHINVSRCALPHA are legacy
			 * source blending values which are still valid up to d3d9. They should
			 * not occur as dest blend values. */
			if(dxblend == D3DBLEND_BOTHSRCALPHA)
			{
				ctx->state.blend_sfactor = GL_SRC_ALPHA;
				ctx->state.blend_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if(dxblend == D3DBLEND_BOTHINVSRCALPHA)
			{
				ctx->state.blend_sfactor = GL_ONE_MINUS_SRC_ALPHA;
				ctx->state.blend_dfactor = GL_SRC_ALPHA;
			}
			else
			{
				ctx->state.blend_sfactor = GetBlendFactor(dxblend);
			}
			ApplyBlend(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_DESTBLEND) /* D3DBLEND */
			TOPIC("BLEND", "D3DRENDERSTATE_DESTBLEND = 0x%X", state->dwArg[0]);
			ctx->state.blend_dfactor = GetBlendFactor((D3DBLEND)state->dwArg[0]);
			ApplyBlend(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAPBLEND) /* D3DTEXTUREBLEND */
			ctx->state.tmu[0].texblend = (D3DTEXTUREBLEND)state->dwArg[0];
			ctx->state.tmu[0].dx6_blend = FALSE;
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_CULLMODE) /* D3DCULL */
			switch(state->dwArg[0])
			{
				case D3DCULL_NONE:
					GL_CHECK(entry->proc.pglDisable(GL_CULL_FACE));
					break;
				case D3DCULL_CW:
					GL_CHECK(entry->proc.pglEnable(GL_CULL_FACE));
					GL_CHECK(entry->proc.pglCullFace(GL_FRONT));
					break;
				case D3DCULL_CCW:
					GL_CHECK(entry->proc.pglEnable(GL_CULL_FACE));
					GL_CHECK(entry->proc.pglCullFace(GL_BACK));
					break;
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ZFUNC) /* D3DCMPFUNC */
			GL_CHECK(entry->proc.pglDepthFunc(GetGLCmpFunc(state->dwArg[0])));
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAREF) /* D3DFIXED */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAREF = 0x%X", state->dwArg[0]);
			ctx->state.alpha.ref = (GLclampf)(state->dwArg[0] * MESA_1OVER255); 
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAFUNC) /* D3DCMPFUNC */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAFUNC = 0x%X", state->dwArg[0]);
			ctx->state.alpha.func = GetGLCmpFunc(state->dwArg[0]);
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_DITHERENABLE) /* TRUE to enable dithering */
			if(state->dwArg[0])
			{
				GL_CHECK(entry->proc.pglEnable(GL_DITHER));
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_DITHER);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHABLENDENABLE) /* TRUE to enable alpha blending */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHABLENDENABLE = 0x%X", state->dwArg[0]);
			ctx->state.blend = (state->dwArg[0] != 0) ? TRUE : FALSE;
			ApplyBlend(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_FOGENABLE) /* TRUE to enable fog */
			ctx->state.fog.enabled = state->dwArg[0] == 0 ? FALSE : TRUE;
			TOPIC("FOG", "D3DRENDERSTATE_FOGENABLE=%X", state->dwArg[0]);
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_SPECULARENABLE) /* TRUE to enable specular */
			// need EXT_SECONDARY_COLOR
			if(state->dwArg[0])
			{
				ctx->state.specular = TRUE;
			}
			else
			{
				ctx->state.specular = FALSE;
			}
			MesaApplyColorMaterial(ctx);
			TOPIC("LIGHT", "SPECULARENABLE=%d", state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_ZVISIBLE) /* TRUE to enable z checking */
			ctx->state.zvisible = state->dwArg[0];
			break;
		RENDERSTATE(D3DRENDERSTATE_SUBPIXEL) /* TRUE to enable subpixel correction (<= d3d6) */
			break;
		RENDERSTATE(D3DRENDERSTATE_SUBPIXELX) /* TRUE to enable correction in X only (<= d3d6) */
			break;
		RENDERSTATE(D3DRENDERSTATE_STIPPLEDALPHA) /* TRUE to enable stippled alpha */
			break;
		RENDERSTATE(D3DRENDERSTATE_FOGCOLOR) /* D3DCOLOR */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwArg[0];
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
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEMODE=%X", state->dwArg[0]);
  		ctx->state.fog.tmode = GetGLFogMode(state->dwArg[0]);
  		MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLESTART) /* Fog table start (float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLESTART=%f", state->dvArg[0]);
			ctx->state.fog.start = state->dvArg[0];
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLEEND) /* Fog table end (float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEEND=%f", state->dvArg[0]);
			ctx->state.fog.end = state->dvArg[0];
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGTABLEDENSITY) /* Fog table density (probably float)	*/
		{
			TOPIC("FOG", "D3DRENDERSTATE_FOGTABLEDENSITY=%f", state->dvArg[0]);
			ctx->state.fog.density = state->dvArg[0];
			MesaSetFog(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_STIPPLEENABLE) /* TRUE to enable stippling (<= d3d6) */
			if(state->dwArg[0])
			{
				GL_CHECK(entry->proc.pglEnable(GL_POLYGON_STIPPLE));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_POLYGON_STIPPLE));
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_EDGEANTIALIAS) /* TRUE to enable edge antialiasing */
			// !
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORKEYENABLE) /* TRUE to enable source colorkeyed textures */
			ctx->state.tmu[0].colorkey = (state->dwArg[0] == 0) ? FALSE : TRUE;
			ctx->state.tmu[0].reload = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_BORDERCOLOR) /* Border color for texturing w/border */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwArg[0];
			MESA_D3DCOLOR_TO_FV(c, ctx->state.tmu[0].border);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESSU) /* Texture addressing mode for U coordinate */
			ctx->state.tmu[0].texaddr_u = state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESSV) /* Texture addressing mode for V coordinate */
			ctx->state.tmu[0].texaddr_v = state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_MIPMAPLODBIAS) /* D3DVALUE Mipmap LOD bias */
			ctx->state.tmu[0].miplodbias = state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_ZBIAS) /* LONG Z bias */
		{
			LONG bias = (LONG)state->dwArg[0];
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
			TOPIC("FOG", "D3DRENDERSTATE_RANGEFOGENABLE=%X", state->dwArg[0]);
			ctx->state.fog.range = state->dwArg[0] == 0 ? FALSE : TRUE;
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_ANISOTROPY) /* Max. anisotropy. 1 = no anisotropy */
			ctx->state.tmu[0].anisotropy = state->dwArg[0];
			break;
		RENDERSTATE(D3DRENDERSTATE_FLUSHBATCH) /* Explicit flush for DP batching (DX5 Only) */
			/* nop */
			break;
		/* d3d6 */
		RENDERSTATE(D3DRENDERSTATE_STENCILENABLE) /* BOOL enable/disable stenciling */
			ctx->state.stencil.enabled = state->dwArg[0] == 0 ? FALSE : TRUE;
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFAIL) /* D3DSTENCILOP to do if stencil test fails */
			ctx->state.stencil.sfail = DXSencilToGL(state->dwArg[0]);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILZFAIL) /* D3DSTENCILOP to do if stencil test passes and Z test fails */
			ctx->state.stencil.dpfail = DXSencilToGL(state->dwArg[0]);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILPASS) /* D3DSTENCILOP to do if both stencil and Z tests pass */
			ctx->state.stencil.dppass = DXSencilToGL(state->dwArg[0]);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFUNC) /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
			ctx->state.stencil.func = GetGLCmpFunc(state->dwArg[0]);
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILREF) /* Reference value used in stencil test */
			ctx->state.stencil.ref = state->dwArg[0];
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILMASK) /* Mask value used in stencil test */
			ctx->state.stencil.mask = state->dwArg[0];
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILWRITEMASK) /* Write mask applied to values written to stencil buffer */
			ctx->state.stencil.writemask = state->dwArg[0];
			MesaStencilApply(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREFACTOR) /* D3DCOLOR used for multi-texture blend */
		{
			D3DCOLOR c = (D3DCOLOR)state->dwArg[0];
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
			ctx->state.stipple[type - D3DRENDERSTATE_STIPPLEPATTERN00] = state->dwArg[0];
			GL_CHECK(entry->proc.pglPolygonStipple((GLubyte*)&ctx->state.stipple[0]));
			break;
		}
		case D3DRENDERSTATE_WRAP0 ... D3DRENDERSTATE_WRAP7: /* wrap for 1-8 texture coord. set */
		{
			ctx->state.tmu[type - D3DRENDERSTATE_WRAP0].wrap = state->dwArg[0];
			ctx->state.tmu[type - D3DRENDERSTATE_WRAP0].update = TRUE;
			break;
		}
		/* d3d7 */
		RENDERSTATE(D3DRENDERSTATE_CLIPPING)
			ctx->state.clipping.enabled = (state->dwArg[0] == 0) ? FALSE : TRUE;
			MesaSetClipping(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_LIGHTING)
			if(state->dwArg[0])
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
			WARN("D3DRENDERSTATE_EXTENTS=0x%X", state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENT)
		{
			GLfloat v[4];
			MESA_D3DCOLOR_TO_FV(state->dwArg[0], v);
			TOPIC("LIGHT", "global ambient = (%f %f %f %f)", v[0], v[1], v[2], v[3]);
			GL_CHECK(entry->proc.pglLightModelfv(GL_LIGHT_MODEL_AMBIENT, &v[0]));
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_FOGVERTEXMODE)
			TOPIC("FOG", "D3DRENDERSTATE_FOGVERTEXMODE=0x%X", state->dwArg[0]);
			ctx->state.fog.vmode = GetGLFogMode(state->dwArg[0]);
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORVERTEX)
			ctx->state.material.color_vertex = state->dwArg[0] != 0 ? TRUE : FALSE;
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_LOCALVIEWER)
			GL_CHECK(entry->proc.pglLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, state->dwArg[0] == 0 ? 0 : 1));
			break;
		RENDERSTATE(D3DRENDERSTATE_NORMALIZENORMALS)
			if(state->dwArg[0] != 0)
			{
				GL_CHECK(entry->proc.pglEnable(GL_NORMALIZE));
			}
			else
			{
				GL_CHECK(entry->proc.pglDisable(GL_NORMALIZE));
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORKEYBLENDENABLE)
			WARN("D3DRENDERSTATE_COLORKEYBLENDENABLE=0x%X", state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE)
			ctx->state.material.diffuse_source = state->dwArg[0];
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_SPECULARMATERIALSOURCE)
			ctx->state.material.specular_source = state->dwArg[0];
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENTMATERIALSOURCE)
			ctx->state.material.ambient_source = state->dwArg[0];
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE)
			ctx->state.material.emissive_source = state->dwArg[0];
			MesaApplyColorMaterial(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_VERTEXBLEND)
			switch(state->dwArg[0])
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
			int cnt = NOCRT_MIN(MESA_CLIPS_MAX, VMHALenv.num_clips);
			DWORD b = state->dwArg[0];
			for(i = 0; i < cnt; i++)
			{
				ctx->state.clipping.activeplane[i] = ((b & 0x1) == 0) ? FALSE : TRUE;
				b >>= 1;
			}
			MesaSetClipping(ctx);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT)
			WARN("D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=0x%X", state->dwArg[0]);
			break;
		default:
			switch((DWORD)type)
			{
				RENDERSTATE(D3DRENDERSTATE_SCENECAPTURE)
					TOPIC("READBACK", "D3DRENDERSTATE_SCENECAPTURE %d", state->dwArg[0]);
					switch(state->dwArg[0])
					{
						case 0: /* just before flip (end scene) */
							MesaSceneEnd(ctx);
							break;
						case 1: /* after clear (start scene) */
							MesaSceneBegin(ctx);
							break;
					}
					break;
				default:
					WARN("Unknown render state: %d (0x%X)", type, type);
					break;
			}
			/* NOP */
			break;
	}
}

#undef RENDERSTATE

static GLenum DX2GLPrimitive(D3DPRIMITIVETYPE dx_type)
{
	GLenum gl_type = -1;
	
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

static GLenum nonMipFilter(GLenum filter)
{
	switch(filter)
	{
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
			return GL_LINEAR;
	}

	return GL_NEAREST;
}

static void D3DTA2GL(DWORD dxarg, GLint *gl_src, GLint *gl_op)
{
	switch(dxarg & D3DTA_SELECTMASK)
	{
		case D3DTA_CURRENT:
			*gl_src = GL_PREVIOUS;
			break;
		case D3DTA_DIFFUSE:
			*gl_src = GL_PRIMARY_COLOR;
			break;
		case D3DTA_TFACTOR:
			*gl_src = GL_CONSTANT_EXT;
			break;
		case D3DTA_TEXTURE:
		case D3DTA_SPECULAR: /* not possible, fail to texture */
		default:
			*gl_src = GL_TEXTURE;
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

static void ApplyTextureState(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, int tmu)
{
	TRACE("ApplyTextureState(..., ..., %d)", tmu);
	GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0+tmu));
	
	struct mesa3d_tmustate *ts = &ctx->state.tmu[tmu];
	BOOL color_key = FALSE;
	GLenum target = GL_TEXTURE_2D;

	if(ts->image)
	{
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_CUBE_MAP));
		ts->active = TRUE;
		
		if(ts->image->colorkey && tmu == 0)
		{
			color_key = TRUE;
		}
		
		if(!ts->image->cube)
		{
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ts->image->gltex));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, 0));
		}
		else
		{
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, 0));
			GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_CUBE_MAP, ts->image->gltex));
		}

		/*
		 * texture filtering
		 */
		if(ts->image->cube)
		{
			target = GL_TEXTURE_CUBE_MAP;
		}

		if(ts->dx6_filter)
		{
			BOOL use_anisotropic = FALSE;
			if(ts->image->mipmap && ts->dx_mip != D3DTFP_NONE)
			{
				GLint maxlevel = ts->image->mipmap_level;
				GLint minlevel = ts->mipmaxlevel;
				if(minlevel > maxlevel)
				{
					minlevel = maxlevel;
				}

				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_BASE_LEVEL, minlevel));
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxlevel));
				GL_CHECK(entry->proc.pglTexParameterf(target, GL_TEXTURE_LOD_BIAS, ts->miplodbias));

				switch(ts->dx_min)
				{
					case D3DTFN_ANISOTROPIC:
						GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
						use_anisotropic = TRUE;
						break;
					case D3DTFN_LINEAR:
						switch(ts->dx_mip)
						{
							case D3DTFP_LINEAR:
								GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
								break;
							case D3DTFP_POINT:
							default:
								GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
								break;
						}
						
						break;
					case D3DTFN_POINT:
					default:
						switch(ts->dx_mip)
						{
							case D3DTFP_LINEAR:
								GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR));
								break;
							case D3DTFP_POINT:
							default:
								GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
								break;
						}
						break;
				}
			}
			else /* non mipmap */
			{
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0));
				switch(ts->dx_min)
				{
					case D3DTFN_LINEAR:
						use_anisotropic = TRUE;
						/* thru */
					case D3DTFN_ANISOTROPIC:
						GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
						break;
					case D3DTFN_POINT:
					default:
						GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
						break;
				}
			}

			switch(ts->dx_mag)
			{
				case D3DTFG_ANISOTROPIC:
					use_anisotropic = TRUE;
					/* thru */
				case D3DTFG_LINEAR:
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
					break;
				case D3DTFG_POINT:
				default:
					GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
					break;
			}
			
			
			/* needs EXT_texture_filter_anisotropic */
			if(VMHALenv.max_anisotropy > 1)
			{
				GLfloat fanisotropy = 1.0;

				if(use_anisotropic)
					fanisotropy = ts->anisotropy;

				if(fanisotropy < 1.0)
					fanisotropy = 1.0;
		
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, fanisotropy));
			}
		}
		else /* !dx6_filter */
		{
			if(ts->image->mipmap)
			{
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, ts->image->mipmap_level));
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, ts->texmag));
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, ts->texmin));
			}
			else
			{
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0));
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MAG_FILTER, nonMipFilter(ts->texmag)));
				GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_MIN_FILTER, nonMipFilter(ts->texmin)));
			}
		}
	}
	else /* !image */
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_CUBE_MAP));
		ts->active = FALSE;
		//GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, 0));
	}
	
	/*
	 * Texture blend
	 */
	if(ts->dx6_blend)
	{
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
		GLint alpha_arg3_op = GL_SRC_COLOR;
		
		D3DTA2GL(ts->color_arg1, &color_arg1_source, &color_arg1_op);
		D3DTA2GL(ts->color_arg2, &color_arg2_source, &color_arg2_op);
		D3DTA2GL(ts->alpha_arg1|D3DTA_ALPHAREPLICATE, &alpha_arg1_source, &alpha_arg1_op);
		D3DTA2GL(ts->alpha_arg2|D3DTA_ALPHAREPLICATE, &alpha_arg2_source, &alpha_arg2_op);
		
		GLfloat color_mult = 1.0f;
		GLfloat alpha_mult = 1.0f;
		
		switch(ts->color_op)
		{
			case D3DTOP_DISABLE:
				color_fn = GL_REPLACE;
				color_arg1_source = GL_PREVIOUS; //GL_CONSTANT;
				color_arg1_op = GL_SRC_COLOR;
				break;
			case D3DTOP_SELECTARG1:
				color_fn = GL_REPLACE;
				break;
			case D3DTOP_SELECTARG2:
				color_fn = GL_REPLACE;
				color_arg1_source = color_arg2_source;
				color_arg1_op = color_arg2_op;
				break;
			case D3DTOP_MODULATE:
				color_fn = GL_MODULATE;
				break;
			case D3DTOP_MODULATE2X:
				color_fn = GL_MODULATE;
				color_mult = 2.0f;
				break;
			case D3DTOP_MODULATE4X:
				color_fn = GL_MODULATE;
				color_mult = 4.0f;
				break;
			case D3DTOP_ADD:
				color_fn = GL_ADD;
				break;
			case D3DTOP_ADDSIGNED:
				color_fn = GL_ADD_SIGNED;
				break;
			case D3DTOP_ADDSIGNED2X:
				color_fn = GL_ADD_SIGNED;
				color_mult = 2.0f;
				break;
			case D3DTOP_SUBTRACT:
				color_fn = GL_SUBTRACT;
				break;
			case D3DTOP_BLENDDIFFUSEALPHA:
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_PRIMARY_COLOR;
				color_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDTEXTUREALPHA:
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_TEXTURE;
				color_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDFACTORALPHA:
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_CONSTANT;
				color_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDCURRENTALPHA:
				color_fn = GL_INTERPOLATE;
				color_arg3_source = GL_PREVIOUS;
				color_arg3_op = GL_SRC_ALPHA;
				break;
			/*
					NOTE: for another states we can use this:
						https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_env_combine4.tx
			*/
			default:
				WARN("Unknown color texture blend operation: %d, TMU: %d", ts->color_op, tmu);
				break;
				
		}
		
		switch(ts->alpha_op)
		{
			case D3DTOP_DISABLE:
				alpha_fn = GL_REPLACE;
				alpha_arg1_source = GL_PREVIOUS;
				alpha_arg1_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_SELECTARG1:
				alpha_fn = GL_REPLACE;
				break;
			case D3DTOP_SELECTARG2:
				alpha_fn = GL_REPLACE;
				alpha_arg1_source = alpha_arg2_source;
				alpha_arg1_op = alpha_arg2_op;
				break;
			case D3DTOP_MODULATE:
				alpha_fn = GL_MODULATE;
				break;
			case D3DTOP_MODULATE2X:
				alpha_fn = GL_MODULATE;
				alpha_mult = 2.0f;
				break;
			case D3DTOP_MODULATE4X:
				alpha_fn = GL_MODULATE;
				alpha_mult = 4.0f;
				break;
			case D3DTOP_ADD:
				alpha_fn = GL_ADD;
				break;
			case D3DTOP_ADDSIGNED:
				alpha_fn = GL_ADD_SIGNED;
				break;
			case D3DTOP_ADDSIGNED2X:
				alpha_fn = GL_ADD_SIGNED;
				alpha_mult = 2.0f;
				break;
			case D3DTOP_SUBTRACT:
				alpha_fn = GL_SUBTRACT;
				break;
			case D3DTOP_BLENDDIFFUSEALPHA:
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_PRIMARY_COLOR;
				alpha_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDTEXTUREALPHA:
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_TEXTURE;
				alpha_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDFACTORALPHA:
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_CONSTANT;
				alpha_arg3_op = GL_SRC_ALPHA;
				break;
			case D3DTOP_BLENDCURRENTALPHA:
				alpha_fn = GL_INTERPOLATE;
				alpha_arg3_source = GL_PREVIOUS;
				alpha_arg3_op = GL_SRC_ALPHA;
				break;
			default:
				WARN("Unknown alpha texture blend operation: %d, TMU: %d", ts->color_op, tmu);
				break;
		}

	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, color_fn));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, alpha_fn));
	  
		GL_CHECK(entry->proc.pglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &ctx->state.tfactor[0]));
		
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, color_arg1_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, color_arg1_op));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, color_arg2_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, color_arg2_op));

	  if(color_fn == GL_INTERPOLATE)
	  {
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, color_arg3_source));
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, color_arg3_op));
	  }

	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, alpha_arg1_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, alpha_arg1_op));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, alpha_arg2_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, alpha_arg2_op));
	  
	  if(alpha_fn == GL_INTERPOLATE)
	  {
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_ALPHA, alpha_arg3_source));
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, alpha_arg3_op));
	  }

	  GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,   color_mult));
	  GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, alpha_mult));

	}
	else
	{
		switch(ts->texblend)
		{
			case D3DTBLEND_DECALMASK: /* failback to decal */
			case D3DTBLEND_DECAL:
			case D3DTBLEND_COPY:
				/* 
				 * cPix = cTex
				 * aPix = aTex
				 */
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
				TOPIC("BLEND", "D3DTBLEND_DECAL");
				break;
			case D3DTBLEND_MODULATEMASK:  /* failback to modulate */
			case D3DTBLEND_MODULATE:
				/*
				 * cPix = cSrc * cTex
				 * aPix = aTex
				 */
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
				TOPIC("BLEND", "D3DTBLEND_MODULATE");
				break;
			case D3DTBLEND_MODULATEALPHA:
				/*
				 * cPix = cSrc * cTex
				 * aPix = aSrc * aTex
				 */
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE));			
				TOPIC("BLEND", "D3DTBLEND_MODULATEALPHA");
				break;
			case D3DTBLEND_ADD:
				/*
				 * cPix = cSrc + cTex
				 * aPix = aSrc + aTex
				 */
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD));
				TOPIC("BLEND", "D3DTBLEND_ADD");
				break;
			case D3DTBLEND_DECALALPHA:
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_TEXTURE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA));

				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR));
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA));
				break;
			default:
				TOPIC("BLEND", "Wrong state: %d", ts->texblend);
				/* NOP */
				break;

			GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,   1.0));
			GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0));
		}
	} // !dx6_blend

	/*
	 * Texture addressing
	 */
	if(ts->texaddr_u == D3DTADDRESS_BORDER ||
		ts->texaddr_v == D3DTADDRESS_BORDER)
	{
		GL_CHECK(entry->proc.pglTexParameterfv(target,
			GL_TEXTURE_BORDER_COLOR, &(ts->border[0])));
	}

	switch(ts->texaddr_u)
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
		case D3DTADDRESS_WRAP:
		default:
			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
			break;
	}

	switch(ts->texaddr_v)
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
		case D3DTADDRESS_WRAP:
		default:
			GL_CHECK(entry->proc.pglTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
			break;
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

static void setTexGen(mesa3d_entry_t *entry, mesa3d_ctx_t *ctx, int num_coords)
{
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
}

void MesaDrawRefreshState(mesa3d_ctx_t *ctx)
{
	int i;
	static const GLfloat s_plane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat t_plane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	static const GLfloat r_plane[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	static const GLfloat q_plane[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	for(i = 0; i < ctx->tmu_count; i++)
	{
		if(ctx->state.tmu[i].image != NULL)
		{
			if(ctx->state.tmu[i].image->dirty)
			{
				ctx->state.tmu[i].reload = TRUE;
			}
		}
		
		if(ctx->state.tmu[i].reload)
		{
			if(ctx->state.tmu[i].image != NULL)
			{
				MesaReloadTexture(ctx->state.tmu[i].image, i);
			}
			ctx->state.tmu[i].update = TRUE;
			ctx->state.tmu[i].reload = FALSE;
		}
		
		if(ctx->state.tmu[i].update)
		{
			ApplyTextureState(ctx->entry, ctx, i);
			ctx->state.tmu[i].update = FALSE;
		}
		
		if(ctx->state.tmu[i].move)
		{
			mesa3d_entry_t *entry = ctx->entry;
			
			GL_CHECK(entry->proc.pglActiveTexture(GL_TEXTURE0 + i));
//			GL_CHECK(entry->proc.pglMatrixMode(GL_TEXTURE));
//			GL_CHECK(entry->proc.pglLoadMatrixf(&ctx->state.tmu[i].matrix[0]));

			switch(ctx->state.tmu[i].mapping)
			{
				case D3DTSS_TCI_CAMERASPACENORMAL:
					TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACENORMAL: %d", i, ctx->state.tmu[i].coordscalc);

					MesaSpaceModelviewSet(ctx);
					GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
					MesaSpaceModelviewReset(ctx);

					GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));
					GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));
					GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP));

					setTexGen(entry, ctx, ctx->state.tmu[i].coordscalc);
					break;
				case D3DTSS_TCI_CAMERASPACEPOSITION:
					TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACEPOSITION: %d", i, ctx->state.tmu[i].coordscalc);

					MesaSpaceModelviewSet(ctx);
					GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
					MesaSpaceModelviewReset(ctx);

					GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
					GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
					GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));

					setTexGen(entry, ctx, ctx->state.tmu[i].coordscalc);
					break;
				case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
					TOPIC("MAPPING", "(%d)D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR: %d", i, ctx->state.tmu[i].coordscalc);

					MesaSpaceModelviewSet(ctx);
					GL_CHECK(entry->proc.pglTexGenfv(GL_S, GL_EYE_PLANE, s_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_T, GL_EYE_PLANE, t_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_R, GL_EYE_PLANE, r_plane));
					GL_CHECK(entry->proc.pglTexGenfv(GL_Q, GL_EYE_PLANE, q_plane));
					MesaSpaceModelviewReset(ctx);

					GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));
					GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));
					GL_CHECK(entry->proc.pglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP));

					setTexGen(entry, ctx, ctx->state.tmu[i].coordscalc);
					break;
				case D3DTSS_TCI_SPHEREMAP:
					TOPIC("MAPPING", "(%d)D3DTSS_TCI_SPHEREMAP: %d", i, ctx->state.tmu[i].coordscalc);
					
					GL_CHECK(entry->proc.pglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP));
					GL_CHECK(entry->proc.pglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP));

					setTexGen(entry, ctx, 2);
					break;
				case D3DTSS_TCI_PASSTHRU:
				default:
					TOPIC("MAPPING", "(%d)D3DTSS_TCI_PASSTHRU: %d", i, ctx->state.tmu[i].coordscalc);
					setTexGen(entry, ctx, 0);
					break;
			}
			
			ctx->state.tmu[i].move = FALSE;
		}
	}
}

void MesaDraw(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype, LPVOID vertices, DWORD verticesCnt)
{
	mesa3d_entry_t *entry = ctx->entry;

//	MesaDrawSetSurfaces(ctx);

	GLenum gl_ptype = DX2GLPrimitive(dx_ptype);

	if(gl_ptype != -1)
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

void MesaDraw3(mesa3d_ctx_t *ctx, DWORD op, void *prim, LPBYTE vertices)
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

void MesaDrawIndex(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype,
	LPVOID vertices, DWORD verticesCnt,
	LPWORD indices, DWORD indicesCnt)
{
	mesa3d_entry_t *entry = ctx->entry;
	DWORD i;
	
//	MesaDrawSetSurfaces(ctx);

	GLenum gl_ptype = DX2GLPrimitive(dx_ptype);

	if(gl_ptype != -1)
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

static BOOL IsFullSurface(mesa3d_ctx_t *ctx, RECT *rect)
{
	return
		rect->left == 0 &&
		rect->top == 0 &&
		rect->right == ctx->state.sw &&
		rect->bottom == ctx->state.sh;
}

void MesaClear(mesa3d_ctx_t *ctx, DWORD flags, D3DCOLOR color, D3DVALUE depth, DWORD stencil, int rects_cnt, RECT *rects)
{
	GLfloat cv[4];
	int i;
	mesa3d_entry_t *entry = ctx->entry;
		
	TOPIC("READBACK", "Clear=%X", flags);

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
		TOPIC("CLEAR", "full clear");
		entry->proc.pglClear(mask);
		//entry->proc.pglFlush();
	}
	else
	{
		TOPIC("CLEAR", "partly clean");
#if 0
		if(flags & D3DCLEAR_TARGET)
		{
			void *ptr = SurfaceGetVidMem(ctx->backbuffer);
			if(ptr)
			{
				MesaBufferUploadColor(ctx, ptr);
			}
		}
		
		if(ctx->depth_bpp && (flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)))
		{
			void *ptr = SurfaceGetVidMem(ctx->depth);
			MesaBufferUploadDepth(ctx, ptr);
		}
#endif
		TOPIC("READBACK", "Clear %X => %X, %f, %X", flags, color, depth, stencil);
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

void MesaBlockLock(mesa3d_ctx_t *ctx)
{
	LONG tmp;
	do
	{
		tmp = InterlockedExchange(&ctx->thread_lock, 1);
	} while(tmp == 1);
}

void MesaBlockUnlock(mesa3d_ctx_t *ctx)
{
	InterlockedExchange(&ctx->thread_lock, 0);
}

void MesaSceneBegin(mesa3d_ctx_t *ctx)
{
	if(!ctx->render.dirty)
	{
		void *ptr = SurfaceGetVidMem(ctx->backbuffer);
		if(ptr)
		{
			MesaBufferUploadColor(ctx, ptr);
		}
	}
}

void MesaSceneEnd(mesa3d_ctx_t *ctx)
{
	BOOL is_visible = MesaBackbufferIfFront(ctx);
	void *ptr = SurfaceGetVidMem(ctx->backbuffer);
	if(ptr)
	{
		if(is_visible) /* fixme: check for DDSCAPS_PRIMARYSURFACE */
			FBHDA_access_begin(0);

		TOPIC("TARGET", "MesaBufferDownloadColor(ctx, 0x%X)", ptr);
		if(ctx->render.dirty)
		{
			MesaBufferDownloadColor(ctx, ptr);
		}
		
		if(ctx->state.textarget)
		{
			DDSURF *surf = SurfaceGetSURF(ctx->backbuffer);
			TOPIC("TEXTARGET", "Render to texture");
			SurfaceToMesa(surf->lpLcl, TRUE);
		}

		if(is_visible)
			FBHDA_access_end(0);
	}

	//ctx->entry->proc.pglFinish();

	if(mesa_dump_key() == MESA_KEY_DUMP)
	{
		mesa_dump(ctx);
		mesa_dump_inc();
	}
}

#define COPY_MATRIX(_srcdx, _dst) do{ \
	(_dst)[ 0]=(_srcdx)->_11; (_dst)[ 1]=(_srcdx)->_12; (_dst)[ 2]=(_srcdx)->_13; (_dst)[ 3]=(_srcdx)->_14; \
	(_dst)[ 4]=(_srcdx)->_21; (_dst)[ 5]=(_srcdx)->_22; (_dst)[ 6]=(_srcdx)->_23; (_dst)[ 7]=(_srcdx)->_24; \
	(_dst)[ 8]=(_srcdx)->_31; (_dst)[ 9]=(_srcdx)->_32; (_dst)[10]=(_srcdx)->_33; (_dst)[11]=(_srcdx)->_34; \
	(_dst)[12]=(_srcdx)->_41; (_dst)[13]=(_srcdx)->_42; (_dst)[14]=(_srcdx)->_43; (_dst)[15]=(_srcdx)->_44; \
	}while(0)

void MesaSetTransform(mesa3d_ctx_t *ctx, DWORD xtype, D3DMATRIX *matrix)
{
	TOPIC("MATRIX", "MesaSetTransform(ctx, 0x%X, %p)", xtype, matrix);

	/**
		This is undocumented and comes from reference driver.
		Original note:
		  BUGBUG is there a define for 0x80000000?
	**/
	BOOL set_identity = (xtype & 0x80000000) == 0 ? FALSE : TRUE;
	D3DTRANSFORMSTATETYPE state = (D3DTRANSFORMSTATETYPE)(xtype & 0x7FFFFFFF);
	GLfloat m[16];
	if(!set_identity && matrix != NULL)
	{
		COPY_MATRIX(matrix, m);
	}
	else
	{
		MesaIdentity(m);
	}
	
	switch(state)
	{
		case D3DTRANSFORMSTATE_WORLD:
			memcpy(ctx->matrix.world[0], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_VIEW:
			if(memcmp(ctx->matrix.view, m, sizeof(m)) != 0)
			{
				memcpy(ctx->matrix.view, m, sizeof(m));
				MesaApplyTransform(ctx, MESA_TF_VIEW);
			}
			break;
		case D3DTRANSFORMSTATE_PROJECTION:
			if(memcmp(ctx->matrix.proj, m, sizeof(m)) != 0)
			{
				memcpy(ctx->matrix.proj, m, sizeof(m));
				MesaApplyTransform(ctx, MESA_TF_PROJECTION);
			}
			break;
		case D3DTRANSFORMSTATE_WORLD1:
			memcpy(ctx->matrix.world[1], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_WORLD2:
			memcpy(ctx->matrix.world[2], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
		case D3DTRANSFORMSTATE_WORLD3:
			memcpy(ctx->matrix.world[3], m, sizeof(m));
			MesaApplyTransform(ctx, MESA_TF_WORLD);
			break;
#if 0 /* disabled for now */
		case D3DTRANSFORMSTATE_TEXTURE0:
			memcpy(ctx->state.tmu[0].matrix, m, sizeof(m));
			ctx->state.tmu[0].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE1:
			memcpy(ctx->state.tmu[1].matrix, m, sizeof(m));
			ctx->state.tmu[1].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE2:
			memcpy(ctx->state.tmu[2].matrix, m, sizeof(m));
			ctx->state.tmu[2].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE3:
			memcpy(ctx->state.tmu[3].matrix, m, sizeof(m));
			ctx->state.tmu[3].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE4:
			memcpy(ctx->state.tmu[4].matrix, m, sizeof(m));
			ctx->state.tmu[4].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE5:
			memcpy(ctx->state.tmu[5].matrix, m, sizeof(m));
			ctx->state.tmu[5].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE6:
			memcpy(ctx->state.tmu[6].matrix, m, sizeof(m));
			ctx->state.tmu[6].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
		case D3DTRANSFORMSTATE_TEXTURE7:
			memcpy(ctx->state.tmu[7].matrix, m, sizeof(m));
			ctx->state.tmu[7].move = TRUE;
			MesaDrawRefreshState(ctx);
			break;
#else
		case D3DTRANSFORMSTATE_TEXTURE0...D3DTRANSFORMSTATE_TEXTURE7:
			break;
#endif
		default:
			WARN("MesaSetTransform: invalid state=%d, xtype=%X", state, xtype);
			break;
	}
}

mesa_surfaces_table_t *MesaSurfacesTableGet(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, DWORD max_id)
{
	mesa_surfaces_table_t *table = NULL;
	int i;
	for(i = 0; i < SURFACE_TABLES_PER_ENTRY; i++)
	{
		if(table == NULL && entry->surfaces_tables[i].lpDDLcl == NULL)
		{
			table = &entry->surfaces_tables[i];
		}
		
		if(entry->surfaces_tables[i].lpDDLcl == lpDDLcl)
		{
			table = &entry->surfaces_tables[i];
			break;
		}
	}

	if(table)
	{
		table->lpDDLcl = lpDDLcl;
		if(max_id >= table->table_size)
		{
			DWORD new_size = ((max_id + SURFACES_TABLE_POOL)/SURFACES_TABLE_POOL) * SURFACES_TABLE_POOL;
			
			if(table->table == NULL)
			{
				table->table = HeapAlloc(hSharedHeap, HEAP_ZERO_MEMORY, sizeof(surface_id)*new_size);
			}
			else
			{
				table->table = HeapReAlloc(hSharedHeap, HEAP_ZERO_MEMORY, table->table, sizeof(surface_id)*new_size);
			}
			
			table->table_size = new_size;
		}
	}
	
	return table;
}

void MesaSurfacesTableRemoveSurface(mesa3d_entry_t *entry, surface_id sid)
{
	int t, i;
	for(t = 0; t < SURFACE_TABLES_PER_ENTRY; t++)
	{
		for(i = 0; i < entry->surfaces_tables[t].table_size; i++)
		{
			if(entry->surfaces_tables[t].table[i] == sid)
			{
				entry->surfaces_tables[t].table[i] = 0;
			}
		}
	}
}

void MesaSurfacesTableRemoveDDLcl(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl)
{
	int t;
	for(t = 0; t < SURFACE_TABLES_PER_ENTRY; t++)
	{
		if(entry->surfaces_tables[t].lpDDLcl == lpDDLcl)
		{
			HeapFree(hSharedHeap, 0, entry->surfaces_tables[t].table);
			entry->surfaces_tables[t].table = NULL;
			entry->surfaces_tables[t].table_size = 0;
			
			entry->surfaces_tables[t].lpDDLcl = NULL;
		}
	}
}

void MesaSurfacesTableInsertHandle(mesa3d_entry_t *entry, LPDDRAWI_DIRECTDRAW_LCL lpDDLcl, DWORD handle, surface_id sid)
{
	mesa_surfaces_table_t *table = MesaSurfacesTableGet(entry, lpDDLcl, handle);
	if(table)
	{
		table->table[handle] = sid;
	}
}

mesa3d_texture_t *MesaTextureFromSurfaceHandle(mesa3d_ctx_t *ctx, DWORD handle)
{
	if(ctx->surfaces)
	{
		surface_id sid = ctx->surfaces->table[handle];
		if(sid)
		{
			mesa3d_texture_t *tex;
			tex = SurfaceGetTexture(sid, ctx, 0, 0);
			if(tex == NULL)
			{
				tex = MesaCreateTexture(ctx, sid);
			}
			return tex;
		}
	}
	return NULL;
}

void *MesaTempAlloc(mesa3d_ctx_t *ctx, DWORD w, DWORD size)
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
		ctx->temp.buf = HeapAlloc(hSharedLargeHeap, 0, ctx->temp.size);
	}

	if(ctx->temp.buf == NULL ||
		w > ctx->temp.width ||
		size > ctx->temp.size ||
		ctx->temp.lock != 0
		)
	{
		return HeapAlloc(hSharedLargeHeap, 0, size);
	}
	
	ctx->temp.lock = 1;
	return ctx->temp.buf;
}

void MesaTempFree(mesa3d_ctx_t *ctx, void *ptr)
{
	if(ptr != NULL)
	{
		if(ptr == ctx->temp.buf)
		{
			ctx->temp.lock = 0;
		}
		else
		{
			HeapFree(hSharedLargeHeap, 0, ptr);
		}
	}
}
