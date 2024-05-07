#include "util/u_debug.h"

#include "ljmgpu_context.h"

struct ljmgpu_query
{
   uint8_t pad;
};

static struct pipe_query *
ljmgpu_create_query(struct pipe_context *ctx, unsigned query_type, unsigned index)
{
    return NULL;
}

static void
ljmgpu_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
}

static bool
ljmgpu_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
    return true;
}

static bool
ljmgpu_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
    return true;
}

static bool
ljmgpu_get_query_result(struct pipe_context *ctx, struct pipe_query *query,
                     bool wait, union pipe_query_result *vresult)
{
    return true;
}

static void
ljmgpu_set_active_query_state(struct pipe_context *pipe, bool enable)
{

}

void
ljmgpu_query_init(struct ljmgpu_context *pctx)
{
   pctx->base.create_query = ljmgpu_create_query;
   pctx->base.destroy_query = ljmgpu_destroy_query;
   pctx->base.begin_query = ljmgpu_begin_query;
   pctx->base.end_query = ljmgpu_end_query;
   pctx->base.get_query_result = ljmgpu_get_query_result;
   pctx->base.set_active_query_state = ljmgpu_set_active_query_state;
}


