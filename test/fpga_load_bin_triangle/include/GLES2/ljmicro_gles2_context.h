//////////////////////////////////////////////////////////////////////////////
/// 
/// \internal
/// @file ljmicro_gles2_context.h
/// 
/// @copyright
/// Copyright 2022-2023 LjMicro. All Rights Reserved.
///
/// @brief OpenGL ES2.0 图形库头文件
///
/// @version 0.3
///
/// @details
///         型号：          eGP-01
///         API:            GLES2
///     VERSION:            v0.1
///
/// \internal
/// \note
/// MODIFICATIONS:
///         2022/10/14      创建
///         2023/05/31      重构
/// 
//////////////////////////////////////////////////////////////////////////////
#ifndef ES2_STRUCTS_H
#define ES2_STRUCTS_H
#include "GLES2/gl2.h"

//#define _SOFTWARE_DRAW_ELEMENTS
//#define printf(x) printf("")

// Object name-index map
// name zero for TEXTURE_2D: textures[0]
// name zero for TEXTURE_CUBE_MAP: textures[1]
// ES2 generated name for both: textures[2..255]
#define idxTEX(name)    ((name==0 && target == GL_TEXTURE_2D) ? name : name + 1)
#define idxTEX1(target,name) ((name == 0 && target == GL_TEXTURE_2D) ? name : name + 1)
#define idxOBJ(name)    name - 1 // objects[0..(max-1)]
#define idxBUF(name)    name - 1 // buffers[0..63]
#define idxPRG(name)    name - 1 // programs[0..3]
#define idxSHD(name)    name - 1 // shaders[0..7]

//-----------------------------------------------------------------------------
// 所有指令码已废弃，仅PA的有效
//-----------------------------------------------------------------------------
#define API_DRAW_ARRAYS_MODE_PA 0x00003800
#define API_DRAW_ELEMENTS_MODE_PA 0x00003820

//-----------------------------------------------------------------------------
//                                  状态规格参数
//-----------------------------------------------------------------------------
/* 规范中为 MAX_VERTEX_ATTRIBUTES? */
#define ES2_MAX_ATTRIBUTES 8 // 16 ?
#define ES2_MAX_UNIFORMS 128
// MAX_VERTEX_UNIFORM_VECTORS
// MAX_FRAGMENT_UNIFORM_VECTORS
#define ES2_MAX_TEXTURE_OBJS 256
#define ES2_MAX_TEXTURE_UNITS \
	8 // 256      // [SPEC pg.66]         \
		// MAX_TEXTURE_IMAGE_UNITS        \
		// MAX_VERTEX_TEXTURE_IMAGE_UNITS \
		// MAX_COMBINED_TEXTURE_IMAGE_UNITS

// #define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 32      // MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define ES2_MAX_BUFFER_OBJS 64
#define ES2_MAX_SHADER_OBJS 8
#define ES2_MAX_PROGRAM_OBJS 4
#define ES2_MAX_ERRORS 32
#define ES2_MAX_IMAGE_BUFFERS 2 // 双重缓冲? not sure
#define ES2_MAX_TEXTURE_SIZE \
	65536 // 16bit  GL_MAX_TEXTURE_SIZE  GL_MAX_CUBE_MAP_TEXTURE_SIZE \
		// MAX_CUBE_MAP_TEXTURE_SIZE                                  \
		// MAX_RENDERBUFFER_SIZE
#define ES2_MAX_POINT_SIZE 20
#define ES2_MAX_LINE_WIDTH 2047

#define ES2_MAX_VARYING_VECTORS 8

#define ES2_NUM_COMPRESSED_TEXTURE_FORMATS 8 //GL_NUM_COMPRESSED_TEXTURE_FORMATS

// apply identical value for now
#define ES2_MAX_CUBE_MAP_TEXTURE_SIZE ES2_MAX_TEXTURE_SIZE

// viewport limits
#define ES2_MAX_VIEWPORT 8192


typedef struct {
	GLubyte r;
	GLubyte g;
	GLubyte b;
} RGB;

typedef struct {
	GLushort b : 5;
	GLushort g : 6;
	GLushort r : 5;
} ushort_565;

typedef struct {
	GLushort a : 4;
	GLushort b : 4;
	GLushort g : 4;
	GLushort r : 4;
} ushort_4444;

typedef struct {
	GLushort a : 1;
	GLushort b : 5;
	GLushort g : 5;
	GLushort r : 5;
} ushort_5551;

/// @brief Uniform存储类型
typedef struct {
    GLenum  vec_type;
    GLint   vec_length;
    union {
        GLfloat fvec[4];
        GLshort svec[4];
        GLbyte  bvec[4];
        GLint   ivec[4];
    } vec4;
} vec_t;

// //float packing?
// typedef union {
//     float f;
//     GLuint64 lw;
//     struct {
//         GLubyte byte0 : 8;
//         GLubyte byte1 : 8;
//         GLubyte byte2 : 8;
//         GLubyte byte3 : 8;
//         GLubyte byte4 : 8;
//         GLubyte byte5 : 8;
//         GLubyte byte6 : 8;
//         GLubyte byte7 : 8;
//     } bytes;
// } word_t;


/* -+-+-+-+-+-+-+-+-+-+-+- 数据 & 着色器 -+-+-+-+-+-+-+-+-+-+-+- */

typedef struct {
    // GLboolean   inUse;         // ? added in 20221206 ? may be can be ignored? moved to framebuffer_t
    //GLenum      format;         // be Framebuffer/Renderbuffer ?
    GLenum      image_type;     ///< GL_TEXTURE(no need, already have texture object)、GL_FRAMEBUFFER、GL_RENDERBUFFER

    // [SPEC 4.4.1 pg.108] ? where to specify the attachments ?
    // there is one color attachment point,
    // plus one each for the depth and stencil attachment points
    //
    // Application created framebuffers have 
    // !!!modifiable attachment points(COLOR_ATTACHMENT0, DEPTH...T, STENCIL...T)
    // for each logical buffer in the framebuffer.
    // Framebuffer attachable images can be attached to and detached from
    // these attachment points.
    void*       data;           

    /* [SPEC pg.107]
     * logical buffer   =>  color, depth, stencil (aka rendered output)
     * rendered output  =>  framebuffer-attachable image (texture image, renderbuffer images)
     * attachable image =>  attached to the framebuffer(application-created)
     */

    // framebuffer_t frameBuffer;
    // renderbuffer_t renderBuffer;
} image_t;  ///< @brief 图像缓存


/// @brief Renderbuffer
/// @remark
/// https://www.khronos.org/opengl/wiki/Framebuffer_Object#Framebuffer_Object_Structure
/// renderbuffers are only used in two cases:
///      1. create the storage for them (give a size and format)
///      2. attach to FBOs
typedef struct {
    GLuint inUse;
    GLuint deleted;         ///< pending delete mark
    // GLuint binding;      // use current_renderbuffer instead
    GLuint width;
    GLuint height;
    GLuint internal_format; ///< initial value is RGBA4
    GLuint red_size;
    GLuint green_size;
    GLuint blue_size;
    GLuint alpha_size;
    GLuint depth_size;
    GLuint stencil_size;
    image_t *image;         ///< image of renderbuffer object
    // image_t *color_buf;  // need all three separately?
    // image_t *depth_buf;
    // image_t *stencil_buf
} renderbuffer_t;

typedef struct {
    // [pg.110] per-logical buffer attachment point state
    // GLuint binding;  // Seems no used ?
    GLuint object_type;     ///< NONE, TEXTURE, RENDERBUFFER
    GLuint object_name;     ///< name of texture object, renderbuffer object, cubemap face
    GLuint texture_level;
    GLuint texture_cubemap_face;
    // image_t *image;  // no need, attachment only use name to identify logical buffers
} framebuffer_attachment_t; ///< Framebuffer Attachment


typedef struct {
    GLboolean inUse;
    GLboolean deleted;

    // [SPEC pg.108 There is one color attachment point, plus one each for the depth and stencil attachment points.]
    // framebuffer attachable images(image_t?) can be attached to and detached from these attachment points
    //      1. no visible front or back buffer bitplanes;
    //      2. each of 1 attachment point of color, depth and stencil;
    //      3. no multisample buffer, SAMPLES and SAMPLE_BUFFERS are both 0.
    //  [pg.110]
    //      4. a single image may be attached to multiple application-create framebuffer objects
    //
    // framebuffer object stores logical buffer's attachment point state
    framebuffer_attachment_t color_attachment;      ///< color bit planes     COLOR_ATTACHMENT0   object_name is renderbuffer or texture object name
    framebuffer_attachment_t depth_attachment;      ///< depth bit plane      DEPTH_ATTACHMENT
    framebuffer_attachment_t stencil_attachment;    ///< stencil bit plane    STENCIL_ATTACHMENT

    // for default framebuffer, need a device name field? and its w,h is always equals to the viewport w,h?
    void *back_buffer;   ///< ColorBaseAddr
    void *depth_buffer;  ///< DepthBaseAddr (with bits of depth(24) and stencil(8))

    // [SPEC Table 6.21]
    GLuint red_bits;
    GLuint green_bits;
    GLuint blue_bits;
    GLuint alpha_bits;
    GLuint depth_bits;
    GLuint stencil_bits;
    GLuint color_read_type;     ///< implementation prefered pixel type
    GLuint color_read_format;   ///< implementation prefered pixel format
} framebuffer_t;    ///< Framebuffer Object


typedef struct {
    GLsizei     width;
    GLsizei     height;
    GLenum      data_type;      ///< UNSIGNED(BYTE, SHORT_(5_6_5,4_4_4_4,5_5_5_1))
    GLenum      pixel_format;   ///< RGB、(TODO RGBA and others)
    GLuint      internalformat; ///< RGB、(TODO RGBA and others)
    // GLint       border;         // not supported by this implementation
    GLuint      mipmaps;        ///< mipmaps count
    GLuint      size;           ///< use GLuint instead of GLsizei, (with mipmap space, original size should be size/(4/3))
    void*       data;           // data here is a temporary storage in client for storing decoded data before upload to GPU
                                // in st_tex_image_2d and eg01_send_tex_image_to_gpu, is where uploading occurs
    //NEW
} texture_image_t;  ///< 纹理图像


typedef struct {
    GLboolean   inUse;          ///< 是否绑定到了纹理单元(aka created?)

    /// sampler数组，记录当前纹理对象所绑定的采样器，亦即纹理对象所绑定的纹理单元所对应的sampler
    /// this sample name now also in texture_unit_t
    GLchar*     sampler;        
    // GLchar**     sampler;    // for simplicity, currently one object is not supported to bound to multiple units
    GLenum      target;         ///< 绑定类型：2D、CUBE_MAP
    GLuint      addr;           ///< (TODO) zero if not on server side
    GLenum      mag_type;
    GLenum      min_type;
    GLenum      wrap_mode_s;
    GLenum      wrap_mode_t;
    GLfloat     wrap_value_s;
    GLfloat     wrap_value_t;
    //GLenum pname;    // MAG_FILTER, MIN_FILTER, WRAP_S, WRAP_T
    texture_image_t *texture_image;  ///< TexImage2D assigned temporary local storage buffer

    /// maked deleted do not mean to delete immediately
    /// when the texture object is bound to a Framebuffer Object
    /// that is not bound to the context, the FBO will still
    /// be functional after deleting the texture(only marked as).
    /// Only when the FBO is either deleted or a new texture
    /// attachment replaces the old will the texture finally deleted.
    GLboolean   deleted;        
} texture_t;    ///< 纹理对象


typedef struct {
    /// 绑定的纹理对象名称
    /// [SPEC 3.7.12 pg.84] (? the texture state should be in texture_t)
    /// 7 sets of mipmap arrays & their numbers
    /// 2 sets of texture properties
    GLuint      texture_object_name;
    GLchar*     sampler;
    // when target binding to 0, each target has a default texture object
	GLenum      target;
    texture_t   texture_object_default;      
} texture_unit_t;   ///< 纹理单元

typedef struct {
    GLboolean   used;           ///< (aka created) 缓存attribute array数据
    GLboolean   isBuffer;       ///< 缓存是否存在（生成且绑定过的buffer对象属于缓存对象）
    GLenum      buffer_type;    ///< GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    GLsizei     size;           ///< 缓存的array数据量(float、uint)（BUFFER_SIZE)
    GLenum      usage;          ///< STATIC_DRAW、DYNAMIC_DRAW、STREAM_DRAW
    void*       pointer;        ///< 数组入口 (VERTEX_ATTRIB_ARRAY_POINTER)，
} buffer_t;     ///< 数组缓存


typedef struct {
    // need active ?    Active inputs are those that the compiler/linker detects are actually in use.

    // this indicates if attribute is active in shader
    // if active and enabled, use array buffer data
    // if active but disabled, use current value instead
    GLboolean   active;  

    // 是否启用某属性 (VERTEX_ATTRIB_ARRAY_ENABLED)
    // different from actived, actived means declared in shader source
    GLboolean   enabled;        
    GLsizei     size;           ///< 单个属性元素(SPEC pg.20 array element)大小 (VERTEX_ATTRIB_ARRAY_SIZE)
    GLsizei     stride;         ///< 元素步长 (VERTEX_ATTRIB_ARRAY_STRIDE)
    GLenum      arrayType;      ///< 数组元素部件类型: half-float, float, byte, short, int, etc.
    GLboolean   normalize;      ///< 整型转浮点方式，标准化到区间[0, 1]/[-1, 1] (VERTEX_ATTRIB_ARRAY_NORMALIZED)
    //buffer_t    *pointer;       // 属性数据缓存地址或名称(Server)
    void*   pointer;       ///< 改为空指针以便存储缓存偏移(0, 12, ...)或者顶点数组地址(0xABCDEFAB, ...)

#ifdef _SOFTWARE_DRAW_ELEMENTS
    // for software implemented DrawElements only
    // Save data from DrawElements constructed new arrays on GPU
    // when pointer indicates those array addresses on HOST side.
    void*   pointer_construct;  
#endif  //_SOFTWARE_DRAW_ELEMENTS

    GLchar*     name;           ///< 属性名称（着色器中）
    // 存放属性值 (x, y, z, w)、(r, g, b, a)、(s, t, p, q)
    vec_t       vec_data[4];    ///< 向量
    GLuint      bufName;        ///< VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 默认为0表示未设置pointer
} attribute_t;      ///< 顶点属性


typedef struct {
    // need active ?   https://www.khronos.org/opengl/wiki/Vertex_Shader#Inputs
    GLsizei     size;           ///< the "i" in vec{i}、mat{i}、sampler{i}D
    GLenum      uType;          ///< float、int、short、matrix、sampler、array, etc
    GLenum      type_vec_or_mat;///< GL_UNIFORM_VECTOR、GL_UNIFORM_MATRIX
    // GLaddr      uLocation;      // uniform注册空间偏移量
    vec_t       vec_data[4];    ///< 数据, 数组(like mat4)或向量(like vec4)
    GLchar*     name;           ///< 着色器中名称
} uniform_t;    ///< Uniform变量

typedef struct {
    GLenum      shader_type;
    GLboolean   compiled;
    GLboolean   deleted;
    GLsizei     src_lines_count;
    GLsizei     src_length;      
    GLboolean   created; ///< 着色器是否已创建（从着色器单元中选取）
    GLchar**    src_string;
	GLenum      binary_format;
    GLchar*     binary;
	GLsizei     bin_size;

    /*
        ?Question, (para 2.10.3) says:
        "However, a single shader object may be attached to more than one program object."
        So, here could it be an array of attached program names?
    */
    // GLuint      attached_progrms[ES2_MAX_PROGRAM_OBJS];    // 着色器所附加到的程序名
    GLuint      attached_programs;      // 着色器附加到的程序数；
    
    // (TODO) info log for each shader object
    // INFO_LOG_LENGTH          --missing
    // INFO_LOG_STRING          --missing or somewhere else
    // CURRENT_VERTEX_ATTRIB    --missing    (16* fvec[4])
    // GLuint      infoLogLength;
} shader_t;     ///< 着色器对象类型

typedef struct {
    GLboolean   linked;     ///< 程序已链接
    GLboolean   deleted;    ///< 程序已删除
    GLboolean   validated;  ///< 程序已验证成功
    GLboolean   created;    ///< flag: 程序是否已创建（即从程序单元中选取）
    /// \todo
    /// Should shader binaries be added in?
    /// Should more shaders be added in(currently only 2)?
    /// (same types of shaders are limited to have one main() function)

    // Active shaders, -1 if nothing attached
    GLint       vShader;    ///< 顶点着色器
    GLint       fShader;    ///< 像素着色器
    GLuint      active_uniforms;    ///< 程序中活动的uniform变量数
    GLuint      active_attributes;  ///< 程序中活动的通用属性数
    GLuint      active_uniform_max_length;  ///< 活动uniform变量名最大长度
    GLuint      active_attribute_max_length;///< 活动通用属性变量名最大长度
    
    // 以下临时定义，将会被符号表替代（when program linked, a symbol table is created）
    GLchar*     attributes[ES2_MAX_ATTRIBUTES];
    GLint       attribute_locations[ES2_MAX_ATTRIBUTES];
    GLchar*     uniforms[ES2_MAX_UNIFORMS];
    GLint       uniform_locations[ES2_MAX_UNIFORMS];

    // info log state for program
    // ATTACHED_SHADERS       --missing
    // INFO_LOG_LENGTH        --missing
    // INFO_LOG_STRING        --missing or somewhere else
    GLuint      attached_shaders;

    GLint inUse;    ///< 是否正被某上下文对象使用

    // upload below to GPU when UseProgram() is called
    // (TODO) i dont know whether to upload different offset
    // (eg, vshaderbin, fshaderbin) in one binary (the "executable").
    // Or upload two binary blocks(vShaderBinary, fShaderBinary) respectively.
    // So, for now, use the easy and explict way to do that. 
    GLint v_bin_size;
	GLint f_bin_size;
	GLubyte *vShaderBinary;
	GLubyte *fShaderBinary;
	//GLubyte *executable; // created by linking shaders
} program_t;    ///< 程序对象类型


/* -+-+-+-+-+-+-+-+-+-+-+- context 状态 -+-+-+-+-+-+-+-+-+-+-+- */

// (single thread no dispatching context)
typedef struct {
    // Add more Implementation Dependent Values [SPEC pg.152]?
    GLuint active_texture_unit;         // ([SPEC pg.66]symbolic constant)TEXTURE0, etc. (? GL_ACTIVE_TEXTURE)
    GLuint current_array_buffer;        // 使用 ARRAY_BUFFER_BINDING 查询
    GLuint current_element_buffer;      // 使用 ELEMENT_ARRAY_BUFFER_BINDING 查询
    GLuint current_vertex_attribute;    // SPEC.6.1 pg.133  last queried value of GetVertexAttribfv/iv
    GLuint current_uniform;
    // GLuint current_image_buffer;         // ? shall we separate it into framebuffer and renderbuffer explictly ?
    GLuint current_framebuffer;         // FRAMEBUFFER_BINDING
    GLuint current_renderbuffer;
    GLuint current_program;

    GLuint  error_num;     // 当前错误号 flags   int or uint ?
    GLenum  error_queue[ES2_MAX_ERRORS];//　错误队列 codes

    //　管线状态
    /* PROGRAM STATE */
    GLint       num_shaders, num_programs;
    shader_t    shaders[ES2_MAX_SHADER_OBJS];     // 表 — 着色器对象
    program_t   programs[ES2_MAX_PROGRAM_OBJS];   // 表 — 程序对象

    /* DATA STATE
     * Objects are created in the context and ready to use,
     * their pointers indicate the VRAM area on GPU side.
     */
    GLuint          num_attributes, num_elements, num_uniforms;
    GLuint          num_buffers, num_textures, num_texture_units;
    GLuint          num_framebuffers, num_renderbuffers;
    attribute_t     attributes[ES2_MAX_ATTRIBUTES];         // 表　—　属性
    uniform_t       uniforms[ES2_MAX_UNIFORMS];             // 表　—　uniforms
    buffer_t        buffers[ES2_MAX_BUFFER_OBJS];           // 表　—　缓存对象(GenBuffer to assign)
    texture_unit_t  texture_units[ES2_MAX_TEXTURE_UNITS];   // 表　—　纹理单元
    texture_t       textures[ES2_MAX_TEXTURE_OBJS];         // 表　—　纹理对象
    // image_t         imageBuffers[ES2_MAX_IMAGE_BUFFERS];    // 表　—　Framebuffers、RenderBuffers
    framebuffer_t   imagebuffers[ES2_MAX_IMAGE_BUFFERS];    // Framebuffer(0) front and back buffer
    // modify framebuffer and renderbuffer related implementations 20230302
    framebuffer_t   framebuffers[ES2_MAX_BUFFER_OBJS];      // framebuffer objects
    renderbuffer_t  renderbuffers[ES2_MAX_BUFFER_OBJS];     // renderbuffer objects

    // GENERATE_MIPMAP_HINT
    GLenum          generate_mipmap_hint;         

    /* VIEWPORT STATE */
    // Viewport
    GLint       x, y, w, h;
    // Clipping
    GLfloat     near, far;

    /* RASTER STATE */
    // Culling
    GLboolean   cull_face_enabled;
    GLenum      front_face;
    GLenum      cull_face_mode;
    // Rasterizing
    GLfloat     line_width;
    // GLfloat     aliasedPointSizesmallest, aliasedPointSizelargest;
    // GLfloat     aliasedLineWidthsmallest, aliasedLineWidthlargest; 
    GLfloat     point_size;      // 点大小
    GLfloat     factor, units;  // glPolygonOffset
    GLboolean   polygon_offset_fill_enabled;   // 是否应用　POLYGON_OFFSET_FILL

    /* ROP STATE */
    /*
        单像素操作（Xw,Yw）
            （下述状态应用于发送给帧缓冲的单个像素的操作）
        相关参数：
            =1=  NEVER、LESS(_init_)、EQUAL、LEQUAL、GREATER、NOTEQUAL、GEQUAL、_ALWAYS
            =2=  GL_COLOR_BUFFER_BIT、DEPTH_BUFFER_BIT、STENCIL_BUFFER_BIT
            =3=  FUNC_ADD、FUNC_SUBTRACT、FUNC_REVERSE_SUBTRACT
            =4=  GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT和GL_STENCIL_BUFFER_BIT
            =5=  ZERO, ONE, CONSTANT_RGB, CONSTANT_ALPHA, etc.
    */
    GLboolean   scissor_test_enabled;
    GLint       left, bottom;
    GLsizei     width, height;
    GLboolean   stencil_test_enabled;    /* 模板测试 */
    GLenum      face;               // Separate函数, FRONT、BACK、FRONT_AND_BACK
    GLenum      func_front;          // =1=
    GLenum      func_back;           // =1=
    GLint       ref_front;           // StencilFunc
    GLint       ref_back;            // StencilFunc
    GLuint      mask_front;          // =2=
    GLuint      mask_back;           // =2=
    GLenum      sfail_front, zfail_front, zpass_front; // StencilOp
    GLenum      sfail_back, zfail_back, zpass_back;
    GLboolean   depth_test_enabled;      /* 深度测试 */
    GLenum      func_depth;
    GLboolean   blend_enabled;        /* 调色测试 */
    GLenum      blendRGBEquation;   // =3=
    GLenum      blendAlphaEquation; // =3=
    GLenum      srcRGB, dstRGB;     // Separate函数, RGB, =5=
    GLenum      srcAlpha, dstAlpha; // Separate函数, Alpha, =5=
    GLclampf    blendR, blendG, blendB, blendA;    // glBlendColor
    GLboolean   dither_enabled;       /* 抖动测试？ */
    // Alpha test/Multisample Fragment Operation
    /*
    GLclampf    value;    //SampleCoverage
    GLboolean   invert;   //SampleCoverage*/
    /*
        全缓冲区操作
            （下述状态描述作用于整个帧缓冲的操作）
    */
    GLboolean   maskR, maskG, maskB, maskA;     //glColorMask
    GLclampf    clearR, clearG, clearB, clearA; //glClearColor
    GLboolean   depthFlag;      // 操作：DepthMask
    // GLboolean   stencilMask; // unused
    GLclampf    clearDepth;     // 操作：ClearDepthf
    GLint       clearStencil;   // 操作：ClearStencil
    // clip plane parameters
    //GLfloat     near, far;

    // should separate pack and unpack values into two?
    GLuint      pixelStoreValue;     // PACK_ALIGNMENT, UNPACK_ALIGNMENT
    GLfloat     coverage_value;
    GLboolean   coverage_invert;
    GLboolean   coveraged;          //SAMPLE COVERAGE, (the same with sample_buffers?)
    GLboolean   alpha_coveraged;     //SAMPLE ALPHA TO COVERAGE
    GLbitfield  mask;              // =4=   gl_samples????
    GLubyte     samples;    // GL_SAMPLES

    GLint alphabitplanes ;     //the number of alpha bitplanes in the color buffer of the currently bound framebuffer

    //new
    GLint subpixel_bits;
    // GLint MAX_TEXTURE_SIZE;  // unused
    // GLint viewport_width;    // unused
    // GLint viewport_height;   // unused
    GLenum szpass;       //GL_STENCIL_PASS_DEPTH_PASS
    GLboolean  shader_compiler;  //GL_SHADER_COMPILER

} context_t;    ///< 状态上下文对象


//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
void ResetBuffer(GLuint buffer, GLboolean del);
void ResetFramebuffer(GLuint framebuffer);
void ResetRenderbuffer(GLuint renderbuffer, GLboolean del);
void ResetRenderbufferImage(image_t *image);
void ResetTexture(GLuint texture, GLboolean del);
void ResetTextureImageData(texture_image_t *image, GLboolean del);
void ResetProgram(GLuint i);
extern GLsizeiptr initContext();
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------

#endif // ES2_STRUCTS_H
