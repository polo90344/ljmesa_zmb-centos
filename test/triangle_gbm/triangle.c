#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>

#include <gbm.h>
#include <png.h>

// #include <epoxy/gl.h>
// #include <epoxy/egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

GLuint program;
EGLDisplay display;
EGLSurface surface;
EGLContext context;
struct gbm_device *gbm;
struct gbm_surface *gs;

#define TARGET_SIZE 256
#define TARGET_WIDTH 1024
#define TARGET_HEIGHT 768

static EGLConfig get_config(void)
{
	EGLint egl_config_attribs[] = {
		EGL_BUFFER_SIZE,	32,
		EGL_DEPTH_SIZE,		EGL_DONT_CARE,
		EGL_STENCIL_SIZE,	EGL_DONT_CARE,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_NONE,
	};

	EGLint num_configs;
	assert(eglGetConfigs(display, NULL, 0, &num_configs) == EGL_TRUE);

	EGLConfig *configs = malloc(num_configs * sizeof(EGLConfig));
	assert(eglChooseConfig(display, egl_config_attribs,
			       configs, num_configs, &num_configs) == EGL_TRUE);
	assert(num_configs);
	printf("num config %d\n", num_configs);

	// Find a config whose native visual ID is the desired GBM format.
	for (int i = 0; i < num_configs; ++i) {
		EGLint gbm_format;

		assert(eglGetConfigAttrib(display, configs[i],
					  EGL_NATIVE_VISUAL_ID, &gbm_format) == EGL_TRUE);
		printf("gbm format %x\n", gbm_format);

		if (gbm_format == GBM_FORMAT_ARGB8888) {
			EGLConfig ret = configs[i];
			free(configs);
			return ret;
		}
	}

	// Failed to find a config with matching GBM format.
	abort();
}

//初始化 EGL 和 GBM，创建 EGL 表面和上下文，并将它们绑定到当前线程，以便进行 OpenGL ES 的渲染操作
static void render_target_init(void)
{
//	assert(epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_MESA_platform_gbm"));
	//打开渲染设备文件 /dev/dri/renderD128 
	int fd = open("/dev/dri/renderD129", O_RDWR);
	assert(fd >= 0);
	//创建 GBM 设备
	gbm = gbm_create_device(fd);
	assert(gbm != NULL);

//	display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm, NULL);
	//获取 EGL 显示
    display = eglGetDisplay (gbm);
	assert(display != EGL_NO_DISPLAY);

	//初始化 EGL，并检查 EGL 版本
	EGLint majorVersion;
	EGLint minorVersion;
	assert(eglInitialize(display, &majorVersion, &minorVersion) == EGL_TRUE);

	// /绑定 EGL API 为 OpenGL ES
	assert(eglBindAPI(EGL_OPENGL_ES_API) == EGL_TRUE);

	//获取合适的 EGL 配置
	EGLConfig config = get_config();
	//创建 GBM surface并将其赋值给变量 gs。
	gs = gbm_surface_create(
		gbm, TARGET_WIDTH, TARGET_HEIGHT, GBM_BO_FORMAT_ARGB8888,
		GBM_BO_USE_LINEAR|GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
	assert(gs);

	// surface = eglCreatePlatformWindowSurfaceEXT(display, config, gs, NULL);
	// /创建 EGL 表面并将其赋值给变量 surface。
	surface = eglCreateWindowSurface (display, config, gs, NULL);
	assert(surface != EGL_NO_SURFACE);

	const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	//创建 EGL 上下文并将其赋值给变量 context。
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	assert(context != EGL_NO_CONTEXT);
	//将 EGL 表面和上下文绑定到当前线程。
	assert(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE);
}

//编译指定类型的着色器源码，并返回相应的着色器对象
static GLuint compile_shader(const char *source, GLenum type)
{
	GLuint shader;
	GLint compiled;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

//定义了两个字符串常量 vertex_shader 和 fragment_shader，分别表示顶点着色器和片段着色器的源码。
static const char vertex_shader[] =
	"attribute vec3 positionIn;"
	"void main() {"
	"    gl_Position = vec4(positionIn, 1);"
	"}";

static const char fragment_shader[] =
	"void main() {"
	"    gl_FragColor = vec4(1.0, 0.0, 0.0, 1);"
	"}";


//初始化 OpenGL ES，并创建、编译、链接着色器程序
static void init_gles(void)
{
	GLint linked;
	GLuint vertexShader;
	GLuint fragmentShader;
	assert((vertexShader = compile_shader(vertex_shader, GL_VERTEX_SHADER)) != 0);
	assert((fragmentShader = compile_shader(fragment_shader, GL_FRAGMENT_SHADER)) != 0);
	assert((program = glCreateProgram()) != 0);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		/*
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetProgramInfoLog(program, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(program);
		exit(1);
		*/
	}

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, TARGET_WIDTH, TARGET_HEIGHT);

	glUseProgram(program);
}

//执行渲染操作
static void render(void)
{
	GLfloat vertex[] = {
		-1, -1, 0,
		-1, 1, 0,
		1, 1, 0,
	};

	GLint position = glGetAttribLocation(program, "positionIn");
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, vertex);

	printf("app glclear pre\n");
	glClear(GL_COLOR_BUFFER_BIT);
	printf("app glclear end\n");

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//通过调用 eglSwapBuffers 函数将渲染的结果显示在屏幕上
	eglSwapBuffers(display, surface);
}

//将图像数据写入PNG文件
static int write_image(char* filename, int width, int height, int stride,
		       void *buffer, char* title, bool swap_rb)
{
	int code = 0;
	FILE *fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		code = 1;
		goto finalise;
	}

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		code = 1;
		goto finalise;
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 1;
		goto finalise;
	}

	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		code = 1;
		goto finalise;
	}

	png_init_io(png_ptr, fp);

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height,
		     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// Set title
	if (title != NULL) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}

	if (swap_rb)
		png_set_bgr(png_ptr);

	png_write_info(png_ptr, info_ptr);

	// Write image data
	int i;
	for (i = 0; i < height; i++)
		png_write_row(png_ptr, (png_bytep)buffer + i * stride);

	// End write
	png_write_end(png_ptr, NULL);

finalise:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	return code;
}

//将渲染的输出保存为图像文件
static void dump_output(void)
{
	/*在第一个分支（#if 0）中，使用了OpenGL的函数glReadPixels来读取当前帧缓冲区的像素数据。
	读取的数据存储在名为result的缓冲区中，该缓冲区的大小为TARGET_SIZE * TARGET_SIZE * 4字节
	（每个像素4个字节，包括RGBA通道）。然后调用write_image函数将数据保存为PNG图像文件。
	函数的参数包括图像文件名、宽度、高度、每行字节的步幅、像素数据缓冲区、标题和是否交换红蓝通道的标志。*/	
#if 0
	GLubyte result[TARGET_SIZE * TARGET_SIZE * 4] = {0};
	glReadPixels(0, 0, TARGET_SIZE, TARGET_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, result);
	assert(glGetError() == GL_NO_ERROR);

	assert(!write_image("screenshot.png", TARGET_SIZE, TARGET_SIZE,
			    TARGET_SIZE * 4, result, "hello", false));
#else
	/*在第二个分支中，使用了GBM（Generic Buffer Manager）的函数来获取前端缓冲区的数据。
	首先通过gbm_surface_lock_front_buffer获取GBM的缓冲对象bo，然后使用gbm_bo_map映射缓冲对象的数据，
	并返回映射后的指针map_data和步幅stride。获取到的数据存储在名为result的缓冲区中。
	接着调用write_image函数将数据保存为PNG图像文件，参数与前一个分支相同。
	最后，使用gbm_bo_unmap解除映射并释放GBM缓冲对象。*/
	struct gbm_bo *bo = gbm_surface_lock_front_buffer(gs);
	assert(bo);

	uint32_t stride;
	void *map_data = NULL;
	GLubyte *result = gbm_bo_map(
		bo, 0, 0, TARGET_WIDTH, TARGET_HEIGHT,
		GBM_BO_TRANSFER_READ, &stride, &map_data);
	assert(result);

	assert(!write_image("screenshot.png", TARGET_WIDTH, TARGET_HEIGHT,
			    stride, result, "hello", true));

	gbm_bo_unmap(bo, map_data);
	gbm_surface_release_buffer(gs, bo);
#endif
}

int main(void)
{
	printf("app render_target_init pre\n");
	render_target_init();
	printf("app render_target_init end\n");
	init_gles();
	printf("app init_gles pre\n");
	render();
	printf("app render end\n");

	// dump output to a png file
	//dump_output();
	printf("app dump_output end\n");

	return 0;
}

//gcc egl_test2.c -o egl_test2 -lgbm -lEGL -lGLESv2 -lpng

