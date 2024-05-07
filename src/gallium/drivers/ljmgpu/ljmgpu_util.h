#ifndef H_LJMGPU_UTIL
#define H_LJMGPU_UTIL

#include <stdint.h>
#include <stdbool.h>

#define EGP_PAGE_SIZE 4096
#define f2ui(f, b) (unsigned)(f * (pow(2.0, b) - 1.0))

struct ljmgpu_dump;

bool ljmgpu_get_absolute_timeout(uint64_t *timeout);

#define MORTON_COMPENSATION 0x0
unsigned checksum_xor(const unsigned *obj, int len);
void GenerateMorton(void *tex_arr, int width, int height, int size);
int IsPowerOfTwo(int value);

#endif

