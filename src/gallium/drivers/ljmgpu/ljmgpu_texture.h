#ifndef H_LJMGPU_TEXTURE
#define H_LJMGPU_TEXTURE

#define ljmgpu_min_tex_desc_size 64

#define LIMA_TEXTURE_TYPE_2D   2
#define LIMA_TEXTURE_TYPE_CUBE 5

typedef struct __attribute__((__packed__)) {
   /* Word 0 */
   uint32_t format : 6;
   uint32_t flag1: 1;
   uint32_t swap_r_b: 1;
   uint32_t unknown_0_1: 8;
   uint32_t stride: 15;
   uint32_t unknown_0_2: 1;

   /* Word 1-3 */
   uint32_t unknown_1_1: 7;
   uint32_t unnorm_coords: 1;
   uint32_t unknown_1_2: 1;
   uint32_t texture_type: 3;
   uint32_t min_lod: 8; /* Fixed point, 4.4, unsigned */
   uint32_t max_lod: 8; /* Fixed point, 4.4, unsigned */
   uint32_t lod_bias: 9; /* Fixed point, signed, 1.4.4 */
   uint32_t unknown_2_1: 3;
   uint32_t has_stride: 1;
   uint32_t min_mipfilter_2: 2; /* 0x3 for linear, 0x0 for nearest */
   uint32_t min_img_filter_nearest: 1;
   uint32_t mag_img_filter_nearest: 1;
   uint32_t wrap_s_clamp_to_edge: 1;
   uint32_t wrap_s_clamp: 1;
   uint32_t wrap_s_mirror_repeat: 1;
   uint32_t wrap_t_clamp_to_edge: 1;
   uint32_t wrap_t_clamp: 1;
   uint32_t wrap_t_mirror_repeat: 1;
   uint32_t unknown_2_2: 3;
   uint32_t width: 13;
   uint32_t height: 13;
   uint32_t unknown_3_1: 1;
   uint32_t unknown_3_2: 15;

   /* Word 4 */
   uint32_t unknown_4;

   /* Word 5 */
   uint32_t unknown_5;

   /* Word 6-15 */
   /* layout is in va[0] bit 13-14 */
   /* VAs start in va[0] at bit 30, each VA is 26 bits (only MSBs are stored), stored
    * linearly in memory */
   union {
      uint32_t va[0];
      struct __attribute__((__packed__)) {
         uint32_t unknown_6_1: 13;
         uint32_t layout: 2;
         uint32_t unknown_6_2: 9;
         uint32_t unknown_6_3: 6;
#define VA_BIT_OFFSET 30
#define VA_BIT_SIZE 26
         uint32_t va_0: VA_BIT_SIZE;
         uint32_t va_0_1: 8;
         uint32_t va_1_x[0];
      } va_s;
   };
} ljmgpu_tex_desc;

void ljmgpu_texture_desc_set_res(struct ljmgpu_context *ctx, ljmgpu_tex_desc *desc,
                               struct pipe_resource *prsc,
                               unsigned first_level, unsigned last_level,
                               unsigned first_layer);
void ljmgpu_update_textures(struct ljmgpu_context *ctx);


static inline int16_t ljmgpu_float_to_fixed8(float f)
{
   return (int)(f * 16.0);
}

static inline float ljmgpu_fixed8_to_float(int16_t i)
{
   float sign = 1.0;

   if (i > 0xff) {
      i = 0x200 - i;
      sign = -1;
   }

   return sign * (float)(i / 16.0);
}

#endif

