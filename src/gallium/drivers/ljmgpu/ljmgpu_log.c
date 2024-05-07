/*************************************************************************
	> File Name: ljmgpu_log.c
	> Author: liukui
	> Mail: 429688325@qq.com 
	> Created Time: 2023年12月25日 星期一 10时46分56秒
 ************************************************************************/

#include<stdio.h>
#include "ljmgpu_log.h"
char * gettime(void)
{
    struct timeval tv;
    struct timezone tz;
    struct tm *t;
    char * str[32];

    gettimeofday(&tv, &tz);
    /*printf("tv_sec:%ld\n",tv.tv_sec);
    printf("tv_usec:%ld\n",tv.tv_usec);
    printf("tz_minuteswest:%d\n",tz.tz_minuteswest);
    printf("tz_dsttime:%d\n",tz.tz_dsttime);
    */

    t = localtime(&tv.tv_sec);
    printf("%d-%d-%d %d:%d:%d.%ld\n", 1900+t->tm_year, 1+t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);
    return NULL;
}


