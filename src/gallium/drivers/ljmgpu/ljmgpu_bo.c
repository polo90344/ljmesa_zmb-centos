#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "xf86drm.h"
#include "drm-uapi/ljmgpu_drm.h"

#include "util/u_hash_table.h"
#include "util/u_math.h"
#include "util/os_time.h"
#include "os/os_mman.h"

#include "frontend/drm_driver.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_util.h"
#include "ljmgpu_log.h"

bool ljmgpu_bo_table_init(struct ljmgpu_screen *screen)
{
    return true;

}

bool ljmgpu_bo_cache_init(struct ljmgpu_screen *screen)
{

    return true;
}

void ljmgpu_bo_table_fini(struct ljmgpu_screen *screen)
{
}

static void
ljmgpu_bo_cache_remove(struct ljmgpu_bo *bo)
{
    list_del(&bo->size_list);
    list_del(&bo->time_list);
}

static void ljmgpu_close_kms_handle(struct ljmgpu_screen *screen, uint32_t handle)
{
    struct drm_gem_close args = {
        .handle = handle,
    };

    drmIoctl(screen->fd, DRM_IOCTL_GEM_CLOSE, &args);
}

static void
ljmgpu_bo_free(struct ljmgpu_bo *bo)
{
    struct ljmgpu_screen *screen = bo->screen;

    if (bo->map)
        ljmgpu_bo_unmap(bo);

    ljmgpu_close_kms_handle(screen, bo->handle);
    free(bo);
}

void ljmgpu_bo_cache_fini(struct ljmgpu_screen *screen)
{
}

static unsigned
ljmgpu_bucket_index(unsigned size)
{
    return 0;
}

static struct list_head *
ljmgpu_bo_cache_get_bucket(struct ljmgpu_screen *screen, unsigned size)
{
    return NULL;
}

static void
ljmgpu_bo_cache_free_stale_bos(struct ljmgpu_screen *screen, time_t time)
{
}

static void
ljmgpu_bo_cache_print_stats(struct ljmgpu_screen *screen)
{
}

static bool
ljmgpu_bo_cache_put(struct ljm_bo *bo)
{
    return true;
}

static struct ljmgpu_bo *
ljmgpu_bo_cache_get(struct ljmgpu_screen *screen, uint32_t size, uint32_t flags)
{
    return NULL;
}

struct ljmgpu_bo *ljmgpu_bo_create(struct ljmgpu_screen *screen, uint32_t size, uint32_t flags)
{
    struct ljmgpu_bo *bo;

    size = align(size, EGP_PAGE_SIZE);

    struct ljmgpu_create_bo req = {
        .size = size,
    };

    if (!(bo = calloc(1, sizeof(*bo))))
        return NULL;

    //list_inithead(&bo->time_list);
    //list_inithead(&bo->size_list);

    if (drmIoctl(screen->fd, LJMGPU_IOCTL_CREATE_BO, &req)) {
        LOGE("ioctl create bo error\n");
        goto err_out0;
    }

    bo->screen = screen;
    bo->size = req.size;

    bo->handle = req.handle;

    p_atomic_set(&bo->refcnt, 1);

    //fprintf(stderr, "%s: %p (size=%d)\n", __func__, bo, bo->size);
    LOGI("galllium ljmgpu_bo_create bo[%p],bo->size[%d]\n", bo, bo->size);

    return bo;

err_out1:
    ljmgpu_close_kms_handle(screen, bo->handle);
err_out0:
    free(bo);
    return NULL;
}

void ljmgpu_bo_unreference(struct ljmgpu_bo *bo)
{
    if (!p_atomic_dec_zero(&bo->refcnt))
        return;

    ljmgpu_bo_free(bo);
}

void *ljmgpu_bo_map(struct ljmgpu_bo *bo)
{
    int ret = -1;
#if EGP_TEST
    struct ljmgpu_map_bo map_bo = {};
    struct ljmgpu_get_bo_offset bo_offset = {};

    /* map bo */
    map_bo.handle = bo->handle;
    ret = drmIoctl(bo->screen->fd, LJMGPU_IOCTL_MAP_BO, &map_bo);
    if (ret) {
        LOGE("DRM_IOCTL_PRIME_MAP_BO failed, ret = %d\n", ret);
        return NULL;
    }
    LOGI("gallium map_bo.handle = %u, map_bo.offset = 0x%llx\n", map_bo.handle, map_bo.offset);

    bo->offset = map_bo.offset;
    bo->phy_addr = map_bo.offset - 0x100000000;
#endif

    if (!bo->map) {
        bo->map = os_mmap(0, bo->size, PROT_READ | PROT_WRITE,
                MAP_SHARED, bo->screen->fd, bo->offset);
        if (bo->map == MAP_FAILED)
            bo->map = NULL;
    }
    LOGI("gallium bo.handle[%d], bo->size[%d], bo->offset[0x%lx], bo->phy_addr[0x%llx], bo->map[%p]\n",
        bo->handle, bo->size, bo->offset, bo->phy_addr, bo->map);
    return bo->map;
}

void ljmgpu_bo_unmap(struct ljmgpu_bo *bo)
{
    if (bo->map) {
        os_munmap(bo->map, bo->size);
        bo->map = NULL;
    }
}

bool ljmgpu_bo_export(struct ljmgpu_bo *bo, struct winsys_handle *handle)
{
    struct ljmgpu_screen *screen = bo->screen;

    if (drmPrimeHandleToFD(screen->fd, bo->handle, DRM_CLOEXEC,
                             (int*)&handle->handle))
         return false;
    return true;
}

struct ljmgpu_bo *ljmgpu_bo_import(struct ljmgpu_screen *screen,
                    struct winsys_handle *handle)
{
    struct ljmgpu_bo *bo = NULL;
    struct drm_gem_open req = {0};
    uint32_t dma_buf_size = 0;
    unsigned h = handle->handle;

    /* Convert a DMA buf handle to a KMS handle now. */
   if (handle->type == WINSYS_HANDLE_TYPE_FD) {
      uint32_t prime_handle;
      off_t size;

      /* Get a KMS handle. */
      if (drmPrimeFDToHandle(screen->fd, h, &prime_handle)) {
         return NULL;
      }

      /* Query the buffer size. */
      size = lseek(h, 0, SEEK_END);
      if (size == (off_t)-1) {
         ljmgpu_close_kms_handle(screen, prime_handle);
         return NULL;
      }
      lseek(h, 0, SEEK_SET);

      dma_buf_size = size;
      h = prime_handle;
   }

    return NULL;
}

bool ljmgpu_bo_wait(struct ljmgpu_bo *bo, uint32_t op, uint64_t timeout_ns)
{
    int64_t abs_timeout;

    if (timeout_ns == 0)
        abs_timeout = 0;
    else
        abs_timeout = os_time_get_absolute_timeout(timeout_ns);

    if (abs_timeout == OS_TIMEOUT_INFINITE)
        abs_timeout = INT64_MAX;
    /*
    struct drm_ljmgpu_gem_wait req = {
        .handle = bo->handle,
        .op = op,
        .timeout_ns = abs_timeout,
    };

    return drmIoctl(bo->screen->fd, DRM_IOCTL_LJM_GEM_WAIT, &req) == 0;
    */
    return true;
}
struct ljmgpu_bo * ljmgpu_bo_create_multi(struct ljmgpu_screen *screen, uint32_t size, uint32_t flags)
{
    struct ljmgpu_bo * bo = ljmgpu_bo_create(screen, size, flags);
    if(bo == NULL) {
	    LOGE("ljmgpu_bo_create_multi create error\n");
        return NULL;
    }
    if(ljmgpu_bo_map(bo) == NULL) {
	    LOGE("ljmgpu_bo_create_multi map error\n");
        return NULL;
    }

    return bo;
}
