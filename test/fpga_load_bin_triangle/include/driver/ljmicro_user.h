#ifndef LJMICRO_USER_H
#define LJMICRO_USER_H
#include "ljmicro_state.h"

struct pp_context{
    void (*ljm_pp_draw)(
        void* pp_ctx,
        struct pp_draw * draw,
        struct pp_cull_raster * accel,
        struct pp_viewport * viewport,
        struct buffer_t * buffers,
        struct pp_attribs * attribs,
        struct pp_textures * textures,
        struct pp_rop * rop,
        struct pp_uniform * uniforms,
        unsigned int current_array_buffer);

    void (*ljm_pp_clear)(void* pp_ctx, struct pp_clear_flush * fb_clear);
    void (*ljm_pp_flush)(void *pp_ctx, struct pp_clear_flush * fb_clear);
    void (*ljm_pp_buffer_data)(void *pp_ctx, unsigned size, void *data, unsigned name, struct buffer_t * buffer);
    void (*ljm_pp_buffer_sub_data)(void* pp_ctx, int offset, unsigned size, void* data, unsigned name, struct buffer_t * buffer);
    //void (*ljm_pp_tex_image_2d)(void* pp_ctx, texture_t texture);
    void (*ljm_pp_create_tex_image_buffer_on_gpu)(void *pp_ctx, int name, struct texture_t * texture);
    void (*ljm_pp_send_tex_image_to_gpu)(void *pp_ctx, int name, int face, struct texture_t * texture);
    void (*ljm_pp_use_program)(void *pp_ctx, unsigned program, struct program_t * programs);

#if 0 
    void (*ljm_pp_destroy)(struct pp_context * );

     /**
    * Clear a color rendertarget surface.
    * \param color  pointer to an union of fiu array for each of r, g, b, a.
    */
   void (*clear_render_target)(struct pipe_context *pipe,
                               struct pipe_surface *dst,
                               const union pipe_color_union *color,
                               unsigned dstx, unsigned dsty,
                               unsigned width, unsigned height,
                               bool render_condition_enabled);

   /**
    * Clear a depth-stencil surface.
    * \param clear_flags  bitfield of PIPE_CLEAR_DEPTH/STENCIL values.
    * \param depth  depth clear value in [0,1].
    * \param stencil  stencil clear value
    */
   void (*clear_depth_stencil)(struct pipe_context *pipe,
                               struct pipe_surface *dst,
                               unsigned clear_flags,
                               double depth,
                               unsigned stencil,
                               unsigned dstx, unsigned dsty,
                               unsigned width, unsigned height,
                               bool render_condition_enabled);

   /**
    * Clear the texture with the specified texel. Not guaranteed to be a
    * renderable format. Data provided in the resource's format.
    */
   void (*clear_texture)(struct pipe_context *pipe,
                         struct pipe_resource *res,
                         unsigned level,
                         const struct pipe_box *box,
                         const void *data);

   /**
    * Clear a buffer. Runs a memset over the specified region with the element
    * value passed in through clear_value of size clear_value_size.
    */
   void (*clear_buffer)(struct pipe_context *pipe,
                        struct pipe_resource *res,
                        unsigned offset,
                        unsigned size,
                        const void *clear_value,
                        int clear_value_size);

     void (*flush)(struct pipe_context *pipe,
                 struct pipe_fence_handle **fence,
                 unsigned flags);
    /**
    * Create a fence from a fd.
    *
    * This is used for importing a foreign/external fence fd.
    *
    * \param fence  if not NULL, an old fence to unref and transfer a
    *    new fence reference to
    * \param fd     fd representing the fence object
    * \param type   indicates which fence types backs fd
    */
   void (*create_fence_fd)(struct pipe_context *pipe,
                           struct pipe_fence_handle **fence,
                           int fd,
                           enum pipe_fd_type type);

   /**
    * Insert commands to have GPU wait for fence to be signaled.
    */
   void (*fence_server_sync)(struct pipe_context *pipe,
                             struct pipe_fence_handle *fence);

   /**
    * Insert commands to have the GPU signal a fence.
    */
   void (*fence_server_signal)(struct pipe_context *pipe,
                               struct pipe_fence_handle *fence);

    /**
    * Map a resource.
    *
    * Transfers are (by default) context-private and allow uploads to be
    * interleaved with rendering.
    *
    * out_transfer will contain the transfer object that must be passed
    * to all the other transfer functions. It also contains useful
    * information (like texture strides for texture_map).
    */
   void *(*buffer_map)(struct pipe_context *,
		       struct pipe_resource *resource,
		       unsigned level,
		       unsigned usage,  /* a combination of PIPE_MAP_x */
		       const struct pipe_box *,
		       struct pipe_transfer **out_transfer);

    void (*buffer_unmap)(struct pipe_context *,
			struct pipe_transfer *transfer);

   void *(*texture_map)(struct pipe_context *,
			struct pipe_resource *resource,
			unsigned level,
			unsigned usage,  /* a combination of PIPE_MAP_x */
			const struct pipe_box *,
			struct pipe_transfer **out_transfer);

   void (*texture_unmap)(struct pipe_context *,
			 struct pipe_transfer *transfer);

    /* One-shot transfer operation with data supplied in a user
    * pointer.
    */
   void (*buffer_subdata)(struct pipe_context *,
                          struct pipe_resource *,
                          unsigned usage, /* a combination of PIPE_MAP_x */
                          unsigned offset,
                          unsigned size,
                          const void *data);

   void (*texture_subdata)(struct pipe_context *,
                           struct pipe_resource *,
                           unsigned level,
                           unsigned usage, /* a combination of PIPE_MAP_x */
                           const struct pipe_box *,
                           const void *data,
                           unsigned stride,
                           unsigned layer_stride);

    /**
    * Create a 64-bit texture handle.
    *
    * \param ctx        pipe context
    * \param view       pipe sampler view object
    * \param state      pipe sampler state template
    * \return           a 64-bit texture handle if success, 0 otherwise
    */
   uint64_t (*create_texture_handle)(struct pipe_context *ctx,
                                     struct pipe_sampler_view *view,
                                     const struct pipe_sampler_state *state);

   /**
    * Delete a texture handle.
    *
    * \param ctx        pipe context
    * \param handle     64-bit texture handle
    */
   void (*delete_texture_handle)(struct pipe_context *ctx, uint64_t handle);
#endif

};

void ljm_driver_init(struct pp_context* pp_ctx);

#endif
