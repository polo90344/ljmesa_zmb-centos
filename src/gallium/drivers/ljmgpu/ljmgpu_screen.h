#ifndef H_LJMGPU_SCREEN
#define H_LJMGPU_SCREEN

#include <stdio.h>

#include "util/slab.h"
#include "util/list.h"
#include "util/disk_cache.h"
#include "os/os_thread.h"

#include "pipe/p_screen.h"

#define LIMA_DEBUG_DUMP           (1 << 0)
#define LIMA_DEBUG_SHADERDB       (1 << 1)
#define LIMA_DEBUG_NO_TILING      (1 << 2)
#define LIMA_DEBUG_NO_GROW_HEAP   (1 << 3)
#define LIMA_DEBUG_SINGLE_JOB     (1 << 4)
#define LIMA_DEBUG_PRECOMPILE     (1 << 5)
#define LIMA_DEBUG_DISK_CACHE     (1 << 6)


#define EGP_TEST 1

extern uint32_t ljmgpu_debug;

struct ra_regs;

struct ljmgpu_screen {
   struct pipe_screen base;
   struct renderonly *ro;

   int refcnt;
   void *winsys_priv;

   int fd;
   int gpu_type;
   int num_pp;
   uint32_t plb_max_blk;

   /* bo table */
   mtx_t bo_table_lock;
   mtx_t bo_cache_lock;
   struct hash_table *bo_handles;
   struct hash_table *bo_flink_names;
   struct list_head bo_cache_time;

   struct slab_parent_pool transfer_pool;

   struct ra_regs *pp_ra;

   struct ljmgpu_bo *pp_buffer;

   bool has_growable_heap_buffer;

   struct disk_cache *disk_cache;
};

static inline struct ljmgpu_screen *
ljmgpu_screen(struct pipe_screen *pscreen)
{
   return (struct ljmgpu_screen *)pscreen;
}

struct pipe_screen *
ljmgpu_screen_create(int fd, struct renderonly *ro);

#endif
