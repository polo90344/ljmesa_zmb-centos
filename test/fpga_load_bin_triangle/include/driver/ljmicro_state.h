#ifndef LJMICRO_STATE_H
#define  LJMICRO_STATE_H

#define ES2_MAX_ATTRIBUTES 8 // 16 ?
#define ES2_MAX_UNIFORMS 128
#define ES2_MAX_TEXTURE_OBJS 256
#define ES2_MAX_TEXTURE_UNITS \
    8 // 256      // [SPEC pg.66]         \
        // MAX_TEXTURE_IMAGE_UNITS        \
        // MAX_VERTEX_TEXTURE_IMAGE_UNITS \
        // MAX_COMBINED_TEXTURE_IMAGE_UNITS

struct buffer_t{
    unsigned char  used;           ///< (aka created) 缓存attribute array数据
    unsigned char   isBuffer;       ///< 缓存是否存在（生成且绑定过的buffer对象属于缓存对象）
    unsigned short   buffer_type;    ///< GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    int     size;           ///< 缓存的array数据量(float、uint)（BUFFER_SIZE)
    unsigned int   usage;          ///< STATIC_DRAW、DYNAMIC_DRAW、STREAM_DRAW
    void*       pointer;        ///< 数组入口 (VERTEX_ATTRIB_ARRAY_POINTER)，
};     ///< 数组缓存

struct texture_image_t{
    int     width;
    int     height;
    unsigned short      data_type;      ///< UNSIGNED(BYTE, SHORT_(5_6_5,4_4_4_4,5_5_5_1))
    unsigned short      pixel_format;   ///< RGB、(TODO RGBA and others)
    unsigned int        internalformat; ///< RGB、(TODO RGBA and others)
    // GLint       border;         // not supported by this implementation
    unsigned int      mipmaps;        ///< mipmaps count
    unsigned int      size;           ///< use GLuint instead of GLsizei, (with mipmap space, original size should be size/(4/3))
    void*       data;           // data here is a temporary storage in client for storing decoded data before upload to GPU
                                // in st_tex_image_2d and eg01_send_tex_image_to_gpu, is where uploading occurs
    //NEW
};  ///< 纹理图像
struct texture_t{
    unsigned char   inUse;          ///< 是否绑定到了纹理单元(aka created?)

    /// sampler数组，记录当前纹理对象所绑定的采样器，亦即纹理对象所绑定的纹理单元所对应的sampler
    /// this sample name now also in texture_unit_t
    char*     sampler;
    // GLchar**     sampler;    // for simplicity, currently one object is not supported to bound to multiple units
    unsigned short      target;         ///< 绑定类型：2D、CUBE_MAP
    unsigned int      addr;           ///< (TODO) zero if not on server side
    unsigned short      mag_type;
    unsigned short      min_type;
    unsigned short      wrap_mode_s;
    unsigned short      wrap_mode_t;
    float     wrap_value_s;
    float     wrap_value_t;
    //GLenum pname;    // MAG_FILTER, MIN_FILTER, WRAP_S, WRAP_T
    struct texture_image_t *texture_image;  ///< TexImage2D assigned temporary local storage buffer

    /// maked deleted do not mean to delete immediately
    /// when the texture object is bound to a Framebuffer Object
    /// that is not bound to the context, the FBO will still
    /// be functional after deleting the texture(only marked as).
    /// Only when the FBO is either deleted or a new texture
    /// attachment replaces the old will the texture finally deleted.
    unsigned char   deleted;
};    ///< 纹理对象

struct texture_unit_t{
    /// 绑定的纹理对象名称
    /// [SPEC 3.7.12 pg.84] (? the texture state should be in texture_t)
    /// 7 sets of mipmap arrays & their numbers
    /// 2 sets of texture properties
    unsigned int      texture_object_name;
    char*     sampler;
    // when target binding to 0, each target has a default texture object
    unsigned short      target;
    struct texture_t   texture_object_default;
};   ///< 纹理单元

struct program_t{
    unsigned char   linked;     ///< 程序已链接
    unsigned char   deleted;    ///< 程序已删除
    unsigned char   validated;  ///< 程序已验证成功
    unsigned char   created;    ///< flag: 程序是否已创建（即从程序单元中选取）
    /// \todo
    /// Should shader binaries be added in?
    /// Should more shaders be added in(currently only 2)?
    /// (same types of shaders are limited to have one main() function)

    // Active shaders, -1 if nothing attached
    int        vShader;    ///< 顶点着色器
    int        fShader;    ///< 像素着色器
    unsigned int      active_uniforms;    ///< 程序中活动的uniform变量数
    unsigned int      active_attributes;  ///< 程序中活动的通用属性数
    unsigned int      active_uniform_max_length;  ///< 活动uniform变量名最大长度
    unsigned int      active_attribute_max_length;///< 活动通用属性变量名最大长度

    // 以下临时定义，将会被符号表替代（when program linked, a symbol table is created）
    char*            attributes[ES2_MAX_ATTRIBUTES];
    int       attribute_locations[ES2_MAX_ATTRIBUTES];
    char*            uniforms[ES2_MAX_UNIFORMS];
    int       uniform_locations[ES2_MAX_UNIFORMS];

    // info log state for program
    // ATTACHED_SHADERS       --missing
    // INFO_LOG_LENGTH        --missing
    // INFO_LOG_STRING        --missing or somewhere else
    unsigned int      attached_shaders;

    int inUse;    ///< 是否正被某上下文对象使用

    // upload below to GPU when UseProgram() is called
    // (TODO) i dont know whether to upload different offset
    // (eg, vshaderbin, fshaderbin) in one binary (the "executable").
    // Or upload two binary blocks(vShaderBinary, fShaderBinary) respectively.
    // So, for now, use the easy and explict way to do that.
    int v_bin_size;
    int f_bin_size;
    unsigned char *vShaderBinary;
    unsigned char *fShaderBinary;
    //GLubyte *executable; // created by linking shaders
};    ///< 程序对象类型
/***draw******/

struct pp_cull_raster{
    unsigned line_width;    ///< 线宽，默认值1
    unsigned cull_face_enabled: 1;      ///< 启用背面消隐，默认值GL_FALSE
    unsigned front_face:        1;      ///< 设置消隐面前面，默认值GL_CCW
    unsigned cull_face_mode:    4;      ///< 设置消隐面背面，默认值GL_BACK
    unsigned polygonoffsetfill_enabled; ///< 默认值GL_FALSE
    unsigned polygonoffsetfill_factor;  ///< 默认值0
    unsigned polygonoffsetfill_unit;    ///< 默认值0
};

/// @brief 视口状态.
struct pp_viewport{
    unsigned near;      ///< 视口近平面，默认值0
    unsigned far;       ///< 视口原屏幕，默认值1
    unsigned x: 16;     ///< 视口左下角x坐标，默认值0
    unsigned y: 16;     ///< 视口左下角y坐标，默认值0
    unsigned w: 16;     ///< 视口宽度，默认值为系统创建的窗口宽度
    unsigned h: 16;     ///< 视口高度，默认值为系统创建的窗口高度
};
/// @brief 渲染绘制和图元装配状态.
struct pp_draw{
    unsigned enabled: 3;  // enabled2, not texture_enabled2
    unsigned mark;  // highest bit h1(drawarrays, drawelements), 2nd hightest bit h2(draw ends)
    unsigned indices; // drawarrays-first, drawelements-indices
    unsigned count; // drawarrays-vertex cnt, drawelements-index count
    unsigned api;   // pa state: drawarrays, drawelements
    unsigned mode;  /**< the mode of the primitive */
};

//----------------------------------------------------------------------------
//                      fragment and texture state
//----------------------------------------------------------------------------
/// @brief 纹理状态.
struct pp_texture_unit{
    unsigned level:         4;  // A1   ///< mipmap级别，默认值0
    unsigned internalformat:4;          ///< 纹理格式，默认值GL_RGBA
    unsigned type:          4;          ///< 像素数据类型，默认值GL_UNSIGNED_BYTE
    unsigned min_or_mag:    1;          ///< 缩放标识
    unsigned min_filter:    3;          ///< 降采样滤波模式
    unsigned mag_filter:    1;          ///< 超采样滤波模式
    unsigned wrap_s:        2;          ///< s方向缠绕模式
    unsigned wrap_t:        2;          ///< t方向缠绕模式，默认值
    unsigned user_define:   1;          ///< 用户定义参数，0：Image2D，1：CompressedImage2D
    // unsigned reserved_bits: 10;  // useless
    unsigned width:         16; // A2   ///< 纹理图像宽度
    unsigned height:        16;         ///< 纹理图像高度
    unsigned* data_pointer;      // A3   ///< 像素数据地址
};

struct _texture_unit{
	unsigned target;
	unsigned texture_object_name;
};
/// @brief 纹理单元和纹理配置状态.
struct pp_textures{
    unsigned enabled1;   // A0  (texture enabled1，纹理单元使能，每bit位控制一个纹理单元)
    unsigned enabled2;   //     (texture enabled2，纹理配置使能，每bit位控制一个纹理配置，注：bit0标识enabled1)
    struct pp_texture_unit texture_units[ES2_MAX_TEXTURE_UNITS];
    struct _texture_unit units_target_object[ES2_MAX_TEXTURE_UNITS];
};


/***draw******/
/**rop**/
//----------------------------------------------------------------------------
//                              rop state
//----------------------------------------------------------------------------

/// @brief GL测试启用或禁用配置数据（0：禁用，1：启用）
struct pp_rop_gltests{
    unsigned enable_blend;
    unsigned enable_scissor_test;
    unsigned enable_stencil_test;
    unsigned enable_depth_test;
};

/// @brief 混合颜色状态，默认值(0, 0, 0, 0)
struct pp_rop_blend_color{
    unsigned red:   8;      ///< 混合Red值
    unsigned green: 8;      ///< 混合Green值
    unsigned blue:  8;      ///< 混合Blue值
    unsigned alpha: 8;      ///< 混合Alpha值
};

/// @brief 帧缓存颜色缓存可写入状态，默认值均为GL_TRUE.
struct pp_rop_color_mask{
    unsigned r: 1;
    unsigned g: 1;
    unsigned b: 1;
    unsigned a: 1;
};

/// @brief 深度缓存可写入状态，默认值GL_TRUE.
struct pipe_rop_depth_mask{
    unsigned depth_mask: 1;
};
/// @brief 剪刀状态.
struct pp_rop_scissor{
    unsigned x: 16;         ///< 左下角x坐标，默认值0
    unsigned y: 16;         ///< 左下角y坐标，默认值0
    unsigned width:  16;    ///< 宽度，默认值为窗口宽度
    unsigned height: 16;    ///< 高度，默认值为窗口高度
};

/// @brief 模板前后面缓存位可写入状态，默认值所有比特位均为1.
struct pp_rop_stencil_mask{
    unsigned front_mask: 8;
    unsigned back_mask:  8;
};
/// @brief 混合方程状态.
struct pp_rop_blend_equation{
    unsigned mode_rgb:   8;         ///< RGB混合方程
    unsigned mode_alpha: 8;         ///< Alpha混合方程
};

/// @brief 混合像素算术因子.
struct pp_rop_blend_func{
    unsigned src_rgb:   8;          ///< 默认值GL_ONE
    unsigned dst_rgb:   8;          ///< 默认值GL_ZERO
    unsigned src_alpha: 8;          ///< 默认值GL_ONE
    unsigned dst_alpha: 8;          ///< 默认值GL_ZERO
};
/// @brief 模板测试状态.
struct pp_rop_stencil_func{
    unsigned front_func: 4;         ///< 默认值GL_ALWAYS
    unsigned front_ref:  8;         ///< 默认值0
    unsigned func_front_mask: 8;    ///< 默认值所有比特位均为1
    unsigned back_func:  4;         ///< 默认值GL_ALWAYS
    unsigned back_ref:   8;         ///< 默认值0
    unsigned func_back_mask: 8;     ///< 默认值所有比特位均为1
};

/// @brief 模板测试行为状态.
struct pp_rop_stencil_op{
    unsigned front_sfail:   4;      ///< 模板测试失败时的正面模板行为
    unsigned front_dpfail:  4;      ///< 模板测试成功但深度测试失败时的正面模板行为
    unsigned front_dppass:  4;      ///< 模板测试和深度测试成功时的正面模板行为
    unsigned back_sfail:    4;      ///< 模板测试失败时的背面模板行为
    unsigned back_dpfail:   4;      ///< 模板测试成功但深度测试失败时的背面模板行为
    unsigned back_dppass:   4;      ///< 模板测试和深度测试成功时的背面模板行为
};

/// @brief 深度测试状态.
struct pp_rop_depth_func{
    unsigned depth_func: 3; ///< 深度测试方式，默认值GL_LESS
};

/// @brief 深度缓存可写入状态，默认值GL_TRUE.
struct pp_rop_depth_mask{
    unsigned depth_mask: 1;
};
/// @brief 像素级rop状态.
struct pp_rop{
    unsigned enabled;   // blend, scissor, stencil, depth tests use enabled bits(0, 1, 2, 3 respectively) only
    unsigned _reserved_zero; // reserved
    struct pp_rop_gltests gltests;   // bit 1~4
    struct pp_rop_blend_color blend_color;         // bit 5
    struct pp_rop_color_mask color_mask;           // bit 6
    struct pp_rop_depth_mask depth_mask;           // bit 7
    struct pp_rop_scissor scissor;                 // bit 8, 9
    struct pp_rop_stencil_mask stencil_mask;       // bit 10
    struct pp_rop_blend_equation blend_equation;   // bit 11
    struct pp_rop_blend_func blend_func;           // bit 12
    struct pp_rop_stencil_func stencil_func;       // bit 13, 14
    struct pp_rop_stencil_op stencil_op;           // bit 15
    struct pp_rop_depth_func depth_func;           // bit 16
};




/**rop**/

//----------------------------------------------------------------------------
//                          vertex attribute state
//----------------------------------------------------------------------------
/// @brief 属性状态.
struct pp_attrib{
    unsigned index: 3;      ///< 属性索引
    unsigned size:  2;      ///< 属性元素数，默认值4
    unsigned type:  3;      ///< 元素类型，默认值GL_FLOAT
    unsigned normalized: 1; ///< 归一化转换标识，默认值GL_FALSE
    unsigned stride;        ///< 步长，默认值0
    unsigned* pointer;      ///< 属性数组地址，默认值NULL
};

/// @brief 属性数组状态.
struct pp_attribs{
    unsigned enabled:   24;    // enabled1, not texture_enabled1
    struct pp_attrib attribs[ES2_MAX_ATTRIBUTES];  ///< 所有属性状态
};


#define ST_UNIFORM_VALUES_CNT ES2_MAX_UNIFORMS*16
struct pp_uniform{
    unsigned indicator[ST_UNIFORM_VALUES_CNT/32]; ///< Uniform数据标识，0：该位数据无效，1：该位数据有效
    unsigned data[ST_UNIFORM_VALUES_CNT]; ///< Uniform数据(仅32位浮点型)
    int cnt; // Uniform scalar values count
}; // (TODO) or use uniforms[] here, and data in pipe_context?
//----------------------------------------------------------------------------
//                  (rop)whole framebuffer operation state
//----------------------------------------------------------------------------
/// @brief 帧缓存整体rop操作状态.
struct pp_clear_flush{
    unsigned enabled;
    unsigned _reserved_zero;        // enable bit 0     ///< 0 as reserved
    unsigned color_base_addr;       // enable bit 1     ///< 颜色缓存地址
    unsigned depth_base_addr;       // enable bit 2     ///< 深度缓存地址
    unsigned frame_width:   16;     // enable bit 3     ///< 视口宽度
    unsigned frame_height:  16;                         ///< 视口高度
    unsigned clear_red:     8;      // enable bit 4     ///< 设置帧缓存Red清除值，默认值0
    unsigned clear_green:   8;                          ///< 设置帧缓存Green清除值，默认值0
    unsigned clear_blue:    8;                          ///< 设置帧缓存Blue清除值，默认值0
    unsigned clear_alpha:   8;                          ///< 设置帧帧缓存Alpha清除值，默认值1
    unsigned clear_depth:  24;      // enable bit 5     ///< 设置帧缓存深度缓存清除值，默认值1
    unsigned clear_s:       8;      // (s - stencil)    ///< 设置帧缓存模板缓存清楚值，默认值0
    unsigned mask;                  // enable bit 6     ///< 设置帧缓存清除mask位
    unsigned flush;                 // enable bit 7     ///< 设置flush命令状态数据为1
    // flush will use enabled bit(bit 7) only
};   // non-pixel related whole framebuffer state

#endif

