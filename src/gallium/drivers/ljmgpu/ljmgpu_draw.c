#include "util/format/u_format.h"
#include "util/u_debug.h"
#include "util/u_draw.h"
#include "util/half_float.h"
#include "util/u_helpers.h"
#include "util/u_inlines.h"
#include "util/u_pack_color.h"
#include "util/u_split_draw.h"
#include "util/u_upload_mgr.h"
#include "util/u_prim.h"
#include "util/u_vbuf.h"
#include "util/hash_table.h"

#include "ljmgpu_context.h"
#include "ljmgpu_screen.h"
#include "ljmgpu_resource.h"
#include "ljmgpu_program.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_job.h"
#include "ljmgpu_texture.h"
#include "ljmgpu_util.h"
#include "ljmgpu_gpu.h"
#include "ljmgpu_log.h"

//#include "pan_minmax_cache.h"

//#include <drm-uapi/ljmgpu_drm.h>
#include "drm-uapi/ljmgpu_drm.h"



static void
ljmgpu_clip_scissor_to_viewport(struct ljmgpu_context *ctx)
{
   struct ljmgpu_context_framebuffer *fb = &ctx->framebuffer;
   struct pipe_scissor_state *cscissor = &ctx->clipped_scissor;
   int viewport_left, viewport_right, viewport_bottom, viewport_top;

   if (ctx->rasterizer && ctx->rasterizer->base.scissor) {
      struct pipe_scissor_state *scissor = &ctx->scissor;
      cscissor->minx = scissor->minx;
      cscissor->maxx = scissor->maxx;
      cscissor->miny = scissor->miny;
      cscissor->maxy = scissor->maxy;
   } else {
      cscissor->minx = 0;
      cscissor->maxx = fb->base.width;
      cscissor->miny = 0;
      cscissor->maxy = fb->base.height;
   }

   viewport_left = MAX2(ctx->viewport.left, 0);
   cscissor->minx = MAX2(cscissor->minx, viewport_left);
   viewport_right = MIN2(MAX2(ctx->viewport.right, 0), fb->base.width);
   cscissor->maxx = MIN2(cscissor->maxx, viewport_right);
   if (cscissor->minx > cscissor->maxx)
      cscissor->minx = cscissor->maxx;

   viewport_bottom = MAX2(ctx->viewport.bottom, 0);
   cscissor->miny = MAX2(cscissor->miny, viewport_bottom);
   viewport_top = MIN2(MAX2(ctx->viewport.top, 0), fb->base.height);
   cscissor->maxy = MIN2(cscissor->maxy, viewport_top);
   if (cscissor->miny > cscissor->maxy)
      cscissor->miny = cscissor->maxy;
}

static bool
ljmgpu_is_scissor_zero(struct ljmgpu_context *ctx)
{
   struct pipe_scissor_state *cscissor = &ctx->clipped_scissor;

   return cscissor->minx == cscissor->maxx || cscissor->miny == cscissor->maxy;
}

static void
ljmgpu_damage_rect_union(struct pipe_scissor_state *rect,
                       unsigned minx, unsigned maxx,
                       unsigned miny, unsigned maxy)
{
   rect->minx = MIN2(rect->minx, minx);
   rect->miny = MIN2(rect->miny, miny);
   rect->maxx = MAX2(rect->maxx, maxx);
   rect->maxy = MAX2(rect->maxy, maxy);
}

static void
ljmgpu_clear(struct pipe_context *pctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
           const union pipe_color_union *color, double depth, unsigned stencil)
{
    LOGI("gallium ljmgpu_clear in\n");
    
    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);
    struct ljmgpu_job *job = ljmgpu_job_get(ctx);
  
    struct ljmgpu_job_clear *clear = &job->clear;
    clear->buffers = buffers;
    if (buffers & PIPE_CLEAR_COLOR0) {
       clear->color_8pc =
          ((uint32_t)float_to_ubyte(color->f[3]) << 24) |
          ((uint32_t)float_to_ubyte(color->f[2]) << 16) |
          ((uint32_t)float_to_ubyte(color->f[1]) << 8) |
          float_to_ubyte(color->f[0]);

       clear->color_16pc =
          ((uint64_t)float_to_ushort(color->f[3]) << 48) |
          ((uint64_t)float_to_ushort(color->f[2]) << 32) |
          ((uint64_t)float_to_ushort(color->f[1]) << 16) |
          float_to_ushort(color->f[0]);
    }
    LOGI("gallium ljmgpu_clear clear->color_8pc[%08x]\n", clear->color_8pc);
    struct ljmgpu_surface *zsbuf = ljmgpu_surface(ctx->framebuffer.base.zsbuf);

   if (buffers & PIPE_CLEAR_DEPTH) {
      clear->depth = util_pack_z(PIPE_FORMAT_Z24X8_UNORM, depth);
      if (zsbuf)
         zsbuf->reload &= ~PIPE_CLEAR_DEPTH;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      clear->stencil = stencil;
      if (zsbuf)
         zsbuf->reload &= ~PIPE_CLEAR_STENCIL;
   }

   //ctx->dirty |= LJMGPU_CONTEXT_DIRTY_CLEAR;

   ljmgpu_damage_rect_union(&job->damage_rect,
                          0, ctx->framebuffer.base.width,
                          0, ctx->framebuffer.base.height);


    /***for bo test start***/
    int size = 1024 * 1024;
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, size, 0);
    char buffer[128] = "ljmgpu ljmgpu_clear bo create inside test";
    memcpy((char *)bo->map, buffer, 50);
    LOGI("gallium bo->map[%s]\n", (char *)bo->map);
    /***for bo test over***/

    ljmgpu_do_job_clear(job);

    LOGI("gallium ljmgpu_clear over\n");
}
static void ljmgpu_prepare_startS_dat(struct ljmgpu_context* ctx)
{

}
static void ljmgpu_prepare_startS_attrib_config_data()
{


}

static void ljmgpu_prepare_pa_dat(struct ljmgpu_context *ctx, const struct pipe_draw_info *info)
{
    ctx->vbo.pa.mode = info->mode;
}

static void ljmgpu_prepare_cull_raster_dat(struct ljmgpu_context *ctx)
{
   int cf = ctx->rasterizer->base.cull_face;
   int ccw = ctx->rasterizer->base.front_ccw;
   uint32_t cull = 0;
   bool force_point_size = false;

   if (cf != PIPE_FACE_NONE) {
      if (cf & PIPE_FACE_FRONT)
         cull |= ccw ? 0x00040000 : 0x00020000;
      if (cf & PIPE_FACE_BACK)
         cull |= ccw ? 0x00020000 : 0x00040000;
   }

}

/// @brief 获取Viewport配置数据.
/// @param pipe_ctx pipe_context
static void ljmgpu_prepare_viewport_dat(struct ljmgpu_context *ctx)
{
    /*ctx->vbo.viewport.near = ctx->st_ctx->viewport.near;
    ctx->vbo.viewport.far = pipe_ctx->st_ctx->viewport.far;
	ctx->vbo.viewport.xy = 0; // reset
	ctx->vbo.viewport.wh = 0;
    ctx->vbo.viewport.xy |= pipe_ctx->st_ctx->viewport.x;
    ctx->vbo.viewport.xy |= pipe_ctx->st_ctx->viewport.y << 16;
    ctx->vbo.viewport.wh |= pipe_ctx->st_ctx->viewport.w;
    ctx->vbo.viewport.wh |= pipe_ctx->st_ctx->viewport.h << 16;

    fui(ctx->viewport.left)
    fui(ctx->viewport.right)
    fui(ctx->viewport.bottom)
    fui(ctx->viewport.top)*/
}
static void ljmgpu_get_texture_block(struct ljmgpu_context* ctx, const struct pipe_draw_info *info)
{
    // after 
}

static unsigned* ljmgpu_prepare_accel_dat(struct ljmgpu_context *ctx, const struct pipe_draw_info *info)
{
    int dword_cnt = 11; // enabled, PA(1), CULL_RASTER(5), VIEWPORT(4)
    ljmgpu_prepare_pa_dat(ctx, info);
    ljmgpu_prepare_cull_raster_dat(ctx);
    ljmgpu_prepare_viewport_dat(ctx);

    unsigned *tmp_data = NULL;
    if (!(tmp_data = (unsigned *)malloc(dword_cnt * sizeof(unsigned)))) {
	    LOGE("In ljmgpu_eg01_draw: failed malloc tmp_data!\n");
	    return NULL;
    }

    *tmp_data = 0x3ff; // fixed enabled (0x000003ff)
    //*tmp_data = 0x1ff; // fixed enabled (0x000001ff)
    memcpy((unsigned *)tmp_data + 1, &ctx->vbo.pa, 1 * sizeof(unsigned)); // PA
    memcpy((unsigned *)tmp_data + 2, &ctx->vbo.cull_raster, 5 * sizeof(unsigned)); // CULL_RASTER
    memcpy((unsigned *)tmp_data + 7, &ctx->vbo.viewport, 4 * sizeof(unsigned)); // VIEWPORT

    return tmp_data;
}
static unsigned* ljmgpu_prepare_texture_dat(struct ljmgpu_context *ctx, const struct pipe_draw_info *info)
{
    // after
    return NULL;
}
static unsigned* ljmgpu_prepare_rop_dat(struct ljmgpu_context *ctx, const struct pipe_draw_info *info)
{
    ctx->vbo.rop.data = (unsigned *)malloc(TMP_MALLOC_SIZE * sizeof(unsigned));
    unsigned* ptr = ctx->vbo.rop.data;
    if (!ptr) {
	    LOGE("In gen_rop_dat: data is NULL!\n");
	    return 0;
    }
	memset(ctx->vbo.rop.data, 0, TMP_MALLOC_SIZE * sizeof(unsigned));
    int dword_cnt = 0;

    return NULL;
}
static unsigned* ljmgpu_prepare_uniform_dat(struct ljmgpu_context *ctx, const struct pipe_draw_info *info)
{
    if (ctx->vbo.uniform.data) {    // and fix it here
	    free(ctx->vbo.uniform.data);
	    ctx->vbo.uniform.data = NULL;
    }


    int cnt = ST_UNIFORM_VALUES_CNT / 32 + ST_UNIFORM_VALUES_CNT; // size of indicators and all uniform data

    struct ljmgpu_bo* bo_uniform = &ctx->vbo.bo_uniform;
    bo_uniform->size = cnt * sizeof(unsigned);
    

    return NULL;
}
static void ljmgpu_get_rop_block(struct ljmgpu_context* ctx, const struct pipe_draw_info *info)
{
    unsigned *new_data = NULL; // Stores head, cnt, data, tail, checksum
    int dword_cnt = 11; // enabled, PA(1), CULL_RASTER(4), VIEWPORT(4), head, cnt, tail, checksum

    unsigned* tmp_data = ljmgpu_prepare_rop_dat(ctx, info);
    if (!tmp_data) {
	    LOGE("In ljmgpu_eg01_get_rop_block: failed to get tmp_data!\n");
	    return;
    }

    if (!(new_data = ljmgpu_vbo_add_head_tail_checksum(tmp_data, dword_cnt, COMMAND_DRAW_HEADER, COMMAND_DRAW_TAIL))) {
	    LOGE("In ljmgpu_eg01_draw: new_data for PA&VIEWPORT is NULL!\n");
	    return;
    }
    dword_cnt += 4;


    ctx->vbo.bo_rop.size = dword_cnt * sizeof(unsigned);
    /*ljmgpu_bo_create_multi(&ctx->vbo.bo_accel);
    memcpy(ctx->vbo.bo_accel.map, new_data, dword_cnt * sizeof(unsigned));
    unsigned int *ptr_dev = ctx->vbo.bo_accel.map;
    printf("---------driver vbo accel----------\n");
    for(int i = 0; i < dword_cnt; i++)
        printf("%d: %p, %08x\n", i, (ptr_dev + i), ptr_dev[i]);
    //ljmgpu_bo_debug(&ctx->vbo.bo_accel);
    */
    free(tmp_data);
    tmp_data = NULL;
    free(new_data);
    new_data = NULL;
}
static void ljmgpu_get_uniform_block(struct ljmgpu_context* ctx, const struct pipe_draw_info *info)
{
    unsigned *new_data = NULL; // Stores head, cnt, data, tail, checksum
    int dword_cnt = 11; // enabled, PA(1), CULL_RASTER(4), VIEWPORT(4), head, cnt, tail, checksum

    unsigned* tmp_data = ljmgpu_prepare_uniform_dat(ctx, info);
    if (!tmp_data) {
	    LOGE("In ljmgpu_eg01_get_rop_block: failed to get tmp_data!\n");
	    return;
    }

    if (!(new_data = ljmgpu_vbo_add_head_tail_checksum(tmp_data, dword_cnt, COMMAND_DRAW_HEADER, COMMAND_DRAW_TAIL))) {
	    LOGE("In ljmgpu_eg01_draw: new_data for PA&VIEWPORT is NULL!\n");
	    return;
    }
    dword_cnt += 4;


    ctx->vbo.bo_uniform.size = dword_cnt * sizeof(unsigned);
    /*ljmgpu_bo_create_multi(&ctx->vbo.bo_accel);
    memcpy(ctx->vbo.bo_accel.map, new_data, dword_cnt * sizeof(unsigned));
    unsigned int *ptr_dev = ctx->vbo.bo_accel.map;
    printf("---------driver vbo accel----------\n");
    for(int i = 0; i < dword_cnt; i++)
        printf("%d: %p, %08x\n", i, (ptr_dev + i), ptr_dev[i]);
    //ljmgpu_bo_debug(&ctx->vbo.bo_accel);
    */
    free(tmp_data);
    tmp_data = NULL;
    free(new_data);
    new_data = NULL;
}
static void ljmgpu_get_accel_block(struct ljmgpu_context* ctx, const struct pipe_draw_info *info)
{
    unsigned *new_data = NULL; // Stores head, cnt, data, tail, checksum
    int dword_cnt = 11; // enabled, PA(1), CULL_RASTER(4), VIEWPORT(4), head, cnt, tail, checksum

    unsigned* tmp_data = ljmgpu_prepare_accel_dat(ctx, info);
    if (!tmp_data) {
	    LOGE("In ljmgpu_eg01_get_accel_block: failed to get tmp_data!\n");
	    return;
    }

    if (!(new_data = ljmgpu_vbo_add_head_tail_checksum(tmp_data, dword_cnt, COMMAND_DRAW_HEADER, COMMAND_DRAW_TAIL))) {
	    LOGE("In ljmgpu_eg01_draw: new_data for PA&VIEWPORT is NULL!\n");
	    return;
    }
    dword_cnt += 4;


    ctx->vbo.bo_accel.size = dword_cnt * sizeof(unsigned);
    /*ljmgpu_bo_create_multi(&ctx->vbo.bo_accel);
    memcpy(ctx->vbo.bo_accel.map, new_data, dword_cnt * sizeof(unsigned));
    unsigned int *ptr_dev = ctx->vbo.bo_accel.map;
    printf("---------driver vbo accel----------\n");
    for(int i = 0; i < dword_cnt; i++)
        printf("%d: %p, %08x\n", i, (ptr_dev + i), ptr_dev[i]);
    //ljmgpu_bo_debug(&ctx->vbo.bo_accel);
    */
    free(tmp_data);
    tmp_data = NULL;
    free(new_data);
    new_data = NULL;
}
static void ljmgpu_get_shader_data(struct ljmgpu_context* ctx)
{
    //copy shader bin to vram
}
static void ljmgpu_get_vbo_data(struct ljmgpu_context* ctx)
{
    //buffer data  buffer sub
}
static void
ljmgpu_draw_vbo(struct pipe_context *pctx,
              const struct pipe_draw_info *info,
              unsigned drawid_offset,
              const struct pipe_draw_indirect_info *indirect,
              const struct pipe_draw_start_count_bias *draws,
              unsigned num_draws)
{
    LOGI("gallium ljm_draw_vbo in\n");

    struct ljmgpu_context *ctx = ljmgpu_context(pctx);
    struct ljmgpu_screen *screen = ljmgpu_screen(ctx->base.screen);

    int size = 1024 * 1024;
    struct ljmgpu_bo *bo = ljmgpu_bo_create_multi(screen, size, 0);
    char buffer[128] = "ljmgpu ljmgpu_draw_vbo bo create inside test";
    memcpy((char *)bo->map, buffer, 50);
    LOGI("gallium bo->map[%s]\n", (char *)bo->map);

	ljmgpu_prepare_startS_dat(ctx);

	ljmgpu_prepare_startS_attrib_config_data();

    ljmgpu_get_accel_block(ctx, info);
    ljmgpu_get_texture_block(ctx, info);
    ljmgpu_get_rop_block(ctx, info);
    ljmgpu_get_uniform_block(ctx, info);

    ljmgpu_get_vbo_data(ctx);
    ljmgpu_get_shader_data(ctx);
#if 0

    /*------------------------------ Draw命令包 ------------------------------*/
    driver_memblk *buf = &driver_ctx->command.cmd_buffers.command_draw_buf;
    driver_memblk *packet = &driver_ctx->command.cmd_packets.command_draw_packet;
    ljmgpu_driver_prepare_draw_command_data();

    ljmgpu_driver_get_draw_command_buffer();
    ljmgpu_driver_get_draw_command_packet();
    ljmgpu_driver_get_draw_packet_into_memory_block();
#endif
    LOGI("gallium ljm_draw_vbo over\n");
}

void
ljmgpu_draw_init(struct ljmgpu_context *ctx)
{
   ctx->base.clear = ljmgpu_clear;
   ctx->base.draw_vbo = ljmgpu_draw_vbo;
}
