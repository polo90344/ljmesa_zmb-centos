#include <stdlib.h>
#include <string.h>

#include "xf86drm.h"
#include "drm-uapi/ljmgpu_drm.h"

#include "util/u_math.h"
#include "util/ralloc.h"
#include "util/os_time.h"
#include "util/hash_table.h"
#include "util/format/u_format.h"
#include "util/u_upload_mgr.h"
#include "util/u_inlines.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_job.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_util.h"
#include "ljmgpu_format.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_texture.h"
#include "ljmgpu_fence.h"
#include "ljmgpu_gpu.h"
#include "ljmgpu_log.h"

#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

static void ljmgpu_get_fb_info(struct ljmgpu_job *job)
{
    struct ljmgpu_context *ctx = job->ctx;
    struct ljmgpu_job_fb_info *fb = &job->fb;

    fb->width = ctx->framebuffer.base.width;
    fb->height = ctx->framebuffer.base.height;

    int width = align(fb->width, 16) >> 4;
    int height = align(fb->height, 16) >> 4;
  
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    fb->tiled_w = width;
    fb->tiled_h = height;
    fb->shift_h = 0;
    fb->shift_w = 0;
/*
    int limit = screen->plb_max_blk;
    while ((width * height) > limit) {
       if (width >= height) {
          width = (width + 1) >> 1;
          fb->shift_w++;
       } else {
          height = (height + 1) >> 1;
          fb->shift_h++;
       }
    }
    */
    fb->block_w = width;
    fb->block_h = height;
    fb->shift_min = MIN3(fb->shift_w, fb->shift_h, 2);

    LOGD("gallium ljm_get_fb_info ctx->framebuffer.base.width[%d]ctx->framebuffer.base.height[%d]\n", ctx->framebuffer.base.width, ctx->framebuffer.base.height);
}

static struct ljmgpu_job *
ljmgpu_job_create(struct ljmgpu_context *ctx)
{
    LOGD("gallium ljmgpu_job_create in\n");
    struct ljmgpu_job *s;
    struct ljmgpu_screen *screen;
    s = rzalloc(ctx, struct ljmgpu_job);
    if (!s) {
        LOGE("ljmgpu error ljmgpu_job_create rzalloc error\n");
        return NULL;
    }

    s->fd = ljmgpu_screen(ctx->base.screen)->fd;
    LOGD("ljmgpu debug ljmgpu_job_create s->fd[%d]\n", s->fd);
    s->ctx = ctx;

    s->damage_rect.minx = s->damage_rect.miny = 0xffff;
    s->damage_rect.maxx = s->damage_rect.maxy = 0;
    s->draws = 0;

    s->clear.depth = 0x00ffffff;

    for (int i = 0; i < 2; i++) {
       util_dynarray_init(s->gem_bos + i, s);
       util_dynarray_init(s->bos + i, s);
    }

    util_dynarray_init(&s->vs_cmd_array, s);
    util_dynarray_init(&s->plbu_cmd_array, s);
    util_dynarray_init(&s->plbu_cmd_head, s);

    struct ljmgpu_context_framebuffer *fb = &ctx->framebuffer;
    pipe_surface_reference(&s->key.cbuf, fb->base.cbufs[0]);
    pipe_surface_reference(&s->key.zsbuf, fb->base.zsbuf);

    ljmgpu_get_fb_info(s);
    LOGD("gallium ljmgpu_job_create over\n");
    return s;
}

static void
ljmgpu_job_free(struct ljmgpu_job *job)
{
    struct ljmgpu_context *ctx = job->ctx;

    _mesa_hash_table_remove_key(ctx->jobs, &job->key);

    if (job->key.cbuf && (job->resolve & PIPE_CLEAR_COLOR0))
        _mesa_hash_table_remove_key(ctx->write_jobs, job->key.cbuf->texture);
    if (job->key.zsbuf && (job->resolve & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)))
        _mesa_hash_table_remove_key(ctx->write_jobs, job->key.zsbuf->texture);

    pipe_surface_reference(&job->key.cbuf, NULL);
    pipe_surface_reference(&job->key.zsbuf, NULL);

   ralloc_free(job);
}

static struct ljmgpu_job *
_ljmgpu_job_get(struct ljmgpu_context *ctx)
{
    LOGD("ljmgpu debug _ljmgpu_job_get in\n");
    struct ljmgpu_context_framebuffer *fb = &ctx->framebuffer;

    struct ljmgpu_job_key local_key = {
        .cbuf = fb->base.cbufs[0],
        .zsbuf = fb->base.zsbuf,
    };

    struct hash_entry *entry = _mesa_hash_table_search(ctx->jobs, &local_key);
    if (entry) {
        LOGI("_mesa_hash_table_search return entry->data\n");
        return entry->data;
    }
    
    struct ljmgpu_job *job = ljmgpu_job_create(ctx);
    if (!job)
        return NULL;

    LOGD("ljmgpu debug _ljmgpu_job_get over\n");
    return job;
}

/*
 * Note: this function can only be called in draw code path,
 * must not exist in flush code path.
 */
struct ljmgpu_job *
ljmgpu_job_get(struct ljmgpu_context *ctx)
{
    LOGD("ljmgpu debug ljmgpu_job_get in\n");
    if (ctx->job)
        return ctx->job;

    ctx->job = _ljmgpu_job_get(ctx);
    LOGD("ljmgpu debug ljmgpu_job_get over\n");
    return ctx->job;
}

bool ljmgpu_job_add_bo(struct ljmgpu_job *job, int pipe,
                     struct ljmgpu_bo *bo, uint32_t flags)
{
    util_dynarray_foreach(job->gem_bos + pipe, struct drm_ljmgpu_gem_submit_bo, gem_bo) {
        if (bo->handle == gem_bo->handle) {
            gem_bo->flags |= flags;
            return true;
        }
   }

    struct drm_ljmgpu_gem_submit_bo *job_bo =
        util_dynarray_grow(job->gem_bos + pipe, struct drm_ljmgpu_gem_submit_bo, 1);
    job_bo->handle = bo->handle;
    job_bo->flags = flags;

    struct ljmgpu_bo **jbo = util_dynarray_grow(job->bos + pipe, struct ljmgpu_bo *, 1);
    *jbo = bo;

    ljmgpu_bo_reference(bo);
    return true;
}

bool ljmgpu_job_start(struct ljmgpu_job *job, void *data, uint32_t size)
{
    int ret = -1;
    struct ljmgpu_context *ctx = job->ctx;
    struct ljmgpu_cmds req = {};
    LOGD("ljmgpu ljmgpu_job_start job->fd[%d]\n", job->fd);
    unsigned int *command_ptr = data;
	printf("------driver ioctl submit command----cnt[%ld]\n", (size / sizeof(unsigned int)));
	for(int i = 0; i < (size / sizeof(unsigned int)); i++)
		printf("%d:0x%08x\n", i, command_ptr[i]);

    req.cmd_cnt = size / sizeof(unsigned int);
	memcpy(req.cmd, data, size);

    LOGD("gallium ljmgpu_job_start ctx->in_sync_fd[%d]\n", ctx->in_sync_fd);
#if 0
    if (ctx->in_sync_fd >= 0) {
        int err = drmSyncobjImportSyncFile(job->fd, &ctx->in_sync,
                                         ctx->in_sync_fd);
      if (err)
         return false;

      //req.in_sync = ctx->in_sync;
      close(ctx->in_sync_fd);
      ctx->in_sync_fd = -1;
   }
#endif
    ret = drmIoctl(job->fd, LJMGPU_IOCTL_SUBMIT_CMD, &req);
    if(ret < 0) {
        LOGE("LJMGPU_IOCTL_SUBMIT_CMD error[%d]\n", ret);
        return -1;
    } else {
        LOGI("ljmgpu_job_start LJMGPU_IOCTL_SUBMIT_CMD success\n");
    }
    
    return ret;
}

static bool
ljmgpu_job_wait(struct ljmgpu_job *job, int pipe, uint64_t timeout_ns)
{

    int64_t abs_timeout = os_time_get_absolute_timeout(timeout_ns);
    if (abs_timeout == OS_TIMEOUT_INFINITE)
        abs_timeout = INT64_MAX;

    struct ljmgpu_context *ctx = job->ctx;
    return !drmSyncobjWait(job->fd, &ctx->out_sync, 1, abs_timeout, 0, NULL);

    return true;
}

static bool
ljmgpu_job_has_bo(struct ljmgpu_job *job, struct ljm_bo *bo, bool all)
{

   return true;
}

void *
ljmgpu_job_create_stream_bo(struct ljmgpu_job *job, int pipe,
                          unsigned size, uint32_t *va)
{

   return NULL;
}

static inline struct ljmgpu_damage_region *
ljmgpu_job_get_damage(struct ljmgpu_job *job)
{
    return NULL;
}

static bool
ljmgpu_fb_cbuf_needs_reload(struct ljmgpu_job *job)
{
   return true;
}

static bool
ljmgpu_fb_zsbuf_needs_reload(struct ljmgpu_job *job)
{
   return true;
}

static void
ljmgpu_pack_reload_plbu_cmd(struct ljmgpu_job *job, struct pipe_surface *psurf)
{
}

static void
ljmgpu_pack_head_plbu_cmd(struct ljmgpu_job *job)
{
}

static void
hilbert_rotate(int n, int *x, int *y, int rx, int ry)
{
}

static void
hilbert_coords(int n, int d, int *x, int *y)
{
}

static int
ljmgpu_get_pp_stream_size(int num_pp, int tiled_w, int tiled_h, uint32_t *off)
{
    return 0;
}

static void
ljmgpu_generate_pp_stream(struct ljmgpu_job *job, int off_x, int off_y,
                        int tiled_w, int tiled_h)
{
}

static void
ljmgpu_free_stale_pp_stream_bo(struct ljmgpu_context *ctx)
{
}

static void
ljmgpu_update_damage_pp_stream(struct ljmgpu_job *job)
{
}

static bool
ljmgpu_damage_fullscreen(struct ljmgpu_job *job)
{
    return true;
}

static void
ljmgpu_update_pp_stream(struct ljmgpu_job *job)
{
}

static void
ljmgpu_update_job_bo(struct ljmgpu_job *job)
{
}

static void
ljmgpu_finish_plbu_cmd(struct util_dynarray *plbu_cmd_array)
{
}

static void
ljmgpu_pack_wb_zsbuf_reg(struct ljmgpu_job *job, uint32_t *wb_reg, int wb_idx)
{
}

static void
ljmgpu_pack_wb_cbuf_reg(struct ljmgpu_job *job, uint32_t *frame_reg,
                      uint32_t *wb_reg, int wb_idx)
{
}

static void
ljmgpu_pack_pp_frame_reg(struct ljmgpu_job *job, uint32_t *frame_reg,
                       uint32_t *wb_reg)
{
}
void ljmgpu_do_job_draw(struct ljmgpu_job *job)
{

}
void ljmgpu_do_job_flush(struct ljmgpu_job *job)
{
    LOGD("gallium ljmgpu_do_job_flush in\n");
    struct ljmgpu_context *ctx = job->ctx;
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    struct ljmgpu_job_fb_info *fb = &job->fb;


    int dword_cnt = 0;
    if (!(job->clear_data_pack.data = (unsigned *)malloc(TMP_MALLOC_SIZE * sizeof(unsigned)))) {
        LOGE("In ljmgpu_eg01_prepare_clear_vbo_data: data is NULL!\n");
        return;
    }
    memset(job->clear_data_pack.data, 0, TMP_MALLOC_SIZE * sizeof(unsigned));
    unsigned* ptr = job->clear_data_pack.data;

    job->clear.enable = 0x00;

    job->clear.color_base_addr = 0x20000000;
    job->clear.enable |= (1 << 1);

    job->clear.depth_base_addr = 0x207e9000;
    job->clear.enable |= (1 << 2);

    job->clear.enable |= (1 << 3);

    job->clear.enable |= (1 << 4);

    job->clear.enable |= (1 << 7);
    job->clear.flush_enable = 1;
    LOGD("gallium ljmgpu_do_job color_base_addr[0x%lx],depth_base_addr[0x%lx], color_16pc[0x%lx],color_8pc[%x]\n", job->clear.color_base_addr, job->clear.depth_base_addr, job->clear.color_16pc, job->clear.color_8pc);
    LOGD("gallium job->fb.width[%d],job->fb.height[%d]\n", job->fb.width, job->fb.height);
    *(ptr++) = job->clear.enable;
    dword_cnt++;
    for (int i = 1; i <= 7; i++) { // reserved_zero occupies i=0 slot
        if (!(job->clear.enable & (1 << i)))
            continue;
        switch (i) {
        case 1:
	        *ptr = job->clear.color_base_addr;
            break;
        case 2:
	        *ptr = job->clear.depth_base_addr;
            break;
        case 3:
            *ptr |= job->fb.width;
            *ptr |= job->fb.height << 16;
            break;
        case 4:
            *ptr |= job->clear.color_8pc;
            break;
        case 5: //depth+s
            *ptr |= job->clear.depth;
            *ptr |= job->clear.stencil << 24;
            break;
        case 6:
	        *ptr = job->clear.clear_enable;
            break;
        case 7:
            *ptr = job->clear.flush_enable;
            break;
        default:
            break;
	    }
        ptr++;
        dword_cnt++;
    }
    LOGD("gallium pre dword_cnt[%d]\n", dword_cnt);
    unsigned *new_data =
        ljmgpu_vbo_add_head_tail_checksum(job->clear_data_pack.data, dword_cnt, COMMAND_CLEAR_HEADER, COMMAND_CLEAR_TAIL);
    if (!new_data) {
	    LOGE("In ljm_eg01_get_clear_data: new_data is NULL!\n");
	    return;
    }

    unsigned* ptr_clear = new_data;
    printf("----------new data----------------\n");
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %p, %08x\n", i, (ptr_clear + i), ptr_clear[i]);

    
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, ((dword_cnt + 4) * sizeof(unsigned)), 0);
    memcpy((unsigned *)bo->map, new_data, ((dword_cnt + 4) * sizeof(unsigned)));

    printf("***************bo vbo data****************map[%p]phy[0x%llx]\n", bo->map, bo->phy_addr);
    unsigned* ptr_clear_bo = (unsigned *)bo->map;
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %p, %08x\n", i, (ptr_clear_bo + i), ptr_clear_bo[i]);
    /***for bo test over***/

    dword_cnt = 2;  // clear command count is a fixed value 2 (enabled, addr_clear)
    if (!(job->clear_cmd_pack.data = (unsigned *)malloc(dword_cnt * sizeof(unsigned)))) {
	    LOGE("In ljm_eg01_get_clear_command_data: failed malloc buf->data!\n");
	    return;
    }
    *((unsigned*)job->clear_cmd_pack.data) = 0x80000000;
    *((unsigned*)job->clear_cmd_pack.data + 1) = bo->phy_addr;
  
    job->clear_cmd_pack.size = dword_cnt * sizeof(unsigned);
    struct util_dynarray buf = {};
    
    buf.data = job->clear_cmd_pack.data;
    buf.size = job->clear_cmd_pack.size;

    struct util_dynarray* packet = NULL;

    if (!(packet = (struct util_dynarray*)malloc((dword_cnt + 4) * sizeof(unsigned)))) {
	    LOGE("In ljm_eg01_get_clear_command_data: failed malloc buf->data!\n");
	    return;
    }
    
    ljmgpu_cmd_add_head_tail_checksums(&buf, COMMAND_CLEAR_HEADER, COMMAND_CLEAR_TAIL, packet);

    ptr_clear = packet->data;
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %08x\n", i, ptr_clear[i]);

    packet->size = (dword_cnt + 4) * sizeof(unsigned);
    ljmgpu_job_start(job, packet->data, packet->size);
    LOGD("gallium ljmgpu_do_job over\n");
}
void ljmgpu_do_job_clear(struct ljmgpu_job *job)
{
    LOGD("gallium ljmgpu_do_job in\n");
    struct ljmgpu_context *ctx = job->ctx;
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    struct ljmgpu_job_fb_info *fb = &job->fb;


    int dword_cnt = 0;
    if (!(job->clear_data_pack.data = (unsigned *)malloc(TMP_MALLOC_SIZE * sizeof(unsigned)))) {
        LOGE("In ljmgpu_eg01_prepare_clear_vbo_data: data is NULL!\n");
        return;
    }
    memset(job->clear_data_pack.data, 0, TMP_MALLOC_SIZE * sizeof(unsigned));
    unsigned* ptr = job->clear_data_pack.data;

    job->clear.enable = 0x00;

    job->clear.color_base_addr = 0x20000000;
    job->clear.enable |= (1 << 1);

    job->clear.depth_base_addr = 0x207e9000;
    job->clear.enable |= (1 << 2);

    job->clear.enable |= (1 << 3);

    job->clear.enable |= (1 << 4);

    job->clear.enable |= (1 << 6);
    job->clear.clear_enable = 1;
    LOGD("gallium ljmgpu_do_job color_base_addr[0x%lx],depth_base_addr[0x%lx],color_16pc[0x%lx],color_8pc[%x]\n", job->clear.color_base_addr, job->clear.depth_base_addr, job->clear.color_16pc, job->clear.color_8pc);
    LOGD("gallium job->fb.width[%d],job->fb.height[%d]\n", job->fb.width, job->fb.height);
    *(ptr++) = job->clear.enable;
    dword_cnt++;
    for (int i = 1; i <= 7; i++) { // reserved_zero occupies i=0 slot
        if (!(job->clear.enable & (1 << i)))
            continue;
        switch (i) {
        case 1:
	        *ptr = job->clear.color_base_addr;
            break;
        case 2:
	        *ptr = job->clear.depth_base_addr;
            break;
        case 3:
            *ptr |= job->fb.width;
            *ptr |= job->fb.height << 16;
            break;
        case 4:
            *ptr |= job->clear.color_8pc;
            break;
        case 5: //depth+s
            *ptr |= job->clear.depth;
            *ptr |= job->clear.stencil << 24;
            break;
        case 6:
	        *ptr = job->clear.clear_enable;
            break;
        case 7:
            *ptr = job->clear.flush_enable;
            break;
        default:
            break;
	    }
        ptr++;
        dword_cnt++;
    }
    LOGD("gallium pre dword_cnt[%d]\n", dword_cnt);
    unsigned *new_data =
        ljmgpu_vbo_add_head_tail_checksum(job->clear_data_pack.data, dword_cnt, COMMAND_CLEAR_HEADER, COMMAND_CLEAR_TAIL);
    if (!new_data) {
	    LOGE("In ljm_eg01_get_clear_data: new_data is NULL!\n");
	    return;
    }

    unsigned* ptr_clear = new_data;
    printf("----------new data----------------\n");
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %p, %08x\n", i, (ptr_clear + i), ptr_clear[i]);

    
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, ((dword_cnt + 4) * sizeof(unsigned)), 0);

    memcpy((unsigned *)bo->map, new_data, ((dword_cnt + 4) * sizeof(unsigned)));

    printf("***************bo vbo data****************map[%p]phy[0x%llx]\n", bo->map, bo->phy_addr);
    unsigned* ptr_clear_bo = (unsigned *)bo->map;
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %p, %08x\n", i, (ptr_clear_bo + i), ptr_clear_bo[i]);


    dword_cnt = 2;  // clear command count is a fixed value 2 (enabled, addr_clear)
    if (!(job->clear_cmd_pack.data = (unsigned *)malloc(dword_cnt * sizeof(unsigned)))) {
	    LOGE("In ljm_eg01_get_clear_command_data: failed malloc buf->data!\n");
	    return;
    }
    *((unsigned*)job->clear_cmd_pack.data) = 0x80000000;
    *((unsigned*)job->clear_cmd_pack.data + 1) = bo->phy_addr;
  
    job->clear_cmd_pack.size = dword_cnt * sizeof(unsigned);
    struct util_dynarray buf = {};
    
    buf.data = job->clear_cmd_pack.data;
    buf.size = job->clear_cmd_pack.size;

    struct util_dynarray* packet = NULL;

    if (!(packet = (struct util_dynarray*)malloc((dword_cnt + 4) * sizeof(unsigned)))) {
	    LOGE("In ljm_eg01_get_clear_command_data: failed malloc buf->data!\n");
	    return;
    }
    
    ljmgpu_cmd_add_head_tail_checksums(&buf, COMMAND_CLEAR_HEADER, COMMAND_CLEAR_TAIL, packet);

    ptr_clear = packet->data;
    for(int i = 0; i < dword_cnt + 4; i++)
                printf("%d: %08x\n", i, ptr_clear[i]);

    packet->size = (dword_cnt + 4) * sizeof(unsigned);
    ljmgpu_job_start(job, packet->data, packet->size);
    LOGD("gallium ljmgpu_do_job over\n");
}

void
ljmgpu_flush(struct ljmgpu_context *ctx)
{
    LOGD("gallium ljmgpu_flush in\n");
    //ljmgpu_do_job_flush(ctx->job);
    LOGD("gallium ljmgpu_flush over\n");
}

void
ljmgpu_flush_job_accessing_bo(
   struct ljmgpu_context *ctx, struct ljmgpu_bo *bo, bool write)
{
}

/*
 * This is for current job flush previous job which write to the resource it wants
 * to read. Tipical usage is flush the FBO which is used as current task's texture.
 */
void
ljmgpu_flush_previous_job_writing_resource(
   struct ljmgpu_context *ctx, struct pipe_resource *prsc)
{
}

static void
ljmgpu_pipe_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
                unsigned flags)
{
    LOGD("gallium ljmgpu_pipe_flush in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    LOGI("gallium ljmgpu_pipe_flush fd[%d]\n", ljmgpu_screen(ctx->base.screen)->fd);
    //LOGI("gallium ljmgpu_pipe_flush *fence->fd[%d]\n", (*fence)->fd);
    //ljmgpu_flush(ctx);
    //ljmgpu_fence_create_pan(ctx);
    ljmgpu_do_job_flush(ctx->job);
    if (fence) {
        struct pipe_fence_handle *f = ljmgpu_fence_create_pan(ctx);
            pctx->screen->fence_reference(pctx->screen, fence, NULL);
        *fence = f;
        LOGI("gallium ljmgpu_pipe_flush ljmgpu_fence_create_pan in\n");
    }
   LOGD("gallium ljmgpu_pipe_flush over\n");
}

static bool
ljmgpu_job_compare(const void *s1, const void *s2)
{
    return memcmp(s1, s2, sizeof(struct ljmgpu_job_key)) == 0;
}

static uint32_t
ljmgpu_job_hash(const void *key)
{
    return _mesa_hash_data(key, sizeof(struct ljmgpu_job_key));
}

bool ljmgpu_job_init(struct ljmgpu_context *ctx)
{
    LOGI("gallium ljmgpu_job_init in\n");
    
    int fd = ljmgpu_screen(ctx->base.screen)->fd;

    ctx->jobs = _mesa_hash_table_create(ctx, ljmgpu_job_hash, ljmgpu_job_compare);
    if (!ctx->jobs)
        return false;

    ctx->write_jobs = _mesa_hash_table_create(
        ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
    if (!ctx->write_jobs)
        return false;

    ctx->in_sync_fd = -1;

    drmSyncobjCreate(fd, DRM_SYNCOBJ_CREATE_SIGNALED, &ctx->in_sync);
    drmSyncobjCreate(fd, DRM_SYNCOBJ_CREATE_SIGNALED, &ctx->out_sync);
    LOGD("gallium ljmgpu_job_init ctx->in_sync[%d],ctx->out_sync[%d]\n", ctx->in_sync, ctx->out_sync);
    ctx->base.flush = ljmgpu_pipe_flush;
    LOGD("gallium ljmgpu_job_init over\n");
    return true;
}
void ljmgpu_job_fini(struct ljmgpu_context *ctx)
{
    int fd = ljmgpu_screen(ctx->base.screen)->fd;

    ljmgpu_flush(ctx);

    if (ctx->in_sync)
        drmSyncobjDestroy(fd, ctx->in_sync);
    if (ctx->out_sync)
        drmSyncobjDestroy(fd, ctx->out_sync);

    if (ctx->in_sync_fd >= 0)
        close(ctx->in_sync_fd);
}

