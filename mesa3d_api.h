MESA_API(OSMesaCreateContextExt, OSMesaContext, (GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist))
MESA_API(OSMesaMakeCurrent, GLboolean, (OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height))
MESA_API(OSMesaGetCurrentContext, OSMesaContext, (void))
MESA_API(OSMesaDestroyContext, void, (OSMesaContext ctx))
MESA_API(OSMesaGetDepthBuffer, GLboolean, (OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer))
MESA_API(OSMesaGetColorBuffer, GLboolean, (OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer))
MESA_API(OSMesaPixelStore, void, (GLint pname, GLint value))
MESA_API(glGenTextures, void, (GLsizei n,	GLuint *textures))
MESA_API(glDeleteTextures, void, (GLsizei n, const GLuint *textures))
MESA_API(glPixelStorei, void, (GLenum pname, GLint param))
MESA_API(glTexImage2D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *data))
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
MESA_API(glVertex4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w))
MESA_API(glEnable, void, (GLenum cap))
MESA_API(glDisable, void, (GLenum cap))
MESA_API(glFinish, void, (void))
MESA_API(glClear, void, (GLbitfield mask))
MESA_API(glClearColor, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
MESA_API(glClearDepth, void, (GLdouble depth))
MESA_API(glBindTexture, void, (GLenum target, GLuint texture))
MESA_API(glActiveTexture, void, (GLenum texture))
MESA_API(glFrontFace, void, (GLenum mode))
MESA_API(glPolygonMode, void, (GLenum face, GLenum mode))
MESA_API(glTexCoord2f, void, (GLfloat s, GLfloat t))
MESA_API(glGetString, const GLubyte *, (GLenum name))
MESA_API(glCullFace, void, (GLenum mode))
MESA_API(glShadeModel, void, (GLenum mode))
MESA_API(glLoadIdentity, void, (void))
MESA_API(glMatrixMode, void, (GLenum mode))
MESA_API(glDepthMask, void, (GLboolean flag))
MESA_API(glGenFramebuffers, void, (GLsizei n, GLuint *ids))
MESA_API(glBindFramebuffer, void, (GLenum target, GLuint framebuffer))
MESA_API(glFramebufferTexture2D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
MESA_API(glBlitFramebuffer, void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))
MESA_API(glDepthFunc, void, (GLenum func))
MESA_API(glTexParameteri, void, (GLenum target, GLenum pname, GLint param))
MESA_API(glTexParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params))
MESA_API(glBlendFunc, void, (GLenum sfactor, GLenum dfactor))
MESA_API(glCheckFramebufferStatus, GLenum, (GLenum target))
MESA_API(glRenderbufferStorage, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height))
MESA_API(glGetFloatv, void, (GLenum pname, GLfloat *data))
MESA_API(glGetIntegerv, void, (GLenum pname, GLint *data))
MESA_API(glAlphaFunc, void, (GLenum func, GLclampf ref))
MESA_API(glTexEnvi, void, (GLenum target, GLenum pname, GLint param))
MESA_API(glSecondaryColor3ubEXT, void, (GLubyte red, GLubyte green, GLubyte blue))
MESA_API(glFogi, void, (GLenum pname, GLint param))
MESA_API(glFogf, void, (GLenum pname, GLfloat param))
MESA_API(glFogfv, void, (GLenum pname, const GLfloat * params))
MESA_API(glPolygonStipple, void, (const GLubyte * pattern))
