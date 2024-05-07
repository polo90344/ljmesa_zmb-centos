#include "util/u_memory.h"
#include "util/ralloc.h"
#include "util/u_debug.h"

//#include "tgsi/tgsi_dump.h"
//#include "compiler/nir/nir.h"
//#include "compiler/nir/nir_serialize.h"
//#include "nir/tgsi_to_nir.h"

#include "pipe/p_state.h"
#include "stdio.h"

#include "ljmgpu_screen.h"
#include "ljmgpu_context.h"
#include "ljmgpu_job.h"
#include "ljmgpu_program.h"
#include "ljmgpu_bo.h"
#include "ljmgpu_disk_cache.h"
#include "ljmgpu_log.h"

//#include "ir/ljmgpu_ir.h"

const void *
ljmgpu_program_get_compiler_options(enum pipe_shader_type shader)
{
    return NULL;
}

static int
type_size(const struct glsl_type *type, bool bindless)
{
    return 0;
}

void
ljmgpu_program_optimize_vs_nir(struct nir_shader *s)
{
}

void
ljmgpu_program_optimize_fs_nir(struct nir_shader *s,
        struct nir_lower_tex_options *tex_options)
{
}

static bool
ljmgpu_fs_compile_shader(struct ljmgpu_context *ctx,
        struct ljmgpu_fs_key *key,
        struct ljmgpu_fs_uncompiled_shader *ufs,
        struct ljmgpu_fs_compiled_shader *fs)
{
    return true;
}

static bool
ljmgpu_fs_upload_shader(struct ljmgpu_context *ctx,
        struct ljmgpu_fs_compiled_shader *fs)
{
    return true;
}

static struct ljmgpu_fs_compiled_shader *
ljmgpu_get_compiled_fs(struct ljmgpu_context *ctx,
        struct ljmgpu_fs_uncompiled_shader *ufs,
        struct ljmgpu_fs_key *key)
{
    return NULL;
}

static void *
ljmgpu_create_vs_state(struct pipe_context *pctx,
        const struct pipe_shader_state *cso)
{
    LOGI("gallium create_vs_state\n");
    return NULL;
}

static void *
ljmgpu_create_fs_state(struct pipe_context *pctx,
        const struct pipe_shader_state *cso)
{
    LOGI("gallium create_fs_state\n");
    return NULL;
}

static void
ljmgpu_bind_fs_state(struct pipe_context *pctx, void *hwcso)
{
}

static void
ljmgpu_delete_fs_state(struct pipe_context *pctx, void *hwcso)
{
}

static bool
ljmgpu_vs_compile_shader(struct ljmgpu_context *ctx,
        struct ljmgpu_vs_key *key,
        struct ljmgpu_vs_uncompiled_shader *uvs,
        struct ljmgpu_vs_compiled_shader *vs)
{
    return true;
}

static bool
ljmgpu_vs_upload_shader(struct ljmgpu_context *ctx,
        struct ljmgpu_vs_compiled_shader *vs)
{
    return true;
}

static struct ljmgpu_vs_compiled_shader *
ljmgpu_get_compiled_vs(struct ljmgpu_context *ctx,
        struct ljmgpu_vs_uncompiled_shader *uvs,
        struct ljmgpu_vs_key *key)
{
    return NULL;
}

bool
ljmgpu_update_vs_state(struct ljmgpu_context *ctx)
{

    return true;
}

bool
ljmgpu_update_fs_state(struct ljmgpu_context *ctx)
{
    return true;
}

static void
ljmgpu_bind_vs_state(struct pipe_context *pctx, void *hwcso)
{
}

static void
ljmgpu_delete_vs_state(struct pipe_context *pctx, void *hwcso)
{
}

static uint32_t
ljmgpu_fs_cache_hash(const void *key)
{
    return 0;
}

static uint32_t
ljmgpu_vs_cache_hash(const void *key)
{
    return 0;
}

static bool
ljmgpu_fs_cache_compare(const void *key1, const void *key2)
{
    return true;
}

static bool
ljmgpu_vs_cache_compare(const void *key1, const void *key2)
{
    return true;
}

void
ljmgpu_program_init(struct ljmgpu_context *ctx)
{
    LOGI("gallium ljmgpu_program_init in\n");
    ctx->base.create_fs_state = ljmgpu_create_fs_state;
    ctx->base.bind_fs_state = ljmgpu_bind_fs_state;
    ctx->base.delete_fs_state = ljmgpu_delete_fs_state;

    ctx->base.create_vs_state = ljmgpu_create_vs_state;
    ctx->base.bind_vs_state = ljmgpu_bind_vs_state;
    ctx->base.delete_vs_state = ljmgpu_delete_vs_state;
}

void ljmgpu_program_fini(struct ljmgpu_context *ctx)
{
}

