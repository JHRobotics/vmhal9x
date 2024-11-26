#ifndef __MESA3D_H__INCLUDED__
#define __MESA3D_H__INCLUDED__

#include <GL/gl.h>
#include <GL/glext.h>
#include <d3dtypes.h>


typedef void (*OSMESAproc)();
typedef void *OSMesaContext;

struct mesa3d_texture;
struct mesa3d_ctx;
struct mesa3d_entry;

typedef OSMESAproc (APIENTRYP OSMesaGetProcAddress_h)(const char *funcName);

#define MESA_API(_n, _t, _p) \
	typedef _t (APIENTRYP _n ## _h)_p;

#include "mesa3d_api.h"
#undef MESA_API

#define MESA3D_MAX_TEXS 65536
#define MESA3D_MAX_CTXS 128
#define MESA3D_MAX_MIPS 16

typedef struct mesa3d_texture
{
	BOOL    alloc;
	GLuint  gltex;
	GLuint  width;
	GLuint  height;
	GLuint  bpp;
	GLint   internalformat;
	GLenum  format;
	GLenum  type;
	FLATPTR data_ptr[MESA3D_MAX_MIPS];
	struct mesa3d_ctx *ctx;
	LPDDRAWI_DDRAWSURFACE_INT ddsurf;
	BOOL mipmap;
	int mipmap_level;
	BOOL colorkey;
	BOOL compressed;
} mesa3d_texture_t;

typedef struct mesa3d_ctx
{
	mesa3d_texture_t tex[MESA3D_MAX_TEXS];
	struct mesa3d_entry *entry;
	int id; /* mesa3d_entry.ctx[_id_] */
	OSMesaContext *mesactx;
	GLenum gltype; /* GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5 */
	/* OS buffers (extra memory, don't map it directly to surfaces) */
	void *osbuf;
	DWORD ossize;
	DWORD thread_id;
	GLint front_bpp;
	GLint depth_bpp;
	LPDDRAWI_DDRAWSURFACE_INT front;
	LPDDRAWI_DDRAWSURFACE_INT depth;
	LPDDRAWI_DIRECTDRAW_GBL dd;

	/* render state */
	struct {
		mesa3d_texture_t *tex;
		//GLint last_tex;
		// screen coordinates
		GLint sw;
		GLint sh;
		GLenum blend_sfactor;
		GLenum blend_dfactor;
		D3DTEXTUREADDRESS texaddr_u; // wrap, mirror, clamp, border
		D3DTEXTUREADDRESS texaddr_v;
		D3DTEXTUREBLEND   texblend;
		BOOL zvisible; // Z-visibility testing
		GLenum texmin;
		GLenum texmag;
		GLenum alphafunc;
		GLclampf alpharef;
		BOOL specular;
		BOOL colorkey;
		DWORD overrides[4]; // 128 bit set
		DWORD stipple[32];
		struct {
			BOOL enabled;
			GLenum  tmode;
			GLenum  vmode;
			GLfloat start;
			GLfloat end;
			GLfloat density;
			GLfloat color[4];
			BOOL range;
		} fog;
		BOOL reload_tex;
		BOOL reload_tex_par;
	} state;

	/* fbo */
	struct {
		GLuint color_tex;
		GLuint color_fb;
		GLuint depth_tex;
		GLuint depth_fb;
	} fbo;

	/* dimensions */
	struct {
		GLfloat model[16];
		GLfloat proj[16];
		GLint viewport[4];
		/* precalculated values */
		GLfloat fnorm[16];
		GLfloat vnorm[4];
		BOOL fnorm_is_idx;
	} matrix;
} mesa3d_ctx_t;

#define CONV_U_TO_S(_u) (_u)
#define CONV_V_TO_T(_v) (_v)

#define MESA_API(_n, _t, _p) _n ## _h p ## _n;
typedef struct mesa3d_entry
{
	DWORD pid;
	struct mesa3d_entry *next;
	HANDLE lib;
	mesa3d_ctx_t *ctx[MESA3D_MAX_CTXS];
	OSMesaGetProcAddress_h GetProcAddress;
	struct {
#include "mesa3d_api.h"
	} proc;
} mesa3d_entry_t;
#undef MESA_API

#define MESA_LIB_NAME "mesa3d.dll"

mesa3d_entry_t *Mesa3DGet(DWORD pid);
void Mesa3DFree(DWORD pid);

#define GL_BLOCK_BEGIN(_ctx_h) \
	do{ \
		mesa3d_ctx_t *ctx = MESA_HANDLE_TO_CTX(_ctx_h); \
		mesa3d_entry_t *entry = ctx->entry; \
		OSMesaContext *oldmesa = NULL; \
		if(ctx->thread_id != GetCurrentThreadId()){ \
			TOPIC("GL", "Thread switch %s:%d", __FILE__, __LINE__); \
			if(!MesaSetCtx(ctx)){break;} \
		}else{ \
		oldmesa = entry->proc.pOSMesaGetCurrentContext(); \
		if(oldmesa != ctx->mesactx){ \
			TOPIC("GL", "Wrong context in %s:%d", __FILE__, __LINE__); \
			if(!MesaSetCtx(ctx)){break;}}}

#define GL_BLOCK_END \
		if(oldmesa != ctx->mesactx && oldmesa != NULL){ \
			entry->proc.pOSMesaMakeCurrent(NULL, NULL, 0, 0, 0);} \
	}while(0);

#define MESA_TEX_TO_HANDLE(_p) ((DWORD)(_p))
#define MESA_HANDLE_TO_TEX(_h) ((mesa3d_texture_t*)(_h))

#define MESA_CTX_TO_HANDLE(_p) ((DWORD)(_p))
#define MESA_HANDLE_TO_CTX(_h) ((mesa3d_ctx_t*)(_h))

#define MESA_MTX_TO_HANDLE(_p) ((DWORD)(_p))
#define MESA_HANDLE_TO_MTX(_h) ((GLfloat*)(_h))

#ifdef TRACE_ON
# ifndef DEBUG_GL_TOPIC
#  define GL_ERR_TOPIC "GL"
# else
#  define GL_ERR_TOPIC VMHAL_DSTR(DEBUG_GL_TOPIC)
# endif

#define GL_CHECK(_code) _code; \
	do{ GLenum err; \
		while((err = entry->proc.pglGetError()) != GL_NO_ERROR){ \
			TOPIC(GL_ERR_TOPIC, "GL error: %s = %X", #_code, err); \
			ERR("%s = %X", #_code, err); \
	}}while(0)
#else
#define GL_CHECK(_code) _code
#endif

mesa3d_ctx_t *MesaCreateCtx(mesa3d_entry_t *entry,
	LPDDRAWI_DDRAWSURFACE_INT dds,
	LPDDRAWI_DDRAWSURFACE_INT ddz);
void MesaDestroyCtx(mesa3d_ctx_t *ctx);
void MesaDestroyAllCtx(mesa3d_entry_t *entry);
void MesaInitCtx(mesa3d_ctx_t *ctx);
BOOL MesaSetCtx(mesa3d_ctx_t *ctx);

/* NOT needs GL_BLOCK */
void MesaUpdateSurface(SurfaceInfo_t *info);

/* needs GL_BLOCK */
mesa3d_texture_t *MesaCreateTexture(mesa3d_ctx_t *ctx, LPDDRAWI_DDRAWSURFACE_INT surf);
void MesaReloadTexture(mesa3d_texture_t *tex);
void MesaDestroyTexture(mesa3d_texture_t *tex);

void MesaSetRenderState(mesa3d_ctx_t *ctx, LPD3DSTATE state);
void MesaDraw(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype, LPVOID vertices, DWORD verticesCnt);
void MesaDrawIndex(mesa3d_ctx_t *ctx, D3DPRIMITIVETYPE dx_ptype, D3DVERTEXTYPE vtype,
	LPVOID vertices, DWORD verticesCnt,
	LPWORD indices, DWORD indicesCnt);

void MesaRender(mesa3d_ctx_t *ctx);
void MesaReadback(mesa3d_ctx_t *ctx);
BOOL MesaSetTarget(mesa3d_ctx_t *ctx, LPDDRAWI_DDRAWSURFACE_INT dss, LPDDRAWI_DDRAWSURFACE_INT dsz);

void MesaBufferClear(LPDDRAWI_DDRAWSURFACE_GBL buf, DWORD d3d_color);
DWORD DDSurf_GetBPP(LPDDRAWI_DDRAWSURFACE_INT surf);

/* buffer (needs GL_BLOCK) */
void MesaBufferUploadColor(mesa3d_ctx_t *ctx, const void *src);
void MesaBufferDownloadColor(mesa3d_ctx_t *ctx, void *dst);
void MesaBufferUploadDepth(mesa3d_ctx_t *ctx, const void *src);
void MesaBufferDownloadDepth(mesa3d_ctx_t *ctx, void *dst);
void MesaBufferUploadTexture(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level);
void MesaBufferUploadTextureChroma(mesa3d_ctx_t *ctx, mesa3d_texture_t *tex, int level, DWORD chroma_lw, DWORD chroma_hi);

/* calculation */
void MesaUnproject(mesa3d_ctx_t *ctx, GLfloat winx, GLfloat winy, GLfloat winz,
GLfloat *objx, GLfloat *objy, GLfloat *objz);
void MesaUnprojectCalc(mesa3d_ctx_t *ctx);

/* vertex (needs GL_BLOCK + glBegin) */
void MesaDrawVertex(mesa3d_ctx_t *ctx, LPD3DVERTEX vertex);
void MesaDrawLVertex(mesa3d_ctx_t *ctx, LPD3DLVERTEX vertex);
void MesaDrawTLVertex(mesa3d_ctx_t *ctx, LPD3DTLVERTEX vertex);

/* chroma to alpha conversion */
void *MesaChroma32(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey);
void *MesaChroma24(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey);
void *MesaChroma16(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey);
void *MesaChroma15(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey);
void *MesaChroma12(const void *buf, DWORD w, DWORD h, DWORD lwkey, DWORD hikey);
void MesaChromaFree(void *ptr);

#define MESA_1OVER255	0.003921568627451f

#define MESA_D3DCOLOR_TO_FV(_c, _v) \
	(_v)[0] = RGBA_GETRED(_c)*MESA_1OVER255; \
	(_v)[1] = RGBA_GETGREEN(_c)*MESA_1OVER255; \
	(_v)[2] = RGBA_GETBLUE(_c)*MESA_1OVER255; \
	(_v)[3] = RGBA_GETALPHA(_c)*MESA_1OVER255

#endif /* __MESA3D_H__INCLUDED__ */
