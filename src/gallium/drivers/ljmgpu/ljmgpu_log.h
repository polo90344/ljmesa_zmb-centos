#ifndef LJMGPU_LOG_H
#define LJMGPU_LOG_H
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

char * gettime(void);
#define LOGD(...) {printf("[ljmgpu][D][%s][%d] ", __func__, __LINE__); printf(__VA_ARGS__);}
#define LOGI(...) {printf("[ljmgpu][I][%s][%d] ", __func__, __LINE__); printf(__VA_ARGS__);}
#define LOGE(...) {printf("[ljmgpu][E][%s][%s][%d] ",  __FILE__, __func__, __LINE__); printf(__VA_ARGS__);}

#endif

