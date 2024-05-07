#ifndef H_LJMGPU_FENCE
#define H_LJMGPU_FENCE

struct pipe_fence_handle;
struct ljmgpu_context;
struct ljmgpu_screen;

struct pipe_fence_handle *ljmgpu_fence_create(int fd);
struct pipe_fence_handle *ljmgpu_fence_create_pan(struct ljmgpu_context *ctx);
void ljmgpu_fence_screen_init(struct ljmgpu_screen *screen);
void ljmgpu_fence_context_init(struct ljmgpu_context *ctx);

#endif
