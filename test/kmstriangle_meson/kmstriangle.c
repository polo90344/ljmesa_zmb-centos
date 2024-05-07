/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "drm-common.h"

static const struct egl *egl;
static const struct gbm *gbm;
static const struct drm *drm;

static struct {
        struct egl egl;
        GLfloat aspect;
        GLuint program;
        GLuint vbo;
        GLuint vao;
} gl;


static const GLfloat vVertices[] = {
                // front
                -1.0f, -1.0f, +1.0f,
                +1.0f, -1.0f, +1.0f,
                -1.0f, +1.0f, +1.0f,
                +1.0f, +1.0f, +1.0f,
};

static const char *vertex_shader_source =
                "attribute vec4 in_position;        \n"
                "void main()                        \n"
                "{                                  \n"
                "    gl_Position = vec4(in_position.x, in_position.y, in_position.z, 1.0);\n"
                "}                                  \n";

static const char *fragment_shader_source =
                "void main()                        \n"
                "{                                  \n"
                "    gl_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);  \n"
                "}                                  \n";



static void draw_triangle(unsigned i)
{
        /* clear the color buffer */
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
}

int main(int argc, char *argv[])
{
	const char *device = NULL;
	char mode_str[DRM_DISPLAY_MODE_LEN] = "";
	uint32_t format = DRM_FORMAT_XRGB8888;
	uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
	int samples = 0;
	unsigned int vrefresh = 0;
	unsigned int count = ~0;
        unsigned int ret;

	drm = init_drm_atomic(device, mode_str, vrefresh, count);
	if (!drm) {
		printf("failed to initialize GBM\n");
		return -1;
	}

	gbm = init_gbm(drm->fd, drm->mode->hdisplay, drm->mode->vdisplay,
			format, modifier);
	if (!gbm) {
		printf("failed to initialize GBM\n");
		return -1;
	}

        ret = init_egl(&gl.egl, gbm, samples);
	/*
	if (!ret)
		printf("failed to initialize EGL\n");
                return -1;
	*/
        gl.aspect = (GLfloat)(gbm->height) / (GLfloat)(gbm->width);
        printf("height[%d],width[%d]\n",gbm->height,gbm->width);
        ret = create_program(vertex_shader_source, fragment_shader_source);
	if (ret < 0)
                return NULL;
	gl.program = ret;

        ret = link_program(gl.program);
	if (ret)
                return NULL;

        glUseProgram(gl.program);
        glViewport(0, 0, 300, 300);
        glEnable(GL_CULL_FACE);

	glGenBuffers(1, &gl.vbo);

        glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

        gl.egl.draw = draw_triangle;
	egl = &gl.egl;

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	drm->run(gbm, egl);

	return 0;
}
