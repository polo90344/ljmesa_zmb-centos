#include <fcntl.h>
#include <libsync.h>
#include <xf86drm.h>
#include "util/os_file.h"
#include <util/u_memory.h>
#include <util/u_inlines.h>
#include "util/os_time.h"
#include "drm-uapi/ljmgpu_drm.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_fence.h"
#include "ljmgpu_job.h"
#include "ljmgpu_log.h"
struct pipe_fence_handle {
    struct pipe_reference reference;
    int fd;
    uint32_t syncobj;
    bool signaled;
};

static void
ljmgpu_create_fence_fd(struct pipe_context *pctx,
                     struct pipe_fence_handle **fence,
                     int fd, enum pipe_fd_type type)
{
    assert(type == PIPE_FD_TYPE_NATIVE_SYNC);
    *fence = ljmgpu_fence_create(os_dupfd_cloexec(fd));
}

static void
ljmgpu_fence_server_sync(struct pipe_context *pctx,
                       struct pipe_fence_handle *fence)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    LOGD("gallium ljmgpu_fence_server_sync fence->fd[%d]\n", fence->fd);
    sync_accumulate("ljmgpu", &ctx->in_sync_fd, fence->fd);
    LOGD("gallium ljmgpu_fence_server_sync ctx->in_sync_fd[%d]\n", ctx->in_sync_fd);
}
static void
ljmgpu_fence_server_signal(struct pipe_context *pctx,
                       struct pipe_fence_handle *fence)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);

    LOGD("gallium ljmgpu_fence_server_signal fence->fd[%d]\n", fence->fd);
    if (fence->syncobj) {
      drmSyncobjSignal(screen->fd, &fence->syncobj, 1);
    }

    LOGD("gallium ljmgpu_fence_server_signal fence->syncobj[%d]\n", fence->syncobj);
}
void ljmgpu_fence_context_init(struct ljmgpu_context *ctx)
{
    ctx->base.create_fence_fd = ljmgpu_create_fence_fd;
    ctx->base.fence_server_sync = ljmgpu_fence_server_sync;
    ctx->base.fence_server_sync = ljmgpu_fence_server_signal;
}

struct pipe_fence_handle *
ljmgpu_fence_create(int fd)
{
    struct pipe_fence_handle *fence;

    fence = CALLOC_STRUCT(pipe_fence_handle);
    if (!fence)
        return NULL;

    pipe_reference_init(&fence->reference, 1);
    fence->fd = fd;

    return fence;
}
struct pipe_fence_handle *
ljmgpu_fence_create_pan(struct ljmgpu_context *ctx)
{
    int fd = -1, ret;
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    struct pipe_fence_handle *f = calloc(1, sizeof(*f));
    if (!f)
        return NULL;

    LOGI("gallium ljmgpu_fence_create_pan ctx->syncobj[%d]\n", ctx->syncobj);
    ret = drmSyncobjExportSyncFile(screen->fd, ctx->syncobj, &fd);
    if (ret || fd == -1) {
        LOGE("gallium ljmgpu_fence_create_pan drmSyncobjExportSyncFile error\n");
        return NULL;
    }
    LOGI("gallium ljmgpu_fence_create_pan fd[%d]\n", fd);
    ret = drmSyncobjCreate(screen->fd, 0, &f->syncobj);
    if (ret) {
        LOGE("gallium ljmgpu_fence_create_pan drmSyncobjCreate error\n");
        return NULL;
    }

    ret = drmSyncobjImportSyncFile(screen->fd, f->syncobj, fd);
    if (ret) {
            LOGE("gallium ljmgpu_fence_create_pan drmSyncobjImportSyncFile error\n");
            return NULL;
    }
    LOGI("gallium ljmgpu_fence_create_pan 2 f->syncobj[%d],fd[%d]\n", f->syncobj, fd);
    close(fd);
    pipe_reference_init(&f->reference, 1);
    LOGI("gallium ljmgpu_fence_create_pan over\n");
    return f;
}
static int
ljmgpu_fence_get_fd(struct pipe_screen *pscreen,
                  struct pipe_fence_handle *fence)
{
    return os_dupfd_cloexec(fence->fd);
}

static void
ljmgpu_fence_destroy(struct pipe_fence_handle *fence)
{
    if (fence->fd >= 0)
        close(fence->fd);
    FREE(fence);
}

static void
ljmgpu_fence_reference(struct pipe_screen *pscreen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
    if (pipe_reference(&(*ptr)->reference, &fence->reference))
        ljmgpu_fence_destroy(*ptr);
    *ptr = fence;
}

static bool
ljmgpu_fence_finish(struct pipe_screen *pscreen, struct pipe_context *pctx,
                  struct pipe_fence_handle *fence, uint64_t timeout)
{
    return !sync_wait(fence->fd, timeout / 1000000);
}
static bool
ljmgpu_fence_finish_pan(struct pipe_screen *pscreen,
                      struct pipe_context *pctx,
                      struct pipe_fence_handle *fence,
                      uint64_t timeout)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    int ret;

    if (fence->signaled)
        return true;

    uint64_t abs_timeout = os_time_get_absolute_timeout(timeout);
    if (abs_timeout == OS_TIMEOUT_INFINITE)
        abs_timeout = INT64_MAX;

    ret = drmSyncobjWait(screen->fd, &fence->syncobj,
                        1,
                        abs_timeout, DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL,
                        NULL);

        fence->signaled = (ret >= 0);
        return fence->signaled;
}

void
ljmgpu_fence_screen_init(struct ljmgpu_screen *screen)
{
   screen->base.fence_reference = ljmgpu_fence_reference;
   //screen->base.fence_finish = ljmgpu_fence_finish;
   screen->base.fence_finish = ljmgpu_fence_finish_pan;
   screen->base.fence_get_fd = ljmgpu_fence_get_fd;
}
