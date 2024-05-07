#include "util/u_memory.h"
#include "util/u_blitter.h"
#include "util/u_upload_mgr.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/ralloc.h"
#include "util/u_inlines.h"
#include "util/hash_table.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_job.h"
#include "ljmgpu_util.h"
#include "ljmgpu_fence.h"
#include "ljmgpu_log.h"

//#include <drm-uapi/ljmgpu_drm.h>
#include <xf86drm.h>

/*
static int
ljmgpu_context_create_drm_ctx(struct ljmgpu_screen *screen)
{
   struct drm_ljmgpu_ctx_create req = {0};

   int ret = drmIoctl(screen->fd, DRM_IOCTL_LJM_CTX_CREATE, &req);
   if (ret)
      return errno;

   return req.id;
}

static void
ljmgpu_context_free_drm_ctx(struct ljmgpu_screen *screen, int id)
{
   struct drm_ljmgpu_ctx_free req = {
      .id = id,
   };

   drmIoctl(screen->fd, DRM_IOCTL_LJM_CTX_FREE, &req);
}
*/
static void
ljmgpu_context_destroy(struct pipe_context *pctx)
{
    LOGD("ljmgpu debug ljmgpu_context_destroy in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(pctx->screen);

    ljmgpu_program_fini(ctx);
    ljmgpu_state_fini(ctx);

    if (ctx->blitter)
        util_blitter_destroy(ctx->blitter);

    if (ctx->uploader)
        u_upload_destroy(ctx->uploader);

    if (ctx->plb_gp_stream)
        ljmgpu_bo_unreference(ctx->plb_gp_stream);

    if (ctx->gp_output)
        ljmgpu_bo_unreference(ctx->gp_output);

    ralloc_free(ctx);
    LOGD("ljmgpu debug ljmgpu_context_destroy end\n");
}

static void
ljmgpu_set_debug_callback(struct pipe_context *pctx,
                        const struct pipe_debug_callback *cb)
{
    LOGD("ljmgpu debug ljm_set_debug_callback in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    if (cb)
        ctx->debug = *cb;
    else
        memset(&ctx->debug, 0, sizeof(ctx->debug));
    LOGD("ljmgpu debug ljm_set_debug_callback end\n");
}
static void
ljmgpu_invalidate_resource(struct pipe_context *pctx, struct pipe_resource *prsc)
{

    LOGD("ljmgpu debug ljm_invalidate_resource in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    LOGD("ljmgpu debug ljm_invalidate_resource end\n");
}

struct pipe_context *
ljmgpu_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
{
    LOGD("ljmgpu debug ljmgpu_context_create in\n");
    int ret = -1;
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    struct ljmgpu_context *ctx;
    int sync_fd;

    ctx = rzalloc(screen, struct ljmgpu_context);
    if (!ctx)
        return NULL;

    ctx->id = 1;
    ctx->base.screen = pscreen;
    ctx->base.destroy = ljmgpu_context_destroy;
    ctx->base.set_debug_callback = ljmgpu_set_debug_callback;
    ctx->base.invalidate_resource = ljmgpu_invalidate_resource;

    ljmgpu_resource_context_init(ctx);
    ljmgpu_fence_context_init(ctx);
    ljmgpu_state_init(ctx);
    ljmgpu_draw_init(ctx);
    ljmgpu_program_init(ctx);
    ljmgpu_query_init(ctx);

    slab_create_child(&ctx->transfer_pool, &screen->transfer_pool);

    ctx->blitter = util_blitter_create(&ctx->base);
    if (!ctx->blitter) {
        LOGE("ljmgpu debug util_blitter_create error\n");
        goto err_out;
    }


    ctx->uploader = u_upload_create_default(&ctx->base);

    ljmgpu_job_init(ctx);
    int size = 1024 * 1024;
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, size, 0);
    char buffer[128] = "ljmgpu context bo create inside test";
    memcpy((char *)bo->map, buffer, 50);
    LOGD("ljmgpu debug bo->map[%s]\n", (char *)bo->map);
    

    LOGD("ljmgpu debug ljmgpu_context_create screen->fd[%d]\n", screen->fd);
    ret = drmSyncobjCreate(screen->fd, DRM_SYNCOBJ_CREATE_SIGNALED, &ctx->syncobj);
    if(ret) {
        LOGE("ljmgpu debug drmSyncobjCreate error\n");
	    return NULL;
    }
    LOGD("gallium ljmgpu_context_create ctx->syncobj[%d]\n", ctx->syncobj);
#if 0
    printf("ljmgpu debug ctx->syncobj[%d]\n", ctx->syncobj);
    ret = drmSyncobjExportSyncFile(screen->fd, ctx->syncobj, &sync_fd);
    if(ret) {
        printf("ljmgpu debug drmSyncobjExportSyncFile error\n");
	return NULL;
    }
    printf("ljmgpu debug export sync_fd[%d]\n", sync_fd);

    ret = drmSyncobjImportSyncFile(screen->fd, ctx->syncobj, sync_fd);
    if(ret) {
        printf("ljmgpu debug drmSyncobjExportSyncFile error\n");
	return NULL;
    }
    printf("ljmgpu debug import sync_fd[%d]\n", sync_fd);
    printf("ljmgpu debug ljmgpu_context_create &ctx->base[%p]\n", &ctx->base);
#endif
    LOGD("ljmgpu debug ljmgpu_context_create over\n");
    return &ctx->base;

err_out:
   ljmgpu_context_destroy(&ctx->base);
   return NULL;
}
