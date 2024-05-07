#include <string.h>

#include "util/ralloc.h"
#include "util/u_debug.h"
#include "util/u_screen.h"
#include "renderonly/renderonly.h"

#include "drm-uapi/drm_fourcc.h"
#include "drm-uapi/egp_drm.h"

#include "egp_screen.h"
//#include "xf86drm.h"

struct pipe_screen *
egp_screen_create(int fd, struct renderonly *ro)
{
    printf("liukui debug egp_screen_create in----------\n");
    struct egp_screen *screen;

    screen = rzalloc(NULL, struct egp_screen);
    if (!screen)
        return NULL;

    screen->fd = fd;
    screen->ro = ro;

    return &screen->pipe_scren;
}

