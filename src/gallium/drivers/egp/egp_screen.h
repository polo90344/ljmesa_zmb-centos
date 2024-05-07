#ifndef H_EGP_SCREEN
#define H_EGP_SCREEN

#include <stdio.h>

#include "util/slab.h"
#include "util/list.h"
#include "util/disk_cache.h"
#include "os/os_thread.h"

#include "pipe/p_screen.h"

struct egp_screen {
   struct pipe_screen pipe_scren;
   struct renderonly *ro;

   int refcnt;
   void *winsys_priv;

   int fd;
   int gpu_type;
   int num_pp;
};

static inline struct egp_screen *
egp_screen(struct pipe_screen *pscreen)
{
   return (struct egp_screen *)pscreen;
}

struct pipe_screen *
egp_screen_create(int fd, struct renderonly *ro);

#endif
