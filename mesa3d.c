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

extern HANDLE hSharedHeap;

#define MESA_HT_MOD 113

#define MESA_LIB_SW_NAME "mesa3d.dll"
#define MESA_LIB_SVGA_NAME "vmwsgl32.dll"

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

		//mesa->GetProcAddress = (OSMesaGetProcAddress_h)GetProcAddress(mesa->lib, "OSMesaGetProcAddress");
		mesa->GetProcAddress = NULL;
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

static HWND MesaCreateWindow(int w, int h)
{
	WNDCLASS wc      = {0};
	wc.lpfnWndProc   = DefWindowProc;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = FBO_WND_CLASS_NAME;
	wc.style         = CS_OWNDC;
	wc.hInstance     = GetModuleHandle(NULL);
	
	RegisterClass(&wc);
	
	return CreateWindowA(FBO_WND_CLASS_NAME, "fbo", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 0,0, w,h, 0,0, NULL, 0);
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

static void UpdateScreenCoords(mesa3d_ctx_t *ctx, GLfloat w, GLfloat h)
{	
	ctx->entry->proc.pglGetFloatv(GL_MODELVIEW_MATRIX, &ctx->matrix.model[0]);
	ctx->entry->proc.pglGetFloatv(GL_PROJECTION_MATRIX, &ctx->matrix.proj[0]);
	ctx->entry->proc.pglGetIntegerv(GL_VIEWPORT, &ctx->matrix.viewport[0]);
	
	TOPIC("MATRIX", "GL_MODELVIEW_MATRIX");
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.model[0], ctx->matrix.model[1], ctx->matrix.model[2], ctx->matrix.model[3]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.model[4], ctx->matrix.model[5], ctx->matrix.model[6], ctx->matrix.model[7]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.model[8], ctx->matrix.model[9], ctx->matrix.model[10], ctx->matrix.model[11]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.model[12], ctx->matrix.model[13], ctx->matrix.model[14], ctx->matrix.model[15]);
	TOPIC("MATRIX", "GL_PROJECTION_MATRIX");
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.proj[0], ctx->matrix.proj[1], ctx->matrix.proj[2], ctx->matrix.proj[3]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.proj[4], ctx->matrix.proj[5], ctx->matrix.proj[6], ctx->matrix.proj[7]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.proj[8], ctx->matrix.proj[9], ctx->matrix.proj[10], ctx->matrix.proj[11]);
	TOPIC("MATRIX", "[%f %f %f %f]", ctx->matrix.proj[12], ctx->matrix.proj[13], ctx->matrix.proj[14], ctx->matrix.proj[15]);
	TOPIC("MATRIX", "GL_VIEWPORT");
	TOPIC("MATRIX", "[%d %d %d %d]", ctx->matrix.viewport[0], ctx->matrix.viewport[1], ctx->matrix.viewport[2], ctx->matrix.viewport[3]);
	
	MesaUnprojectCalc(ctx);
}

static BOOL DDSurfaceToGL(LPDDRAWI_DDRAWSURFACE_LCL surf, GLuint *bpp, GLint *internalformat, GLenum *format, GLenum *type, BOOL *compressed)
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
	mesa3d_entry_t *entry = ctx->entry;
	ctx->depth_stencil = FALSE;
	ctx->state.stencil.enabled = FALSE;
	
	if(ctx->depth)
	{
		GL_CHECK(entry->proc.pglEnable(GL_DEPTH_TEST));
		GL_CHECK(entry->proc.pglDepthMask(GL_TRUE));
		ctx->state.depth.enabled = TRUE;
		ctx->state.depth.writable = TRUE;
		
		if(ctx->depth->lpGbl->ddpfSurface.dwFlags & DDPF_STENCILBUFFER)
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

static void MesaLoopFront(mesa3d_ctx_t *ctx, LPDDRAWI_DDRAWSURFACE_LCL dds)
{
	memset(ctx->flips, 0, sizeof(ctx->flips));
	int p = 0;
	
	ctx->flips[p++] = dds;
	
	LPATTACHLIST curr = dds->lpAttachList;
	while(curr)
	{
		if(curr->lpAttached == dds)
		{
			break;
		}
			
		if(curr->lpAttached)
		{
			if(p < MESA3D_MAX_FLIPS)
			{
				ctx->flips[p++] = curr->lpAttached;
			}
			
			curr = curr->lpAttached->lpAttachList;
		}
		else
		{
			curr = NULL;
		}
	}
	
	ctx->flips_cnt = p;
}

static void *MesaFindBackbuffer(mesa3d_ctx_t *ctx, BOOL *is_visible)
{
	int i;
	void *front = NULL;
	
	for(i = 0; i < ctx->flips_cnt; i++)
	{
		if(ctx->flips[i]->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
		{
			if(is_visible != NULL)
			{
				*is_visible = (ctx->flips[i]->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) ? TRUE : FALSE;
			}
			TOPIC("READBACK", "MesaFindBackbuffer: dwCaps=%X", ctx->flips[i]->ddsCaps.dwCaps);
			return (void*)ctx->flips[i]->lpGbl->fpVidMem;
		}
		
		if(ctx->flips[i]->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			TOPIC("READBACK", "MesaFindBackbuffer: FRONT - dwCaps=%X", ctx->flips[i]->ddsCaps.dwCaps);
			front = (void*)ctx->flips[i]->lpGbl->fpVidMem;
		}
	}
	
	if(is_visible != NULL)
		*is_visible = TRUE;

	return front;
}

mesa3d_ctx_t *MesaCreateCtx(mesa3d_entry_t *entry,
	LPDDRAWI_DDRAWSURFACE_LCL dds,
	LPDDRAWI_DDRAWSURFACE_LCL ddz)
{
	int i;
	int bpp = DDSurf_GetBPP(dds);
	int bpp_depth = DDSurf_GetBPP(ddz);
	
	int width  = dds->lpGbl->wWidth;
	int height = dds->lpGbl->wHeight;
	
	mesa3d_ctx_t *ctx = NULL;
	GLenum ostype = OSMESA_BGRA;
	GLenum gltype = GL_UNSIGNED_BYTE;
	
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
	
	switch(bpp)
	{
		case 8:
			ostype = OSMESA_COLOR_INDEX;
			pfd.cColorBits = 8;
			break;
		case 16:
			ostype = OSMESA_RGB_565;
			gltype = GL_UNSIGNED_SHORT_5_6_5;
			pfd.cColorBits = 16;
			break;
		case 24:
			ostype = OSMESA_BGR;
			pfd.cColorBits = 24;
			break;
	}
	
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

				ctx->thread_lock = 0;
				ctx->entry = entry;
				ctx->id = i;
				ctx->gltype = gltype;
				ctx->front_bpp = bpp;
				ctx->depth_bpp = bpp_depth;
				if(ddz)
				{
					ctx->depth = ddz;
				}
				ctx->state.sw = width;
				ctx->state.sh = height;
				
				if(entry->os)
				{
					TOPIC("RS", "OSMesaCreateContextExt(0x%X, %d, 0, 0, NULL)", ostype, bpp_depth);
					
					/* create context every time with 24bit depth buffer and 8bit stencil buffer,
					 * because we can't dynamicaly change depth and stencil plane.
					 */
					ctx->osctx = entry->proc.pOSMesaCreateContextExt(ostype, 32, 8, 0, NULL);
					if(ctx->osctx == NULL)
						break;
	
					ctx->ossize = SurfacePitch(width, bpp)*height;
					ctx->osbuf = HeapAlloc(hSharedHeap, 0, ctx->ossize);
					if(ctx->osbuf == NULL)
						break;
	
					if(!entry->proc.pOSMesaMakeCurrent(ctx->osctx, ctx->osbuf, ctx->gltype, width, height))
						break;
						
					entry->proc.pOSMesaPixelStore(OSMESA_Y_UP, 1);
				}
				else
				{
					int ipixel;
					
					ctx->fbo_win = MesaCreateWindow(width, height);
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
				
				entry->proc.pglGenTextures(1, &ctx->fbo.color_tex);
				entry->proc.pglGenTextures(1, &ctx->fbo.depth_tex);
				entry->proc.pglGenTextures(1, &ctx->fbo.stencil_tex);
				entry->proc.pglGenFramebuffers(1, &ctx->fbo.color_fb);
				entry->proc.pglGenFramebuffers(1, &ctx->fbo.depth_fb);
				entry->proc.pglGenFramebuffers(1, &ctx->fbo.stencil_fb);
				
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
				
				MesaLoopFront(ctx, dds);
				MesaInitCtx(ctx);
				UpdateScreenCoords(ctx, (GLfloat)width, (GLfloat)height);

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
							HeapFree(hSharedHeap, 0, ctx->osbuf);
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

BOOL MesaSetTarget(mesa3d_ctx_t *ctx, LPDDRAWI_DDRAWSURFACE_LCL dds, LPDDRAWI_DDRAWSURFACE_LCL ddz)
{
	mesa3d_entry_t *entry = ctx->entry;
	int width  = dds->lpGbl->wWidth;
	int height = dds->lpGbl->wHeight;
	int bpp = DDSurf_GetBPP(dds);
	int bpp_depth = DDSurf_GetBPP(ddz);

	if(ctx->state.sw != width || ctx->state.sh != height || ctx->front_bpp != bpp)
	{
		if(entry->os)
		{
			entry->proc.pOSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
			
			DWORD new_ossize =  SurfacePitch(width, bpp)*height;
			if(new_ossize > ctx->ossize)
			{
				void *new_buf;
				new_buf = HeapReAlloc(hSharedHeap, 0, ctx->osbuf, new_ossize);
				if(new_buf != NULL)
				{
					ctx->osbuf = new_buf;
					ctx->ossize = new_ossize;
				}
				else
				{
					return FALSE;
				}
			}
			
			if(!entry->proc.pOSMesaMakeCurrent(ctx->osctx, ctx->osbuf, ctx->gltype, width, height))
			{
				return FALSE;
			}
		}
		else
		{
			MoveWindow(ctx->fbo_win, 0, 0, width, height, TRUE);
		}
		
		GL_CHECK(entry->proc.pglViewport(0, 0, width, height));
	}
	
	TOPIC("GL", "New target bpp %d, bpz %d", bpp, bpp_depth);
		
	ctx->front_bpp = bpp;
//	ctx->front     = dds;

//	MesaBufferDownloadColor(ctx, (void*)dds->lpGbl->fpVidMem);
//	ctx->render.dirty = FALSE;

	ctx->depth_bpp = bpp_depth;
	if(ddz)
	{
		ctx->depth = ddz;
//		MesaBufferDownloadDepth(ctx, (void*)ddz->lpGbl->fpVidMem);
//		ctx->render.zdirty = FALSE;
	}
	
	ctx->state.sw = width;
	ctx->state.sh = height;
	MesaLoopFront(ctx, dds);
	UpdateScreenCoords(ctx, (GLfloat)width, (GLfloat)height);
	MesaDepthReeval(ctx);
	
	return TRUE;
}

void MesaDestroyCtx(mesa3d_ctx_t *ctx)
{
	TRACE_ENTRY
	int id = ctx->id;
	mesa3d_entry_t *entry = ctx->entry;
	
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

	if(ctx->osbuf != NULL)
	{
		HeapFree(hSharedHeap, 0, ctx->osbuf);
	}

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
	
	ctx->tmu_count = VMHALenv.texture_num_units;
	ctx->fbo.tmu = 0;
	ctx->state.tmu[0].active = 1;

	entry->proc.pglPixelStorei(GL_UNPACK_ALIGNMENT, FBHDA_ROW_ALIGN);
	entry->proc.pglPixelStorei(GL_PACK_ALIGNMENT, FBHDA_ROW_ALIGN);

  entry->proc.pglFrontFace(GL_CCW);
  
  entry->proc.pglMatrixMode(GL_MODELVIEW);
  entry->proc.pglLoadIdentity();
  entry->proc.pglMatrixMode(GL_PROJECTION);
  entry->proc.pglLoadIdentity();
  
	entry->proc.pglDisable(GL_LIGHTING);

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
	}

	entry->proc.pglEnable(GL_BLEND);
	entry->proc.pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	entry->proc.pglEnable(GL_CULL_FACE);
//	entry->proc.pglCullFace(GL_FRONT_AND_BACK);
	
	ctx->state.stencil.sfail     = GL_KEEP;
	ctx->state.stencil.dpfail    = GL_KEEP;
	ctx->state.stencil.dppass    = GL_KEEP;
	ctx->state.stencil.func      = GL_ALWAYS;
	ctx->state.stencil.ref       = 0;
	ctx->state.stencil.mask      = 0xFF;
	ctx->state.stencil.writemask = 0xFF;

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
				ctx->osctx, ctx->osbuf, ctx->gltype, ctx->state.sw, ctx->state.sh))
			{
				UpdateScreenCoords(ctx, ctx->state.sw, ctx->state.sh);
				ctx->thread_id = GetCurrentThreadId();
				return TRUE;
			}
		}
		else
		{
			if(ctx->entry->proc.pDrvSetContext(ctx->dc, ctx->glrc, NULL))
			{
				UpdateScreenCoords(ctx, ctx->state.sw, ctx->state.sh);
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
DWORD DDSurf_GetBPP(LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	if(surf == NULL)
	{
		return 0;
	}

	FBHDA_t *hda = FBHDA_setup();
	
	DWORD bpp = hda->bpp;
	if(surf->dwFlags & DDRAWISURF_HASPIXELFORMAT)
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

mesa3d_texture_t *MesaCreateTexture(mesa3d_ctx_t *ctx, LPDDRAWI_DDRAWSURFACE_LCL surf)
{
	mesa3d_texture_t *tex = NULL;
	int i;
	
	TOPIC("GL", "new texture");
	
	for(i = 0; i < MESA3D_MAX_TEXS; i++)
	{
		if(!ctx->tex[i].alloc)
		{
			mesa3d_entry_t *entry = ctx->entry;
			
			TOPIC("RELOAD", "New texture - position: %d", i);
			
			memset(&ctx->tex[i], 0, sizeof(mesa3d_texture_t)); // FIXME: necessary? Update: yes, it is!
			
			tex           = &ctx->tex[i];
			tex->ctx      = ctx;
			tex->data_surf[0] = surf;
			tex->data_ptr[0]  = surf->lpGbl->fpVidMem;

			if(DDSurfaceToGL(surf, &tex->bpp, &tex->internalformat, &tex->format, &tex->type, &tex->compressed))
			{
				tex->width  = surf->lpGbl->wWidth;
				tex->height = surf->lpGbl->wHeight;
				
				GL_CHECK(entry->proc.pglGenTextures(1, &tex->gltex));
				
				if((surf->dwFlags & DDRAWISURF_HASCKEYSRCBLT) && ctx->state.tmu[0].colorkey)
				{
					if(!tex->compressed)
					{
						tex->colorkey = TRUE;
					}
				}

				SurfaceAttachTexture(surf, tex, 0);
				
				/* mipmaps */	
				if(surf->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
				{
					int level;
					int mipmaps = surf->lpSurfMore->dwMipMapCount;
					LPATTACHLIST item = surf->lpAttachList;
					TOPIC("MIPMAP", "number of mipmaps: %d, original size %d %d", mipmaps, tex->width, tex->height);
					for(level = 1; level <= mipmaps; level++)
					{
						TOPIC("MIPMAP", "Loading mipmap level %d, ptr %X", level, item);
						if(!item) break;
						if(!item->lpAttached) break;
						if(item->lpAttached == surf) break;

						tex->data_ptr[level] = item->lpAttached->lpGbl->fpVidMem;
						tex->data_dirty[level] = TRUE;
						tex->data_surf[level] = item->lpAttached;

						SurfaceAttachTexture(surf, tex, level);

						item = item->lpAttached->lpAttachList;
					}
					
					tex->mipmap_level = level-1;
					tex->mipmap = (level > 1) ? TRUE : FALSE;
					TOPIC("RELOAD", "Created %d with mipmap, at level: %d", tex->gltex, tex->mipmap_level);
				} // mipmaps
				else
				{
					tex->mipmap_level = 0;
					tex->mipmap = FALSE;
					TOPIC("RELOAD", "Created %d without mipmap", tex->gltex);
				}
				
				//ctx->state.reload_tex_par = TRUE;
				tex->alloc = TRUE;
				tex->dirty = TRUE;
			}
			else
			{
				TOPIC("RELOAD", "Failed to identify image type!");
				tex = NULL;
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
	
	for(level = 0; level <= tex->mipmap_level; level++)
	{
		if(reload || tex->data_dirty[level])
		{
			if(tex->ctx->state.tmu[tmu].colorkey && 
				(tex->data_surf[level]->dwFlags & DDRAWISURF_HASCKEYSRCBLT))
			{
				MesaBufferUploadTextureChroma(tex->ctx, tex, level, tmu,
							tex->data_surf[level]->ddckCKSrcBlt.dwColorSpaceLowValue,
							tex->data_surf[level]->ddckCKSrcBlt.dwColorSpaceHighValue);
			}
			else
			{
				MesaBufferUploadTexture(tex->ctx, tex, level, tmu);
			}
			
			tex->data_dirty[level] = FALSE;
			reload_done = TRUE;
			
			TOPIC("RELOAD", "Reloading: %d, level %d", tex->gltex, level);
		}
		tex->dirty = FALSE;
	}
	
	if(reload_done)
	{
		tex->colorkey = tex->ctx->state.tmu[tmu].colorkey;
		tex->tmu[tmu] = TRUE;
	}
}

void MesaDestroyTexture(mesa3d_texture_t *tex)
{
	if(tex->alloc)
	{
		int i;
		mesa3d_entry_t *entry = tex->ctx->entry;
		
		for(i = 0; i < tex->ctx->tmu_count; i++)
		{
			if(tex->ctx->state.tmu[i].image == tex)
			{
				tex->ctx->state.tmu[i].image  = NULL;
				tex->ctx->state.tmu[i].reload = TRUE;
			}
		}
		
		GL_CHECK(entry->proc.pglDeleteTextures(1, &tex->gltex));
		for(i = 0; i <= tex->mipmap_level; i++)
		{
			TRACE("level=%d, ptr=%p, vram=%X", i, tex->data_surf[i], tex->data_ptr[i]);
			//SurfaceDeattachTexture(tex->data_surf[i], tex);
			//FIXME: ^double free here!
		}
		
		tex->alloc = FALSE;
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

#define TSS_DWORD (*((DWORD*)value))
#define TSS_FLOAT (*((D3DVALUE*)value))

#define RENDERSTATE(_c) case _c: TOPIC("RS", "%s=0x%X", #_c, TSS_DWORD);

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
			if(ctx->entry->dx7)
			{
				ts->image = SurfaceNestTexture(TSS_DWORD, ctx);
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
			ts->coordindex = TSS_DWORD;
			TOPIC("TEXCOORDINDEX", "TEXCOORDINDEX %d for unit %d", TSS_DWORD, tmu);
			break;
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
			break;
		/* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
		RENDERSTATE(D3DTSS_MAXMIPLEVEL)
			ts->mipmaxlevel = TSS_DWORD;
			break;
		/* DWORD maximum anisotropy */
		RENDERSTATE(D3DTSS_MAXANISOTROPY)
			break;
		/* D3DVALUE scale for bump map luminance */
		RENDERSTATE(D3DTSS_BUMPENVLSCALE)
			break;
		/* D3DVALUE offset for bump map luminance */
		RENDERSTATE(D3DTSS_BUMPENVLOFFSET)
			break;
		/* D3DTEXTURETRANSFORMFLAGS controls texture transform (DX7) */
		RENDERSTATE(D3DTSS_TEXTURETRANSFORMFLAGS)
			break;
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
	
	switch(type)
	{
		RENDERSTATE(D3DRENDERSTATE_TEXTUREHANDLE) /* Texture handle */
			if(state->dwArg[0] != 0)
			{
				if(ctx->entry->dx7)
				{
					ctx->state.tmu[0].image = SurfaceNestTexture(state->dwArg[0], ctx);
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
			// !
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREADDRESS) /* D3DTEXTUREADDRESS	*/
			ctx->state.tmu[0].texaddr_u = state->dwArg[0];
			ctx->state.tmu[0].texaddr_v = state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREPERSPECTIVE) /* TRUE for perspective correction */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPU) /* TRUE for wrapping in u */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_WRAPV) /* TRUE for wrapping in v */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ZENABLE) /* TRUE to enable z test */
			if(ctx->depth)
			{
				if(state->dwArg[0] == 0)
				{
					ctx->entry->proc.pglDisable(GL_DEPTH_TEST);
					ctx->state.depth.enabled = FALSE;
				}
				else
				{
					ctx->entry->proc.pglEnable(GL_DEPTH_TEST);
					ctx->state.depth.enabled = TRUE;
				}
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_FILLMODE) /* D3DFILL_MODE	*/
			switch((D3DFILLMODE)state->dwArg[0])
			{
				case D3DFILL_POINT:
					ctx->entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
					break;
				case D3DFILL_WIREFRAME:
					ctx->entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					break;
				case D3DFILL_SOLID:
				default:
					ctx->entry->proc.pglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					break;
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_SHADEMODE) /* D3DSHADEMODE */
			switch((D3DSHADEMODE)state->dwArg[0])
			{
        case D3DSHADE_FLAT:
					ctx->entry->proc.pglShadeModel(GL_FLAT);
					break;
				case D3DSHADE_GOURAUD:
				/* Note from WINE: D3DSHADE_PHONG in practice is the same as D3DSHADE_GOURAUD in D3D */
				case D3DSHADE_PHONG:
				default:
					ctx->entry->proc.pglShadeModel(GL_SMOOTH);
					break;
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_LINEPATTERN) /* D3DLINEPATTERN */
			// !
			break;
		RENDERSTATE(D3DRENDERSTATE_MONOENABLE) /* TRUE to enable mono rasterization */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ROP2) /* ROP2 */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_PLANEMASK) /* DWORD physical plane mask */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_ZWRITEENABLE) /* TRUE to enable z writes */
			if(ctx->depth)
			{
				if(state->dwArg[0] != 0)
				{
					ctx->entry->proc.pglDepthMask(GL_TRUE);
					ctx->state.depth.writable = TRUE;
				}
				else
				{
					ctx->entry->proc.pglDepthMask(GL_FALSE);
					ctx->state.depth.writable = FALSE;
				}
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHATESTENABLE) /* TRUE to enable alpha tests */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHATESTENABLE = 0x%X", state->dwArg[0]);
			if(state->dwArg[0] != 0)
			{
				ctx->entry->proc.pglEnable(GL_ALPHA_TEST);
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_ALPHA_TEST);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_LASTPIXEL) /* TRUE for last-pixel on lines */
			/* nop */
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAG) /* D3DTEXTUREFILTER */
			ctx->state.tmu[0].texmag = GetGLTexFilter((D3DTEXTUREFILTER)state->dwArg[0]);
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMIN) /* D3DTEXTUREFILTER */
			ctx->state.tmu[0].texmin = GetGLTexFilter((D3DTEXTUREFILTER)state->dwArg[0]);
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
			
			ctx->entry->proc.pglBlendFunc(ctx->state.blend_sfactor, ctx->state.blend_dfactor);
			break;
		}
		RENDERSTATE(D3DRENDERSTATE_DESTBLEND) /* D3DBLEND */
			TOPIC("BLEND", "D3DRENDERSTATE_DESTBLEND = 0x%X", state->dwArg[0]);
			ctx->state.blend_dfactor = GetBlendFactor((D3DBLEND)state->dwArg[0]);
			
			ctx->entry->proc.pglBlendFunc(ctx->state.blend_sfactor, ctx->state.blend_dfactor);
			break;
		RENDERSTATE(D3DRENDERSTATE_TEXTUREMAPBLEND) /* D3DTEXTUREBLEND */
			ctx->state.tmu[0].texblend = (D3DTEXTUREBLEND)state->dwArg[0];
			ctx->state.tmu[0].update = TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_CULLMODE) /* D3DCULL */
			switch(state->dwArg[0])
			{
				case D3DCULL_NONE:
					ctx->entry->proc.pglDisable(GL_CULL_FACE);
					break;
				case D3DCULL_CW:
					ctx->entry->proc.pglEnable(GL_CULL_FACE);
					ctx->entry->proc.pglCullFace(GL_FRONT);
					break;
				case D3DCULL_CCW:
					ctx->entry->proc.pglEnable(GL_CULL_FACE);
					ctx->entry->proc.pglCullFace(GL_BACK);
					break;
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ZFUNC) /* D3DCMPFUNC */
			ctx->entry->proc.pglDepthFunc(GetGLCmpFunc(state->dwArg[0]));
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAREF) /* D3DFIXED */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAREF = 0x%X", state->dwArg[0]);
			ctx->state.alpharef = (GLclampf)(state->dwArg[0]/255.0f);
			if(ctx->state.alphafunc != 0)
			{
				ctx->entry->proc.pglAlphaFunc(ctx->state.alphafunc, ctx->state.alpharef);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHAFUNC) /* D3DCMPFUNC */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHAFUNC = 0x%X", state->dwArg[0]);
			ctx->state.alphafunc = GetGLCmpFunc(state->dwArg[0]);
			ctx->entry->proc.pglAlphaFunc(ctx->state.alphafunc, ctx->state.alpharef);
			break;
		RENDERSTATE(D3DRENDERSTATE_DITHERENABLE) /* TRUE to enable dithering */
			if(state->dwArg[0])
			{
				ctx->entry->proc.pglEnable(GL_DITHER);
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_DITHER);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_ALPHABLENDENABLE) /* TRUE to enable alpha blending */
			TOPIC("BLEND", "D3DRENDERSTATE_ALPHABLENDENABLE = 0x%X", state->dwArg[0]);
			if(state->dwArg[0])
			{
				ctx->entry->proc.pglEnable(GL_BLEND);
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_BLEND);
			}
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
				ctx->entry->proc.pglEnable(GL_COLOR_SUM);
				ctx->state.specular = TRUE;
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_COLOR_SUM);
				ctx->state.specular = FALSE;
			}
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
				ctx->entry->proc.pglEnable(GL_POLYGON_STIPPLE);
			}
			else
			{
				ctx->entry->proc.pglDisable(GL_POLYGON_STIPPLE);
			}
			break;
		RENDERSTATE(D3DRENDERSTATE_EDGEANTIALIAS) /* TRUE to enable edge antialiasing */
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
			break;
		RENDERSTATE(D3DRENDERSTATE_ZBIAS) /* LONG Z bias */
			break;
		RENDERSTATE(D3DRENDERSTATE_RANGEFOGENABLE) /* Enables range-based fog */
			TOPIC("FOG", "D3DRENDERSTATE_RANGEFOGENABLE=%X", state->dwArg[0]);
			ctx->state.fog.range = state->dwArg[0] == 0 ? FALSE : TRUE;
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_ANISOTROPY) /* Max. anisotropy. 1 = no anisotropy */
			break;
		RENDERSTATE(D3DRENDERSTATE_FLUSHBATCH) /* Explicit flush for DP batching (DX5 Only) */
			break;
		/* d3d6 */
		RENDERSTATE(D3DRENDERSTATE_STENCILENABLE) /* BOOL enable/disable stenciling */
			ctx->state.stencil.enabled = state->dwArg[0] == 0 ? FALSE : TRUE;
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFAIL) /* D3DSTENCILOP to do if stencil test fails */
			ctx->state.stencil.sfail = DXSencilToGL(state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILZFAIL) /* D3DSTENCILOP to do if stencil test passes and Z test fails */
			ctx->state.stencil.dpfail = DXSencilToGL(state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILPASS) /* D3DSTENCILOP to do if both stencil and Z tests pass */
			ctx->state.stencil.dppass = DXSencilToGL(state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILFUNC) /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
			ctx->state.stencil.func = GetGLCmpFunc(state->dwArg[0]);
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILREF) /* Reference value used in stencil test */
			ctx->state.stencil.ref = state->dwArg[0];
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILMASK) /* Mask value used in stencil test */
			ctx->state.stencil.mask = state->dwArg[0];
			break;
		RENDERSTATE(D3DRENDERSTATE_STENCILWRITEMASK) /* Write mask applied to values written to stencil buffer */
			ctx->state.stencil.writemask = state->dwArg[0];
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
			ctx->entry->proc.pglPolygonStipple((GLubyte*)&ctx->state.stipple[0]);
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
			break;
		RENDERSTATE(D3DRENDERSTATE_LIGHTING)
			break;
		RENDERSTATE(D3DRENDERSTATE_EXTENTS)
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENT)
			break;
		RENDERSTATE(D3DRENDERSTATE_FOGVERTEXMODE)
			TOPIC("FOG", "D3DRENDERSTATE_FOGVERTEXMODE=0x%X", state->dwArg[0]);
			ctx->state.fog.vmode = GetGLFogMode(state->dwArg[0]);
			MesaSetFog(ctx);
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORVERTEX)
			break;
		RENDERSTATE(D3DRENDERSTATE_LOCALVIEWER)
			break;
		RENDERSTATE(D3DRENDERSTATE_NORMALIZENORMALS)
			break;
		RENDERSTATE(D3DRENDERSTATE_COLORKEYBLENDENABLE)
			break;
		RENDERSTATE(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE)
			break;
		RENDERSTATE(D3DRENDERSTATE_SPECULARMATERIALSOURCE)
			break;
		RENDERSTATE(D3DRENDERSTATE_AMBIENTMATERIALSOURCE)
			break;
		RENDERSTATE(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE)
			break;
		RENDERSTATE(D3DRENDERSTATE_VERTEXBLEND)
			break;
		RENDERSTATE(D3DRENDERSTATE_CLIPPLANEENABLE)
			break;
		default:
			switch((DWORD)type)
			{
				RENDERSTATE(D3DRENDERSTATE_SCENECAPTURE)
					TOPIC("READBACK", "D3DRENDERSTATE_SCENECAPTURE %d", state->dwArg[0]);
					switch(state->dwArg[0])
					{
						case 0: /* just before flip (end scene) */
							BOOL is_visible;
							void *ptr = MesaFindBackbuffer(ctx, &is_visible);
							if(ptr)
							{
								if(is_visible) /* fixme: check for DDSCAPS_PRIMARYSURFACE */
									FBHDA_access_begin(0);

								MesaBufferDownloadColor(ctx, ptr);

								if(is_visible)
									FBHDA_access_end(0);
							}
							break;
						case 1: /* after clear (start scene) */
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
	
	if(ts->image)
	{
		GL_CHECK(entry->proc.pglEnable(GL_TEXTURE_2D));
		ts->active = TRUE;
		
		GL_CHECK(entry->proc.pglBindTexture(GL_TEXTURE_2D, ts->image->gltex));

		/*
		 * texture filtering
		 */
		if(ts->dx6_filter)
		{
			if(ts->image->mipmap)
			{
				GLint maxlevel = ts->image->mipmap_level;

				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxlevel));
				GL_CHECK(entry->proc.pglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, ts->miplodbias));

				switch(ts->dx_min)
				{
					case D3DTFN_LINEAR:
						switch(ts->dx_mip)
						{
							case D3DTFP_LINEAR:
								GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
								break;
							case D3DTFP_POINT:
							default:
								GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
								break;
						}
						
						break;
					case D3DTFN_POINT:
					default:
						switch(ts->dx_mip)
						{
							case D3DTFP_LINEAR:
								GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR));
								break;
							case D3DTFP_POINT:
							default:
								GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
								break;
						}
						break;
				}
			}
			else /* non mipmap */
			{
				switch(ts->dx_min)
				{
					case D3DTFN_LINEAR:
						GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
						break;
					case D3DTFN_POINT:
					default:
						GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
						break;
				}
			}
			
			switch(ts->dx_mag)
			{
				case D3DTFG_LINEAR:
					GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
					break;
				case D3DTFG_POINT:
				default:
					GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
					break;
			}
		}
		else
		{
			if(ts->image->mipmap)
			{
				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ts->image->mipmap_level));
				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ts->texmag));
				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ts->texmin));
			}
			else
			{
				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nonMipFilter(ts->texmag)));
				GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nonMipFilter(ts->texmin)));
			}
		}
	}
	else
	{
		GL_CHECK(entry->proc.pglDisable(GL_TEXTURE_2D));
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
	  	  
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, color_fn));

	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, alpha_arg1_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, alpha_arg1_op));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, alpha_arg2_source));
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, alpha_arg2_op));
	  
	  if(alpha_fn == GL_INTERPOLATE)
	  {
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, alpha_arg3_source));
	  	GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, alpha_arg3_op));
	  }
	  
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, alpha_fn));

	  GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,   color_mult));
	  GL_CHECK(entry->proc.pglTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, alpha_mult));
	  
	  GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE));
	}
	else
	{
		switch(ts->texblend)
		{
			case D3DTBLEND_DECAL:
			case D3DTBLEND_COPY:
				/* 
				 * cPix = cTex
				 * aPix = aTex
				 */
				GL_CHECK(entry->proc.pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
				TOPIC("BLEND", "D3DTBLEND_DECAL");
				break;
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
			default:
				TOPIC("BLEND", "Wrong state: %d", ts->texblend);
				/* NOP */
				break;
		}
	} // !dx6_blend
		
	/*
	 * Texture addressing
	 */
	if(ts->texaddr_u == D3DTADDRESS_BORDER ||
		ts->texaddr_v == D3DTADDRESS_BORDER)
	{
		GL_CHECK(entry->proc.pglTexParameterfv(GL_TEXTURE_2D,
			GL_TEXTURE_BORDER_COLOR, &(ts->border[0])));
	}
	
	switch(ts->texaddr_u)
	{
		case D3DTADDRESS_MIRROR:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
			break;
		case D3DTADDRESS_CLAMP:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			break;
		case D3DTADDRESS_BORDER:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
			break;
		case D3DTADDRESS_WRAP:
		default:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
			break;
	}
	
	switch(ts->texaddr_v)
	{
		case D3DTADDRESS_MIRROR:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
			break;
		case D3DTADDRESS_CLAMP:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			break;
		case D3DTADDRESS_BORDER:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
			break;
		case D3DTADDRESS_WRAP:
		default:
			GL_CHECK(entry->proc.pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
			break;
	}
}

void MesaDrawRefreshState(mesa3d_ctx_t *ctx)
{
	int i;

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

#if 0
	//if(IsInFront(GetHAL(ctx->dd), (void*)ctx->front->lpGbl->fpVidMem)/* && (flags & D3DCLEAR_TARGET) != 0*/)
	if(IsInFront(GetHAL(ctx->dd), (void*)ctx->front->lpGbl->fpVidMem) && (flags & D3DCLEAR_TARGET) != 0)
	{
		TOPIC("READBACK", "single buffering");
		if(ctx->render.dirty)
		{
			//MesaBufferDownloadColor(ctx, (void*)ctx->front->lpGbl->fpVidMem);
			//ctx->render.dirty = FALSE;
		}
	}
	else
#endif
	{
		if(flags & D3DCLEAR_TARGET)
		{
			void *ptr = MesaFindBackbuffer(ctx, NULL);
			if(ptr)
			{
				MesaBufferUploadColor(ctx, ptr);
			}
		}
		
		if(ctx->depth && (flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)))
			MesaBufferUploadDepth(ctx, (void*)ctx->depth->lpGbl->fpVidMem);
		
		// FIXME: ^when is done full clear, is not needed to readback the surface! 
	}

#if 1
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
#else
	entry->proc.pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif
	
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

