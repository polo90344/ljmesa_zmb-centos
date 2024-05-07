
/*
    Notice:
        g++ x11_triangle.cpp -o x11_triangle -lEGL -lGLESv2 -lX11 -std=c++11
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <GLES3/gl3.h>
using namespace std;

int initializeEGL(Display *xdisp, Window &xwindow, EGLDisplay &display, EGLContext &context, EGLSurface &surface);
void destroyEGL(EGLDisplay &display, EGLContext &context, EGLSurface &surface);
void mainloop(Display *xdisplay, EGLDisplay display, EGLSurface surface);


int main(int argc, char *argv[])
{
    Display *xdisplay = XOpenDisplay(nullptr);
    if (xdisplay == nullptr)
    {
        std::cerr << "Error XOpenDisplay." << std::endl;
        exit(EXIT_FAILURE);
    }

    Window xwindow = XCreateSimpleWindow(xdisplay, DefaultRootWindow(xdisplay), 100, 100, 500, 500,
                                         1, BlackPixel(xdisplay, 0), WhitePixel(xdisplay, 0));

    XMapWindow(xdisplay, xwindow);

    EGLDisplay display = nullptr;
    EGLContext context = nullptr;
    EGLSurface surface = nullptr;
    if (initializeEGL(xdisplay, xwindow, display, context, surface) < 0)
    {
        std::cerr << "Error initializeEGL." << std::endl;
        exit(EXIT_FAILURE);
    }

    mainloop(xdisplay, display, surface);

    destroyEGL(display, context, surface);
    XDestroyWindow(xdisplay, xwindow);
    XCloseDisplay(xdisplay);

    return 0;
}


int initializeEGL(Display *xdisp, Window &xwindow, EGLDisplay &display, EGLContext &context, EGLSurface &surface)
{
    display = eglGetDisplay(static_cast<EGLNativeDisplayType>(xdisp));
    if (display == EGL_NO_DISPLAY)
    {
        std::cerr << "Error eglGetDisplay." << std::endl;
        return -1;
    }
    if (!eglInitialize(display, nullptr, nullptr))
    {
        std::cerr << "Error eglInitialize." << std::endl;
        return -1;
    }

    EGLint attr[] = {EGL_BUFFER_SIZE, 16, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, attr, &config, 1, &numConfigs))
    {
        std::cerr << "Error eglChooseConfig." << std::endl;
        return -1;
    }
    if (numConfigs != 1)
    {
        std::cerr << "Error numConfigs." << std::endl;
        return -1;
    }

    surface = eglCreateWindowSurface(display, config, xwindow, nullptr);
    if (surface == EGL_NO_SURFACE)
    {
        std::cerr << "Error eglCreateWindowSurface. " << eglGetError() << std::endl;
        return -1;
    }

    EGLint ctxattr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxattr);
    if (context == EGL_NO_CONTEXT)
    {
        std::cerr << "Error eglCreateContext. " << eglGetError() << std::endl;
        return -1;
    }

    eglMakeCurrent(display, surface, surface, context);

    return 0;
}


void destroyEGL(EGLDisplay &display, EGLContext &context, EGLSurface &surface)
{
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}



//=============================================================================
//                         GL Testing Code Begins
//=============================================================================
#define STRINGIFY(x) #x
void TestTriangle(Display *xdisplay, EGLDisplay display, EGLSurface surface)
{
    //-------------------------------------------------------------------
    // vertex shader source
    const char* vStr = STRINGIFY(
        attribute vec3 aPos;
        attribute vec3 aColor;
        varying vec3 Color;
        uniform float i;
        void main()
        {
            gl_Position=vec4(aPos, 1.0);
            Color=aColor;
        }
    );
    // fragment shader source
    const char* fStr = STRINGIFY(
        precision highp float;
        varying vec3 Color;
        void main()
        {
            gl_FragColor=vec4(Color, 1.0);
        }        
    );
    //-------------------------------------------------------------------
    // info log
    int success = 0;
    char infoLog[512];
    // program and shader
    GLuint prog, vs, fs;
    vs = glCreateShader(GL_VERTEX_SHADER);
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &vStr, NULL);
    glShaderSource(fs, 1, &fStr, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        cout << ("Failed compiling vs: ") << infoLog << endl;
    }
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        cout << ("Failed compiling fs: ") << infoLog << endl;
    }
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        cout << ("Failed linking prog: ") << infoLog << endl;
    }
    // use program
    glUseProgram(prog);

    //-------------------------------------------------------------------
    // vertices
    float dm = 0.9;
    GLfloat vertices[] = {
        -dm, -dm, 0.0,    0,0,1,  0.0, 0.0,
         dm, -dm, 0.0,    1,0,0,  1.0, 0.0,
          0,  dm, 0.0,    0,1,0,  1.0, 1.0
    };
    //-------------------------------------------------------------------
    // buffers
    GLuint vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // attribute arrays
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GL_FLOAT), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GL_FLOAT), (void*)(3*sizeof(GL_FLOAT)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GL_FLOAT), (void*)(6*sizeof(GL_FLOAT)));
    
    // get GL error
    GLenum err = glGetError();
    cout << ("err: ") << err << endl;

    //-------------------------------------------------------------------
    // drawing
    while(true){
        glClearColor(0.9, 0.9, 0.9, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(display, surface);
    }

}

// test entry
void mainloop(Display *xdisplay, EGLDisplay display, EGLSurface surface)
{
    TestTriangle(xdisplay, display, surface);
}
