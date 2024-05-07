#ifndef __LJMGPU_DRM_PUBLIC_H__
#define __LJMGPU_DRM_PUBLIC_H__

#include <stdbool.h>

struct pipe_screen;
struct renderonly;

struct pipe_screen *ljmgpu_drm_screen_create(int drmFD);
struct pipe_screen *ljmgpu_drm_screen_create_renderonly(struct renderonly *ro);

#endif /* __LJMGPU_DRM_PUBLIC_H__ */
