#ifndef H_LJMGPU_JOB
#define H_LJMGPU_JOB

#include <stdbool.h>
#include <stdint.h>

#include <util/u_dynarray.h>

#include <pipe/p_state.h>

#define MAX_DRAWS_PER_JOB 2500

struct ljmgpu_context;
struct ljmgpu_bo;
struct ljmgpu_dump;
struct pipe_surface;

struct ljmgpu_job_key {
    struct pipe_surface *cbuf;
    struct pipe_surface *zsbuf;
};

struct ljmgpu_job_clear {
    unsigned buffers;
    uint32_t color_8pc;
    uint32_t depth;
    uint32_t stencil;
    uint64_t color_16pc;
    uint64_t color_base_addr;
    uint64_t depth_base_addr;
    uint32_t clear_enable;
    uint32_t flush_enable;
    uint64_t enable;
};

struct ljmgpu_job_fb_info {
    int width, height;
    int tiled_w, tiled_h;
    int shift_w, shift_h;
    int block_w, block_h;
    int shift_min;
};

struct ljmgpu_job {
    int fd;
    struct ljmgpu_context *ctx;

    struct util_dynarray gem_bos[2];
    struct util_dynarray bos[2];

    struct ljmgpu_job_key key;

    struct util_dynarray vs_cmd_array;
    struct util_dynarray plbu_cmd_array;
    struct util_dynarray plbu_cmd_head;
    struct util_dynarray clear_data_pack;
    struct util_dynarray clear_cmd_pack;
    unsigned resolve;

    int pp_max_stack_size;

    struct pipe_scissor_state damage_rect;

    struct ljmgpu_job_clear clear;

    struct ljmgpu_job_fb_info fb;

    int draws;

    /* for dump command stream */
    struct ljmgpu_dump *dump;
};
/* buffer information used by one task */
struct drm_ljmgpu_gem_submit_bo {
	__u32 handle;  /* in, GEM buffer handle */
	__u32 flags;   /* in, buffer read/write by GPU */
};
static inline bool
ljmgpu_job_has_draw_pending(struct ljmgpu_job *job)
{
   return !!job->plbu_cmd_array.size;
}

struct ljmgpu_job *ljmgpu_job_get(struct ljmgpu_context *ctx);

bool ljmgpu_job_add_bo(struct ljmgpu_job *job, int pipe,
                     struct ljmgpu_bo *bo, uint32_t flags);
void *ljmgpu_job_create_stream_bo(struct ljmgpu_job *job, int pipe,
                                unsigned size, uint32_t *va);

void ljmgpu_do_job_draw(struct ljmgpu_job *job);
void ljmgpu_do_job_clear(struct ljmgpu_job *job);
void ljmgpu_do_job_flush(struct ljmgpu_job *job);
bool ljmgpu_job_start(struct ljmgpu_job *job, void *data, uint32_t size);
bool ljmgpu_job_init(struct ljmgpu_context *ctx);
void ljmgpu_job_fini(struct ljmgpu_context *ctx);

#endif

