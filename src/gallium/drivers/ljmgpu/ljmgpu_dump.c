#include "ljmgpu_dump.h"
#include "ljmgpu_log.h"

void ljmgpu_cmd_dump(void *cmd, int cnt)
{
    int count = 0;
    FILE *fp = fopen(DRIVER_TXT_COMMAND_FILE, "a+");
    if (fp == NULL) {
        LOGE("gallium ljmgpu_cmd_dump open file error\n");
        return;
    }

    unsigned int *command_ptr = cmd;
    for (int i = 0; i < cnt; i++) {
        fprintf(fp, "%08lx\t", count * sizeof(unsigned int)); // Writing the increasing hexadecimal number
        fprintf(fp, "%08x\n", command_ptr[i]);
        count++;
    }

    fclose(fp);
    LOGI("gallium ljmgpu_cmd_dump over\n");
}


void ljmgpu_vbo_dump(void *data, size_t size, unsigned long long phy_addr)
{
    FILE *file = fopen(DRIVER_TXT_VBO_FILE, "a+");
    if (file == NULL) {
        LOGE("gallium ljmgpu_vbo_dump open file error\n");
        return;
    }

    unsigned int *ptr_data = data;
    for (int i = 0; i < size / sizeof(unsigned int); i++) {
        fprintf(file, "%08llx\t%08x\n", phy_addr + i * sizeof(unsigned int), ptr_data[i]);
    }

    fclose(file);
    LOGI("gallium ljmgpu_vbo_dump over\n");
}

