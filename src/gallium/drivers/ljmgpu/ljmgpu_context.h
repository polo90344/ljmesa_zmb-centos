#ifndef H_LJMGPU_CONTEXT
#define H_LJMGPU_CONTEXT

#include "util/list.h"
#include "util/slab.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "ljmgpu_bo.h"
#include "drm-uapi/ljmgpu_drm.h"

#define TMP_MALLOC_SIZE 100
#define COMMAND_DRAW_HEADER 0xffffffff
#define COMMAND_CLEAR_HEADER 0xffff0000
#define COMMAND_DRAW_TAIL 0xf0f0f0f0
#define COMMAND_CLEAR_TAIL 0xf0f0f0f0
#define ST_UNIFORM_VALUES_CNT ES2_MAX_UNIFORMS*16

#define ES2_MAX_ATTRIBUTES 8 // 16 ?
#define ES2_MAX_UNIFORMS 128
#define ES2_MAX_TEXTURE_OBJS 256
#define ES2_MAX_BUFFER_OBJS 64
#define ES2_MAX_PROGRAM_OBJS 4


// Object name-index map
// name zero for TEXTURE_2D: textures[0]
// name zero for TEXTURE_CUBE_MAP: textures[1]
// ES2 generated name for both: textures[2..255]
#define idxTEX(name)    ((name==0 && target == GL_TEXTURE_2D) ? name : name + 1)
#define idxTEX1(target,name) ((name == 0 && target == GL_TEXTURE_2D) ? name : name + 1)
#define idxOBJ(name)    name - 1 // objects[0..(max-1)]
#define idxBUF(name)    name - 1 // buffers[0..63]
#define idxPRG(name)    name - 1 // programs[0..3]
#define idxSHD(name)    name - 1 // shaders[0..7]

typedef struct {
    unsigned idxsztpnmlz;   ///< index, size, type, normalized
    unsigned stride;        ///< stride
    unsigned pointer;       ///< pointer
} ljmgpu_v_attribs; ///< 属性数组状态

typedef struct {
    unsigned mode;          ///< Draw/结束标记/type
    unsigned indices;       ///< first(DrawArrays), indices(DrawElements)
    unsigned count;         ///< count
} ljmgpu_draw; ///< draw参数

typedef struct {
    unsigned cnt;           ///< 包含DWORD数据个数
    unsigned addr_enabled;  ///< 地址配置使能标记   (enabled1)
    unsigned addr_v_shader; ///< 顶点染色程序地址   (0x00100000)
    unsigned addr_v_uniform; ///< 顶点uniform变量地址(0x00300000+offset)
    unsigned addr_attrib;   ///< 顶点属性数组缓存   (0x00500000)
    unsigned addr_accel;    ///< 加速单元状态表地址 (0x00300000)
    unsigned addr_f_shader; ///< 像素染色程序地址   (0x00200000)
    unsigned addr_texture;  ///< 纹理状态表地址     (0x00400000+offset)
    unsigned addr_f_uniform; // unused
    unsigned addr_rop;      ///< ROP状态表地址      (0x00400000+offset)
    unsigned attrib_draw_enabled; ///< 属性和Draw使能标记(st[CAT_VTX].enabled)
    ljmgpu_v_attribs attribs[ES2_MAX_ATTRIBUTES]; ///< 属性参数
    ljmgpu_draw draws;             ///< draw参数
    //                      // 页清除(unused)
} ljmgpu_command_draw; ///< draw command 有效数据
typedef struct {
    unsigned enabled;       ///< 使能表 （第31bit标识Clear/Flush命令）
    unsigned addr_clear;    ///< 缓冲区整体ROP状态：0x00400000+offset
} ljmgpu_command_clear; ///< clear command 有效数据
typedef struct {
    void *data; ///< 缓存数据块
    int cnt; ///< 缓存数据DWORD数
} ljmgpu_memblk; ///< memory block buffer raw data or packet

typedef struct {
    ljmgpu_memblk command_draw_buf;   ///< draw command buffer
    ljmgpu_memblk command_clear_buf;  ///< clear command buffer
} ljmgpu_command_buffers;  ///< raw command data

typedef struct {
    ljmgpu_memblk command_draw_packet;    ///< draw command packets
    ljmgpu_memblk command_clear_packet;   ///< clear command packets
} ljmgpu_command_packets; ///< packed command data packets
typedef struct {
    ljmgpu_command_draw cmd_draw_dat; ///< command draw raw data
    ljmgpu_command_clear cmd_clear_dat; ///< command clear raw data
    ljmgpu_command_buffers cmd_buffers; ///< command raw data
    ljmgpu_command_packets cmd_packets; ///< command packet(with header, cnt, tail, checksums)
} ljmgpu_command_block; ///< command data buffers and packets
//----------------------------------------------------------------------------
//                              eg01 vbo packets
//----------------------------------------------------------------------------

typedef struct {
    //unsigned api_name; ///< glDrawArrays, glDrawElements
    unsigned mode; ///< GL_POINTS, GL_LINES, GL_TRIANGLES, etc.
} ljmgpu_vbo_pa; ///< pa raw data

typedef struct {
    unsigned culled_dir_mode;
    unsigned fill;
    unsigned line_width;
    unsigned factor;
    unsigned units;
} ljmgpu_vbo_cull_raster; ///< cull, linewidth, polygonoffset raw data

typedef struct {
    unsigned near;
    unsigned far;
    unsigned xy;
    unsigned wh;
} ljmgpu_vbo_viewport; ///< viewport raw data
typedef struct {
    void *data; // gpu ram space
    unsigned size; // buffer size (in unit of byte)
    unsigned cnt; // buffer blocks (dword_cnt)
} ljmgpu_vbo_buffer; ///< array buffers data

typedef struct {
    void *data; // gpu ram space
    unsigned size; // total size of all texture image data
    unsigned cnt; // (unused)offset count below (index)
    //unsigned offsets[ES2_MAX_TEXTURE_UNITS]; // byte cnt for each texture image data
} ljmgpu_vbo_texture_image; ///< a texture object image pixels data
typedef struct {
    // vbo_attrib  attrib;  // VertexAttribPointer does the job
    ljmgpu_vbo_pa pa;
    ljmgpu_vbo_cull_raster cull_raster;
    ljmgpu_vbo_viewport viewport;
    ljmgpu_vbo_buffer buffers[ES2_MAX_BUFFER_OBJS]; // buffers in GPU memory
    ljmgpu_memblk clear;
    ljmgpu_memblk rop;
    ljmgpu_memblk texture; // MAX_UNITS or MAX_OBJS?
    ljmgpu_memblk uniform;
    //vbo_texture_image texture_image; // combine all texture images in texture object
    ljmgpu_vbo_texture_image texture_images[ES2_MAX_TEXTURE_OBJS]; // combine all texture images in texture object
                                                            // cubemap texture images are also stored in one slot
                                                            // fetch those images/mipmaps use offsets


    struct ljmgpu_bo bo_accel;
    struct ljmgpu_bo bo_buffers[ES2_MAX_BUFFER_OBJS]; // bo buffer
    struct ljmgpu_bo bo_clear; // clear & flush
    struct ljmgpu_bo bo_rop;
    struct ljmgpu_bo bo_texture;
    struct ljmgpu_bo bo_uniform;
    struct ljmgpu_bo bo_texture_images[ES2_MAX_TEXTURE_OBJS];
    struct ljmgpu_bo bo_prog_shd_bins[ES2_MAX_PROGRAM_OBJS][2];
    ljmgpu_memblk v_bin;
    ljmgpu_memblk f_bin;
} ljmgpu_vbo_block; ///< gpu ram data
//----------------------------------------------------------------------------
//                              memory blocks
//----------------------------------------------------------------------------

typedef struct {
    int offset; // starting offset in ADDR
    int dword_cnt;
} ljmgpu_mem_offset; // block info in each ADDR(_memblk)

typedef ljmgpu_memblk mem_command;
typedef ljmgpu_memblk mem_vbo;

 // Offset for addressing. (in unit of dword)
typedef struct {
    ljmgpu_mem_offset DRIVER_OFS[ES2_MAX_TEXTURE_OBJS];
    int cnt; // texture image count
} ljmgpu_ofs_texture;

extern ljmgpu_mem_offset DRIVER_OFS_ACCEL;
extern ljmgpu_mem_offset DRIVER_OFS_UNIFORM;
extern ljmgpu_ofs_texture DRIVER_OFS_TEXTURES; // texture image offset(s)
extern ljmgpu_mem_offset DRIVER_OFS_TEXTURE; // texture state offset
extern ljmgpu_mem_offset DRIVER_OFS_F_UNIFORM; // (unused)
extern ljmgpu_mem_offset DRIVER_OFS_ROP;
extern ljmgpu_mem_offset DRIVER_OFS_CLEAR;
extern ljmgpu_mem_offset DRIVER_OFS_BUFFERS[ES2_MAX_BUFFER_OBJS];
#define ES2_MAX_ATTRIBUTES 8 // 16 ?
#define ES2_MAX_UNIFORMS 128
#define ES2_MAX_TEXTURE_OBJS 256
#define ES2_MAX_BUFFER_OBJS 64


struct ljmgpu_context_framebuffer {
   struct pipe_framebuffer_state base;
   int tiled_w, tiled_h;
   int shift_w, shift_h;
   int block_w, block_h;
   int shift_min;
};

struct ljmgpu_depth_stencil_alpha_state {
   struct pipe_depth_stencil_alpha_state base;
};

struct ljmgpu_fs_compiled_shader {
   struct ljmgpu_bo *bo;
   void *shader;
   struct {
      int shader_size;
      int stack_size;
      bool uses_discard;
   } state;
};

struct ljmgpu_fs_uncompiled_shader {
   struct pipe_shader_state base;
   unsigned char nir_sha1[20];
};

struct ljmgpu_fs_key {
   unsigned char nir_sha1[20];
   struct {
      uint8_t swizzle[4];
   } tex[PIPE_MAX_SAMPLERS];
};

#define LIMA_MAX_VARYING_NUM 13

struct ljmgpu_varying_info {
   int components;
   int component_size;
   int offset;
};

struct ljmgpu_vs_compiled_shader {
   struct ljmgpu_bo *bo;
   void *shader;
   void *constant;
   struct {
      int shader_size;
      int prefetch;
      int uniform_size;
      int constant_size;
      struct ljmgpu_varying_info varying[LIMA_MAX_VARYING_NUM];
      int varying_stride;
      int num_outputs;
      int num_varyings;
      int gl_pos_idx;
      int point_size_idx;
   } state;
};

struct ljmgpu_vs_uncompiled_shader {
   struct pipe_shader_state base;
   unsigned char nir_sha1[20];
};

struct ljmgpu_vs_key {
   unsigned char nir_sha1[20];
};

struct ljmgpu_rasterizer_state {
   struct pipe_rasterizer_state base;
};

struct ljmgpu_blend_state {
   struct pipe_blend_state base;
};

struct ljmgpu_vertex_element_state {
   struct pipe_vertex_element pipe[PIPE_MAX_ATTRIBS];
   unsigned num_elements;
};

struct ljmgpu_context_vertex_buffer {
   struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
   unsigned count;
   uint32_t enabled_mask;
};

struct ljmgpu_context_viewport_state {
   struct pipe_viewport_state transform;
   float left, right, bottom, top;
   float near, far;
};

struct ljmgpu_context_constant_buffer {
   const void *buffer;
   uint32_t size;
   bool dirty;
};

enum ljmgpu_ctx_buff {
   ljmgpu_ctx_buff_gp_varying_info,
   ljmgpu_ctx_buff_gp_attribute_info,
   ljmgpu_ctx_buff_gp_uniform,
   ljmgpu_ctx_buff_pp_plb_rsw,
   ljmgpu_ctx_buff_pp_uniform_array,
   ljmgpu_ctx_buff_pp_uniform,
   ljmgpu_ctx_buff_pp_tex_desc,
   ljmgpu_ctx_buff_num,
   ljmgpu_ctx_buff_num_gp = ljmgpu_ctx_buff_pp_plb_rsw,
};

struct ljmgpu_ctx_buff_state {
   struct pipe_resource *res;
   unsigned offset;
   unsigned size;
};

struct ljmgpu_texture_stateobj {
   struct pipe_sampler_view *textures[PIPE_MAX_SAMPLERS];
   unsigned num_textures;
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   unsigned num_samplers;
};

struct ljmgpu_ctx_plb_pp_stream_key {
   uint16_t plb_index;
   /* Coordinates are in tiles */
   uint16_t minx, miny, maxx, maxy;
   /* FB params */
   uint16_t shift_w, shift_h;
   uint16_t block_w, block_h;
};

struct ljmgpu_ctx_plb_pp_stream {
   struct list_head lru_list;
   struct ljmgpu_ctx_plb_pp_stream_key key;
   struct ljmgpu_bo *bo;
   uint32_t offset[8];
};

struct ljmgpu_pp_stream_state {
   void *map;
   uint32_t va;
   uint32_t offset[8];
};
struct ljmgpu_fb_clear {
    unsigned enable;
	unsigned _reserved_zero;        // enable bit 0     ///< 0 as reserved
    unsigned color_base_addr;       // enable bit 1     ///< 颜色缓存地址
    unsigned depth_base_addr;       // enable bit 2     ///< 深度缓存地址
    unsigned frame_width:   16;     // enable bit 3     ///< 视口宽度
    unsigned frame_height:  16;                         ///< 视口高度
    unsigned clear_red:     8;      // enable bit 4     ///< 设置帧缓存Red清除值，默认值0
    unsigned clear_green:   8;                          ///< 设置帧缓存Green清除值，默认值0
    unsigned clear_blue:    8;                          ///< 设置帧缓存Blue清除值，默认值0
    unsigned clear_alpha:   8;                          ///< 设置帧帧缓存Alpha清除值，默认值1
    unsigned clear_depth:  24;      // enable bit 5     ///< 设置帧缓存深度缓存清除值，默认值1
    unsigned clear_stencil:       8;      // (s - stencil)    ///< 设置帧缓存模板缓存清楚值，默认值0
    unsigned mask;                  // enable bit 6     ///< 设置帧缓存清除mask位
    unsigned flush;                 // enable bit 7     ///< 设置flush命令状态数据为1
    // flush will use enabled bit(bit 7) only

    uint32_t color_8pc;
    uint64_t color_16pc;
    unsigned buffers;
};   // non-pixel related whole framebuffer state

struct ljmgpu_context {
   struct pipe_context base;

   enum {
      EGP_CONTEXT_DIRTY_FRAMEBUFFER  = (1 << 0),
      EGP_CONTEXT_DIRTY_CLEAR        = (1 << 1),
      EGP_CONTEXT_DIRTY_COMPILED_VS  = (1 << 2),
      EGP_CONTEXT_DIRTY_COMPILED_FS  = (1 << 3),
      EGP_CONTEXT_DIRTY_VERTEX_ELEM  = (1 << 4),
      EGP_CONTEXT_DIRTY_VERTEX_BUFF  = (1 << 5),
      EGP_CONTEXT_DIRTY_VIEWPORT     = (1 << 6),
      EGP_CONTEXT_DIRTY_SCISSOR      = (1 << 7),
      EGP_CONTEXT_DIRTY_RASTERIZER   = (1 << 8),
      EGP_CONTEXT_DIRTY_ZSA          = (1 << 9),
      EGP_CONTEXT_DIRTY_BLEND_COLOR  = (1 << 10),
      EGP_CONTEXT_DIRTY_BLEND        = (1 << 11),
      EGP_CONTEXT_DIRTY_STENCIL_REF  = (1 << 12),
      EGP_CONTEXT_DIRTY_CONST_BUFF   = (1 << 13),
      EGP_CONTEXT_DIRTY_TEXTURES     = (1 << 14),
      EGP_CONTEXT_DIRTY_CLIP         = (1 << 15),
      EGP_CONTEXT_DIRTY_UNCOMPILED_VS = (1 << 16),
      EGP_CONTEXT_DIRTY_UNCOMPILED_FS = (1 << 17),
   } dirty;

   struct u_upload_mgr *uploader;
   struct blitter_context *blitter;

   struct slab_child_pool transfer_pool;

   struct ljmgpu_context_framebuffer framebuffer;
   struct ljmgpu_context_viewport_state viewport;
   struct pipe_scissor_state scissor;
   struct pipe_scissor_state clipped_scissor;
   struct ljmgpu_vs_compiled_shader *vs;
   struct ljmgpu_fs_compiled_shader *fs;
   struct ljmgpu_vs_uncompiled_shader *uncomp_vs;
   struct ljmgpu_fs_uncompiled_shader *uncomp_fs;
   struct ljmgpu_vertex_element_state *vertex_elements;
   struct ljmgpu_context_vertex_buffer vertex_buffers;
   struct ljmgpu_rasterizer_state *rasterizer;
   struct ljmgpu_depth_stencil_alpha_state *zsa;
   struct pipe_blend_color blend_color;
   struct ljmgpu_blend_state *blend;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct ljmgpu_context_constant_buffer const_buffer[PIPE_SHADER_TYPES];
   struct ljmgpu_texture_stateobj tex_stateobj;
   struct ljmgpu_pp_stream_state pp_stream;
   struct ljmgpu_fb_clear fb_clear;

   unsigned min_index;
   unsigned max_index;

   #define LIMA_CTX_PLB_MIN_NUM  1
   #define LIMA_CTX_PLB_MAX_NUM  4
   #define LIMA_CTX_PLB_DEF_NUM  2
   #define LIMA_CTX_PLB_BLK_SIZE 512
   unsigned plb_size;
   unsigned plb_gp_size;

   struct ljmgpu_bo *plb[LIMA_CTX_PLB_MAX_NUM];
   struct ljmgpu_bo *gp_tile_heap[LIMA_CTX_PLB_MAX_NUM];
   uint32_t gp_tile_heap_size;
   struct ljmgpu_bo *plb_gp_stream;
   struct ljmgpu_bo *gp_output;
   uint32_t gp_output_varyings_offt;
   uint32_t gp_output_point_size_offt;

   struct hash_table *plb_pp_stream;
   struct list_head plb_pp_stream_lru_list;
   uint32_t plb_index;
   size_t plb_stream_cache_size;

   struct hash_table *fs_cache;
   struct hash_table *vs_cache;

   struct ljmgpu_ctx_buff_state buffer_state[ljmgpu_ctx_buff_num];

   /* current job */
   struct ljmgpu_job *job;

   /* map from ljmgpu_job_key to ljm_job */
   struct hash_table *jobs;

   /* map from pipe_resource to ljmgpu_job which write to it */
   struct hash_table *write_jobs;

   int in_sync_fd;
   uint32_t in_sync;
   uint32_t out_sync;

   uint32_t syncobj;

   int id;

   struct pipe_debug_callback debug;

   unsigned index_offset;
   struct ljmgpu_resource *index_res;

    ljmgpu_command_block command; ///< packets ready for dispatching
    ljmgpu_vbo_block vbo; ///< raw data ready for packeting and dispatching
};

static inline struct ljmgpu_context *
ljmgpu_context(struct pipe_context *pctx)
{
   return (struct ljmgpu_context *)pctx;
}

struct ljmgpu_sampler_state {
   struct pipe_sampler_state base;
};

static inline struct ljmgpu_sampler_state *
ljmgpu_sampler_state(struct pipe_sampler_state *psstate)
{
   return (struct ljmgpu_sampler_state *)psstate;
}

struct ljmgpu_sampler_view {
   struct pipe_sampler_view base;
   uint8_t swizzle[4];
};

static inline struct ljmgpu_sampler_view *
ljmgpu_sampler_view(struct pipe_sampler_view *psview)
{
   return (struct ljmgpu_sampler_view *)psview;
}

uint32_t ljmgpu_ctx_buff_va(struct ljmgpu_context *ctx, enum ljmgpu_ctx_buff buff);
void *ljmgpu_ctx_buff_map(struct ljmgpu_context *ctx, enum ljmgpu_ctx_buff buff);
void *ljmgpu_ctx_buff_alloc(struct ljmgpu_context *ctx, enum ljmgpu_ctx_buff buff,
                          unsigned size);

void ljmgpu_state_init(struct ljmgpu_context *ctx);
void ljmgpu_state_fini(struct ljmgpu_context *ctx);
void ljmgpu_draw_init(struct ljmgpu_context *ctx);
void ljmgpu_program_init(struct ljmgpu_context *ctx);
void ljmgpu_program_fini(struct ljmgpu_context *ctx);
void ljmgpu_query_init(struct ljmgpu_context *ctx);

struct pipe_context *
ljmgpu_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags);

void ljmgpu_flush(struct ljmgpu_context *ctx);
void ljmgpu_flush_job_accessing_bo(
   struct ljmgpu_context *ctx, struct ljmgpu_bo *bo, bool write);
void ljmgpu_flush_previous_job_writing_resource(
   struct ljmgpu_context *ctx, struct pipe_resource *prsc);

int ljmgpu_dispatch_command(void * cmd, int cnt, int is_clear);

#endif

