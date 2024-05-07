#include "ljmgpu_context.h"
#if 0
int ljmgpu_dispatch_command(void * cmd, int cnt, int is_clear)
{
    printf("ljmgpu debug driver ljmgpu_dispatch_command cnt[%d]\n", cnt);
    int ret = -1;
    struct ljmgpu_cmds req = {};
	/*if(strlen(cmd) == 0 || cnt == 0) {
	    printf("ljmgpu_dispatch_command data is null\n");
	    return -1;
	}*/

	unsigned int *command_ptr = cmd;
	printf("------driver ioctl submit command %s\n", is_clear ? "clear" : "draw" );
	for(int i = 0; i < cnt; i++)
		printf("%d:0x%08x\n", i, command_ptr[i]);
	req.cmd_cnt = cnt;
	memcpy(req.cmd, cmd, cnt * sizeof(unsigned int));
    ret = ioctl(driver_ctx->render_fd, LJMGPU_IOCTL_SUBMIT_CMD, &req);
    if(ret < 0) {
            LOGE("DRM_IOCTL_LJM_GEM_SUBMIT error[%d]\n", ret);
            return -1;
    } else {
            LOGD("ljmgpu_dispatch_command ioctl DRM_IOCTL_LJM_GEM_SUBMIT success\n");
    }
	return 0;
}
#endif
