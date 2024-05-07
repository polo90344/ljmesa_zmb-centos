#ifndef H_LJMGPU_RESOURCE
#define H_LJMGPU_RESOURCE

#include "pipe/p_state.h"

/* max texture size is 4096x4096 */
#define LJMGPU_MAX_MIP_LEVELS 13
#define LAYOUT_CONVERT_THRESHOLD 8
struct ljmgpu_screen;

struct ljmgpu_resource_level {
   uint32_t width;
   uint32_t stride;
   uint32_t offset;
   uint32_t layer_stride;
};

struct ljmgpu_damage_region {
   struct pipe_scissor_state *region;
   struct pipe_scissor_state bound;
   unsigned num_region;
   bool aligned;
};

struct ljmgpu_resource {
   struct pipe_resource base;

   struct ljmgpu_damage_region damage;
   struct renderonly_scanout *scanout;
   struct ljmgpu_bo *bo;
   bool tiled;
   bool modifier_constant;
   unsigned full_updates;
   struct ljmgpu_resource_level levels[LJMGPU_MAX_MIP_LEVELS];
};

struct ljmgpu_surface {
   struct pipe_surface base;
   int tiled_w, tiled_h;
   unsigned reload;
};

struct ljmgpu_transfer {
   struct pipe_transfer base;
   void *staging;
};

static inline struct ljmgpu_resource *
ljmgpu_resource(struct pipe_resource *res)
{
   return (struct ljmgpu_resource *)res;
}

static inline struct ljmgpu_surface *
ljmgpu_surface(struct pipe_surface *surf)
{
   return (struct ljmgpu_surface *)surf;
}

static inline struct ljmgpu_transfer *
ljmgpu_transfer(struct pipe_transfer *trans)
{
   return (struct ljmgpu_transfer *)trans;
}

void
ljmgpu_resource_screen_init(struct ljmgpu_screen *screen);

void
ljmgpu_resource_context_init(struct ljmgpu_context *ctx);

#endif
