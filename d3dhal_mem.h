#ifndef __VMHAL9X__D3DHAL_MEM_H__INCLUDED__
#define __VMHAL9X__D3DHAL_MEM_H__INCLUDED__

void *hal3d_malloc(size_t size);
void *hal3d_calloc(size_t size);
void hal3d_free(void **ptr);
BOOL hal3d_realloc(void **mem, size_t newsize);


#endif /* __VMHAL9X__D3DHAL_MEM_H__INCLUDED__ */
