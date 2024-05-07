
files_egp = files(
  'egp_screen.c',
)

libegp = static_library(
  'egp',
  files_egp,
  include_directories : [
    inc_src, inc_include, inc_gallium, inc_gallium_aux, inc_gallium_drivers
  ],
  dependencies : [dep_libdrm,idep_mesautil],
)

driver_egp = declare_dependency(
  compile_args : '-DGALLIUM_EGP',
  link_with : [libegp, libegpwinsys],
)
