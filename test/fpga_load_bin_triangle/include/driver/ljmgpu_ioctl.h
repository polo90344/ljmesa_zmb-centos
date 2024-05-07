#ifndef _LJMGPU_IOCTL_H_
#define _LJMGPU_IOCTL_H_

enum ljmgpu_ioctl_user_cmd {
        IOCTL_CMD_BASE = 0x0,
        IOCTL_GET_DEVINFO,
        IOCTL_GET_PHYADDR,
        IOCTL_SUBMIT_COMMAND,
        IOCTL_SUBMIT_VBO,
        IOCTL_WAIT_COMMAND,
        IOCTL_WAIT_VBO,
        IOCTL_CREATE_CTX,
        IOCTL_FREE_CTX,
        IOCTL_CMD_END,
};

struct ljmgpu_devinfo {
        unsigned short vendor_id;
        unsigned short device_id;
};

struct ljmgpu_commands {
        unsigned int command_count;
        unsigned int command[512];
};

struct ljmgpu_addr {
        unsigned long vaddr;
        unsigned long paddr;
};

#define LJMGPU_IOCTL_BASE 'l'

#define LJMGPU_IOCTL_GET_DEVINFO \
        _IOR(LJMGPU_IOCTL_BASE, IOCTL_GET_DEVINFO, struct ljmgpu_devinfo)
#define LJMGPU_IOCTL_SUBMIT_COMMAND \
        _IOW(LJMGPU_IOCTL_BASE, IOCTL_SUBMIT_COMMAND, struct ljmgpu_commands)
#define LJMGPU_IOCTL_GET_PHYADDR \
        _IOWR(LJMGPU_IOCTL_BASE, IOCTL_GET_PHYADDR, struct ljmgpu_addr)

#endif

