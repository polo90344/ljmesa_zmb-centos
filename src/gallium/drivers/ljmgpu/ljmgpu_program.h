#ifndef H_LJMGPU_PROGRAM
#define H_LJMGPU_PROGRAM

#include "pipe/p_defines.h"

const void *ljmgpu_program_get_compiler_options(enum pipe_shader_type shader);

bool ljmgpu_update_vs_state(struct ljmgpu_context *ctx);
bool ljmgpu_update_fs_state(struct ljmgpu_context *ctx);
struct nir_shader;

void ljmgpu_program_optimize_vs_nir(struct nir_shader *s);

struct nir_lower_tex_options;
void ljmgpu_program_optimize_fs_nir(struct nir_shader *s,
                             struct nir_lower_tex_options *tex_options);

#endif

