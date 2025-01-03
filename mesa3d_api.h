MESA_API_OS(OSMesaCreateContextExt, OSMesaContext, (GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist))
MESA_API_OS(OSMesaMakeCurrent, GLboolean, (OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height))
MESA_API_OS(OSMesaGetCurrentContext, OSMesaContext, (void))
MESA_API_OS(OSMesaDestroyContext, void, (OSMesaContext ctx))
MESA_API_OS(OSMesaGetDepthBuffer, GLboolean, (OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer))
MESA_API_OS(OSMesaGetColorBuffer, GLboolean, (OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer))
MESA_API_OS(OSMesaPixelStore, void, (GLint pname, GLint value))
MESA_API_DRV(DrvCreateLayerContext, HGLRC, (HDC hdc, INT iLayerPlane))
MESA_API_DRV(DrvSetPixelFormat, BOOL, (HDC hdc, LONG iPixelFormat))
MESA_API_DRV(DrvDescribePixelFormat, LONG, (HDC hdc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR* ppfd))
MESA_API_DRV(DrvSetContext, void*, (HDC hdc, HGLRC dhglrc, void *pfnSetProcTable))
MESA_API_DRV(DrvReleaseContext, BOOL, (HGLRC dhglrc))
MESA_API_DRV(DrvDeleteContext, BOOL, (HGLRC dhglrc))
MESA_API_DRV(DrvSwapBuffers, BOOL, (HDC hdc))
MESA_API_DRV(DrvGetCurrentContext, HGLRC, (void))
MESA_API(glGenTextures, void, (GLsizei n,	GLuint *textures))
MESA_API(glDeleteTextures, void, (GLsizei n, const GLuint *textures))
MESA_API(glPixelStorei, void, (GLenum pname, GLint param))
MESA_API(glTexImage2D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *data))
MESA_API(glCompressedTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data))
MESA_API(glGetError, GLenum, (void))
MESA_API(glBegin, void, (GLenum mode))
MESA_API(glEnd, void, (void))
MESA_API(glReadPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *data))
MESA_API(glColor3f, void, (GLfloat red, GLfloat green, GLfloat blue))
MESA_API(glColor4f, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
MESA_API(glColor3b, void, (GLbyte red, GLbyte green, GLbyte blue))
//MESA_API(glColor4b, void, (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha))
MESA_API(glColor4ub, void, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha))
MESA_API(glVertex3f, void, (GLfloat x, GLfloat y, GLfloat z))
MESA_API(glVertex3fv, void, (const GLfloat *v))
MESA_API(glVertex4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w))
MESA_API(glVertex4fv, void, (const GLfloat *v))
MESA_API(glVertex4dv, void, (const GLdouble *v))
MESA_API(glNormal3fv, void, (const GLfloat *v))
MESA_API(glEnable, void, (GLenum cap))
MESA_API(glDisable, void, (GLenum cap))
MESA_API(glFinish, void, (void))
MESA_API(glClear, void, (GLbitfield mask))
MESA_API(glClearColor, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
MESA_API(glClearDepth, void, (GLdouble depth))
MESA_API(glClearStencil, void, (GLint s))
MESA_API(glBindTexture, void, (GLenum target, GLuint texture))
MESA_API(glActiveTexture, void, (GLenum texture))
MESA_API(glFrontFace, void, (GLenum mode))
MESA_API(glPolygonMode, void, (GLenum face, GLenum mode))
MESA_API(glTexCoord2f, void, (GLfloat s, GLfloat t))
MESA_API(glMultiTexCoord1f, void, (GLenum target,	GLfloat s))
MESA_API(glMultiTexCoord2f, void, (GLenum target, GLfloat s, GLfloat t))
MESA_API(glMultiTexCoord3f, void, (GLenum target, GLfloat s, GLfloat t, GLfloat r))
MESA_API(glMultiTexCoord4f, void, (GLenum target, GLfloat s, GLfloat t,	GLfloat r, GLfloat q))
MESA_API(glGetString, const GLubyte *, (GLenum name))
MESA_API(glCullFace, void, (GLenum mode))
MESA_API(glShadeModel, void, (GLenum mode))
MESA_API(glLoadIdentity, void, (void))
MESA_API(glLoadMatrixf, void, (const GLfloat *m))
MESA_API(glMultMatrixf, void, (const GLfloat *m))
MESA_API(glMatrixMode, void, (GLenum mode))
MESA_API(glPushMatrix, void, (void))
MESA_API(glPopMatrix, void, (void))
MESA_API(glDepthMask, void, (GLboolean flag))
MESA_API(glDepthRange, void, (GLdouble nearVal, GLdouble farVal))
MESA_API(glGenFramebuffers, void, (GLsizei n, GLuint *ids))
MESA_API(glBindFramebuffer, void, (GLenum target, GLuint framebuffer))
MESA_API(glFramebufferTexture2D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
MESA_API(glBlitFramebuffer, void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))
MESA_API(glDeleteFramebuffers, void, (GLsizei n, GLuint *framebuffers))
MESA_API(glBindRenderbuffer, void, (GLenum target, GLuint renderbuffer))
MESA_API(glFramebufferRenderbuffer, void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer))
MESA_API(glDepthFunc, void, (GLenum func))
MESA_API(glTexParameteri, void, (GLenum target, GLenum pname, GLint param))
MESA_API(glTexParameterf, void, (GLenum target, GLenum pname, GLfloat param))
MESA_API(glTexParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params))
MESA_API(glBlendFunc, void, (GLenum sfactor, GLenum dfactor))
MESA_API(glBlendFuncSeparate, void, (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha))
MESA_API(glCheckFramebufferStatus, GLenum, (GLenum target))
MESA_API(glRenderbufferStorage, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height))
MESA_API(glGetFloatv, void, (GLenum pname, GLfloat *data))
MESA_API(glGetIntegerv, void, (GLenum pname, GLint *data))
MESA_API(glAlphaFunc, void, (GLenum func, GLclampf ref))
MESA_API(glTexEnvi, void, (GLenum target, GLenum pname, GLint param))
MESA_API(glTexEnvf, void, (GLenum target, GLenum pname, GLfloat param))
MESA_API(glTexEnvfv, void, (GLenum target, GLenum pname, const GLfloat *params))
MESA_API(glSecondaryColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue))
MESA_API(glFogi, void, (GLenum pname, GLint param))
MESA_API(glFogf, void, (GLenum pname, GLfloat param))
MESA_API(glFogfv, void, (GLenum pname, const GLfloat * params))
MESA_API(glPolygonStipple, void, (const GLubyte * pattern))
MESA_API(glScissor, void, (GLint x, GLint y, GLsizei width, GLsizei height))
MESA_API(glStencilFunc, void, (GLenum func, GLint ref, GLuint mask))
MESA_API(glStencilOp, void, (GLenum sfail, GLenum dpfail, GLenum dppass))
MESA_API(glStencilMask, void, (GLuint mask))
MESA_API(glViewport, void, (GLint x, GLint y, GLsizei width, GLsizei height))
MESA_API(glLightfv, void, (GLenum light, GLenum pname, const GLfloat * params))
MESA_API(glLightf, void, (GLenum light,	GLenum pname,	GLfloat param))
MESA_API(glLightModeli, void, (GLenum pname, GLint param))
MESA_API(glLightModelfv, void, (GLenum pname, const GLfloat *params))
MESA_API(glMaterialf, void, (GLenum face, GLenum pname,	GLfloat param))
MESA_API(glMaterialfv, void, (GLenum face, GLenum pname, const GLfloat * params))
MESA_API(glClipPlane, void, (GLenum plane, const GLdouble * equation))

MESA_API(glScalef, void, (GLfloat x, GLfloat y, GLfloat z))
