#ifndef __VMHAL9X__D3DHAL_H__INCLUDED__
#define __VMHAL9X__D3DHAL_H__INCLUDED__

#define HAL3D_USERLIB_NAME "vmhal3d.dll"
#define HAL3D_USERLIB_PREFIX "hal3d_"

#define HAL3D_TMU_CNT 8
#define HAL3D_CLIPS_MAX 8
#define HAL3D_WORLDS_MAX 4

#define HAL3D_MAX_STREAM 16

#define TEX_MAX_DIM_DEF 16384
#define DEF_DDI 9
#define DEF_DDI_TL 9
#define DEF_TEX_UNITS

VMHAL_enviroment_t* __stdcall hal_env();
typedef VMHAL_enviroment_t* (__stdcall *hal_env_t)(void);

typedef struct _hal9x_callbacks_t
{
	hal_env_t env_p;
	ids_proc_t ids;
} hal9x_callbacks_t;

typedef void (__stdcall *hal3d_init_t)(hal9x_callbacks_t *callbacks);

extern ids_proc_t *ids_procs;

#endif /* __VMHAL9X__D3DHAL_H__INCLUDED__ */
