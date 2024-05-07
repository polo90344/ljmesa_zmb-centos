#ifndef H_LJMGPU_BO
#define H_LJMGPU_BO

#include <stdbool.h>
#include <stdint.h>

#include "util/u_atomic.h"
#include "util/list.h"

struct ljmgpu_bo {
    struct ljmgpu_screen *screen;
    struct list_head time_list;
    struct list_head size_list;
    int refcnt;
    bool cacheable;
    time_t free_time;

    uint32_t size;
    uint32_t flags;
    uint32_t handle;
    uint64_t offset;
    uint32_t flink_name;

    void *map;
    uint32_t va;
    unsigned long long phy_addr;
    unsigned int cnt;
};

bool ljmgpu_bo_table_init(struct ljmgpu_screen *screen);
void ljmgpu_bo_table_fini(struct ljmgpu_screen *screen);
bool ljmgpu_bo_cache_init(struct ljmgpu_screen *screen);
void ljmgpu_bo_cache_fini(struct ljmgpu_screen *screen);

/* prevent bo from being freed when job start */
static inline void ljmgpu_bo_reference(struct ljmgpu_bo *bo)
{
    p_atomic_inc(&bo->refcnt);
}

struct ljmgpu_bo *ljmgpu_bo_create(struct ljmgpu_screen *screen, uint32_t size, uint32_t flags);
void ljmgpu_bo_unreference(struct ljmgpu_bo *bo);
void *ljmgpu_bo_map(struct ljmgpu_bo *bo);
void ljmgpu_bo_unmap(struct ljmgpu_bo *bo);

bool ljmgpu_bo_export(struct ljmgpu_bo *bo, struct winsys_handle *handle);
struct ljmgpu_bo *ljmgpu_bo_import(struct ljmgpu_screen *screen, struct winsys_handle *handle);

bool ljmgpu_bo_wait(struct ljmgpu_bo *bo, uint32_t op, uint64_t timeout_ns);
struct ljmgpu_bo * ljmgpu_bo_create_multi(struct ljmgpu_screen *screen, uint32_t size, uint32_t flags);
#endif

