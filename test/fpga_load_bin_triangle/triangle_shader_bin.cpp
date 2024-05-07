#include <iostream>
#include <vector>
#include <unistd.h>

#include "GLES2/gl2.h"

#include "EGL/egl.h"

std::vector<float> vertices{ 0.0f,  0.5f, 0.0f,
							-0.5f, -0.5f, 0.0f,
							 0.5f, -0.5f, 0.0f };

//uint32_t window_width_{ 1024 };
//uint32_t window_height_{ 768 };
int window_width_{ 1024 };
int window_height_{ 768 };
GLbitfield mask_{ GL_COLOR_BUFFER_BIT };
uint32_t vbo_;
GLuint shader_program_;

void getFileBinary(std::string filename, unsigned char **out, int* sz) {
	FILE *f = fopen(filename.c_str(), "rb");
	if (!f) {
		printf("Failed to open shader binary file: %s\n", filename.c_str());
		return;
	}
	fseek(f, 0, SEEK_END);
	*sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (*out) {
		free(*out);
		*out = NULL;
		*out = (unsigned char *)malloc(*sz * sizeof(unsigned char));
	} else {
		*out = (unsigned char *)malloc(*sz * sizeof(unsigned char));
	}

	if (!*out) {
		printf("failed malloc space for \"out\"!\n");
		return;
	}

	fread(*out, sizeof(unsigned char), *sz, f);

	fclose(f);
}

GLuint LoadShader(GLubyte* shader_bin, GLsizei length, GLenum type) {
	GLuint shader = glCreateShader(type);
	glShaderBinary(1, &shader, GL_PROGRAM_BINARY_FORMAT_LJMICRO, shader_bin, length);
	return shader;
}

bool init() {
	std::string process_path_ = getcwd(NULL, 0);

	int vertex_shader_lenth;
	GLubyte* vertex_shader_bin = NULL;
	getFileBinary(process_path_ + "/vs.txt.bin", &vertex_shader_bin, &vertex_shader_lenth);

	int fragment_shader_lenth;
	GLubyte* fragment_shader_bin = NULL;
	getFileBinary(process_path_ + "/fs.txt.bin", &fragment_shader_bin, &fragment_shader_lenth);

	GLuint vertex_shader_ = LoadShader(vertex_shader_bin, vertex_shader_lenth, GL_VERTEX_SHADER);
	GLuint fragment_shader_ = LoadShader(fragment_shader_bin, fragment_shader_lenth, GL_FRAGMENT_SHADER);

	shader_program_ = glCreateProgram();
	glAttachShader(shader_program_, vertex_shader_);
	glAttachShader(shader_program_, fragment_shader_);

	glLinkProgram(shader_program_);
	int success;
	char infoLog[512];
	glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program_, 512, NULL, infoLog);
		std::cout << "infoLog: " << infoLog << std::endl;
		return false;
	}

	glUseProgram(shader_program_);
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	return true;
}

bool DrawGraphics(uint32_t width, uint32_t height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	//glClear(mask_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	return true;
}

EGLDisplay egl_display_;
EGLSurface egl_surface_;
EGLContext egl_context_;

bool InitEGL() {
	egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY, NULL, NULL);
	EGLint MajorVersion;
	EGLint MinorVersion;
	if (!eglInitialize(egl_display_, &MajorVersion, &MinorVersion)) {
		std::cout << "init egl failed" << std::endl;
		return false;
	}
	const EGLint attr[] = {
			EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, 
			EGL_BLUE_SIZE, 8, 
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8, 
			EGL_DEPTH_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, 
			EGL_NONE 
	};
	EGLConfig ecfg;
	EGLint num_config;
	if (!eglChooseConfig(egl_display_, attr, &ecfg, 1, &num_config)) {
		std::cout << "egl choose config failed" << std::endl;
		return false;
	}

	EGLint ctxattr[] = { 
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE 
	};
	egl_context_ = eglCreateContext(egl_display_, ecfg, EGL_NO_CONTEXT, ctxattr);

	const EGLNativeWindowType native_win = (EGLNativeWindowType)NULL;
	static const EGLint pbufferAttribs[] = {
		EGL_WIDTH, window_width_,
		EGL_HEIGHT, window_height_,
		EGL_NONE
	};
	egl_surface_ = eglCreateWindowSurface(egl_display_, ecfg, native_win, pbufferAttribs);
	if (!egl_surface_) {
		std::cout << eglGetError() << std::endl;
	}
	eglBindAPI(EGL_OPENGL_ES_API);
	eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_);
	return true;
}

bool DestoryEGL() {
	eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl_display_, egl_context_);
	eglDestroySurface(egl_display_, egl_surface_);
	eglTerminate(egl_display_);
	return true;
}

int main(void) {
	InitEGL();

	if (!init()) {
		std::cout << "Init Failed" << std::endl;
		return 0;
	}
	DrawGraphics(window_width_, window_height_);
	glFlush();
	eglSwapBuffers(egl_display_, egl_surface_);
	DestoryEGL();
	return 0;
}
