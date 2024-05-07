#ifndef _LJMGPU_IOCTL_H_
#define _LJMGPU_IOCTL_H_

#include <linux/types.h>
#include <drm/drm.h>

enum drm_ioctl_user_cmd {
	DRM_LJMGPU_FENCE_ATTACH = 0x1,
	DRM_LJMGPU_FENCE_SIGNAL,
	DRM_LJMGPU_GET_DEVINFO,
	DRM_LJMGPU_SUBMIT,
	DRM_LJMGPU_CMDS_FLUSHED,
	DRM_LJMGPU_CREATE_BO,
	DRM_LJMGPU_MAP_BO,
	DRM_LJMGPU_GET_BO_OFFSET,
	DRM_LJMGPU_GET_BO_PHYADDR,
};

enum ljmgpu_ioctl_user_cmd {
	IOCTL_CMD_BASE = 0x0,
	IOCTL_GET_DEVINFO,
	IOCTL_GET_PHYADDR,
	IOCTL_SUBMIT_CMD,
	IOCTL_CMDS_FLUSHED,
	IOCTL_CMD_END,
};

#define DRM_IOCTL_LJMGPU_FENCE_ATTACH                        \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_FENCE_ATTACH, \
		 struct ljmgpu_fence_attach)
#define DRM_IOCTL_LJMGPU_FENCE_SIGNAL                       \
	DRM_IOW(DRM_COMMAND_BASE + DRM_LJMGPU_FENCE_SIGNAL, \
		struct ljmgpu_fence_signal)
#define DRM_IOCTL_LJMGPU_GET_DEVINFO                       \
	DRM_IOR(DRM_COMMAND_BASE + DRM_LJMGPU_GET_DEVINFO, \
		struct ljmgpu_devinfo)
#define DRM_IOCTL_LJMGPU_SUBMIT \
	DRM_IOW(DRM_COMMAND_BASE + DRM_LJMGPU_SUBMIT, struct ljmgpu_cmds)
#define DRM_IOCTL_LJMGPU_CMDS_FLUSHED                       \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_CMDS_FLUSHED, \
		struct ljmgpu_cmds_flushed)
#define DRM_IOCTL_LJMGPU_CREATE_BO                        \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_CREATE_BO, \
		 struct ljmgpu_create_bo)
#define DRM_IOCTL_LJMGPU_MAP_BO \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_MAP_BO, struct ljmgpu_map_bo)
#define DRM_IOCTL_LJMGPU_GET_BO_OFFSET                        \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_GET_BO_OFFSET, \
		 struct ljmgpu_get_bo_offset)
#define DRM_IOCTL_LJMGPU_GET_BO_PHYADDR                        \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LJMGPU_GET_BO_PHYADDR, \
		 struct ljmgpu_get_bo_phyaddr)

struct ljmgpu_fence_attach {
	__u32 handle;
	__u32 flags;
#define LJMGPU_FENCE_WRITE 0x1
	__u32 out_fence;
	__u32 pad;
};

struct ljmgpu_fence_signal {
	__u32 fence;
	__u32 flags;
};

struct ljmgpu_create_bo {
	__u32 size;
	__u32 handle;
};

struct ljmgpu_map_bo {
	__u32 handle;
	__u32 pad;
	__u64 offset;
};

struct ljmgpu_get_bo_offset {
	__u32 handle;
	__u32 pad;
	__u64 offset;
};

struct ljmgpu_get_bo_phyaddr {
	__u32 handle;
	__u32 pad;
	__u64 phyaddr;
};

struct ljmgpu_devinfo {
	__u16 vendor_id;
	__u16 device_id;
};

struct ljmgpu_cmds {
	__u32 cmd_cnt;
	__u32 cmd[512];
	__u32 in_sync;
	__u32 out_sync;
};

struct ljmgpu_cmds_flushed {
	__u8 complete;
};

struct ljmgpu_addr {
	__u64 vaddr;
	__u64 paddr;
};

#define GPU_IOCTL_BASE 'l'

#define GPU_IOCTL_GET_DEVINFO \
	_IOR(GPU_IOCTL_BASE, IOCTL_GET_DEVINFO, struct ljmgpu_devinfo)
#define GPU_IOCTL_GET_PHYADDR \
	_IOWR(GPU_IOCTL_BASE, IOCTL_GET_PHYADDR, struct ljmgpu_addr)
#define GPU_IOCTL_SUBMIT_CMD \
	_IOW(GPU_IOCTL_BASE, IOCTL_SUBMIT_CMD, struct ljmgpu_cmds)
#define GPU_IOCTL_CMDS_FLUSHED \
	_IOWR(GPU_IOCTL_BASE, IOCTL_CMDS_FLUSHED, struct ljmgpu_cmds_flushed)


#define USE_DRM

#ifdef USE_DRM
#define ljmgpu_render_fd "/dev/dri/renderD128"
#define LJMGPU_IOCTL_GET_DEVINFO DRM_IOCTL_LJMGPU_GET_DEVINFO
#define LJMGPU_IOCTL_GET_PHYADDR NULL
#define LJMGPU_IOCTL_SUBMIT_CMD DRM_IOCTL_LJMGPU_SUBMIT
#define LJMGPU_IOCTL_CMDS_FLUSHED DRM_IOCTL_LJMGPU_CMDS_FLUSHED
#define LJMGPU_IOCTL_CREATE_BO DRM_IOCTL_LJMGPU_CREATE_BO
#define LJMGPU_IOCTL_MAP_BO DRM_IOCTL_LJMGPU_MAP_BO
#define LJMGPU_IOCTL_GET_BO_OFFSET DRM_IOCTL_LJMGPU_GET_BO_OFFSET
#define LJMGPU_IOCTL_GET_BO_PHYADDR DRM_IOCTL_LJMGPU_GET_BO_PHYADDR
#else
#define ljmgpu_render_fd "/dev/ljmgpu"
#define LJMGPU_IOCTL_GET_DEVINFO GPU_IOCTL_GET_DEVINFO
#define LJMGPU_IOCTL_GET_PHYADDR GPU_IOCTL_GET_PHYADDR
#define LJMGPU_IOCTL_SUBMIT_CMD GPU_IOCTL_SUBMIT_CMD
#define LJMGPU_IOCTL_CMDS_FLUSHED GPU_IOCTL_CMDS_FLUSHED
#endif

#endif
