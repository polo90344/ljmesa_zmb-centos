#ifndef H_LJMGPU_GPU
#define H_LJMGPU_GPU

#include <stdint.h>
#include <assert.h>
#include "ljmgpu_context.h"
#include <util/u_dynarray.h>

void* ljmgpu_vbo_add_head_tail_checksum(void* data, int data_cnt, unsigned head, unsigned tail);
void ljmgpu_cmd_add_head_tail_checksums(struct util_dynarray* buf, unsigned head, unsigned tail, struct util_dynarray* packet);
#endif
