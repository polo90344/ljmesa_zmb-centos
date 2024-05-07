#include "util/format/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_helpers.h"
#include "util/u_debug.h"
#include "util/u_framebuffer.h"
#include "util/u_viewport.h"

#include "pipe/p_state.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_format.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_log.h"

static void
ljmgpu_set_framebuffer_state(struct pipe_context *pctx,
                           const struct pipe_framebuffer_state *framebuffer)
{
    LOGI("gallium ljm_set_framebuffer_state in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    /* make sure there are always single job in this context */
    if (ljmgpu_debug & LIMA_DEBUG_SINGLE_JOB)
        ljmgpu_flush(ctx);

    struct ljmgpu_context_framebuffer *fb = &ctx->framebuffer;

    util_copy_framebuffer_state(&fb->base, framebuffer);

    ctx->job = NULL;
    ctx->dirty |= EGP_CONTEXT_DIRTY_FRAMEBUFFER;
}

static void
ljmgpu_set_polygon_stipple(struct pipe_context *pctx,
                         const struct pipe_poly_stipple *stipple)
{

}

static void *
ljmgpu_create_depth_stencil_alpha_state(struct pipe_context *pctx,
                                      const struct pipe_depth_stencil_alpha_state *cso)
{
    struct ljmgpu_depth_stencil_alpha_state *so;

    so = CALLOC_STRUCT(ljmgpu_depth_stencil_alpha_state);
    if (!so)
        return NULL;

    so->base = *cso;

    return so;
}

static void
ljmgpu_bind_depth_stencil_alpha_state(struct pipe_context *pctx, void *hwcso)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->zsa = hwcso;
    ctx->dirty |= EGP_CONTEXT_DIRTY_ZSA;
}

static void
ljmgpu_delete_depth_stencil_alpha_state(struct pipe_context *pctx, void *hwcso)
{
    FREE(hwcso);
}

static void *
ljmgpu_create_rasterizer_state(struct pipe_context *pctx,
                             const struct pipe_rasterizer_state *cso)
{
    struct ljmgpu_rasterizer_state *so;

    so = CALLOC_STRUCT(ljmgpu_rasterizer_state);
    if (!so)
        return NULL;

    so->base = *cso;

    return so;
}

static void
ljmgpu_bind_rasterizer_state(struct pipe_context *pctx, void *hwcso)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->rasterizer = hwcso;
    ctx->dirty |= EGP_CONTEXT_DIRTY_RASTERIZER;
}

static void
ljmgpu_delete_rasterizer_state(struct pipe_context *pctx, void *hwcso)
{
    FREE(hwcso);
}

static void *
ljmgpu_create_blend_state(struct pipe_context *pctx,
                        const struct pipe_blend_state *cso)
{
    struct ljmgpu_blend_state *so;

    so = CALLOC_STRUCT(ljmgpu_blend_state);
    if (!so)
        return NULL;

    so->base = *cso;

    return so;
}

static void
ljmgpu_bind_blend_state(struct pipe_context *pctx, void *hwcso)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->blend = hwcso;
    ctx->dirty |= EGP_CONTEXT_DIRTY_BLEND;
}

static void
ljmgpu_delete_blend_state(struct pipe_context *pctx, void *hwcso)
{
    FREE(hwcso);
}

static void *
ljmgpu_create_vertex_elements_state(struct pipe_context *pctx, unsigned num_elements,
                                  const struct pipe_vertex_element *elements)
{
    struct ljmgpu_vertex_element_state *so;

    so = CALLOC_STRUCT(ljmgpu_vertex_element_state);
    if (!so)
        return NULL;

    memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
    so->num_elements = num_elements;

    return so;
}

static void
ljmgpu_bind_vertex_elements_state(struct pipe_context *pctx, void *hwcso)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->vertex_elements = hwcso;
    ctx->dirty |= EGP_CONTEXT_DIRTY_VERTEX_ELEM;
}

static void
ljmgpu_delete_vertex_elements_state(struct pipe_context *pctx, void *hwcso)
{
    FREE(hwcso);
}

static void
ljmgpu_set_vertex_buffers(struct pipe_context *pctx,
                        unsigned start_slot, unsigned count,
                        unsigned unbind_num_trailing_slots,
                        bool take_ownership,
                        const struct pipe_vertex_buffer *vb)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_context_vertex_buffer *so = &ctx->vertex_buffers;

    util_set_vertex_buffers_mask(so->vb, &so->enabled_mask,
            vb, start_slot, count,
            unbind_num_trailing_slots,
            take_ownership);
    so->count = util_last_bit(so->enabled_mask);

    ctx->dirty |= EGP_CONTEXT_DIRTY_VERTEX_BUFF;
}

static void
ljmgpu_set_viewport_states(struct pipe_context *pctx,
                         unsigned start_slot,
                         unsigned num_viewports,
                         const struct pipe_viewport_state *viewport)
{
    LOGI("gallium ljm_set_viewport_states in\n");
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    /* reverse calculate the parameter of glViewport */
    ctx->viewport.left = viewport->translate[0] - fabsf(viewport->scale[0]);
    ctx->viewport.right = viewport->translate[0] + fabsf(viewport->scale[0]);
    ctx->viewport.bottom = viewport->translate[1] - fabsf(viewport->scale[1]);
    ctx->viewport.top = viewport->translate[1] + fabsf(viewport->scale[1]);
    LOGI("gallium ljm_set_viewport_states left[%f]right[%f]bottom[%f]top[%f]\n", ctx->viewport.left, ctx->viewport.right, ctx->viewport.bottom, ctx->viewport.top);
    /* reverse calculate the parameter of glDepthRange */
    float near, far;
    bool halfz = ctx->rasterizer && ctx->rasterizer->base.clip_halfz;
    util_viewport_zmin_zmax(viewport, halfz, &near, &far);

    ctx->viewport.near = ctx->rasterizer && ctx->rasterizer->base.depth_clip_near ? near : 0.0f;
    ctx->viewport.far = ctx->rasterizer && ctx->rasterizer->base.depth_clip_far ? far : 1.0f;

    ctx->viewport.transform = *viewport;
    ctx->dirty |= EGP_CONTEXT_DIRTY_VIEWPORT;
    LOGI("gallium ljm_set_viewport_states over\n");
}

static void
ljmgpu_set_scissor_states(struct pipe_context *pctx,
                        unsigned start_slot,
                        unsigned num_scissors,
                        const struct pipe_scissor_state *scissor)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->scissor = *scissor;
    ctx->dirty |= EGP_CONTEXT_DIRTY_SCISSOR;
}

static void
ljmgpu_set_blend_color(struct pipe_context *pctx,
                     const struct pipe_blend_color *blend_color)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->blend_color = *blend_color;
    ctx->dirty |= EGP_CONTEXT_DIRTY_BLEND_COLOR;
}

static void
ljmgpu_set_stencil_ref(struct pipe_context *pctx,
                     const struct pipe_stencil_ref stencil_ref)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);

    ctx->stencil_ref = stencil_ref;
    ctx->dirty |= EGP_CONTEXT_DIRTY_STENCIL_REF;
}

static void
ljmgpu_set_clip_state(struct pipe_context *pctx,
                    const struct pipe_clip_state *clip)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    ctx->clip = *clip;

    ctx->dirty |= EGP_CONTEXT_DIRTY_CLIP;
}

static void
ljmgpu_set_constant_buffer(struct pipe_context *pctx,
                         enum pipe_shader_type shader, uint index,
                         bool pass_reference,
                         const struct pipe_constant_buffer *cb)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_context_constant_buffer *so = ctx->const_buffer + shader;

    assert(index == 0);

    if (unlikely(!cb)) {
        so->buffer = NULL;
        so->size = 0;
    } else {
        assert(!cb->buffer);

        so->buffer = cb->user_buffer + cb->buffer_offset;
        so->size = cb->buffer_size;
    }

    so->dirty = true;
    ctx->dirty |= EGP_CONTEXT_DIRTY_CONST_BUFF;
}

static void *
ljmgpu_create_sampler_state(struct pipe_context *pctx,
                         const struct pipe_sampler_state *cso)
{
    struct ljmgpu_sampler_state *so = CALLOC_STRUCT(ljmgpu_sampler_state);
    if (!so)
        return NULL;

    memcpy(so, cso, sizeof(*cso));

    return so;
}

static void
ljmgpu_sampler_state_delete(struct pipe_context *pctx, void *sstate)
{
    free(sstate);
}

static void
ljmgpu_sampler_states_bind(struct pipe_context *pctx,
                        enum pipe_shader_type shader, unsigned start,
                        unsigned nr, void **hwcso)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_texture_stateobj *ljmgpu_tex = &ctx->tex_stateobj;
    unsigned i;
    unsigned new_nr = 0;

    assert(start == 0);

    for (i = 0; i < nr; i++) {
        if (hwcso[i])
            new_nr = i + 1;
        ljmgpu_tex->samplers[i] = hwcso[i];
    }

    for (; i < ljmgpu_tex->num_samplers; i++) {
        ljmgpu_tex->samplers[i] = NULL;
    }

    ljmgpu_tex->num_samplers = new_nr;
    ctx->dirty |= EGP_CONTEXT_DIRTY_TEXTURES;
}

static struct pipe_sampler_view *
ljmgpu_create_sampler_view(struct pipe_context *pctx, struct pipe_resource *prsc,
                        const struct pipe_sampler_view *cso)
{
    struct ljmgpu_sampler_view *so = CALLOC_STRUCT(ljmgpu_sampler_view);

    if (!so)
        return NULL;

    so->base = *cso;

    pipe_reference(NULL, &prsc->reference);
    so->base.texture = prsc;
    so->base.reference.count = 1;
    so->base.context = pctx;

    uint8_t sampler_swizzle[4] = { cso->swizzle_r, cso->swizzle_g,
        cso->swizzle_b, cso->swizzle_a };
    const uint8_t *format_swizzle = ljmgpu_format_get_texel_swizzle(cso->format);
    util_format_compose_swizzles(format_swizzle, sampler_swizzle, so->swizzle);

    return &so->base;
}

static void
ljmgpu_sampler_view_destroy(struct pipe_context *pctx,
                         struct pipe_sampler_view *pview)
{
    struct ljmgpu_sampler_view *view = ljmgpu_sampler_view(pview);

    pipe_resource_reference(&pview->texture, NULL);

    free(view);
}

static void
ljmgpu_set_sampler_views(struct pipe_context *pctx,
                      enum pipe_shader_type shader,
                      unsigned start, unsigned nr,
                       unsigned unbind_num_trailing_slots,
                       bool take_ownership,
                      struct pipe_sampler_view **views)
{
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_texture_stateobj *ljmgpu_tex = &ctx->tex_stateobj;
    int i;
    unsigned new_nr = 0;

    assert(start == 0);

    for (i = 0; i < nr; i++) {
        if (views[i])
            new_nr = i + 1;

        if (take_ownership) {
            pipe_sampler_view_reference(&ljmgpu_tex->textures[i], NULL);
            ljmgpu_tex->textures[i] = views[i];
        } else {
            pipe_sampler_view_reference(&ljmgpu_tex->textures[i], views[i]);
        }
    }

    for (; i < ljmgpu_tex->num_textures; i++) {
        pipe_sampler_view_reference(&ljmgpu_tex->textures[i], NULL);
    }

    ljmgpu_tex->num_textures = new_nr;
    ctx->dirty |= EGP_CONTEXT_DIRTY_TEXTURES;
}

static void
ljmgpu_set_sample_mask(struct pipe_context *pctx,
                     unsigned sample_mask)
{
}

void ljmgpu_state_init(struct ljmgpu_context *ctx)
{
    ctx->base.set_framebuffer_state = ljmgpu_set_framebuffer_state;
    ctx->base.set_polygon_stipple = ljmgpu_set_polygon_stipple;
    ctx->base.set_viewport_states = ljmgpu_set_viewport_states;
    ctx->base.set_scissor_states = ljmgpu_set_scissor_states;
    ctx->base.set_blend_color = ljmgpu_set_blend_color;
    ctx->base.set_stencil_ref = ljmgpu_set_stencil_ref;
    ctx->base.set_clip_state = ljmgpu_set_clip_state;

    ctx->base.set_vertex_buffers = ljmgpu_set_vertex_buffers;
    ctx->base.set_constant_buffer = ljmgpu_set_constant_buffer;

    ctx->base.create_depth_stencil_alpha_state = ljmgpu_create_depth_stencil_alpha_state;
    ctx->base.bind_depth_stencil_alpha_state = ljmgpu_bind_depth_stencil_alpha_state;
    ctx->base.delete_depth_stencil_alpha_state = ljmgpu_delete_depth_stencil_alpha_state;

    ctx->base.create_rasterizer_state = ljmgpu_create_rasterizer_state;
    ctx->base.bind_rasterizer_state = ljmgpu_bind_rasterizer_state;
    ctx->base.delete_rasterizer_state = ljmgpu_delete_rasterizer_state;

    ctx->base.create_blend_state = ljmgpu_create_blend_state;
    ctx->base.bind_blend_state = ljmgpu_bind_blend_state;
    ctx->base.delete_blend_state = ljmgpu_delete_blend_state;

    ctx->base.create_vertex_elements_state = ljmgpu_create_vertex_elements_state;
    ctx->base.bind_vertex_elements_state = ljmgpu_bind_vertex_elements_state;
    ctx->base.delete_vertex_elements_state = ljmgpu_delete_vertex_elements_state;

    ctx->base.create_sampler_state = ljmgpu_create_sampler_state;
    ctx->base.delete_sampler_state = ljmgpu_sampler_state_delete;
    ctx->base.bind_sampler_states = ljmgpu_sampler_states_bind;

    ctx->base.create_sampler_view = ljmgpu_create_sampler_view;
    ctx->base.sampler_view_destroy = ljmgpu_sampler_view_destroy;
    ctx->base.set_sampler_views = ljmgpu_set_sampler_views;

    ctx->base.set_sample_mask = ljmgpu_set_sample_mask;
}

void ljmgpu_state_fini(struct ljmgpu_context *ctx)
{
    struct ljmgpu_context_vertex_buffer *so = &ctx->vertex_buffers;

    util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, NULL,
            0, 0, ARRAY_SIZE(so->vb), false);

    pipe_surface_reference(&ctx->framebuffer.base.cbufs[0], NULL);
    pipe_surface_reference(&ctx->framebuffer.base.zsbuf, NULL);
}

