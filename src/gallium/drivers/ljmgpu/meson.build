
files_ljmgpu = files(
  'ljmgpu_screen.c',
  'ljmgpu_screen.h',
  'ljmgpu_context.c',
  'ljmgpu_context.h',
  'ljmgpu_resource.c',
  'ljmgpu_resource.h',
  'ljmgpu_state.c',
  'ljmgpu_disk_cache.c',
  'ljmgpu_disk_cache.h',
  'ljmgpu_draw.c',
  'ljmgpu_program.c',
  'ljmgpu_query.c',
  'ljmgpu_bo.c',
  'ljmgpu_bo.h',
  'ljmgpu_job.c',
  'ljmgpu_job.h',
  'ljmgpu_util.c',
  'ljmgpu_util.h',
  'ljmgpu_texture.c',
  'ljmgpu_texture.h',
  'ljmgpu_fence.c',
  'ljmgpu_fence.h',
  'ljmgpu_format.h',
  'ljmgpu_format.c',
  'ljmgpu_gpu.c',
  'ljmgpu_gpu.h',
  'ljmgpu_log.c',
  'ljmgpu_log.h',
  'ljmgpu_dump.c',
  'ljmgpu_dump.h',
  'ljmgpu_dispatch.c',
)

libljmgpu = static_library(
  'ljmgpu',
  files_ljmgpu,
  include_directories : [
    inc_src, inc_include, inc_gallium, inc_gallium_aux, inc_gallium_drivers
  ],
  dependencies : [dep_libdrm,idep_mesautil],
)

driver_ljmgpu = declare_dependency(
  compile_args : '-DGALLIUM_LJMGPU',
  link_with : [libljmgpu, libljmgpuwinsys],
)
