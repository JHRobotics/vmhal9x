/* FROM D3DMINI.C */

#define nullPrimCaps {sizeof(D3DPRIMCAPS), 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0, 0}

#define ALL_D3DPBLENDCAPS (D3DPBLENDCAPS_ZERO|D3DPBLENDCAPS_ONE|D3DPBLENDCAPS_SRCCOLOR| \
	D3DPBLENDCAPS_INVSRCCOLOR|D3DPBLENDCAPS_SRCALPHA|D3DPBLENDCAPS_INVSRCALPHA|D3DPBLENDCAPS_DESTALPHA| \
	D3DPBLENDCAPS_INVDESTALPHA|D3DPBLENDCAPS_DESTCOLOR|D3DPBLENDCAPS_INVDESTCOLOR|\
	D3DPBLENDCAPS_BLENDFACTOR|D3DPBLENDCAPS_SRCALPHASAT)

#define triCaps5 {               \
	sizeof(D3DPRIMCAPS),          \
	/* miscCaps */                \
	D3DPMISCCAPS_CULLNONE         \
	| D3DPMISCCAPS_CULLCW         \
	| D3DPMISCCAPS_CULLCCW        \
	| D3DPMISCCAPS_MASKZ          \
	| D3DPMISCCAPS_LINEPATTERNREP \
	| D3DPMISCCAPS_MASKPLANES     \
	| D3DPMISCCAPS_CONFORMANT,    \
	/* rasterCaps */              \
	D3DPRASTERCAPS_DITHER         \
	/*| D3DPRASTERCAPS_ROP2*/         \
	| D3DPRASTERCAPS_PAT          \
	| D3DPRASTERCAPS_SUBPIXEL     \
	| D3DPRASTERCAPS_FOGTABLE     \
	/*| D3DPRASTERCAPS_FOGRANGE*/     \
	/*| D3DPRASTERCAPS_FOGVERTEX*/    \
	| D3DPRASTERCAPS_STIPPLE      \
	/*| D3DPRASTERCAPS_WFOG*/         \
	/* | D3DPRASTERCAPS_ZTEST*/,  \
	/* zCmpCaps */                \
	D3DPCMPCAPS_NEVER             \
	| D3DPCMPCAPS_LESS            \
	| D3DPCMPCAPS_EQUAL           \
	| D3DPCMPCAPS_LESSEQUAL       \
	| D3DPCMPCAPS_GREATER         \
	| D3DPCMPCAPS_NOTEQUAL        \
	| D3DPCMPCAPS_GREATEREQUAL    \
	| D3DPCMPCAPS_ALWAYS,         \
	/* sourceBlendCaps */         \
	ALL_D3DPBLENDCAPS                      \
	| D3DPBLENDCAPS_BOTHSRCALPHA           \
	| D3DPBLENDCAPS_BOTHINVSRCALPHA,       \
	/* destBlendCaps */           \
	ALL_D3DPBLENDCAPS,            \
	/* alphaCmpCaps */            \
	D3DPCMPCAPS_NEVER             \
	| D3DPCMPCAPS_LESS            \
	| D3DPCMPCAPS_EQUAL           \
	| D3DPCMPCAPS_LESSEQUAL       \
	| D3DPCMPCAPS_GREATER         \
	| D3DPCMPCAPS_NOTEQUAL        \
	| D3DPCMPCAPS_GREATEREQUAL    \
	| D3DPCMPCAPS_ALWAYS,         \
	/* shadeCaps */               \
	D3DPSHADECAPS_COLORFLATRGB            \
	| D3DPSHADECAPS_COLORGOURAUDRGB       \
	| D3DPSHADECAPS_SPECULARFLATRGB       \
	| D3DPSHADECAPS_SPECULARGOURAUDRGB    \
	| D3DPSHADECAPS_SPECULARPHONGRGB      \
	| D3DPSHADECAPS_ALPHAFLATBLEND        \
	| D3DPSHADECAPS_ALPHAGOURAUDBLEND     \
	| D3DPSHADECAPS_ALPHAPHONGBLEND       \
	| D3DPSHADECAPS_FOGFLAT               \
	| D3DPSHADECAPS_FOGPHONG              \
	| D3DPSHADECAPS_FOGGOURAUD,           \
	/* textureCaps */                     \
	D3DPTEXTURECAPS_PERSPECTIVE           \
	| D3DPTEXTURECAPS_POW2                \
	| D3DPTEXTURECAPS_TRANSPARENCY        \
	| D3DPTEXTURECAPS_ALPHA,              \
	/* textureFilterCaps */               \
	D3DPTFILTERCAPS_NEAREST               \
	| D3DPTFILTERCAPS_LINEAR              \
	| D3DPTFILTERCAPS_MIPNEAREST          \
	| D3DPTFILTERCAPS_MIPLINEAR           \
	| D3DPTFILTERCAPS_LINEARMIPNEAREST    \
	| D3DPTFILTERCAPS_LINEARMIPLINEAR,    \
	/* textureBlendCaps */                \
	D3DPTBLENDCAPS_DECAL                  \
	| D3DPTBLENDCAPS_MODULATE             \
	| D3DPTBLENDCAPS_MODULATEALPHA        \
	| D3DPTBLENDCAPS_COPY                 \
	| D3DPTBLENDCAPS_ADD                  \
	| D3DPTBLENDCAPS_DECALMASK            \
	| D3DPTBLENDCAPS_DECALALPHA           \
	| D3DPTBLENDCAPS_MODULATEMASK,        \
	/* textureAddressCaps */              \
	D3DPTADDRESSCAPS_WRAP                 \
	| D3DPTADDRESSCAPS_MIRROR             \
	| D3DPTADDRESSCAPS_CLAMP              \
	| D3DPTADDRESSCAPS_BORDER             \
	| D3DPTADDRESSCAPS_INDEPENDENTUV,     \
	/* stippleWidth */                    \
	32,                                   \
	/* stippleHeight */                   \
	32                                    \
}

#define triCaps6 {               \
	sizeof(D3DPRIMCAPS),          \
	/* miscCaps */                \
	D3DPMISCCAPS_CULLNONE         \
	| D3DPMISCCAPS_CULLCW         \
	| D3DPMISCCAPS_CULLCCW        \
	| D3DPMISCCAPS_MASKZ          \
	| D3DPMISCCAPS_LINEPATTERNREP \
	| D3DPMISCCAPS_MASKPLANES     \
	| D3DPMISCCAPS_CONFORMANT,    \
	/* rasterCaps */              \
	D3DPRASTERCAPS_DITHER         \
	/*| D3DPRASTERCAPS_ROP2*/         \
	| D3DPRASTERCAPS_PAT          \
	| D3DPRASTERCAPS_SUBPIXEL     \
	| D3DPRASTERCAPS_FOGTABLE     \
	| D3DPRASTERCAPS_FOGRANGE     \
	| D3DPRASTERCAPS_FOGVERTEX    \
	| D3DPRASTERCAPS_STIPPLE      \
	| D3DPRASTERCAPS_WFOG         \
	| D3DPRASTERCAPS_WBUFFER      \
	| D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT \
	| D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT \
	| D3DPRASTERCAPS_ANTIALIASEDGES \
	| D3DPRASTERCAPS_MIPMAPLODBIAS \
	| D3DPRASTERCAPS_ZBIAS         \
	| D3DPRASTERCAPS_ANISOTROPY    \
	/* | D3DPRASTERCAPS_ZFOG */    \
	/* | D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT */ \
	| D3DPRASTERCAPS_ZTEST,       \
	/* zCmpCaps */                \
	D3DPCMPCAPS_NEVER             \
	| D3DPCMPCAPS_LESS            \
	| D3DPCMPCAPS_EQUAL           \
	| D3DPCMPCAPS_LESSEQUAL       \
	| D3DPCMPCAPS_GREATER         \
	| D3DPCMPCAPS_NOTEQUAL        \
	| D3DPCMPCAPS_GREATEREQUAL    \
	| D3DPCMPCAPS_ALWAYS,         \
	/* sourceBlendCaps */         \
	ALL_D3DPBLENDCAPS                      \
	| D3DPBLENDCAPS_BOTHSRCALPHA           \
	| D3DPBLENDCAPS_BOTHINVSRCALPHA,       \
	/* destBlendCaps */           \
	ALL_D3DPBLENDCAPS,            \
	/* alphaCmpCaps */            \
	D3DPCMPCAPS_NEVER             \
	| D3DPCMPCAPS_LESS            \
	| D3DPCMPCAPS_EQUAL           \
	| D3DPCMPCAPS_LESSEQUAL       \
	| D3DPCMPCAPS_GREATER         \
	| D3DPCMPCAPS_NOTEQUAL        \
	| D3DPCMPCAPS_GREATEREQUAL    \
	| D3DPCMPCAPS_ALWAYS,         \
	/* shadeCaps */               \
	D3DPSHADECAPS_COLORFLATRGB            \
	| D3DPSHADECAPS_COLORGOURAUDRGB       \
	| D3DPSHADECAPS_SPECULARFLATRGB       \
	| D3DPSHADECAPS_SPECULARGOURAUDRGB    \
	| D3DPSHADECAPS_SPECULARPHONGRGB      \
	| D3DPSHADECAPS_ALPHAFLATBLEND        \
	| D3DPSHADECAPS_ALPHAGOURAUDBLEND     \
	| D3DPSHADECAPS_ALPHAPHONGBLEND       \
	| D3DPSHADECAPS_FOGFLAT               \
	| D3DPSHADECAPS_FOGPHONG              \
	| D3DPSHADECAPS_FOGGOURAUD,           \
	/* textureCaps */                     \
	D3DPTEXTURECAPS_PERSPECTIVE           \
	/*| D3DPTEXTURECAPS_POW2*/                \
	/*| D3DPTEXTURECAPS_NONPOW2CONDITIONAL*/  \
	| D3DPTEXTURECAPS_TRANSPARENCY        \
	| D3DPTEXTURECAPS_ALPHA               \
	| D3DPTEXTURECAPS_BORDER              \
	| D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE \
	| D3DPTEXTURECAPS_PROJECTED           \
	| D3DPTEXTURECAPS_CUBEMAP              \
	/* | D3DPTEXTURECAPS_MIPCUBEMAP (dx9) */    \
	/*| D3DPTEXTURECAPS_CUBEMAP_POW2 (dx9) */         \
	| D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE /* probably... */ \
	/* | D3DPTEXTURECAPS_COLORKEYBLEND */, \
	/* textureFilterCaps */               \
	D3DPTFILTERCAPS_NEAREST               \
	| D3DPTFILTERCAPS_LINEAR              \
	| D3DPTFILTERCAPS_MIPNEAREST          \
	| D3DPTFILTERCAPS_MIPLINEAR           \
	| D3DPTFILTERCAPS_LINEARMIPNEAREST    \
	| D3DPTFILTERCAPS_LINEARMIPLINEAR     \
	| D3DPTFILTERCAPS_MINFPOINT           \
	| D3DPTFILTERCAPS_MINFLINEAR          \
	| D3DPTFILTERCAPS_MINFANISOTROPIC     \
	| D3DPTFILTERCAPS_MIPFPOINT           \
	| D3DPTFILTERCAPS_MIPFLINEAR          \
	| D3DPTFILTERCAPS_MAGFPOINT           \
	| D3DPTFILTERCAPS_MAGFLINEAR          \
	| D3DPTFILTERCAPS_MAGFANISOTROPIC,    \
	/* textureBlendCaps */                \
	D3DPTBLENDCAPS_DECAL                  \
	| D3DPTBLENDCAPS_MODULATE             \
	| D3DPTBLENDCAPS_MODULATEALPHA        \
	| D3DPTBLENDCAPS_COPY                 \
	| D3DPTBLENDCAPS_ADD                  \
	| D3DPTBLENDCAPS_DECALALPHA           \
	| D3DPTBLENDCAPS_DECALMASK            \
	| D3DPTBLENDCAPS_MODULATEMASK,        \
	/* textureAddressCaps */              \
	D3DPTADDRESSCAPS_WRAP                 \
	| D3DPTADDRESSCAPS_MIRROR             \
	| D3DPTADDRESSCAPS_CLAMP              \
	| D3DPTADDRESSCAPS_BORDER             \
	| D3DPTADDRESSCAPS_INDEPENDENTUV,     \
	/* stippleWidth */                    \
	32,                                   \
	/* stippleHeight */                   \
	32                                    \
}

static D3DDEVICEDESC_V1 myCaps = {
	sizeof(D3DDEVICEDESC),      /* dwSize */
	/* dwFlags */
	D3DDD_COLORMODEL
	| D3DDD_DEVCAPS
	| D3DDD_TRICAPS
	| D3DDD_DEVICERENDERBITDEPTH
	| D3DDD_DEVICEZBUFFERBITDEPTH,
	/* dcmColorModel */
	D3DCOLOR_RGB,
	/* devCaps */
	D3DDEVCAPS_FLOATTLVERTEX  /* must be set */
	| D3DDEVCAPS_TEXTUREVIDEOMEMORY
	| D3DDEVCAPS_TLVERTEXSYSTEMMEMORY /* must be set */
	| D3DDEVCAPS_DRAWPRIMTLVERTEX /* must be set */
	| D3DDEVCAPS_EXECUTESYSTEMMEMORY /* must be set */
	| D3DDEVCAPS_TEXTURESYSTEMMEMORY /* cube map needs this */
	/*| D3DDEVCAPS_SORTEXACT*/
	/*| D3DDEVCAPS_SORTINCREASINGZ */,
	{sizeof(D3DTRANSFORMCAPS), 0},		/* dtcTransformCaps */
	FALSE,                      /* bClipping */
	{sizeof(D3DLIGHTINGCAPS), 0},  /* dlcLightingCaps */
	nullPrimCaps,               /* lineCaps */
	triCaps5,                    /* triCaps */
	DDBD_16 | DDBD_32,	/* dwDeviceRenderBitDepth */
	DDBD_16 | DDBD_24 | DDBD_32, /* dwDeviceZBufferBitDepth */
	0,			        /* dwMaxBufferSize */
	0      			/* dwMaxVertexCount */
};

static D3DDEVICEDESC_V1 myCaps6 = {
	sizeof(D3DDEVICEDESC),      /* dwSize */
	/* dwFlags */
	D3DDD_COLORMODEL
	| D3DDD_DEVCAPS
	| D3DDD_TRICAPS
	| D3DDD_LINECAPS
	| D3DDD_DEVICERENDERBITDEPTH
	| D3DDD_DEVICEZBUFFERBITDEPTH
	| D3DDD_BCLIPPING,
	/* dcmColorModel */
	D3DCOLOR_RGB,
	/* devCaps */
	D3DDEVCAPS_FLOATTLVERTEX  /* must be set */
	| D3DDEVCAPS_TEXTUREVIDEOMEMORY /* must be set */
	| D3DDEVCAPS_TEXTURESYSTEMMEMORY /* cube map needs this */
	| D3DDEVCAPS_TLVERTEXSYSTEMMEMORY /* must be set */
	| D3DDEVCAPS_DRAWPRIMTLVERTEX /* must be set */
	| D3DDEVCAPS_EXECUTESYSTEMMEMORY /* must be set */
	| D3DDEVCAPS_DRAWPRIMITIVES2
	| D3DDEVCAPS_DRAWPRIMITIVES2EX
	| D3DDEVCAPS_SORTINCREASINGZ
	| D3DDEVCAPS_SORTEXACT
	| D3DDEVCAPS_HWVERTEXBUFFER
	| D3DDEVCAPS_CANRENDERAFTERFLIP /* must be set on 2k driver */
	| D3DDEVCAPS_HWINDEXBUFFER, /* DX8 */
	{sizeof(D3DTRANSFORMCAPS), 0},		/* dtcTransformCaps */
	TRUE,                      /* bClipping */
	{sizeof(D3DLIGHTINGCAPS), 0},  /* dlcLightingCaps */
	triCaps6,               /* lineCaps */
	triCaps6,                   /* triCaps */
	DDBD_16 | DDBD_32,	/* dwDeviceRenderBitDepth */
	DDBD_16 | DDBD_24 | DDBD_32, /* dwDeviceZBufferBitDepth */
	0,			        /* dwMaxBufferSize */
	0      			/* dwMaxVertexCount */
};

#define CAPS_DX7 (D3DDEVCAPS_HWVERTEXBUFFER|D3DDEVCAPS_CANRENDERAFTERFLIP|D3DDEVCAPS_DRAWPRIMITIVES2EX)
#define CAPS_DX8 (D3DDEVCAPS_SEPARATETEXTUREMEMORIES|D3DDEVCAPS_HWINDEXBUFFER)

/*
D3DDEVCAPS_HWTRANSFORMANDLIGHT
D3DDEVCAPS_CANBLTSYSTONONLOCAL
D3DDEVCAPS_HWRASTERIZATION
*/

#define TEXFORMAT_RGB_mask(_ddpf_flags, _bits, _rmask, _gmask, _bmask, _amask) \
{ sizeof(DDSURFACEDESC),              /* dwSize */ \
	DDSD_CAPS | DDSD_PIXELFORMAT,       /* dwFlags */ \
	0,                                  /* dwHeight */ \
	0,                                  /* dwWidth */ \
	{0},                                /* lPitch */ \
	0,                                  /* dwBackBufferCount */ \
	{0},                                /* dwZBufferBitDepth */ \
	0,                                  /* dwAlphaBitDepth */ \
	0,                                  /* dwReserved */ \
	NULL,                               /* lpSurface */ \
	{ 0, 0 },                           /* ddckCKDestOverlay */ \
	{ 0, 0 },                           /* ddckCKDestBlt */ \
	{ 0, 0 },                           /* ddckCKSrcOverlay */ \
	{ 0, 0 },                           /* ddckCKSrcBlt */ \
	{ sizeof(DDPIXELFORMAT),            /* ddpfPixelFormat.dwSize */ \
		DDPF_RGB | _ddpf_flags,           /* ddpfPixelFormat.dwFlags */ \
		0,				                        /* FOURCC code */ \
		{_bits},                          /* ddpfPixelFormat.dwRGBBitCount */ \
		{_rmask}, \
		{_gmask}, \
		{_bmask}, \
		{_amask} \
	}, {DDSCAPS_TEXTURE}                     /* ddscaps.dwCaps */ \
}

#define TEXFORMAT_ZBUF(_bits, _zmask) \
{ sizeof(DDSURFACEDESC),              /* dwSize */ \
	DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_ZBUFFERBITDEPTH, /* dwFlags */ \
	0,                                  /* dwHeight */ \
	0,                                  /* dwWidth */ \
	{0},                                /* lPitch */ \
	0,                                  /* dwBackBufferCount */ \
	{_bits},                            /* dwZBufferBitDepth */ \
	0,                                  /* dwAlphaBitDepth */ \
	0,                                  /* dwReserved */ \
	NULL,                               /* lpSurface */ \
	{ 0, 0 },                           /* ddckCKDestOverlay */ \
	{ 0, 0 },                           /* ddckCKDestBlt */ \
	{ 0, 0 },                           /* ddckCKSrcOverlay */ \
	{ 0, 0 },                           /* ddckCKSrcBlt */ \
	{ sizeof(DDPIXELFORMAT),            /* ddpfPixelFormat.dwSize */ \
		DDPF_ZBUFFER,                     /* ddpfPixelFormat.dwFlags */ \
		0,				                        /* FOURCC code */ \
		{_bits},                          /* ddpfPixelFormat.dwRGBBitCount */ \
		{0}, \
		{0}, \
		{0}, \
		{_zmask} \
	}, {DDSCAPS_ZBUFFER}                /* ddscaps.dwCaps */ \
}

#define TEXFORMAT_FOURCC(ch0, ch1, ch2, ch3) \
{ sizeof(DDSURFACEDESC),              /* dwSize */ \
	DDSD_CAPS | DDSD_PIXELFORMAT,       /* dwFlags */ \
	0,                                  /* dwHeight */ \
	0,                                  /* dwWidth */ \
	{0},                                /* lPitch */ \
	0,                                  /* dwBackBufferCount */ \
	{0},                                /* dwZBufferBitDepth */ \
	0,                                  /* dwAlphaBitDepth */ \
	0,                                  /* dwReserved */ \
	NULL,                               /* lpSurface */ \
	{ 0, 0 },                           /* ddckCKDestOverlay */ \
	{ 0, 0 },                           /* ddckCKDestBlt */ \
	{ 0, 0 },                           /* ddckCKSrcOverlay */ \
	{ 0, 0 },                           /* ddckCKSrcBlt */ \
	{ sizeof(DDPIXELFORMAT),            /* ddpfPixelFormat.dwSize */ \
		DDPF_FOURCC,                      /* ddpfPixelFormat.dwFlags */ \
		MAKEFOURCC(ch0, ch1, ch2, ch3),   /* FOURCC code */ \
		{0},                              /* ddpfPixelFormat.dwRGBBitCount */ \
		{0}, \
		{0}, \
		{0}, \
		{0}  \
	}, {DDSCAPS_TEXTURE}                /* ddscaps.dwCaps */ \
}

#define TEXFORMAT_LA_mask(_ddpf_flags, _lbits, _lmask, _abits, _amask) \
{ sizeof(DDSURFACEDESC),              /* dwSize */ \
	DDSD_CAPS | DDSD_PIXELFORMAT,       /* dwFlags */ \
	0,                                  /* dwHeight */ \
	0,                                  /* dwWidth */ \
	{0},                                /* lPitch */ \
	0,                                  /* dwBackBufferCount */ \
	{0},                                /* dwZBufferBitDepth */ \
	_abits,                             /* dwAlphaBitDepth */ \
	0,                                  /* dwReserved */ \
	NULL,                               /* lpSurface */ \
	{ 0, 0 },                           /* ddckCKDestOverlay */ \
	{ 0, 0 },                           /* ddckCKDestBlt */ \
	{ 0, 0 },                           /* ddckCKSrcOverlay */ \
	{ 0, 0 },                           /* ddckCKSrcBlt */ \
	{ sizeof(DDPIXELFORMAT),            /* ddpfPixelFormat.dwSize */ \
		_ddpf_flags,                      /* ddpfPixelFormat.dwFlags */ \
		0,				                        /* FOURCC code */ \
		{_lbits + _abits},                /* ddpfPixelFormat.dwRGBBitCount */ \
		{_lmask}, \
		{0}, \
		{0}, \
		{_amask} \
	}, {DDSCAPS_TEXTURE}                     /* ddscaps.dwCaps */ \
}

#define TEXFORMAT_ALPHA(_abits, _amask) \
{ sizeof(DDSURFACEDESC),              /* dwSize */ \
	DDSD_CAPS | DDSD_PIXELFORMAT,       /* dwFlags */ \
	0,                                  /* dwHeight */ \
	0,                                  /* dwWidth */ \
	{0},                                /* lPitch */ \
	0,                                  /* dwBackBufferCount */ \
	{0},                                /* dwZBufferBitDepth */ \
	0,                             /* dwAlphaBitDepth */ \
	0,                                  /* dwReserved */ \
	NULL,                               /* lpSurface */ \
	{ 0, 0 },                           /* ddckCKDestOverlay */ \
	{ 0, 0 },                           /* ddckCKDestBlt */ \
	{ 0, 0 },                           /* ddckCKSrcOverlay */ \
	{ 0, 0 },                           /* ddckCKSrcBlt */ \
	{ sizeof(DDPIXELFORMAT),            /* ddpfPixelFormat.dwSize */ \
		DDPF_ALPHA,                       /* ddpfPixelFormat.dwFlags */ \
		0,				                        /* FOURCC code */ \
		{_abits},                         /* ddpfPixelFormat.dwRGBBitCount */ \
		{0}, \
		{0}, \
		{0}, \
		{/*_amask*/0} \
	}, {DDSCAPS_TEXTURE}                     /* ddscaps.dwCaps */ \
}

#define TEXFORMAT_PAL8 TEXFORMAT_RGB_mask(DDPF_PALETTEINDEXED8, 8, 0, 0, 0, 0)

#define TEXFORMAT_RGB(_bpp, _r, _g, _b) TEXFORMAT_RGB_mask(0, _bpp, _r, _g, _b, 0)
#define TEXFORMAT_RGBA(_bpp, _r, _g, _b, _a) TEXFORMAT_RGB_mask(DDPF_ALPHAPIXELS, _bpp, _r, _g, _b, _a)

#define TEXFORMAT_LUMINANCE(_bits, _mask) TEXFORMAT_LA_mask(DDPF_LUMINANCE, _bits, _mask, 0, 0)
#define TEXFORMAT_LA(_lbits, _lmask, _abits, _amask) TEXFORMAT_LA_mask(DDPF_ALPHAPIXELS | DDPF_LUMINANCE, _lbits, _lmask, _abits, _amask)
//#define TEXFORMAT_ALPHA(_bits, _mask) TEXFORMAT_LA_mask(DDPF_ALPHA, 0, 0, _bits, /*_mask*/0) /* !not works */

static DDSURFACEDESC myTextureFormats[] = {
	TEXFORMAT_RGB(32, 0x00FF0000, 0x0000FF00, 0x000000FF), /* BGRX 8888 */
	TEXFORMAT_RGB(16, 0xF800, 0x07E0, 0x001F), /* RGB 565 */

	TEXFORMAT_RGBA(32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000), /* BGRA 8888 */
	TEXFORMAT_RGBA(16, 0x7C00, 0x03E0, 0x001F, 0x8000), /* BGRA 5551 */
	TEXFORMAT_RGBA(16, 0x0F00, 0x00F0, 0x000F, 0xF000), /* BGRA 4444 */
	TEXFORMAT_RGB(16, 0x7C00, 0x03E0, 0x001F), /* BGRX 5551 */
	TEXFORMAT_RGB(16, 0x0F00, 0x00F0, 0x000F), /* RGBX 4444 */
	TEXFORMAT_RGB( 8, 0xE0, 0x1C, 0x03),       /* RGB 332 */

	TEXFORMAT_FOURCC('D', 'X', 'T', '1'),
	TEXFORMAT_FOURCC('D', 'X', 'T', '2'),
	TEXFORMAT_FOURCC('D', 'X', 'T', '3'),
	TEXFORMAT_FOURCC('D', 'X', 'T', '4'),
	TEXFORMAT_FOURCC('D', 'X', 'T', '5'),

	TEXFORMAT_RGBA(32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x000000FF), /* RGBA 8888 */
	TEXFORMAT_RGBA(16, 0x001F, 0x03E0, 0x7C00, 0x0001), /* RGBA 5551 */
	TEXFORMAT_RGBA(16, 0x000F, 0x00F0, 0x0F00, 0x000F), /* RGBA 4444 */
	TEXFORMAT_ALPHA(8, 0xFF),
	TEXFORMAT_LUMINANCE(8, 0xFF),
	TEXFORMAT_LA(8, 0x00FF, 8, 0xFF00),
	TEXFORMAT_PAL8,

	/* removed formats: */
//	TEXFORMAT_RGB(24, 0x00FF0000, 0x0000FF00, 0x000000FF), /* BGR 888 */ - not needed
//	TEXFORMAT_ZBUF(16, 0x0000FFFF),
//	TEXFORMAT_ZBUF(24, 0x00FFFFFF),
//	TEXFORMAT_ZBUF(32, 0x00FFFFFF),
//	TEXFORMAT_RGB(32, 0x000000FF, 0x0000FF00, 0x00FF0000), /* RGBX 8888 */
//	TEXFORMAT_RGB(24, 0x000000FF, 0x0000FF00, 0x00FF0000), /* RGB 888 */	
//	TEXFORMAT_RGB(16, 0x001F, 0x07E0, 0xF800), /* RGB 565 */
//	TEXFORMAT_RGB(16, 0x001F, 0x03E0, 0x7C00), /* RGBX 5551 */
//	TEXFORMAT_RGB(16, 0x000F, 0x00F0, 0x0F00), /* RGBX 4444 */
//	TEXFORMAT_RGB( 8, 0x03, 0x1C, 0xE0)        /* RGB 332 */
};

#define DX8_FORMAT(FourCC, Ops, dwMSFlipTypes) \
	{ sizeof(DDPIXELFORMAT), DDPF_D3DFORMAT, (FourCC), {0}, {(Ops) /*dwOperations*/ }, \
	{((dwMSFlipTypes) & 0xFFFF ) << 16 | ((dwMSFlipTypes) & 0xFFFF)}, {0}, {0} }

#if DX8_MULTISAMPLING
// Note: For multisampling we need to setup appropriately both the rendertarget
// and the depth buffer format's multisampling fields.
#define D3DMULTISAMPLE_NUM_SAMPLES (1 << (D3DMULTISAMPLE_4_SAMPLES - 1))
#else
#define D3DMULTISAMPLE_NUM_SAMPLES  D3DMULTISAMPLE_NONE
#endif // DX8_MULTISAMPLING

static DDPIXELFORMAT myTextureFormatsDX8[] =
{
	DX8_FORMAT(D3DFMT_X1R5G5B5,
		D3DFORMAT_OP_TEXTURE |
		D3DFORMAT_OP_VOLUMETEXTURE |
		D3DFORMAT_OP_CUBETEXTURE |
		D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET, D3DMULTISAMPLE_NUM_SAMPLES),
	DX8_FORMAT(D3DFMT_A1R5G5B5,
		D3DFORMAT_OP_TEXTURE |
		D3DFORMAT_OP_VOLUMETEXTURE |
		D3DFORMAT_OP_CUBETEXTURE |
		D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET, D3DMULTISAMPLE_NUM_SAMPLES),
	DX8_FORMAT(D3DFMT_R5G6B5,
		D3DFORMAT_OP_TEXTURE | 
		D3DFORMAT_OP_DISPLAYMODE |
		D3DFORMAT_OP_3DACCELERATION |
		D3DFORMAT_OP_CUBETEXTURE |
		D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET, D3DMULTISAMPLE_NUM_SAMPLES),
	DX8_FORMAT(D3DFMT_R5G6B5,
		D3DFORMAT_OP_VOLUMETEXTURE, 0),
	DX8_FORMAT(D3DFMT_X8R8G8B8,
		D3DFORMAT_OP_TEXTURE | 
		D3DFORMAT_OP_DISPLAYMODE | 
		D3DFORMAT_OP_3DACCELERATION |
		D3DFORMAT_OP_CUBETEXTURE |
		D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET, 0),
	DX8_FORMAT(D3DFMT_X8R8G8B8,
		D3DFORMAT_OP_VOLUMETEXTURE,	0),
//	DX8_FORMAT(D3DFMT_P8,	D3DFORMAT_OP_TEXTURE | D3DFORMAT_OP_VOLUMETEXTURE, 0),
	DX8_FORMAT(D3DFMT_A4R4G4B4,
		D3DFORMAT_OP_TEXTURE | D3DFORMAT_OP_VOLUMETEXTURE | D3DFORMAT_OP_CUBETEXTURE, 0),
	DX8_FORMAT(D3DFMT_A8R8G8B8,
		D3DFORMAT_OP_TEXTURE |
		D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
		D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
		D3DFORMAT_OP_CUBETEXTURE |
		D3DFORMAT_OP_VOLUMETEXTURE, 0),
	DX8_FORMAT(D3DFMT_D16_LOCKABLE,
		D3DFORMAT_OP_ZSTENCIL |
		D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, D3DMULTISAMPLE_NUM_SAMPLES),
	DX8_FORMAT(D3DFMT_D32,
		D3DFORMAT_OP_ZSTENCIL |
		D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH,  D3DMULTISAMPLE_NUM_SAMPLES),
	DX8_FORMAT(D3DFMT_S8D24,
		D3DFORMAT_OP_ZSTENCIL |
		D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, D3DMULTISAMPLE_NUM_SAMPLES),
};

#define MYTEXTUREFORMATSDX8_COUNT (sizeof(myTextureFormatsDX8)/sizeof(DDPIXELFORMAT))

#define MYSTENCIL_CAPS (D3DSTENCILCAPS_KEEP | D3DSTENCILCAPS_ZERO	| D3DSTENCILCAPS_REPLACE | \
	D3DSTENCILCAPS_INCRSAT | D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INVERT | \
	D3DSTENCILCAPS_INCR | D3DSTENCILCAPS_DECR)

#define MYTEXOPCAPS (D3DTEXOPCAPS_DISABLE	| \
			D3DTEXOPCAPS_SELECTARG1 | \
			D3DTEXOPCAPS_SELECTARG2 | \
			D3DTEXOPCAPS_MODULATE | \
			D3DTEXOPCAPS_MODULATE2X | \
			D3DTEXOPCAPS_MODULATE4X | \
			D3DTEXOPCAPS_ADD | \
			D3DTEXOPCAPS_ADDSIGNED | \
			D3DTEXOPCAPS_ADDSIGNED2X | \
			D3DTEXOPCAPS_SUBTRACT | \
			D3DTEXOPCAPS_BLENDDIFFUSEALPHA | \
			D3DTEXOPCAPS_BLENDTEXTUREALPHA | \
			D3DTEXOPCAPS_BLENDFACTORALPHA | \
			D3DTEXOPCAPS_BLENDCURRENTALPHA)

/* missing:
	D3DTEXOPCAPS_ADDSMOOTH
	D3DTEXOPCAPS_BLENDTEXTUREALPHAPM
	D3DTEXOPCAPS_PREMODULATE
	D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR
	D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA
	D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR
	D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA
	D3DTEXOPCAPS_DOTPRODUCT3
	D3DTEXOPCAPS_BUMPENVMAP
	D3DTEXOPCAPS_BUMPENVMAPLUMINANCE
*/

#define MYVERTEXPROCCAPS (D3DVTXPCAPS_LOCALVIEWER | \
				D3DVTXPCAPS_MATERIALSOURCE7   | \
				D3DVTXPCAPS_VERTEXFOG         | \
				D3DVTXPCAPS_DIRECTIONALLIGHTS | \
				D3DVTXPCAPS_POSITIONALLIGHTS)

/* missing
	D3DVTXPCAPS_TEXGEN
*/


