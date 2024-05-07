#include <string.h>

#include "util/ralloc.h"
#include "util/u_debug.h"
#include "util/u_screen.h"
#include "renderonly/renderonly.h"

#include "drm-uapi/drm_fourcc.h"
#include "drm-uapi/ljmgpu_drm.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_program.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_fence.h"
#include "util/os_time.h"
//#include "ljmgpu_format.h"
#include "ljmgpu_disk_cache.h"
//#include "ir/ljmgpu_ir.h"
#include "ljmgpu_log.h"

#include "xf86drm.h"

uint32_t ljmgpu_debug;
static void
ljmgpu_screen_destroy(struct pipe_screen *pscreen)
{
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    if (screen->ro)
        screen->ro->destroy(screen->ro);

    if (screen->pp_buffer)
        ljmgpu_bo_unreference(screen->pp_buffer);

    ljmgpu_bo_cache_fini(screen);
    ljmgpu_bo_table_fini(screen);
    //disk_cache_destroy(screen->disk_cache);
    ralloc_free(screen);
}

static const char *
ljmgpu_screen_get_name(struct pipe_screen *pscreen)
{
    return "venus";
}
static const char *
ljmgpu_screen_get_vendor(struct pipe_screen *pscreen)
{
   return "ljmgpu";
}
static const char *
ljmgpu_screen_get_device_vendor(struct pipe_screen *pscreen)
{
   return "LJMICRO";
}
static int
ljmgpu_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
    switch (param) {
        case PIPE_CAP_NPOT_TEXTURES:
        case PIPE_CAP_BLEND_EQUATION_SEPARATE:
        case PIPE_CAP_ACCELERATED:
        case PIPE_CAP_UMA:
        case PIPE_CAP_CLIP_HALFZ:
        case PIPE_CAP_NATIVE_FENCE_FD:
        case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
        case PIPE_CAP_TEXTURE_SWIZZLE:
        case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
            return 1;

            /* Unimplemented, but for exporting OpenGL 2.0 */
        case PIPE_CAP_OCCLUSION_QUERY:
        case PIPE_CAP_POINT_SPRITE:
            return 1;

            /* not clear supported */
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
            return 1;

        case PIPE_CAP_TGSI_FS_POSITION_IS_SYSVAL:
        case PIPE_CAP_TGSI_FS_POINT_IS_SYSVAL:
        case PIPE_CAP_TGSI_FS_FACE_IS_INTEGER_SYSVAL:
            return 1;

        case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
            return 1;

        case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
            return 1 << (LJMGPU_MAX_MIP_LEVELS - 1);
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            return LJMGPU_MAX_MIP_LEVELS;

        case PIPE_CAP_VENDOR_ID:
            return 0x13B5;

        case PIPE_CAP_VIDEO_MEMORY:
            return 0;

        case PIPE_CAP_PCI_GROUP:
        case PIPE_CAP_PCI_BUS:
        case PIPE_CAP_PCI_DEVICE:
        case PIPE_CAP_PCI_FUNCTION:
            return 0;

        case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
        case PIPE_CAP_SHAREABLE_SHADERS:
            return 0;

        case PIPE_CAP_ALPHA_TEST:
            return 1;

        case PIPE_CAP_FLATSHADE:
        case PIPE_CAP_TWO_SIDED_COLOR:
        case PIPE_CAP_CLIP_PLANES:
            return 0;

        case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
            return 1;

        default:
            return u_pipe_screen_get_param_defaults(pscreen, param);
    }
}
static float
ljmgpu_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
    switch (param) {
        case PIPE_CAPF_MAX_LINE_WIDTH:
        case PIPE_CAPF_MAX_LINE_WIDTH_AA:
        case PIPE_CAPF_MAX_POINT_WIDTH:
        case PIPE_CAPF_MAX_POINT_WIDTH_AA:
            return 100.0f;
        case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
            return 16.0f;
        case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            return 15.0f;

        default:
            return 0.0f;
    }
}
static int
ljmgpu_screen_get_shader_param(struct pipe_screen *pscreen, 
    enum pipe_shader_type shader,
    enum pipe_shader_cap param)
{

    return 0;
}
static bool
ljmgpu_screen_is_format_supported(struct pipe_screen *pscreen,
    enum pipe_format format,
    enum pipe_texture_target target,
    unsigned sample_count,
    unsigned storage_sample_count,
    unsigned usage)
{
    return true;
}
static const void *
ljmgpu_screen_get_compiler_options(struct pipe_screen *pscreen,
    enum pipe_shader_ir ir,
    enum pipe_shader_type shader)
{
    return NULL;
}
static bool
ljmgpu_screen_query_info(struct ljmgpu_screen *screen)
{
    /*
    drmVersionPtr version = drmGetVersion(screen->fd);
    if (!version)
        return false;

    drmFreeVersion(version);
    */
    return true;
}
static void
ljmgpu_screen_query_dmabuf_modifiers(struct pipe_screen *pscreen,
    enum pipe_format format, int max,
    uint64_t *modifiers,
    unsigned int *external_only,
    int *count)
{

}
/*
static bool
ljmgpu_screen_is_dmabuf_modifier_supported(struct pipe_screen *pscreen
    uint64_t modifier,
    enum pipe_format format,
    bool *external_only)
{
    return true;
}
*/
static struct disk_cache *
ljmgpu_get_disk_shader_cache (struct pipe_screen *pscreen)
{
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);

    return screen->disk_cache;
    return NULL;
}
static uint64_t
ljmgpu_get_timestamp(struct pipe_screen *screen)
{
        return os_time_get_nano();
}
struct pipe_screen *
ljmgpu_screen_create(int fd, struct renderonly *ro)
{
    LOGD("ljmgpu debug ljmgpu_screen_create in fd[%d]\n", fd);
    struct ljmgpu_screen *screen;

    screen = rzalloc(NULL, struct ljmgpu_screen);
    if (!screen)
        return NULL;

    screen->fd = fd;
    screen->ro = ro;

    ljmgpu_screen_query_info(screen);


    screen->base.destroy = ljmgpu_screen_destroy;
    screen->base.get_name = ljmgpu_screen_get_name;
    screen->base.get_vendor = ljmgpu_screen_get_vendor;
    screen->base.get_device_vendor = ljmgpu_screen_get_device_vendor;
    screen->base.get_param = ljmgpu_screen_get_param;
    screen->base.get_paramf = ljmgpu_screen_get_paramf;
    screen->base.get_shader_param = ljmgpu_screen_get_shader_param;
    screen->base.context_create = ljmgpu_context_create;
    screen->base.is_format_supported = ljmgpu_screen_is_format_supported;
    screen->base.get_compiler_options = ljmgpu_screen_get_compiler_options;
    screen->base.query_dmabuf_modifiers = ljmgpu_screen_query_dmabuf_modifiers;
    //screen->base.is_dmabuf_modifier_supported = ljmgpu_screen_is_dmabuf_modifier_supported;
    screen->base.get_disk_shader_cache = ljmgpu_get_disk_shader_cache;

    screen->base.get_timestamp = ljmgpu_get_timestamp;

    ljmgpu_resource_screen_init(screen);
    ljmgpu_fence_screen_init(screen);
    ljmgpu_disk_cache_init(screen);

    screen->refcnt = 1;

    int size = 2 * 1024 * 1024;
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, size, 0);
    char buffer[128] = "ljmgpu screen bo create inside test";
    memcpy((char *)bo->map, buffer, 50);
    LOGD("ljmgpu debug bo->map[%s]\n", (char *)bo->map);

    LOGD("ljmgpu debug ljmgpu_screen_create end\n");
    return &screen->base;
}
