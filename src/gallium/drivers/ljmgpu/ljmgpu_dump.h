#ifndef LJMGPU_DUMP_H
#define LJMGPU_DUMP_H
#include <stdio.h>
#define DRIVER_TXT_COMMAND_FILE "driver_command.txt"
#define DRIVER_TXT_VBO_FILE "driver_vbo.txt"
void ljmgpu_cmd_dump(void *cmd, int cnt);
void ljmgpu_vbo_dump(void *data, size_t size, unsigned long long phy_addr);

#endif
