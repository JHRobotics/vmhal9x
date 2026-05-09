#ifndef __SURFINDEX_H__
#define __SURFINDEX_H__

#include "nuke.h"

typedef struct _ddsurface_t
{
	void *flatptr;
	DWORD uid; /* get by FBHDA_DD_surface_set */
	DWORD dxid;
	DWORD ctxid; /* 0 for global */
	DWORD level;
	BOOL dirty; /* only THIS surface */
	void *lcl; /* DX7 and older */
	DWORD usermem_pid; /* flatptr is in user memory */
	struct _ddsurface_t *up;
	struct _ddsurface_t *down;
} ddsurface_t;

typedef struct _gldata_t
{
	GLenum target;
 	GLint internalformat;
 	GLsizei width;
 	GLsizei height;
 	GLenum format;
 	GLenum type;
 	BOOL compressed;
 	BOOL palette;
 	BOOL colorkey;
 	BOOL need_convert;
 	DWORD ck_low;
 	DWORD ck_high;
 	DWORD *pal;
 	DWORD pal_count;
 	int depth;
 	int bpp;
 	int dirty; /* full chain */
} gldata_t;

typedef struct _gldata_tex_t
{
	DWORD dxid;
	unsigned int gltex;
	DWORD mipmaps;
	FBHDA_DD_surface_attrs_t loadedstate;
} gldata_tex_t;

typedef struct _surfaceex_t
{
	ddsurface_t *dd;
	FBHDA_DD_surface_t *fb;
	gldata_t *gl;
	gldata_tex_t *tex;
} surfaceex_t;

#define DDSURFACE_CUBE_SIDE_0 0x0100
#define DDSURFACE_CUBE_SIDE_1 0x0200
#define DDSURFACE_CUBE_SIDE_2 0x0300
#define DDSURFACE_CUBE_SIDE_3 0x0400
#define DDSURFACE_CUBE_SIDE_4 0x0500
#define DDSURFACE_CUBE_SIDE_5 0x0600

#define DDSURFACE_CUBE_MASK   0x0700
#define DDSURFACE_LEVEL_MASK  0x00FF

#define DDSURFACE_DEPTH       0x1000

typedef struct mesa3d_ctx mesa3d_ctx_t;
typedef struct mesa3d_entry mesa3d_entry_t;

NUKED_LOCAL ddsurface_t *ddsurf_register(mesa3d_entry_t *entry, LPDDRAWI_DDRAWSURFACE_LCL surf, DWORD ctxid);
NUKED_LOCAL ddsurface_t *ddsurf_get_by_id(mesa3d_ctx_t *ctx, DWORD dxid);
NUKED_LOCAL surfaceex_t *surfex_get(mesa3d_ctx_t *ctx, DWORD dxid, BOOL need_fb, BOOL need_gl);
NUKED_LOCAL BOOL ddsurf_glinfo(ddsurface_t *ddin, gldata_t *out, int inlevel, int *gl_level);
NUKED_LOCAL BOOL ddsurf_swap(mesa3d_ctx_t *ctx, DWORD dxid_1, DWORD dxid_2);

/* broadcast surface delete message  */
NUKED_LOCAL void ddsurf_unregister(mesa3d_entry_t *entry, DWORD ctxid, DWORD dxid);

/* remove surface and free memory */
NUKED_LOCAL void ddsurf_destroy(mesa3d_ctx_t *ctx, DWORD dxid);

#define DXID_OWN 0x80000000

#define DW_FLAT(_ptr) ((DWORD)(_ptr))

#define FOURCC_BUFFER 0x00000000

#endif
