#ifndef __EGP_DRM_PUBLIC_H__
#define __EGP_DRM_PUBLIC_H__

#include <stdbool.h>

struct pipe_screen;
struct renderonly;

struct pipe_screen *egp_drm_screen_create(int drmFD);
struct pipe_screen *egp_drm_screen_create_renderonly(struct renderonly *ro);

#endif /* __EGP_DRM_PUBLIC_H__ */
