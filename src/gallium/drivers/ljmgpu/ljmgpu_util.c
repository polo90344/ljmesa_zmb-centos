#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include <pipe/p_defines.h>

#include "util/u_debug.h"
#include "util/u_memory.h"

#include "ljmgpu_util.h"
//#include "ljmgpu_parser.h"
#include "ljmgpu_screen.h"
#include "ljmgpu_log.h"

struct ljmgpu_dump {
   FILE *fp;
   int id;
};

bool ljmgpu_get_absolute_timeout(uint64_t *timeout)
{
   struct timespec current;
   uint64_t current_ns;

   if (*timeout == PIPE_TIMEOUT_INFINITE)
      return true;

   if (clock_gettime(CLOCK_MONOTONIC, &current))
      return false;

   current_ns = ((uint64_t)current.tv_sec) * 1000000000ull;
   current_ns += current.tv_nsec;
   *timeout += current_ns;

   return true;
}

int IsPowerOfTwo(int value)
{
    return (value & (value - 1)) == 0;
}

/// @brief 计算异或校验.
/// @param obj 原始数据
/// @param len 数据数量
/// @return 计算所得校验值
unsigned checksum_xor(const unsigned *obj, int len)
{
    unsigned *ptr = (unsigned *)obj;
    unsigned ret = *(ptr++);
    for (int i = 1; i < len; i++) {
        ret ^= *(ptr++);
    }
    return ret;
}

//-----------------------------------------------------------------------------
//                              Morton Order
//-----------------------------------------------------------------------------
int pixel_size = 3; // RGB as default
#define MAX_MORTON_SIZE 16 // control morton size
static int MortonValue(int x, int y)
{
    int z = 0;
    for (int i = 0; i < MAX_MORTON_SIZE; i++) {
        z |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
    }
    return z;
}

// (NOTICE) mipmap is required to be power-of-two dimensions, so it's natural
//  not to compensate for mipmap arrays.
// (TODO) consider GL_UNPACK_ALIGNMENT value?
void GenerateMorton(void *tex_arr, int width, int height, int size)
{
    switch (size) {
    case 1:
        pixel_size = 1;
        break;
    case 2:
        pixel_size = 2;
        break;
    case 3:
        pixel_size = 3;
        break;
    case 4:
        pixel_size = 4;
        break;
    }
    //if (!IsPowerOfTwo(width) || !IsPowerOfTwo(height)) {
    //  LOG_GLES2("Dimension(%dx%d) is not power of 2!", width, height);
    //  return;
    //}
    if (width == pixel_size || height == pixel_size)
        return;

    int w_comp = 0;
    int h_comp = 0;
    if (!IsPowerOfTwo(width)) {
        w_comp = pow(2, (int)ceil(log2(width))) - width;
    }
    if (!IsPowerOfTwo(height)) {
        h_comp = pow(2, (int)ceil(log2(height))) - height;
    }

    LOGI("generating morton orders...\n");
    // malloc size with compensated dimension

    int max_dim = (width + w_comp) > (height + h_comp) ? (width + w_comp) : (height + h_comp);
    unsigned char *tmp_arr = (unsigned char *)malloc(max_dim * max_dim * pixel_size);
    //unsigned char *tmp_arr = (unsigned char *)malloc((width + w_comp) * (height + h_comp) * pixel_size);
    //ubyte *tmp_arr = (ubyte *)calloc((width + w_comp) * (height + h_comp) * pixel_size, sizeof(ubyte));
    if (!tmp_arr) {
        LOGE("failed to malloc space for tmp_arr when generating morton ordered data!\n");
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) { // Real row pixels in rect width×height.
            memcpy(tmp_arr + MortonValue(x, y) * pixel_size, (unsigned char *)tex_arr + (y * width + x) * pixel_size,
                   pixel_size);
        }
        // Width compensation.
        memset(tmp_arr + (y * (width + w_comp) + width) * pixel_size, MORTON_COMPENSATION, w_comp * pixel_size);
    }
    // May not needed, cuz at the bottom.
    // Height compensation.
    memset(tmp_arr + (height * (width + w_comp)) * pixel_size, MORTON_COMPENSATION, h_comp * w_comp * pixel_size);

//#define DEBUG
#ifdef DEBUG
    LOGI("---------------------------------------------------\n");
    for (int i = 0; i < width * height; i++) {
        if (i % width == 0)
            printf("\n");
        printf("%d\t", tmp_arr[i]);
    }
#endif // DEBUG
    //#undef DEBUG

    // No need to do this cuz mipmaps are required to be power-of-two arrays.
    // (TODO)But level 0 array do need this to realloc space for storing morton data.
    // (TODO)Also data from CopyTexImage size may not power-of-two arrays.
    // Could do compensation in "ljmgpu_eg01_send_tex_image_to_gpu()",
    // or, if from CopyTexImage2D, do below:
    /*free(tex_arr);
    tex_arr = (ubyte *)malloc((width + w_comp) * (height + h_comp) * pixel_size);*/
    memcpy(tex_arr, tmp_arr, width * height * pixel_size);

    free(tmp_arr);
    tmp_arr = NULL;

    LOGI("morton orders finished.\n");
}
