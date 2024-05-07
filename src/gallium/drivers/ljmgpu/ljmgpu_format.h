#ifndef H_LJMGPU_FORMAT
#define H_LJMGPU_FORMAT

#include <stdbool.h>

#include <pipe/p_format.h>

bool ljmgpu_format_texel_supported(enum pipe_format f);
bool ljmgpu_format_pixel_supported(enum pipe_format f);
int ljmgpu_format_get_texel(enum pipe_format f);
int ljmgpu_format_get_pixel(enum pipe_format f);
int ljmgpu_format_get_texel_reload(enum pipe_format f);
bool ljmgpu_format_get_texel_swap_rb(enum pipe_format f);
bool ljmgpu_format_get_pixel_swap_rb(enum pipe_format f);
const uint8_t *ljmgpu_format_get_texel_swizzle(enum pipe_format f);
uint32_t ljmgpu_format_get_channel_layout(enum pipe_format f);

#endif

