#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "c11/threads.h"
#include "util/os_file.h"
#include "util/u_hash_table.h"
#include "util/u_pointer.h"
#include "renderonly/renderonly.h"

#include "ljmgpu_drm_public.h"

#include "ljmgpu/ljmgpu_screen.h"

static struct hash_table *fd_tab = NULL;
static mtx_t ljmgpu_screen_mutex = _MTX_INITIALIZER_NP;


static void
ljmgpu_drm_screen_destroy(struct pipe_screen *pscreen)
{
   struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
   boolean destroy;
   int fd = screen->fd;

   mtx_lock(&ljmgpu_screen_mutex);
   destroy = --screen->refcnt == 0;
   if (destroy) {
      _mesa_hash_table_remove_key(fd_tab, intptr_to_pointer(fd));

      if (!fd_tab->entries) {
         _mesa_hash_table_destroy(fd_tab, NULL);
         fd_tab = NULL;
      }
   }
   mtx_unlock(&ljmgpu_screen_mutex);

   if (destroy) {
      pscreen->destroy = screen->winsys_priv;
      pscreen->destroy(pscreen);
      close(fd);
   }
}

struct pipe_screen *
ljmgpu_drm_screen_create(int fd)
{
    printf("ljmgpu debug winsys fd[%d]\n", fd);
   struct pipe_screen *pscreen = NULL;

   mtx_lock(&ljmgpu_screen_mutex);
   if (!fd_tab) {
      fd_tab = util_hash_table_create_fd_keys();
      if (!fd_tab)
         goto unlock;
   }

   pscreen = util_hash_table_get(fd_tab, intptr_to_pointer(fd));
   if (pscreen) {
      ljmgpu_screen(pscreen)->refcnt++;
   } else {
      int dup_fd = os_dupfd_cloexec(fd);

      pscreen = ljmgpu_screen_create(dup_fd, NULL);
      if (pscreen) {
         _mesa_hash_table_insert(fd_tab, intptr_to_pointer(dup_fd), pscreen);

         /* Bit of a hack, to avoid circular linkage dependency,
          * ie. pipe driver having to call in to winsys, we
          * override the pipe drivers screen->destroy():
          */
         ljmgpu_screen(pscreen)->winsys_priv = pscreen->destroy;
         pscreen->destroy = ljmgpu_drm_screen_destroy;
      }
   }

unlock:
   mtx_unlock(&ljmgpu_screen_mutex);
   return pscreen;
}

struct pipe_screen *
ljmgpu_drm_screen_create_renderonly(struct renderonly *ro)
{
   printf("ljmgpu debug winsys ro->gpu_fd[%d]\n", ro->gpu_fd);
   return ljmgpu_screen_create(os_dupfd_cloexec(ro->gpu_fd), ro);
}

