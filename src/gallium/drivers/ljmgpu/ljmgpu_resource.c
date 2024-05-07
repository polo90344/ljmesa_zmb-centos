#include "util/u_memory.h"
#include "util/u_blitter.h"
#include "util/format/u_format.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/hash_table.h"
#include "util/ralloc.h"
#include "util/u_drm.h"
#include "renderonly/renderonly.h"

#include "frontend/drm_driver.h"

#include "drm-uapi/drm_fourcc.h"
#include "drm-uapi/ljmgpu_drm.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_util.h"
#include "ljmgpu_log.h"

//#include "pan_minmax_cache.h"
//#include "pan_tiling.h"

static struct pipe_resource *
ljmgpu_resource_create_scanout(struct pipe_screen *pscreen,
                             const struct pipe_resource *templat,
                             unsigned width, unsigned height)
{
    LOGI("gallium ljmgpu_resource_create_scanout width[%u]height[%u]\n", width, height);
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    struct renderonly_scanout *scanout;
    struct winsys_handle handle;
    struct pipe_resource *pres;

    struct pipe_resource scanout_templat = *templat;
    scanout_templat.width0 = width;
    scanout_templat.height0 = height;
    scanout_templat.screen = pscreen;

    scanout = renderonly_scanout_for_resource(&scanout_templat,
                                             screen->ro, &handle);

    if (!scanout)
       return NULL;

    assert(handle.type == WINSYS_HANDLE_TYPE_FD);
    pres = pscreen->resource_from_handle(pscreen, templat, &handle,
                                        PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);

    close(handle.handle);
    if (!pres) {
       renderonly_scanout_destroy(scanout, screen->ro);
       return NULL;
    }

    struct ljmgpu_resource *res = ljmgpu_resource(pres);
    res->scanout = scanout;

    return pres;
}

static uint32_t
setup_miptree(struct ljmgpu_resource *res,
              unsigned width0, unsigned height0,
              bool should_align_dimensions)
{
    struct pipe_resource *pres = &res->base;
    unsigned level;
    unsigned width = width0;
    unsigned height = height0;
    unsigned depth = pres->depth0;
    uint32_t size = 0;

    return size;
}

static struct pipe_resource *
ljmgpu_resource_create_bo(struct pipe_screen *pscreen,
                        const struct pipe_resource *templat,
                        unsigned width, unsigned height,
                        bool should_align_dimensions)
{
    LOGI("gallium ljmgpu_resource_create_bo width[%u]height[%u]\n", width, height);
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    struct ljmgpu_resource *res;
    struct pipe_resource *pres;

    res = CALLOC_STRUCT(ljmgpu_resource);
    if (!res)
       return NULL;

    res->base = *templat;
    res->base.screen = pscreen;
    pipe_reference_init(&res->base.reference, 1);

    pres = &res->base;

    uint32_t size = (width * height * 32) / 8;

    res->bo = ljmgpu_bo_create(screen, size, 0);
    if (!res->bo) {
       FREE(res);
       return NULL;
    }

    LOGI("gallium ljmgpu_resource_create_bo over\n");
    return pres;
}

static struct pipe_resource *
_ljmgpu_resource_create_with_modifiers(struct pipe_screen *pscreen,
                                     const struct pipe_resource *templat,
                                     const uint64_t *modifiers,
                                     int count)
{
    LOGI("gallium _ljmgpu_resource_create_with_modifiers templat->width0[%d],templat->height0[%d]\n", templat->width0, templat->height0);
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    bool should_tile;
    unsigned width, height;
    bool should_align_dimensions;
    bool has_user_modifiers = true;

    if (count == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)
       has_user_modifiers = false;
   
    /* VBOs/PBOs are untiled (and 1 height). */
    if (templat->target == PIPE_BUFFER)
       should_tile = false;

    if (templat->bind & (PIPE_BIND_LINEAR | PIPE_BIND_SCANOUT))
       should_tile = false;

    /* If there's no user modifiers and buffer is shared we use linear */
    if (!has_user_modifiers && (templat->bind & PIPE_BIND_SHARED))
       should_tile = false;

    if (has_user_modifiers &&
       !drm_find_modifier(DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED,
                         modifiers, count))
       should_tile = false;

    if (should_tile || (templat->bind & PIPE_BIND_RENDER_TARGET) ||
        (templat->bind & PIPE_BIND_DEPTH_STENCIL)) {
       should_align_dimensions = true;
       width = align(templat->width0, 16);
       height = align(templat->height0, 16);
    }
    else {
       should_align_dimensions = false;
       width = templat->width0;
       height = templat->height0;
    }

    struct pipe_resource *pres;
    if (screen->ro && (templat->bind & PIPE_BIND_SCANOUT))
       pres = ljmgpu_resource_create_scanout(pscreen, templat, width, height);
    else
       pres = ljmgpu_resource_create_bo(pscreen, templat, width, height,
                                     should_align_dimensions);

   /*debug_printf("gallium %s: pres=%p width=%u height=%u depth=%u target=%d "
                   "bind=%x usage=%d tile=%d last_level=%d\n", __func__,
                   pres, pres->width0, pres->height0, pres->depth0,
                   pres->target, pres->bind, pres->usage, should_tile, templat->last_level);*/
      
    LOGI("gallium _ljmgpu_resource_create_with_modifiers over\n");
    return pres;
}

static struct pipe_resource *
ljmgpu_resource_create(struct pipe_screen *pscreen,
                     const struct pipe_resource *templat)
{
    LOGI("gallium ljmgpu_resource_create templat->width0[%d],templat->height0[%d]\n", templat->width0, templat->height0);
    const uint64_t mod = DRM_FORMAT_MOD_INVALID;

    return _ljmgpu_resource_create_with_modifiers(pscreen, templat, &mod, 1);
}

static struct pipe_resource *
ljmgpu_resource_create_with_modifiers(struct pipe_screen *pscreen,
                                    const struct pipe_resource *templat,
                                    const uint64_t *modifiers,
                                    int count)
{
    LOGI("gallium ljmgpu_resource_create_with_modifiers templat->width0[%d],templat->height0[%d]\n", templat->width0, templat->height0);
    struct pipe_resource tmpl = *templat;
    if (drm_find_modifier(DRM_FORMAT_MOD_LINEAR, modifiers, count))
      tmpl.bind |= PIPE_BIND_SCANOUT;

   return _ljmgpu_resource_create_with_modifiers(pscreen, &tmpl, modifiers, count);

}

static void
ljmgpu_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *pres)
{
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    struct ljmgpu_resource *res = ljmgpu_resource(pres);

}

static struct pipe_resource *
ljmgpu_resource_from_handle(struct pipe_screen *pscreen,
        const struct pipe_resource *templat,
        struct winsys_handle *handle, unsigned usage)
{
#if 0
    if (templat->bind & (PIPE_BIND_SAMPLER_VIEW |
                        PIPE_BIND_RENDER_TARGET |
                        PIPE_BIND_DEPTH_STENCIL)) {
       /* sampler hardware need offset alignment 64, while render hardware
       * need offset alignment 8, but due to render target may be reloaded
       * which uses the sampler, set alignment requrement to 64 for all
       */
    if (handle->offset & 0x3f) {
         debug_error("import buffer offset not properly aligned\n");
         return NULL;
   }
   }

   struct lima_resource *res = CALLOC_STRUCT(lima_resource);
   if (!res)
      return NULL;

   struct pipe_resource *pres = &res->base;
   *pres = *templat;
   pres->screen = pscreen;
   pipe_reference_init(&pres->reference, 1);
   res->levels[0].offset = handle->offset;
   res->levels[0].stride = handle->stride;

   struct lima_screen *screen = lima_screen(pscreen);
   res->bo = lima_bo_import(screen, handle);
   if (!res->bo) {
      FREE(res);
      return NULL;
   }

#endif
    return NULL;
}

static bool
ljmgpu_resource_get_handle(struct pipe_screen *pscreen,
                         struct pipe_context *pctx,
                         struct pipe_resource *pres,
                         struct winsys_handle *handle, unsigned usage)
{
    struct ljmgpu_screen *screen = ljmgpu_screen(pscreen);
    struct ljmgpu_resource *res = ljmgpu_resource(pres);

    if (res->tiled)
       handle->modifier = DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED;
    else
       handle->modifier = DRM_FORMAT_MOD_LINEAR;

    res->modifier_constant = true;

    if (handle->type == WINSYS_HANDLE_TYPE_KMS && screen->ro)
       return renderonly_get_handle(res->scanout, handle);

    if (!ljmgpu_bo_export(res->bo, handle))
       return false;

    handle->offset = res->levels[0].offset;
    handle->stride = res->levels[0].stride;
    return true;
}

static bool
ljmgpu_resource_get_param(struct pipe_screen *pscreen,
                        struct pipe_context *pctx,
                        struct pipe_resource *pres,
                        unsigned plane, unsigned layer, unsigned level,
                        enum pipe_resource_param param,
                        unsigned usage, uint64_t *value)
{
    return true;
}

static void
get_scissor_from_box(struct pipe_scissor_state *s,
                     const struct pipe_box *b, int h)
{
}

static void
get_damage_bound_box(struct pipe_resource *pres,
                     const struct pipe_box *rects,
                     unsigned int nrects,
                     struct pipe_scissor_state *bound)
{

}

static void
ljmgpu_resource_set_damage_region(struct pipe_screen *pscreen,
                                struct pipe_resource *pres,
                                unsigned int nrects,
                                const struct pipe_box *rects)
{

}

void
ljmgpu_resource_screen_init(struct ljmgpu_screen *screen)
{
    screen->base.resource_create = ljmgpu_resource_create;
    screen->base.resource_create_with_modifiers = ljmgpu_resource_create_with_modifiers;
    screen->base.resource_from_handle = ljmgpu_resource_from_handle;
    screen->base.resource_destroy = ljmgpu_resource_destroy;
    screen->base.resource_get_handle = ljmgpu_resource_get_handle;
    screen->base.resource_get_param = ljmgpu_resource_get_param;
    screen->base.set_damage_region = ljmgpu_resource_set_damage_region;
}

static struct pipe_surface *
ljmgpu_surface_create(struct pipe_context *pctx,
                    struct pipe_resource *pres,
                    const struct pipe_surface *surf_tmpl)
{
    LOGI("gallium ljmgpu_surface_create in\n");
    struct ljmgpu_surface *surf = CALLOC_STRUCT(ljmgpu_surface);

    if (!surf) {
        LOGE("ljmgpu_surface_create surf is null\n");
        return NULL;
    }

    assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);

    struct pipe_surface *psurf = &surf->base;
    unsigned level = surf_tmpl->u.tex.level;

    pipe_reference_init(&psurf->reference, 1);
    pipe_resource_reference(&psurf->texture, pres);

    psurf->context = pctx;
    psurf->format = surf_tmpl->format;
    psurf->width = u_minify(pres->width0, level);
    psurf->height = u_minify(pres->height0, level);
    psurf->u.tex.level = level;
    psurf->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
    psurf->u.tex.last_layer = surf_tmpl->u.tex.last_layer;

    surf->tiled_w = align(psurf->width, 16) >> 4;
    surf->tiled_h = align(psurf->height, 16) >> 4;
    LOGI("gallium ljmgpu_surface_create psurf->width[%d],psurf->height[%d]\n", psurf->width, psurf->height);
    surf->reload = 0;
    if (util_format_has_stencil(util_format_description(psurf->format)))
       surf->reload |= PIPE_CLEAR_STENCIL;
    if (util_format_has_depth(util_format_description(psurf->format)))
       surf->reload |= PIPE_CLEAR_DEPTH;
    if (!util_format_is_depth_or_stencil(psurf->format))
       surf->reload |= PIPE_CLEAR_COLOR0;

    LOGI("gallium ljmgpu_surface_create over\n");
    return &surf->base;
}

static void
ljmgpu_surface_destroy(struct pipe_context *pctx, struct pipe_surface *psurf)
{
}
static void *
ljmgpu_transfer_map(struct pipe_context *pctx,
                  struct pipe_resource *pres,
                  unsigned level,
                  unsigned usage,
                  const struct pipe_box *box,
                  struct pipe_transfer **pptrans)
{
    return NULL;
}
static void
ljmgpu_transfer_flush_region(struct pipe_context *pctx,
                           struct pipe_transfer *ptrans,
                           const struct pipe_box *box)
{

}

static bool
ljmgpu_should_convert_linear(struct ljmgpu_resource *res,
                           struct pipe_transfer *ptrans)
{
    return true;
}

static void
ljmgpu_transfer_unmap_inner(struct ljmgpu_context *ctx,
                          struct pipe_transfer *ptrans)
{
}

static void
ljmgpu_transfer_unmap(struct pipe_context *pctx,
                    struct pipe_transfer *ptrans)
{
}

static void
ljmgpu_util_blitter_save_states(struct ljmgpu_context *ctx)
{
    util_blitter_save_blend(ctx->blitter, (void *)ctx->blend);
    util_blitter_save_depth_stencil_alpha(ctx->blitter, (void *)ctx->zsa);
    util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
    util_blitter_save_rasterizer(ctx->blitter, (void *)ctx->rasterizer);
    util_blitter_save_fragment_shader(ctx->blitter, ctx->uncomp_fs);
    util_blitter_save_vertex_shader(ctx->blitter, ctx->uncomp_vs);
    util_blitter_save_viewport(ctx->blitter,
                              &ctx->viewport.transform);
    util_blitter_save_scissor(ctx->blitter, &ctx->scissor);
    util_blitter_save_vertex_elements(ctx->blitter,
                                     ctx->vertex_elements);
    util_blitter_save_vertex_buffer_slot(ctx->blitter,
                                        ctx->vertex_buffers.vb);

    util_blitter_save_framebuffer(ctx->blitter, &ctx->framebuffer.base);

    util_blitter_save_fragment_sampler_states(ctx->blitter,
                                             ctx->tex_stateobj.num_samplers,
                                             (void**)ctx->tex_stateobj.samplers);
    util_blitter_save_fragment_sampler_views(ctx->blitter,
                                            ctx->tex_stateobj.num_textures,
					    ctx->tex_stateobj.textures);
}

static void
ljmgpu_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
{
    LOGI("gallium ljm_blit in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct pipe_blit_info info = *blit_info;

    if (util_try_blit_via_copy_region(pctx, &info)) {
        return; /* done */
    }

    if (info.mask & PIPE_MASK_S) {
       debug_printf("ljmgpu: cannot blit stencil, skipping\n");
       info.mask &= ~PIPE_MASK_S;
    }

    if (!util_blitter_is_blit_supported(ctx->blitter, &info)) {
       debug_printf("ljmgpu: blit unsupported %s -> %s\n",
                   util_format_short_name(info.src.resource->format),
                   util_format_short_name(info.dst.resource->format));
       return;
    }

    ljmgpu_util_blitter_save_states(ctx);

    util_blitter_blit(ctx->blitter, &info);
    LOGI("gallium ljm_blit over\n");
}

static void
ljmgpu_flush_resource(struct pipe_context *pctx, struct pipe_resource *resource)
{

}

static void
ljmgpu_texture_subdata(struct pipe_context *pctx,
                     struct pipe_resource *prsc,
                     unsigned level,
                     unsigned usage,
                     const struct pipe_box *box,
                     const void *data,
                     unsigned stride,
                     unsigned layer_stride)
{
}

void
ljmgpu_resource_context_init(struct ljmgpu_context *ctx)
{
    LOGI("gallium ljmgpu_resource_context_init in\n");
    ctx->base.create_surface = ljmgpu_surface_create;
    ctx->base.surface_destroy = ljmgpu_surface_destroy;

    ctx->base.buffer_subdata = u_default_buffer_subdata;
    ctx->base.texture_subdata = ljmgpu_texture_subdata;
    ctx->base.resource_copy_region = util_resource_copy_region;
    ctx->base.blit = ljmgpu_blit;
    ctx->base.buffer_map = ljmgpu_transfer_map;
    ctx->base.texture_map = ljmgpu_transfer_map;
    ctx->base.transfer_flush_region = ljmgpu_transfer_flush_region;
    ctx->base.buffer_unmap = ljmgpu_transfer_unmap;
    ctx->base.texture_unmap = ljmgpu_transfer_unmap;
    ctx->base.flush_resource = ljmgpu_flush_resource;
    LOGI("gallium ljmgpu_resource_context_init over\n");
}
