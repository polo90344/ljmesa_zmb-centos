#pragma once
//#include <X11/Xlib.h>
//#include <X11/Xatom.h>
//#include <X11/Xutil.h>
#include "GLES2/gl2.h"
#include "GLES2/ljmicro_gles2_context.h"

struct Display {

};
typedef Display * EGLNativeDisplayType;
typedef unsigned int EGLBoolean;
typedef int EGLint;
typedef unsigned int EGLenum;
typedef void * EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef unsigned long EGLNativeWindowType;
#if defined(__cplusplus)
#define EGL_CAST(type, value) (static_cast<type>(value))
#else
#define EGL_CAST(type,value) ((type)(value))
#endif

#define EGL_DEFAULT_DISPLAY  EGL_CAST(EGLNativeDisplayType, 0)
#define EGL_PBUFFER_BIT   0x0001
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_NONE   0x3038
#define EGL_RED_SIZE   0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_DEPTH_SIZE 0x3025
#define EGL_STENCIL_SIZE 0x3026
#define EGL_SURFACE_TYPE 0x3033
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_HEIGHT 0x3056
#define EGL_WIDTH    0x3057
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_OPENGL_ES2_BIT  0x0004 
#define EGL_NO_SURFACE ((EGLSurface)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
context_t * ctx;

EGLDisplay eglGetDisplay(EGLDisplay dpy, EGLint * major, EGLint * minor) {
    int * i = new int();
    return (void *)i;
}
EGLBoolean eglInitialize(EGLDisplay dpy, EGLint * major, EGLint * minor){
    return true;
}

EGLBoolean eglChooseConfig (EGLDisplay dpy, const EGLint * attrib_list, EGLConfig * configs, EGLint config_size, EGLint *num_config){
    return true;
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint * attrib_list) {
    int *i = new int();
    return (void *)i;
}
EGLint eglGetError(void){
    return 0;
}
EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint * attrib_list){
    initContext();
    int * i = new int();
    return(void *)i;
}
EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    return true;
}
EGLBoolean eglTerminate(EGLDisplay dpy){
    return true;
}
EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface){
    glFlush();
    return true;
}
EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    return true;
}
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface){
    return true;
}
EGLBoolean eglBindAPI(EGLenum api){
    return true;
}
